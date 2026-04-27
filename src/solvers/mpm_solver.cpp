#include "particle_methods/solvers/mpm_solver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

#include "particle_methods/core/vec2.hpp"

namespace {

struct GridNode {
  float mass {0.0F};
  pm::Vec2 velocity {};
  pm::Vec2 internal_force {};
};

struct GridIndex {
  int x {0};
  int y {0};
};

float clamp01(float value) {
  return std::clamp(value, 0.0F, 1.0F);
}

void apply_boundary(pm::Vec2& node_position, pm::Vec2& velocity, const pm::SimulationConfig& config) {
  if (node_position.y <= config.floor_y && velocity.y < 0.0F) {
    velocity.y = -velocity.y * config.restitution;
    velocity.x *= (1.0F - clamp01(config.boundary_friction));
  }

  if (node_position.x <= config.bounds_left && velocity.x < 0.0F) {
    velocity.x = -velocity.x * config.restitution;
    velocity.y *= (1.0F - clamp01(config.boundary_friction));
  }

  if (node_position.x >= config.bounds_right && velocity.x > 0.0F) {
    velocity.x = -velocity.x * config.restitution;
    velocity.y *= (1.0F - clamp01(config.boundary_friction));
  }

  if (node_position.y >= config.ceiling_y && velocity.y > 0.0F) {
    velocity.y = -velocity.y * config.restitution;
    velocity.x *= (1.0F - clamp01(config.boundary_friction));
  }
}

}  // namespace

