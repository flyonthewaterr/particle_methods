#pragma once

#include <cmath>

namespace pm {

struct Vec2 {
  float x {0.0F};
  float y {0.0F};

  Vec2& operator+=(const Vec2& other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  Vec2& operator-=(const Vec2& other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  Vec2& operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
  }
};

inline Vec2 operator+(Vec2 lhs, const Vec2& rhs) {
  lhs += rhs;
  return lhs;
}

inline Vec2 operator-(Vec2 lhs, const Vec2& rhs) {
  lhs -= rhs;
  return lhs;
}

inline Vec2 operator*(Vec2 value, float scalar) {
  value *= scalar;
  return value;
}

inline Vec2 operator*(float scalar, Vec2 value) {
  value *= scalar;
  return value;
}

inline float dot(const Vec2& a, const Vec2& b) {
  return a.x * b.x + a.y * b.y;
}

inline float length_squared(const Vec2& value) {
  return dot(value, value);
}

inline float length(const Vec2& value) {
  return std::sqrt(length_squared(value));
}

}  // namespace pm
