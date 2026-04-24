#pragma once

#include <cstdint>
#include <string_view>

#include "particle_methods/core/particle_system.hpp"

namespace pm::scenes {

enum class DEMBenchmarkScene : int {
  SingleBounce = 0,
  PileFormation = 1,
  Hopper = 2,
};

[[nodiscard]] DEMBenchmarkScene parse_dem_scene(std::string_view scene_name);
[[nodiscard]] std::string_view to_string(DEMBenchmarkScene scene);

void initialize_dem_scene(
    ParticleSystem& system,
    SimulationConfig& config,
    DEMBenchmarkScene scene,
    int particle_count,
    std::uint32_t seed);

}  // namespace pm::scenes