namespace pm {

std::string MPMSolver::name() const {
  return "MPM (CPU)";
}

void MPMSolver::ensure_particle_state(const ParticleSystem& system, const SimulationConfig& config) {
  if (particle_state_.size() == system.size()) {
    return;
  }

  particle_state_.clear();
  particle_state_.resize(system.size());

  const float min_density = std::max(config.mpm_reference_density, 1.0F);
  const auto& particles = system.particles();
  for (std::size_t i = 0; i < particles.size(); ++i) {
    particle_state_[i].volume0 = std::max(particles[i].mass / min_density, 1.0e-6F);
  }
}

void MPMSolver::step(ParticleSystem& system, const SimulationConfig& config, int substeps) {
  auto& particles = system.particles();
  if (particles.empty()) {
    return;
  }

  ensure_particle_state(system, config);

  const int clamped_substeps = std::max(substeps, 1);
  const float dt = config.dt / static_cast<float>(clamped_substeps);
  const float h = std::max(config.mpm_grid_spacing, 1.0e-4F);
  const float inv_h = 1.0F / h;
  const float min_mass = 1.0e-7F;
  const float poisson = std::clamp(config.mpm_poissons_ratio, 0.0F, 0.49F);
  const float young = std::max(config.mpm_youngs_modulus, 0.0F);
  const float lambda = (young * poisson) / ((1.0F + poisson) * (1.0F - 2.0F * poisson));
  const float mu = young / (2.0F * (1.0F + poisson));

  for (int substep = 0; substep < clamped_substeps; ++substep) {
    float min_x = particles.front().position.x;
    float max_x = particles.front().position.x;
    float min_y = particles.front().position.y;
    float max_y = particles.front().position.y;
    for (const auto& particle : particles) {
      min_x = std::min(min_x, particle.position.x);
      max_x = std::max(max_x, particle.position.x);
      min_y = std::min(min_y, particle.position.y);
      max_y = std::max(max_y, particle.position.y);
    }

    min_x -= 2.0F * h;
    max_x += 2.0F * h;
    min_y -= 2.0F * h;
    max_y += 2.0F * h;

    const int nx = std::max(3, static_cast<int>(std::ceil((max_x - min_x) * inv_h)) + 1);
    const int ny = std::max(3, static_cast<int>(std::ceil((max_y - min_y) * inv_h)) + 1);
    const int node_count = nx * ny;

    std::vector<GridNode> grid(static_cast<std::size_t>(node_count));
    auto grid_index = [nx, ny](int ix, int iy) {
      const int cx = std::clamp(ix, 0, nx - 1);
      const int cy = std::clamp(iy, 0, ny - 1);
      return static_cast<std::size_t>(cy * nx + cx);
    };

    std::vector<std::array<GridIndex, 4>> stencils(particles.size());
    std::vector<std::array<float, 4>> weights(particles.size());
    std::vector<std::array<Vec2, 4>> gradients(particles.size());

    for (std::size_t p_idx = 0; p_idx < particles.size(); ++p_idx) {
      const auto& particle = particles[p_idx];
      const float gx = (particle.position.x - min_x) * inv_h;
      const float gy = (particle.position.y - min_y) * inv_h;
      const int ix = static_cast<int>(std::floor(gx));
      const int iy = static_cast<int>(std::floor(gy));
      const float tx = gx - static_cast<float>(ix);
      const float ty = gy - static_cast<float>(iy);

      stencils[p_idx] = {
          GridIndex {ix, iy},
          GridIndex {ix + 1, iy},
          GridIndex {ix, iy + 1},
          GridIndex {ix + 1, iy + 1},
      };

      weights[p_idx] = {
          (1.0F - tx) * (1.0F - ty),
          tx * (1.0F - ty),
          (1.0F - tx) * ty,
          tx * ty,
      };

      gradients[p_idx] = {
          Vec2 {-(1.0F - ty) * inv_h, -(1.0F - tx) * inv_h},
          Vec2 {(1.0F - ty) * inv_h, -tx * inv_h},
          Vec2 {-ty * inv_h, (1.0F - tx) * inv_h},
          Vec2 {ty * inv_h, tx * inv_h},
      };

      for (int k = 0; k < 4; ++k) {
        auto& node = grid[grid_index(stencils[p_idx][k].x, stencils[p_idx][k].y)];
        const float weighted_mass = particle.mass * weights[p_idx][k];
        node.mass += weighted_mass;
        node.velocity += particle.velocity * weighted_mass;
      }
    }

    for (auto& node : grid) {
      if (node.mass > min_mass) {
        node.velocity *= (1.0F / node.mass);
      }
    }

    for (std::size_t p_idx = 0; p_idx < particles.size(); ++p_idx) {
      const auto& particle_state = particle_state_[p_idx];
      const float trace = particle_state.exx + particle_state.eyy;
      const float sxx = lambda * trace + 2.0F * mu * particle_state.exx;
      const float syy = lambda * trace + 2.0F * mu * particle_state.eyy;
      const float sxy = 2.0F * mu * particle_state.exy;

      for (int k = 0; k < 4; ++k) {
        const Vec2 grad = gradients[p_idx][k];
        Vec2 sigma_grad {
            sxx * grad.x + sxy * grad.y,
            sxy * grad.x + syy * grad.y,
        };
        auto& node = grid[grid_index(stencils[p_idx][k].x, stencils[p_idx][k].y)];
        node.internal_force -= sigma_grad * (particle_state.volume0 * weights[p_idx][k]);
      }
    }

    for (int iy = 0; iy < ny; ++iy) {
      for (int ix = 0; ix < nx; ++ix) {
        auto& node = grid[grid_index(ix, iy)];
        if (node.mass <= min_mass) {
          continue;
        }

        node.velocity += (node.internal_force * (dt / node.mass));
        node.velocity.y += config.gravity_y * dt;

        Vec2 node_position {
            min_x + static_cast<float>(ix) * h,
            min_y + static_cast<float>(iy) * h,
        };
        apply_boundary(node_position, node.velocity, config);
      }
    }

    for (std::size_t p_idx = 0; p_idx < particles.size(); ++p_idx) {
      auto& particle = particles[p_idx];
      Vec2 next_velocity {0.0F, 0.0F};
      float lxx = 0.0F;
      float lxy = 0.0F;
      float lyx = 0.0F;
      float lyy = 0.0F;

      for (int k = 0; k < 4; ++k) {
        const auto& node = grid[grid_index(stencils[p_idx][k].x, stencils[p_idx][k].y)];
        next_velocity += node.velocity * weights[p_idx][k];

        const Vec2 grad = gradients[p_idx][k];
        lxx += node.velocity.x * grad.x;
        lxy += node.velocity.x * grad.y;
        lyx += node.velocity.y * grad.x;
        lyy += node.velocity.y * grad.y;
      }

      particle_state_[p_idx].exx += lxx * dt;
      particle_state_[p_idx].eyy += lyy * dt;
      particle_state_[p_idx].exy += 0.5F * (lxy + lyx) * dt;

      particle.velocity = next_velocity;
      particle.position += particle.velocity * dt;
    }
  }
}

}  // namespace pm
