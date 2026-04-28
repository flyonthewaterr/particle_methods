#include <cmath>
#include <iostream>
#include <stdexcept>

#include "particle_methods/scenes/sph_scenes.hpp"
#include "particle_methods/solvers/sph_solver.hpp"

namespace {

bool almost_equal(float a, float b, float tolerance = 1.0e-5F) {
  return std::fabs(a - b) <= tolerance;
}

void expect(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_deterministic_scene_initialization() {
  pm::ParticleSystem system_a;
  pm::ParticleSystem system_b;
  pm::SimulationConfig config_a;
  pm::SimulationConfig config_b;

  pm::scenes::initialize_sph_scene(system_a, config_a, pm::scenes::SPHBenchmarkScene::DamBreak, 120, 1337U);
  pm::scenes::initialize_sph_scene(system_b, config_b, pm::scenes::SPHBenchmarkScene::DamBreak, 120, 1337U);

  expect(system_a.size() == system_b.size(), "Deterministic SPH scene size mismatch");
  for (std::size_t i = 0; i < system_a.size(); ++i) {
    expect(almost_equal(system_a.particles()[i].position.x, system_b.particles()[i].position.x),
           "Deterministic SPH scene x mismatch");
    expect(almost_equal(system_a.particles()[i].position.y, system_b.particles()[i].position.y),
           "Deterministic SPH scene y mismatch");
  }
}

void test_pressure_repulsion() {
  pm::ParticleSystem system;

  pm::Particle a;
  a.position = pm::Vec2 {0.0F, 0.0F};
  a.velocity = pm::Vec2 {0.0F, 0.0F};
  a.radius = 0.02F;
  a.mass = 1.0F;

  pm::Particle b;
  b.position = pm::Vec2 {0.03F, 0.0F};
  b.velocity = pm::Vec2 {0.0F, 0.0F};
  b.radius = 0.02F;
  b.mass = 1.0F;

  system.add_particle(a);
  system.add_particle(b);

  pm::SimulationConfig config;
  config.gravity_y = 0.0F;
  config.dt = 0.001F;
  config.boundary_mode = pm::BoundaryMode::Walls;
  config.bounds_left = -1.0F;
  config.bounds_right = 1.0F;
  config.floor_y = -1.0F;
  config.ceiling_y = 1.0F;
  config.sph_smoothing_length = 0.1F;
  config.sph_rest_density = 1.0F;
  config.sph_pressure_stiffness = 12.0F;
  config.sph_viscosity = 0.0F;
  config.sph_cfl_factor = 0.0F;
  config.sph_boundary_stiffness = 0.0F;
  config.sph_boundary_damping = 0.0F;

  pm::SPHSolver solver;
  solver.step(system, config, 1);

  expect(system.particles()[0].velocity.x < 0.0F, "Left particle should move left due to pressure");
  expect(system.particles()[1].velocity.x > 0.0F, "Right particle should move right due to pressure");
}

void test_boundary_force() {
  pm::ParticleSystem system;

  pm::Particle particle;
  particle.position = pm::Vec2 {-0.99F, 0.0F};
  particle.velocity = pm::Vec2 {-0.2F, 0.0F};
  particle.radius = 0.02F;
  particle.mass = 1.0F;

  system.add_particle(particle);

  pm::SimulationConfig config;
  config.gravity_y = 0.0F;
  config.dt = 0.001F;
  config.boundary_mode = pm::BoundaryMode::Walls;
  config.bounds_left = -1.0F;
  config.bounds_right = 1.0F;
  config.floor_y = -1.0F;
  config.ceiling_y = 1.0F;
  config.sph_smoothing_length = 0.1F;
  config.sph_rest_density = 1.0F;
  config.sph_pressure_stiffness = 0.0F;
  config.sph_viscosity = 0.0F;
  config.sph_cfl_factor = 0.0F;
  config.sph_boundary_stiffness = 5000.0F;
  config.sph_boundary_damping = 20.0F;

  pm::SPHSolver solver;
  solver.step(system, config, 1);

  expect(system.particles()[0].velocity.x > 0.0F, "Boundary force should push particle away from wall");
}

}  // namespace

int main() {
  try {
    test_deterministic_scene_initialization();
    test_pressure_repulsion();
    test_boundary_force();
  } catch (const std::exception& error) {
    std::cerr << "sph_phase2_tests failed: " << error.what() << '\n';
    return 1;
  }

  std::cout << "sph_phase2_tests passed\n";
  return 0;
}
