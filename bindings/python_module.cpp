#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "particle_methods/core/particle_system.hpp"
#include "particle_methods/scenes/dem_scenes.hpp"
#include "particle_methods/scenes/mpm_scenes.hpp"
#include "particle_methods/solvers/dem_solver.hpp"
#include "particle_methods/solvers/mpm_solver.hpp"

namespace py = pybind11;

namespace {

std::vector<std::array<float, 2>> gather_positions(const pm::ParticleSystem& system) {
  const auto& particles = system.particles();
  std::vector<std::array<float, 2>> result;
  result.reserve(particles.size());

  for (const auto& particle : particles) {
    result.push_back({particle.position.x, particle.position.y});
  }

  return result;
}

class FallingParticles2D {
 public:
  FallingParticles2D(int particle_count, float spacing, float start_height, bool use_cuda)
      : solver_(use_cuda ? pm::DEMSolver::Backend::CUDA : pm::DEMSolver::Backend::CPU) {
    if (particle_count <= 0) {
      throw std::invalid_argument("particle_count must be > 0");
    }
    system_.reserve(static_cast<std::size_t>(particle_count));
    reset_particles(particle_count, spacing, start_height);
  }

  void step(int steps, int substeps) {
    const int clamped_steps = std::max(steps, 1);
    const int clamped_substeps = std::max(substeps, 1);
    for (int i = 0; i < clamped_steps; ++i) {
      solver_.step(system_, config_, clamped_substeps);
    }
  }

  void set_dt(float dt) {
    config_.dt = dt;
  }

  void set_gravity(float gravity_y) {
    config_.gravity_y = gravity_y;
  }

  [[nodiscard]] std::vector<std::array<float, 2>> positions() const {
    return gather_positions(system_);
  }

  [[nodiscard]] py::ssize_t particle_count() const {
    return static_cast<py::ssize_t>(system_.size());
  }

  void reset_scene(const std::string& scene_name, int particle_count, std::uint32_t seed = 42U) {
    const auto scene = pm::scenes::parse_dem_scene(scene_name);
    pm::scenes::initialize_dem_scene(system_, config_, scene, particle_count, seed);
  }

 private:
  void reset_particles(int particle_count, float spacing, float start_height) {
    system_.clear();

    const int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(particle_count))));
    const float start_x = -0.6F;

    for (int i = 0; i < particle_count; ++i) {
      const int col = i % columns;
      const int row = i / columns;

      pm::Particle p;
      p.position.x = start_x + static_cast<float>(col) * spacing;
      p.position.y = start_height + static_cast<float>(row) * spacing;
      p.radius = 0.02F;
      p.mass = 1.0F;
      system_.add_particle(p);
    }
  }

  pm::ParticleSystem system_;
  pm::SimulationConfig config_;
  pm::DEMSolver solver_;
};

std::vector<std::array<float, 2>> run_falling_particles(int particle_count, int steps, float dt, bool use_cuda) {
  FallingParticles2D simulation(particle_count, 0.06F, 0.2F, use_cuda);
  simulation.set_dt(dt);
  simulation.step(steps, 4);
  return simulation.positions();
}

std::vector<std::array<float, 2>> run_dem_scene(
    const std::string& scene_name,
    int particle_count,
    int steps,
    std::uint32_t seed,
    bool use_cuda,
    int substeps) {
  pm::ParticleSystem system;
  pm::SimulationConfig config;
  pm::scenes::initialize_dem_scene(system, config, pm::scenes::parse_dem_scene(scene_name), particle_count, seed);

  pm::DEMSolver solver(use_cuda ? pm::DEMSolver::Backend::CUDA : pm::DEMSolver::Backend::CPU);
  for (int s = 0; s < std::max(steps, 1); ++s) {
    solver.step(system, config, std::max(substeps, 1));
  }

  return gather_positions(system);
}

std::vector<std::array<float, 2>> run_mpm_scene(
    const std::string& scene_name,
    int particle_count,
    int steps,
    std::uint32_t seed,
    int substeps) {
  pm::ParticleSystem system;
  pm::SimulationConfig config;
  pm::scenes::initialize_mpm_scene(system, config, pm::scenes::parse_mpm_scene(scene_name), particle_count, seed);

  pm::MPMSolver solver;
  for (int s = 0; s < std::max(steps, 1); ++s) {
    solver.step(system, config, std::max(substeps, 1));
  }

  return gather_positions(system);
}

}  // namespace

PYBIND11_MODULE(_core, m) {
  m.doc() = "particle_methods Python bindings";

  py::class_<FallingParticles2D>(m, "FallingParticles2D")
      .def(py::init<int, float, float, bool>(), py::arg("particle_count"), py::arg("spacing") = 0.06F,
           py::arg("start_height") = 0.2F, py::arg("use_cuda") = false)
      .def("step", &FallingParticles2D::step, py::arg("steps") = 1, py::arg("substeps") = 4)
      .def("set_dt", &FallingParticles2D::set_dt, py::arg("dt"))
      .def("set_gravity", &FallingParticles2D::set_gravity, py::arg("gravity_y"))
      .def("reset_scene", &FallingParticles2D::reset_scene, py::arg("scene_name"), py::arg("particle_count"),
           py::arg("seed") = 42U)
      .def("positions", &FallingParticles2D::positions)
      .def_property_readonly("particle_count", &FallingParticles2D::particle_count);

  m.def("run_falling_particles", &run_falling_particles, py::arg("particle_count") = 200, py::arg("steps") = 400,
        py::arg("dt") = 0.004F, py::arg("use_cuda") = false,
        "Run a baseline 2D falling-particles DEM simulation and return [[x, y], ...] positions.");

  m.def("run_dem_scene", &run_dem_scene, py::arg("scene_name") = "pile_formation", py::arg("particle_count") = 200,
         py::arg("steps") = 400, py::arg("seed") = 42U, py::arg("use_cuda") = false, py::arg("substeps") = 2,
         "Run a deterministic DEM benchmark scene (single_bounce, pile_formation, hopper). ");

  m.def("run_mpm_scene", &run_mpm_scene, py::arg("scene_name") = "falling_block", py::arg("particle_count") = 196,
        py::arg("steps") = 300, py::arg("seed") = 42U, py::arg("substeps") = 1,
        "Run a concise MPM benchmark scene (falling_block).");
}
