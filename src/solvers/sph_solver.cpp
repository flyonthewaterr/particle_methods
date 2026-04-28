#include "particle_methods/solvers/sph_solver.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kEpsilon = 1.0e-6F;

struct CellCoord {
  int x {0};
  int y {0};

  bool operator==(const CellCoord& other) const = default;
};

struct CellCoordHash {
  std::size_t operator()(const CellCoord& coord) const noexcept {
    const std::uint64_t ux = static_cast<std::uint64_t>(static_cast<std::uint32_t>(coord.x));
    const std::uint64_t uy = static_cast<std::uint64_t>(static_cast<std::uint32_t>(coord.y));
    return static_cast<std::size_t>((ux * 0x9E3779B185EBCA87ULL) ^ (uy * 0xC2B2AE3D27D4EB4FULL));
  }
};

int to_cell(float coordinate, float inv_cell_size) {
  return static_cast<int>(std::floor(coordinate * inv_cell_size));
}

struct KernelCoefficients {
  float poly6 {0.0F};
  float spiky_grad {0.0F};
  float viscosity_laplacian {0.0F};
};

KernelCoefficients make_kernel_coeff(float h) {
  // Kernel families follow SPlisHSPlasH-style poly6/spiky/viscosity forms.
  const float h2 = h * h;
  const float h4 = h2 * h2;
  const float h5 = h4 * h;
  const float h8 = h4 * h4;

  KernelCoefficients coeff;
  coeff.poly6 = 4.0F / (kPi * h8);
  coeff.spiky_grad = -30.0F / (kPi * h5);
  coeff.viscosity_laplacian = 40.0F / (kPi * h5);
  return coeff;
}

float poly6_value(float r2, float h2, float coeff) {
  if (r2 >= h2) {
    return 0.0F;
  }

  const float diff = h2 - r2;
  return coeff * diff * diff * diff;
}

float spiky_grad_factor(float r, float h, float coeff) {
  if (r >= h || r <= kEpsilon) {
    return 0.0F;
  }

  const float diff = h - r;
  return coeff * diff * diff / r;
}

float viscosity_laplacian_value(float r, float h, float coeff) {
  if (r >= h) {
    return 0.0F;
  }

  return coeff * (h - r);
}

float compute_cfl_dt(const std::vector<pm::Particle>& particles, const pm::SimulationConfig& config, float h) {
  if (config.sph_cfl_factor <= 0.0F) {
    return std::numeric_limits<float>::infinity();
  }

  float max_speed = 0.0F;
  for (const auto& particle : particles) {
    max_speed = std::max(max_speed, pm::length(particle.velocity));
  }

  const float denom = config.sph_sound_speed + max_speed;
  if (denom <= kEpsilon) {
    return std::numeric_limits<float>::infinity();
  }

  return config.sph_cfl_factor * h / denom;
}

pm::Vec2 boundary_force(const pm::Particle& particle, const pm::SimulationConfig& config) {
  const float stiffness = config.sph_boundary_stiffness;
  const float damping = config.sph_boundary_damping;
  if (stiffness <= 0.0F) {
    return pm::Vec2 {};
  }

  const float min_x = config.bounds_left + particle.radius;
  const float max_x = config.bounds_right - particle.radius;
  const float min_y = config.floor_y + particle.radius;
  const float max_y = config.ceiling_y - particle.radius;

  pm::Vec2 force {0.0F, 0.0F};

  auto apply_axis = [&](float penetration, float normal, float velocity_component, float& out_force) {
    if (penetration <= 0.0F) {
      return;
    }

    float force_mag = stiffness * penetration;
    const float normal_velocity = velocity_component * normal;
    if (normal_velocity < 0.0F) {
      force_mag += -damping * normal_velocity;
    }

    out_force += normal * force_mag;
  };

  const auto mode = config.boundary_mode;
  if (mode == pm::BoundaryMode::Walls || mode == pm::BoundaryMode::Box) {
    apply_axis(min_x - particle.position.x, 1.0F, particle.velocity.x, force.x);
    apply_axis(particle.position.x - max_x, -1.0F, particle.velocity.x, force.x);
  }

  if (mode == pm::BoundaryMode::Floor || mode == pm::BoundaryMode::Box) {
    apply_axis(min_y - particle.position.y, 1.0F, particle.velocity.y, force.y);
  }

  if (mode == pm::BoundaryMode::Box) {
    apply_axis(particle.position.y - max_y, -1.0F, particle.velocity.y, force.y);
  }

  return force;
}

}  // namespace

