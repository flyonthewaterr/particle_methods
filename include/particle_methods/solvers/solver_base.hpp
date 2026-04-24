#pragma once

#include <string>

#include "particle_methods/core/particle_system.hpp"

namespace pm {

class SolverBase {
 public:
  virtual ~SolverBase() = default;

  [[nodiscard]] virtual std::string name() const = 0;
  virtual void step(ParticleSystem& system, const SimulationConfig& config, int substeps = 1) = 0;
};

}  // namespace pm
