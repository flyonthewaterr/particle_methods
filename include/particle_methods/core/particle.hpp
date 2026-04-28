#pragma once

#include "particle_methods/core/vec2.hpp"

namespace pm {

enum class BoundaryMode : int {
  Floor = 0,
  Walls = 1,
  Box = 2,
};

struct Particle {
  Vec2 position {};
  Vec2 velocity {};
  float mass {1.0F};
  float radius {0.02F};
};

struct SimulationConfig {
  float gravity_y {-9.81F};
  float dt {0.005F};

  // Legacy boundary controls retained for compatibility with existing code.
  float floor_y {0.0F};
  float restitution {0.4F};
  float bounds_left {-1.0F};
  float bounds_right {1.0F};

  // Phase-1 boundary options.
  BoundaryMode boundary_mode {BoundaryMode::Box};
  float ceiling_y {1.2F};
  float boundary_friction {0.15F};

  // Optional floor opening for hopper-like discharge scenes.
  bool floor_opening_enabled {false};
  float floor_opening_left {-0.15F};
  float floor_opening_right {0.15F};

  // Phase-1 DEM contact model parameters.
  float dem_normal_stiffness {9000.0F};
  float dem_normal_damping {45.0F};
  float dem_tangential_friction {0.45F};
  float dem_tangential_damping {30.0F};
  float dem_contact_epsilon {1.0e-6F};

  // Phase-2 SPH baseline parameters.
  float sph_rest_density {1000.0F};
  float sph_smoothing_length {0.08F};
  float sph_pressure_stiffness {2000.0F};
  float sph_viscosity {0.1F};
  float sph_sound_speed {20.0F};
  float sph_cfl_factor {0.4F};
  float sph_boundary_stiffness {3000.0F};
  float sph_boundary_damping {25.0F};
};

}  // namespace pm
