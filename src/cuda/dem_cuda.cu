#include "particle_methods/cuda/dem_cuda.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include <cuda_runtime.h>

namespace {

void check_cuda(cudaError_t status, const char* context) {
  if (status != cudaSuccess) {
    throw std::runtime_error(std::string(context) + ": " + cudaGetErrorString(status));
  }
}

__global__ void dem_step_kernel(
    pm::Particle* particles,
    std::size_t count,
    float dt,
    float gravity_y,
  int boundary_mode,
    float floor_y,
  float ceiling_y,
    float restitution,
  float boundary_friction,
  int floor_opening_enabled,
  float floor_opening_left,
  float floor_opening_right,
    float bounds_left,
    float bounds_right) {
  const std::size_t idx = static_cast<std::size_t>(blockDim.x) * blockIdx.x + threadIdx.x;
  if (idx >= count) {
    return;
  }

  pm::Particle& particle = particles[idx];

  particle.velocity.y += gravity_y * dt;
  particle.position.x += particle.velocity.x * dt;
  particle.position.y += particle.velocity.y * dt;

  const float friction = fminf(fmaxf(boundary_friction, 0.0F), 1.0F);

  if (boundary_mode == 0 || boundary_mode == 2) {
    const bool in_opening =
        floor_opening_enabled != 0 &&
        particle.position.x > floor_opening_left &&
        particle.position.x < floor_opening_right;

    const float min_y = floor_y + particle.radius;
    if (!in_opening && particle.position.y < min_y) {
      particle.position.y = min_y;
      if (particle.velocity.y < 0.0F) {
        particle.velocity.y = -particle.velocity.y * restitution;
      }
      particle.velocity.x *= (1.0F - friction);
    }
  }

  if (boundary_mode == 1 || boundary_mode == 2) {
    const float min_x = bounds_left + particle.radius;
    const float max_x = bounds_right - particle.radius;

    if (particle.position.x < min_x) {
      particle.position.x = min_x;
      if (particle.velocity.x < 0.0F) {
        particle.velocity.x = -particle.velocity.x * restitution;
      }
      particle.velocity.y *= (1.0F - friction);
    }

    if (particle.position.x > max_x) {
      particle.position.x = max_x;
      if (particle.velocity.x > 0.0F) {
        particle.velocity.x = -particle.velocity.x * restitution;
      }
      particle.velocity.y *= (1.0F - friction);
    }
  }

  if (boundary_mode == 2) {
    const float max_y = ceiling_y - particle.radius;
    if (particle.position.y > max_y) {
      particle.position.y = max_y;
      if (particle.velocity.y > 0.0F) {
        particle.velocity.y = -particle.velocity.y * restitution;
      }
      particle.velocity.x *= (1.0F - friction);
    }
  }
}

}  // namespace

namespace pm::cuda {

bool is_available() {
  int count = 0;
  return cudaGetDeviceCount(&count) == cudaSuccess && count > 0;
}

void step_dem(Particle* particles, std::size_t count, const SimulationConfig& config, int substeps) {
  if (particles == nullptr || count == 0) {
    return;
  }

  const int clamped_substeps = std::max(substeps, 1);
  const float dt = config.dt / static_cast<float>(clamped_substeps);

  Particle* device_particles = nullptr;
  const std::size_t bytes = count * sizeof(Particle);

  check_cuda(cudaMalloc(&device_particles, bytes), "cudaMalloc failed");
  check_cuda(cudaMemcpy(device_particles, particles, bytes, cudaMemcpyHostToDevice), "cudaMemcpy H2D failed");

  const int threads = 256;
  const int blocks = static_cast<int>((count + static_cast<std::size_t>(threads) - 1) / static_cast<std::size_t>(threads));

  for (int s = 0; s < clamped_substeps; ++s) {
    dem_step_kernel<<<blocks, threads>>>(
        device_particles,
        count,
        dt,
        config.gravity_y,
        static_cast<int>(config.boundary_mode),
        config.floor_y,
        config.ceiling_y,
        config.restitution,
        config.boundary_friction,
        config.floor_opening_enabled ? 1 : 0,
        config.floor_opening_left,
        config.floor_opening_right,
        config.bounds_left,
        config.bounds_right);

    check_cuda(cudaGetLastError(), "kernel launch failed");
  }

  check_cuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize failed");
  check_cuda(cudaMemcpy(particles, device_particles, bytes, cudaMemcpyDeviceToHost), "cudaMemcpy D2H failed");
  check_cuda(cudaFree(device_particles), "cudaFree failed");
}

}  // namespace pm::cuda
