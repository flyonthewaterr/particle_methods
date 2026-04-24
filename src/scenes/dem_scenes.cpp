#include "particle_methods/scenes/dem_scenes.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <stdexcept>

namespace pm::scenes {

namespace {

int clamp_particle_count(int particle_count) {
  return std::max(particle_count, 1);
}

}  // namespace

DEMBenchmarkScene parse_dem_scene(std::string_view scene_name) {
  if (scene_name == "single_bounce" || scene_name == "single-bounce") {
    return DEMBenchmarkScene::SingleBounce;
  }

  if (scene_name == "pile" || scene_name == "pile_formation" || scene_name == "pile-formation") {
    return DEMBenchmarkScene::PileFormation;
  }

  if (scene_name == "hopper") {
    return DEMBenchmarkScene::Hopper;
  }

  throw std::invalid_argument("Unknown DEM scene name: " + std::string(scene_name));
}

std::string_view to_string(DEMBenchmarkScene scene) {
  switch (scene) {
    case DEMBenchmarkScene::SingleBounce:
      return "single_bounce";
    case DEMBenchmarkScene::PileFormation:
      return "pile_formation";
    case DEMBenchmarkScene::Hopper:
      return "hopper";
  }

  return "single_bounce";
}

void initialize_dem_scene(
    ParticleSystem& system,
    SimulationConfig& config,
    DEMBenchmarkScene scene,
    int particle_count,
    std::uint32_t seed) {
  const int count = clamp_particle_count(particle_count);
  system.clear();
  system.reserve(static_cast<std::size_t>(count));

  config.gravity_y = -9.81F;
  config.dt = 0.0025F;
  config.floor_y = -0.9F;
  config.restitution = 0.25F;
  config.bounds_left = -1.0F;
  config.bounds_right = 1.0F;
  config.ceiling_y = 1.15F;
  config.boundary_friction = 0.14F;
  config.dem_normal_stiffness = 9000.0F;
  config.dem_normal_damping = 45.0F;
  config.dem_tangential_friction = 0.45F;
  config.dem_tangential_damping = 30.0F;
  config.floor_opening_enabled = false;
  config.floor_opening_left = -0.15F;
  config.floor_opening_right = 0.15F;

  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> jitter(-0.0015F, 0.0015F);

  switch (scene) {
    case DEMBenchmarkScene::SingleBounce: {
      config.boundary_mode = BoundaryMode::Floor;

      for (int i = 0; i < count; ++i) {
        Particle particle;
        particle.position = Vec2 {0.0F, 0.65F + static_cast<float>(i) * 0.055F};
        particle.velocity = Vec2 {0.0F, -0.15F};
        particle.radius = 0.03F;
        particle.mass = 1.0F;
        system.add_particle(particle);
      }
      break;
    }

    case DEMBenchmarkScene::PileFormation: {
      config.boundary_mode = BoundaryMode::Box;

      const float spacing = 0.044F;
      const int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));
      const float width = static_cast<float>(columns - 1) * spacing;
      const float start_x = -0.5F * width;
      const float start_y = 0.12F;

      for (int i = 0; i < count; ++i) {
        const int col = i % columns;
        const int row = i / columns;

        Particle particle;
        particle.position.x = start_x + static_cast<float>(col) * spacing + jitter(rng);
        particle.position.y = start_y + static_cast<float>(row) * spacing + jitter(rng);
        particle.velocity = Vec2 {0.0F, 0.0F};
        particle.radius = 0.02F;
        particle.mass = 1.0F;
        system.add_particle(particle);
      }
      break;
    }

    case DEMBenchmarkScene::Hopper: {
      config.boundary_mode = BoundaryMode::Box;
      config.floor_opening_enabled = true;
      config.floor_opening_left = -0.16F;
      config.floor_opening_right = 0.16F;
      config.floor_y = -0.75F;

      const float spacing = 0.042F;
      const int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));
      const float width = static_cast<float>(columns - 1) * spacing;
      const float start_x = -0.5F * width;
      const float start_y = 0.35F;

      for (int i = 0; i < count; ++i) {
        const int col = i % columns;
        const int row = i / columns;

        Particle particle;
        particle.position.x = start_x + static_cast<float>(col) * spacing + jitter(rng);
        particle.position.y = start_y + static_cast<float>(row) * spacing + jitter(rng);
        particle.velocity = Vec2 {0.0F, 0.0F};
        particle.radius = 0.019F;
        particle.mass = 1.0F;
        system.add_particle(particle);
      }
      break;
    }
  }
}

}  // namespace pm::scenes
