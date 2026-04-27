#include "particle_methods/scenes/mpm_scenes.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <string>

namespace pm::scenes {

namespace {

int clamp_particle_count(int particle_count) {
  return std::max(particle_count, 1);
}

}  // namespace

MPMBenchmarkScene parse_mpm_scene(std::string_view scene_name) {
  if (scene_name == "falling_block" || scene_name == "falling-block" || scene_name == "block") {
    return MPMBenchmarkScene::FallingBlock;
  }

  throw std::invalid_argument("Unknown MPM scene name: " + std::string(scene_name));
}

std::string_view to_string(MPMBenchmarkScene scene) {
  switch (scene) {
    case MPMBenchmarkScene::FallingBlock:
      return "falling_block";
  }

  throw std::invalid_argument("Unknown MPM scene enum value.");
}

void initialize_mpm_scene(
    ParticleSystem& system,
    SimulationConfig& config,
    MPMBenchmarkScene scene,
    int particle_count,
    std::uint32_t seed) {
  const int count = clamp_particle_count(particle_count);
  system.clear();
  system.reserve(static_cast<std::size_t>(count));

  config.dt = 0.002F;
  config.gravity_y = -9.81F;
  config.floor_y = -0.9F;
  config.bounds_left = -1.0F;
  config.bounds_right = 1.0F;
  config.ceiling_y = 1.0F;
  config.restitution = 0.1F;
  config.boundary_friction = 0.05F;
  config.mpm_grid_spacing = 0.04F;
  config.mpm_reference_density = 1000.0F;
  config.mpm_youngs_modulus = 25000.0F;
  config.mpm_poissons_ratio = 0.2F;

  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> jitter(-0.001F, 0.001F);

  switch (scene) {
    case MPMBenchmarkScene::FallingBlock: {
      const float spacing = 0.032F;
      const int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));
      const float width = static_cast<float>(columns - 1) * spacing;
      const float start_x = -0.5F * width;
      const float start_y = 0.25F;

      for (int i = 0; i < count; ++i) {
        const int col = i % columns;
        const int row = i / columns;

        Particle particle;
        particle.position.x = start_x + static_cast<float>(col) * spacing + jitter(rng);
        particle.position.y = start_y + static_cast<float>(row) * spacing + jitter(rng);
        particle.velocity = Vec2 {0.0F, 0.0F};
        particle.mass = 0.8F;
        particle.radius = 0.012F;
        system.add_particle(particle);
      }
      break;
    }
  }
}

}  // namespace pm::scenes
