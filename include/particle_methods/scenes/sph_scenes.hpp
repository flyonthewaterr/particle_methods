#pragma once

#include <cstdint>
#include <string_view>

#include "particle_methods/core/particle_system.hpp"

namespace pm::scenes {

enum class SPHBenchmarkScene : int {
  DamBreak = 0,
  Droplet = 1,
};

[[nodiscard]] SPHBenchmarkScene parse_sph_scene(std::string_view scene_name);
[[nodiscard]] std::string_view to_string(SPHBenchmarkScene scene);

void initialize_sph_scene(
    ParticleSystem& system,
    SimulationConfig& config,
    SPHBenchmarkScene scene,
    int particle_count,
    std::uint32_t seed);

}  // namespace pm::scenes
