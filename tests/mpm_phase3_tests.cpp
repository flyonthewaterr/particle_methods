#include <cmath>
#include <iostream>
#include <stdexcept>

#include "particle_methods/scenes/mpm_scenes.hpp"
#include "particle_methods/solvers/mpm_solver.hpp"

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

  pm::scenes::initialize_mpm_scene(system_a, config_a, pm::scenes::MPMBenchmarkScene::FallingBlock, 81, 77U);
  pm::scenes::initialize_mpm_scene(system_b, config_b, pm::scenes::MPMBenchmarkScene::FallingBlock, 81, 77U);

  expect(system_a.size() == system_b.size(), "Deterministic MPM scene size mismatch");
  for (std::size_t i = 0; i < system_a.size(); ++i) {
    expect(almost_equal(system_a.particles()[i].position.x, system_b.particles()[i].position.x),
           "Deterministic MPM scene x mismatch");
    expect(almost_equal(system_a.particles()[i].position.y, system_b.particles()[i].position.y),
           "Deterministic MPM scene y mismatch");
  }
}

void test_zero_gravity_static_block() {
  pm::ParticleSystem system;
  pm::SimulationConfig config;
  pm::scenes::initialize_mpm_scene(system, config, pm::scenes::MPMBenchmarkScene::FallingBlock, 16, 1U);

  config.gravity_y = 0.0F;
  config.mpm_youngs_modulus = 0.0F;

  const auto initial_positions = system.particles();

  pm::MPMSolver solver;
  solver.step(system, config, 6);

  for (std::size_t i = 0; i < system.size(); ++i) {
    expect(almost_equal(system.particles()[i].position.x, initial_positions[i].position.x, 2.0e-3F),
           "Particle x drifted in zero-gravity static test");
    expect(almost_equal(system.particles()[i].position.y, initial_positions[i].position.y, 2.0e-3F),
           "Particle y drifted in zero-gravity static test");
  }
}

void test_gravity_accelerates_downward() {
  pm::ParticleSystem system;
  pm::Particle particle;
  particle.position = pm::Vec2 {0.0F, 0.45F};
  particle.velocity = pm::Vec2 {0.0F, 0.0F};
  particle.mass = 1.0F;
  particle.radius = 0.01F;
  system.add_particle(particle);

  pm::SimulationConfig config;
  config.gravity_y = -9.81F;
  config.dt = 0.002F;
  config.floor_y = -1.0F;
  config.bounds_left = -1.0F;
  config.bounds_right = 1.0F;
  config.ceiling_y = 1.0F;
  config.mpm_grid_spacing = 0.03F;
  config.mpm_youngs_modulus = 0.0F;

  pm::MPMSolver solver;
  solver.step(system, config, 10);

  expect(system.particles()[0].velocity.y < -1.0e-3F, "Gravity should produce negative vertical velocity");
  expect(system.particles()[0].position.y < 0.45F, "Gravity should move particle downward");
}

}  // namespace

int main() {
  try {
    test_deterministic_scene_initialization();
    test_zero_gravity_static_block();
    test_gravity_accelerates_downward();
  } catch (const std::exception& error) {
    std::cerr << "mpm_phase3_tests failed: " << error.what() << '\n';
    return 1;
  }

  std::cout << "mpm_phase3_tests passed\n";
  return 0;
}
