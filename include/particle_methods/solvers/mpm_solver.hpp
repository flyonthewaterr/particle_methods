#pragma once

#include "particle_methods/solvers/solver_base.hpp"

namespace pm {

class MPMSolver final : public SolverBase {
 public:
  [[nodiscard]] std::string name() const override;
  void step(ParticleSystem& system, const SimulationConfig& config, int substeps = 1) override;
};

}  // namespace pm
