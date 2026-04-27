#pragma once

#include <vector>

#include "particle_methods/solvers/solver_base.hpp"

namespace pm {

class MPMSolver final : public SolverBase {
 public:
  [[nodiscard]] std::string name() const override;
  void step(ParticleSystem& system, const SimulationConfig& config, int substeps = 1) override;

 private:
  struct ParticleState {
    float volume0 {0.0F};
    float exx {0.0F};
    float eyy {0.0F};
    float exy {0.0F};
  };

  void ensure_particle_state(const ParticleSystem& system, const SimulationConfig& config);

  std::vector<ParticleState> particle_state_;
};

}  // namespace pm
