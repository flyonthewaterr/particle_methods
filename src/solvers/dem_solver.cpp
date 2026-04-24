#include "particle_methods/solvers/dem_solver.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "particle_methods/cuda/dem_cuda.hpp"

namespace {

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

float clamp01(float value) {
  return std::clamp(value, 0.0F, 1.0F);
}

int to_cell(float coordinate, float inv_cell_size) {
  return static_cast<int>(std::floor(coordinate * inv_cell_size));
}

void resolve_floor(pm::Particle& particle, const pm::SimulationConfig& config) {
  const float min_y = config.floor_y + particle.radius;
  if (particle.position.y >= min_y) {
    return;
  }

  if (config.floor_opening_enabled &&
      particle.position.x > config.floor_opening_left &&
      particle.position.x < config.floor_opening_right) {
    return;
  }

  particle.position.y = min_y;
  if (particle.velocity.y < 0.0F) {
    particle.velocity.y = -particle.velocity.y * config.restitution;
  }

  particle.velocity.x *= (1.0F - clamp01(config.boundary_friction));
}

void resolve_side_walls(pm::Particle& particle, const pm::SimulationConfig& config) {
  const float min_x = config.bounds_left + particle.radius;
  const float max_x = config.bounds_right - particle.radius;

  if (particle.position.x < min_x) {
    particle.position.x = min_x;
    if (particle.velocity.x < 0.0F) {
      particle.velocity.x = -particle.velocity.x * config.restitution;
    }
    particle.velocity.y *= (1.0F - clamp01(config.boundary_friction));
  }

  if (particle.position.x > max_x) {
    particle.position.x = max_x;
    if (particle.velocity.x > 0.0F) {
      particle.velocity.x = -particle.velocity.x * config.restitution;
    }
    particle.velocity.y *= (1.0F - clamp01(config.boundary_friction));
  }
}

void resolve_ceiling(pm::Particle& particle, const pm::SimulationConfig& config) {
  const float max_y = config.ceiling_y - particle.radius;
  if (particle.position.y <= max_y) {
    return;
  }

  particle.position.y = max_y;
  if (particle.velocity.y > 0.0F) {
    particle.velocity.y = -particle.velocity.y * config.restitution;
  }

  particle.velocity.x *= (1.0F - clamp01(config.boundary_friction));
}

void resolve_boundaries(pm::Particle& particle, const pm::SimulationConfig& config) {
  switch (config.boundary_mode) {
    case pm::BoundaryMode::Floor:
      resolve_floor(particle, config);
      break;
    case pm::BoundaryMode::Walls:
      resolve_side_walls(particle, config);
      break;
    case pm::BoundaryMode::Box:
      resolve_floor(particle, config);
      resolve_side_walls(particle, config);
      resolve_ceiling(particle, config);
      break;
  }
}

}  // namespace

