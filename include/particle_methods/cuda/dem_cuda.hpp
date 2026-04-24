#pragma once

#include <cstddef>

#include "particle_methods/core/particle.hpp"

namespace pm::cuda {

[[nodiscard]] bool is_available();
void step_dem(Particle* particles, std::size_t count, const SimulationConfig& config, int substeps = 1);

}  // namespace pm::cuda
