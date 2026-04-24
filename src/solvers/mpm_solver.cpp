#include "particle_methods/solvers/mpm_solver.hpp"

#include <stdexcept>

namespace pm {

std::string MPMSolver::name() const {
  return "MPM";
}

void MPMSolver::step(ParticleSystem&, const SimulationConfig&, int) {
  throw std::runtime_error("MPM solver is not implemented yet. See docs/ROADMAP.md for milestones.");
}

}  // namespace pm
