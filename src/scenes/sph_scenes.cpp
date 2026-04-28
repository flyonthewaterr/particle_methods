#include "particle_methods/scenes/sph_scenes.hpp"

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

void apply_sph_defaults(SimulationConfig& config, float spacing) {
  config.gravity_y = -9.81F;
  config.dt = 0.0015F;
  config.bounds_left = -1.0F;
  config.bounds_right = 1.0F;
  config.floor_y = -0.9F;
  config.ceiling_y = 0.9F;
  config.boundary_mode = BoundaryMode::Box;

  config.sph_rest_density = 1000.0F;
  config.sph_smoothing_length = std::max(spacing * 2.0F, 1.0e-4F);
  config.sph_pressure_stiffness = 2000.0F;
  config.sph_viscosity = 0.1F;
  config.sph_sound_speed = 20.0F;
  config.sph_cfl_factor = 0.4F;
  config.sph_boundary_stiffness = 3000.0F;
  config.sph_boundary_damping = 25.0F;
}

void add_particle_grid(
    ParticleSystem& system,
    int count,
    float spacing,
    float start_x,
    float start_y,
    std::mt19937& rng) {
  const int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count))));
  const float jitter_scale = spacing * 0.15F;
  std::uniform_real_distribution<float> jitter(-jitter_scale, jitter_scale);

  for (int i = 0; i < count; ++i) {
    const int col = i % columns;
    const int row = i / columns;

    Particle particle;
    particle.position.x = start_x + static_cast<float>(col) * spacing + jitter(rng);
    particle.position.y = start_y + static_cast<float>(row) * spacing + jitter(rng);
    particle.velocity = Vec2 {0.0F, 0.0F};
    particle.radius = spacing * 0.45F;
    particle.mass = 1.0F;
    system.add_particle(particle);
  }
}

}  // namespace

SPHBenchmarkScene parse_sph_scene(std::string_view scene_name) {
  if (scene_name == "dam_break" || scene_name == "dam-break") {
    return SPHBenchmarkScene::DamBreak;
  }

  if (scene_name == "droplet" || scene_name == "falling_droplet" || scene_name == "falling-droplet") {
    return SPHBenchmarkScene::Droplet;
  }

  throw std::invalid_argument("Unknown SPH scene name: " + std::string(scene_name));
}

std::string_view to_string(SPHBenchmarkScene scene) {
  switch (scene) {
    case SPHBenchmarkScene::DamBreak:
      return "dam_break";
    case SPHBenchmarkScene::Droplet:
      return "droplet";
  }

  return "dam_break";
}

void initialize_sph_scene(
    ParticleSystem& system,
    SimulationConfig& config,
    SPHBenchmarkScene scene,
    int particle_count,
    std::uint32_t seed) {
  const int count = clamp_particle_count(particle_count);
  system.clear();
  system.reserve(static_cast<std::size_t>(count));

  std::mt19937 rng(seed);

  switch (scene) {
    case SPHBenchmarkScene::DamBreak: {
      const float spacing = 0.045F;
      apply_sph_defaults(config, spacing);

      const float mass = config.sph_rest_density * spacing * spacing;
      const float start_x = config.bounds_left + 0.12F;
      const float start_y = config.floor_y + 0.12F;

      add_particle_grid(system, count, spacing, start_x, start_y, rng);
      for (auto& particle : system.particles()) {
        particle.mass = mass;
      }
      break;
    }

    case SPHBenchmarkScene::Droplet: {
      const float spacing = 0.04F;
      apply_sph_defaults(config, spacing);
      config.dt = 0.001F;

      const float mass = config.sph_rest_density * spacing * spacing;
      const float center_x = 0.0F;
      const float center_y = 0.35F;
      const float radius = 0.18F;

      const int grid = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(count) * 1.4F)));
      const float jitter_scale = spacing * 0.1F;
      std::uniform_real_distribution<float> jitter(-jitter_scale, jitter_scale);

      int added = 0;
      for (int row = -grid; row <= grid && added < count; ++row) {
        for (int col = -grid; col <= grid && added < count; ++col) {
          const float x = center_x + static_cast<float>(col) * spacing;
          const float y = center_y + static_cast<float>(row) * spacing;
          const float dx = x - center_x;
          const float dy = y - center_y;
          if ((dx * dx + dy * dy) > radius * radius) {
            continue;
          }

          Particle particle;
          particle.position.x = x + jitter(rng);
          particle.position.y = y + jitter(rng);
          particle.velocity = Vec2 {0.0F, 0.0F};
          particle.radius = spacing * 0.45F;
          particle.mass = mass;
          system.add_particle(particle);
          ++added;
        }
      }

      while (added < count) {
        Particle particle;
        particle.position.x = center_x + jitter(rng) * 2.0F;
        particle.position.y = center_y + jitter(rng) * 2.0F;
        particle.velocity = Vec2 {0.0F, 0.0F};
        particle.radius = spacing * 0.45F;
        particle.mass = mass;
        system.add_particle(particle);
        ++added;
      }
      break;
    }
  }
}

}  // namespace pm::scenes
