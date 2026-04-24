#include "particle_methods/core/particle_system.hpp"

namespace pm {

void ParticleSystem::clear() {
  particles_.clear();
}

void ParticleSystem::reserve(std::size_t count) {
  particles_.reserve(count);
}

void ParticleSystem::add_particle(const Particle& particle) {
  particles_.push_back(particle);
}

std::size_t ParticleSystem::size() const {
  return particles_.size();
}

std::vector<Particle>& ParticleSystem::particles() {
  return particles_;
}

const std::vector<Particle>& ParticleSystem::particles() const {
  return particles_;
}

}  // namespace pm
