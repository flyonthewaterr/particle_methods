#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

#include "particle_methods/core/particle_system.hpp"
#include "particle_methods/scenes/dem_scenes.hpp"
#include "particle_methods/solvers/dem_solver.hpp"

namespace {

int parse_int_or_default(const char* text, int fallback) {
  try {
    return std::stoi(text);
  } catch (...) {
    return fallback;
  }
}

std::uint32_t parse_uint_or_default(const char* text, std::uint32_t fallback) {
  try {
    return static_cast<std::uint32_t>(std::stoul(text));
  } catch (...) {
    return fallback;
  }
}

float average_height(const pm::ParticleSystem& system) {
  if (system.size() == 0) {
    return 0.0F;
  }

  float sum = 0.0F;
  for (const auto& particle : system.particles()) {
    sum += particle.position.y;
  }

  return sum / static_cast<float>(system.size());
}

float max_speed(const pm::ParticleSystem& system) {
  float max_sq = 0.0F;
  for (const auto& particle : system.particles()) {
    max_sq = std::max(max_sq, pm::length_squared(particle.velocity));
  }

  return std::sqrt(max_sq);
}

}  // namespace

int main(int argc, char** argv) {
  int particle_count = 200;
  int steps = 500;
  int substeps = 2;
  bool use_cuda = false;
  std::uint32_t seed = 42U;
  pm::scenes::DEMBenchmarkScene scene = pm::scenes::DEMBenchmarkScene::PileFormation;

  for (int i = 1; i < argc; ++i) {
    const std::string arg(argv[i]);
    if (arg == "--particles" && i + 1 < argc) {
      particle_count = parse_int_or_default(argv[++i], particle_count);
    } else if (arg == "--steps" && i + 1 < argc) {
      steps = parse_int_or_default(argv[++i], steps);
    } else if (arg == "--substeps" && i + 1 < argc) {
      substeps = parse_int_or_default(argv[++i], substeps);
    } else if (arg == "--seed" && i + 1 < argc) {
      seed = parse_uint_or_default(argv[++i], seed);
    } else if (arg == "--scene" && i + 1 < argc) {
      try {
        scene = pm::scenes::parse_dem_scene(argv[++i]);
      } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
      }
    } else if (arg == "--cuda") {
      use_cuda = true;
    }
  }

  pm::ParticleSystem system;
  pm::SimulationConfig config;
  pm::scenes::initialize_dem_scene(system, config, scene, particle_count, seed);

  pm::DEMSolver solver(use_cuda ? pm::DEMSolver::Backend::CUDA : pm::DEMSolver::Backend::CPU);

  std::cout << "Running " << solver.name() << " scene=" << pm::scenes::to_string(scene)
            << " particles=" << system.size() << " steps=" << steps
            << " substeps=" << substeps << " seed=" << seed << "\n";

  try {
    for (int s = 0; s < steps; ++s) {
      solver.step(system, config, std::max(substeps, 1));
      if ((s + 1) % 100 == 0) {
        const auto& first = system.particles().front();
        std::cout << "step=" << (s + 1)
                  << " first=(" << first.position.x << ", " << first.position.y << ")"
                  << " avg_y=" << average_height(system)
                  << " max_speed=" << max_speed(system) << "\n";
      }
    }
  } catch (const std::exception& ex) {
    std::cerr << "Simulation failed: " << ex.what() << '\n';
    return 1;
  }

  std::cout << "Final particle sample (first 10):\n";
  for (std::size_t i = 0; i < std::min<std::size_t>(10, system.size()); ++i) {
    const auto& p = system.particles()[i];
    std::cout << i << "," << p.position.x << "," << p.position.y
              << "," << p.velocity.x << "," << p.velocity.y << "\n";
  }

  return 0;
}