namespace pm {

std::string SPHSolver::name() const {
  return "SPH";
}

void SPHSolver::step(ParticleSystem& system, const SimulationConfig& config, int substeps) {
  auto& particles = system.particles();
  if (particles.empty()) {
    return;
  }

  const int clamped_substeps = std::max(substeps, 1);
  const float h = config.sph_smoothing_length;
  if (h <= kEpsilon) {
    throw std::runtime_error("SPH smoothing length must be > 0.");
  }

  const float h2 = h * h;
  const float inv_cell_size = 1.0F / h;
  const KernelCoefficients kernel = make_kernel_coeff(h);

  std::vector<float> densities(particles.size());
  std::vector<float> pressures(particles.size());
  std::vector<Vec2> forces(particles.size());
  std::unordered_map<CellCoord, std::vector<std::size_t>, CellCoordHash> cell_list;
  cell_list.reserve(particles.size() * 2);

  for (int step_index = 0; step_index < clamped_substeps; ++step_index) {
    cell_list.clear();
    for (std::size_t i = 0; i < particles.size(); ++i) {
      const CellCoord cell {
          to_cell(particles[i].position.x, inv_cell_size),
          to_cell(particles[i].position.y, inv_cell_size),
      };
      cell_list[cell].push_back(i);
    }

    for (std::size_t i = 0; i < particles.size(); ++i) {
      float density = 0.0F;
      const auto& pi = particles[i];
      const CellCoord base_cell {
          to_cell(pi.position.x, inv_cell_size),
          to_cell(pi.position.y, inv_cell_size),
      };

      for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
          const CellCoord neighbor_cell {base_cell.x + dx, base_cell.y + dy};
          const auto neighbor_it = cell_list.find(neighbor_cell);
          if (neighbor_it == cell_list.end()) {
            continue;
          }

          for (const std::size_t j : neighbor_it->second) {
            const Vec2 delta = pi.position - particles[j].position;
            const float r2 = length_squared(delta);
            density += particles[j].mass * poly6_value(r2, h2, kernel.poly6);
          }
        }
      }

      densities[i] = std::max(density, kEpsilon);
      pressures[i] = config.sph_pressure_stiffness * std::max(densities[i] - config.sph_rest_density, 0.0F);
    }

    for (std::size_t i = 0; i < particles.size(); ++i) {
      const auto& pi = particles[i];
      Vec2 force {0.0F, 0.0F};
      const CellCoord base_cell {
          to_cell(pi.position.x, inv_cell_size),
          to_cell(pi.position.y, inv_cell_size),
      };

      for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
          const CellCoord neighbor_cell {base_cell.x + dx, base_cell.y + dy};
          const auto neighbor_it = cell_list.find(neighbor_cell);
          if (neighbor_it == cell_list.end()) {
            continue;
          }

          for (const std::size_t j : neighbor_it->second) {
            if (j == i) {
              continue;
            }

            const auto& pj = particles[j];
            const Vec2 delta = pi.position - pj.position;
            const float r2 = length_squared(delta);
            if (r2 >= h2) {
              continue;
            }

            const float r = std::sqrt(std::max(r2, kEpsilon));
            const float neighbor_density = std::max(densities[j], kEpsilon);
            const float pressure_term = (pressures[i] + pressures[j]) / (2.0F * neighbor_density);
            const float grad_factor = spiky_grad_factor(r, h, kernel.spiky_grad);
            const Vec2 grad = delta * grad_factor;
            force += grad * (-pj.mass * pressure_term);

            const float visc_factor = config.sph_viscosity * pj.mass / neighbor_density
                                      * viscosity_laplacian_value(r, h, kernel.viscosity_laplacian);
            force += (pj.velocity - pi.velocity) * visc_factor;
          }
        }
      }

      force += boundary_force(pi, config);
      forces[i] = force;
    }

    float dt = config.dt / static_cast<float>(clamped_substeps);
    const float cfl_dt = compute_cfl_dt(particles, config, h);
    if (std::isfinite(cfl_dt)) {
      dt = std::min(dt, cfl_dt);
    }

    for (std::size_t i = 0; i < particles.size(); ++i) {
      auto& particle = particles[i];
      const float density = std::max(densities[i], kEpsilon);
      Vec2 acceleration {0.0F, config.gravity_y};
      acceleration += forces[i] * (1.0F / density);

      particle.velocity += acceleration * dt;
      particle.position += particle.velocity * dt;
    }
  }
}

}  // namespace pm
