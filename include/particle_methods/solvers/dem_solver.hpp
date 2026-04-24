#pragma once

#include "particle_methods/solvers/solver_base.hpp"

namespace pm {

class DEMSolver final : public SolverBase {
 public:
  enum class Backend {
    CPU,
    CUDA,
  };

  explicit DEMSolver(Backend backend = Backend::CPU);

  [[nodiscard]] std::string name() const override;
  void step(ParticleSystem& system, const SimulationConfig& config, int substeps = 1) override;

 private:
  void step_cpu(ParticleSystem& system, const SimulationConfig& config, int substeps);

  Backend backend_;
};

}  // namespace pm
