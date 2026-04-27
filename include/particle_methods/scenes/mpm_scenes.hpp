#pragma once

#include <cstdint>
#include <string_view>

#include "particle_methods/core/particle_system.hpp"

namespace pm::scenes {

enum class MPMBenchmarkScene : int {
  FallingBlock = 0,
};

[[nodiscard]] MPMBenchmarkScene parse_mpm_scene(std::string_view scene_name);
[[nodiscard]] std::string_view to_string(MPMBenchmarkScene scene);

void initialize_mpm_scene(
    ParticleSystem& system,
    SimulationConfig& config,
    MPMBenchmarkScene scene,
    int particle_count,
    std::uint32_t seed);

}  // namespace pm::scenes
