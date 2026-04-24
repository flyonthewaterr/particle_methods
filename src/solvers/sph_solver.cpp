#include "particle_methods/solvers/sph_solver.hpp"

#include <stdexcept>

namespace pm {

std::string SPHSolver::name() const {
  return "SPH";
}

void SPHSolver::step(ParticleSystem&, const SimulationConfig&, int) {
  throw std::runtime_error("SPH solver is not implemented yet. See docs/ROADMAP.md for milestones.");
}

}  // namespace pm