namespace pm {

DEMSolver::DEMSolver(Backend backend)
    : backend_(backend) {}

std::string DEMSolver::name() const {
  return backend_ == Backend::CUDA ? "DEM (CUDA)" : "DEM (CPU)";
}

void DEMSolver::step(ParticleSystem& system, const SimulationConfig& config, int substeps) {
  const int clamped_substeps = std::max(substeps, 1);

  if (backend_ == Backend::CUDA) {
#if PM_ENABLE_CUDA
    if (!cuda::is_available()) {
      throw std::runtime_error("CUDA backend requested but not available.");
    }
    auto& particles = system.particles();
    cuda::step_dem(particles.data(), particles.size(), config, clamped_substeps);
    return;
#else
    throw std::runtime_error("CUDA backend requested but this build has PM_ENABLE_CUDA=0.");
#endif
  }

  step_cpu(system, config, clamped_substeps);
}

void DEMSolver::step_cpu(ParticleSystem& system, const SimulationConfig& config, int substeps) {
  auto& particles = system.particles();
  if (particles.empty()) {
    return;
  }

  const float dt = config.dt / static_cast<float>(substeps);
  const float epsilon = std::max(config.dem_contact_epsilon, 1.0e-7F);

  float max_radius = 0.0F;
  for (const auto& particle : particles) {
    max_radius = std::max(max_radius, particle.radius);
  }
  const float cell_size = std::max(max_radius * 2.2F, 1.0e-4F);
  const float inv_cell_size = 1.0F / cell_size;

  std::vector<Vec2> forces(particles.size());
  std::unordered_map<CellCoord, std::vector<std::size_t>, CellCoordHash> cell_list;
  cell_list.reserve(particles.size() * 2);

  for (int s = 0; s < substeps; ++s) {
    for (std::size_t i = 0; i < particles.size(); ++i) {
      forces[i] = Vec2 {0.0F, particles[i].mass * config.gravity_y};
    }

    cell_list.clear();
    for (std::size_t i = 0; i < particles.size(); ++i) {
      const CellCoord cell {
          to_cell(particles[i].position.x, inv_cell_size),
          to_cell(particles[i].position.y, inv_cell_size),
      };
      cell_list[cell].push_back(i);
    }

    for (const auto& [base_cell, indices] : cell_list) {
      for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
          const CellCoord neighbor_cell {base_cell.x + dx, base_cell.y + dy};
          const auto neighbor_it = cell_list.find(neighbor_cell);
          if (neighbor_it == cell_list.end()) {
            continue;
          }

          const auto& neighbor_indices = neighbor_it->second;
          for (std::size_t i_idx = 0; i_idx < indices.size(); ++i_idx) {
            const std::size_t i = indices[i_idx];
            for (std::size_t j_idx = 0; j_idx < neighbor_indices.size(); ++j_idx) {
              const std::size_t j = neighbor_indices[j_idx];
              if (j <= i) {
                continue;
              }

              auto& pi = particles[i];
              auto& pj = particles[j];

              const Vec2 delta = pj.position - pi.position;
              const float distance_sq = length_squared(delta);
              const float contact_radius = pi.radius + pj.radius;
              if (distance_sq >= contact_radius * contact_radius) {
                continue;
              }

              const float distance = std::sqrt(std::max(distance_sq, epsilon));
              const Vec2 normal = (distance > epsilon) ? (delta * (1.0F / distance)) : Vec2 {1.0F, 0.0F};
              const float overlap = contact_radius - distance;

              const Vec2 relative_velocity = pj.velocity - pi.velocity;
              const float normal_velocity = dot(relative_velocity, normal);

              float normal_force_mag =
                  config.dem_normal_stiffness * overlap - config.dem_normal_damping * normal_velocity;
              normal_force_mag = std::max(normal_force_mag, 0.0F);
              const Vec2 normal_force = normal * normal_force_mag;

              const Vec2 tangential_velocity = relative_velocity - normal * normal_velocity;
              const float tangential_speed = length(tangential_velocity);

              Vec2 tangential_force {0.0F, 0.0F};
              if (tangential_speed > epsilon) {
                const Vec2 tangent = tangential_velocity * (1.0F / tangential_speed);
                const float viscous_mag = config.dem_tangential_damping * tangential_speed;
                const float coulomb_limit = config.dem_tangential_friction * normal_force_mag;
                const float tangential_force_mag = std::min(viscous_mag, coulomb_limit);
                tangential_force = tangent * tangential_force_mag;
              }

              // Contact force acts in opposite directions on each particle pair.
              forces[i] += tangential_force - normal_force;
              forces[j] += normal_force - tangential_force;
            }
          }
        }
      }
    }

#if PM_HAS_OPENMP
#pragma omp parallel for
#endif
    for (std::size_t i = 0; i < particles.size(); ++i) {
      auto& particle = particles[i];

      const float inv_mass = particle.mass > epsilon ? (1.0F / particle.mass) : 0.0F;
      particle.velocity += forces[i] * (inv_mass * dt);
      particle.position += particle.velocity * dt;

      resolve_boundaries(particle, config);
    }
  }
}

}  // namespace pm
