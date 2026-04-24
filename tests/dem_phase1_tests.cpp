#include <cmath>
#include <iostream>
#include <stdexcept>

#include "particle_methods/scenes/dem_scenes.hpp"
#include "particle_methods/solvers/dem_solver.hpp"

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

  pm::scenes::initialize_dem_scene(system_a, config_a, pm::scenes::DEMBenchmarkScene::PileFormation, 120, 1337U);
  pm::scenes::initialize_dem_scene(system_b, config_b, pm::scenes::DEMBenchmarkScene::PileFormation, 120, 1337U);

  expect(system_a.size() == system_b.size(), "Deterministic scene size mismatch");
  for (std::size_t i = 0; i < system_a.size(); ++i) {
    expect(almost_equal(system_a.particles()[i].position.x, system_b.particles()[i].position.x),
           "Deterministic scene x mismatch");
    expect(almost_equal(system_a.particles()[i].position.y, system_b.particles()[i].position.y),
           "Deterministic scene y mismatch");
  }
}

void test_contact_repulsion() {
  pm::ParticleSystem system;

  pm::Particle a;
  a.position = pm::Vec2 {0.0F, 0.0F};
  a.velocity = pm::Vec2 {0.0F, 0.0F};
  a.radius = 0.03F;
  a.mass = 1.0F;

  pm::Particle b;
  b.position = pm::Vec2 {0.045F, 0.0F};
  b.velocity = pm::Vec2 {0.0F, 0.0F};
  b.radius = 0.03F;
  b.mass = 1.0F;

  system.add_particle(a);
  system.add_particle(b);

  pm::SimulationConfig config;
  config.gravity_y = 0.0F;
  config.dt = 0.002F;
  config.boundary_mode = pm::BoundaryMode::Walls;
  config.bounds_left = -2.0F;
  config.bounds_right = 2.0F;
  config.dem_normal_stiffness = 12000.0F;
  config.dem_normal_damping = 40.0F;
  config.dem_tangential_friction = 0.6F;

  const float initial_distance = pm::length(system.particles()[1].position - system.particles()[0].position);

  pm::DEMSolver solver;
  solver.step(system, config, 1);

  const float final_distance = pm::length(system.particles()[1].position - system.particles()[0].position);
  expect(final_distance > initial_distance, "Contact model did not separate overlapping particles");
  expect(system.particles()[0].velocity.x < 0.0F, "Left particle should move left after contact");
  expect(system.particles()[1].velocity.x > 0.0F, "Right particle should move right after contact");
}

void test_boundary_modes() {
  pm::DEMSolver solver;

  // Floor mode: no side-wall constraints.
  {
    pm::ParticleSystem system;
    pm::Particle p;
    p.position = pm::Vec2 {-1.5F, 0.5F};
    p.velocity = pm::Vec2 {-1.0F, 0.0F};
    system.add_particle(p);

    pm::SimulationConfig config;
    config.gravity_y = 0.0F;
    config.dt = 0.01F;
    config.boundary_mode = pm::BoundaryMode::Floor;
    config.bounds_left = -1.0F;
    config.bounds_right = 1.0F;

    solver.step(system, config, 1);
    expect(system.particles()[0].position.x < -1.45F, "Floor mode should not clamp x to side walls");
  }

  // Walls mode: no floor clamping.
  {
    pm::ParticleSystem system;
    pm::Particle p;
    p.position = pm::Vec2 {0.0F, -1.5F};
    p.velocity = pm::Vec2 {0.0F, 0.0F};
    system.add_particle(p);

    pm::SimulationConfig config;
    config.gravity_y = 0.0F;
    config.dt = 0.01F;
    config.floor_y = -0.9F;
    config.boundary_mode = pm::BoundaryMode::Walls;

    solver.step(system, config, 1);
    expect(system.particles()[0].position.y < -1.4F, "Walls mode should not clamp y to floor");
  }

  // Box mode: floor clamping active.
  {
    pm::ParticleSystem system;
    pm::Particle p;
    p.position = pm::Vec2 {0.0F, -1.5F};
    p.velocity = pm::Vec2 {0.0F, 0.0F};
    p.radius = 0.02F;
    system.add_particle(p);

    pm::SimulationConfig config;
    config.gravity_y = 0.0F;
    config.dt = 0.01F;
    config.floor_y = -0.9F;
    config.boundary_mode = pm::BoundaryMode::Box;

    solver.step(system, config, 1);
    expect(system.particles()[0].position.y >= (config.floor_y + p.radius - 1.0e-6F),
           "Box mode should clamp y to floor");
  }
}

}  // namespace

int main() {
  try {
    test_deterministic_scene_initialization();
    test_contact_repulsion();
    test_boundary_modes();
  } catch (const std::exception& error) {
    std::cerr << "dem_phase1_tests failed: " << error.what() << '\n';
    return 1;
  }

  std::cout << "dem_phase1_tests passed\n";
  return 0;
}
