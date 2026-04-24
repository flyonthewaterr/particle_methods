#pragma once

#include <cstddef>
#include <vector>

#include "particle_methods/core/particle.hpp"

namespace pm {

class ParticleSystem {
 public:
  void clear();
  void reserve(std::size_t count);
  void add_particle(const Particle& particle);

  [[nodiscard]] std::size_t size() const;

  std::vector<Particle>& particles();
  const std::vector<Particle>& particles() const;

 private:
  std::vector<Particle> particles_;
};

}  // namespace pm
