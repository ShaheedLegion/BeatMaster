#ifndef _MATH_HPP
#define _MATH_HPP
#pragma once

#include <cmath>
#include <chrono>
// Copyright (c) - 2015, Shaheed Abdol.

namespace math {

template <typename T, int C> class Vector {
public:
  static const int length = C;
  T v[C];

  bool equals(const Vector<T, C> &other) {
    for (int i = 0; i < length; ++i)
      if (v[i] != other.v[i])
        return false;
    return true;
  }

  Vector<T, C> operator*(const Vector<T, C> &right) const {
    vector<T, C> out;

    for (int i = 0; i < length; ++i)
      out.v[i] = v[i] * right.v[i];

    return out;
  }

  Vector<T, C> operator*(const T right) const {
    Vector<T, C> out;

    for (int i = 0; i < length; ++i)
      out.v[i] = v[i] * right;

    return out;
  }

  Vector<T, C> operator-(const T right) const {
    Vector<T, C> out;

    for (int i = 0; i < length; ++i)
      out.v[i] = v[i] - right;

    return out;
  }

  Vector<T, C> operator-(const Vector<T, C> &right) const {
    Vector<T, C> out;

    for (int i = 0; i < length; ++i)
      out.v[i] = v[i] - right.v[i];

    return out;
  }

  Vector<T, C> operator+(const T right) const {
    Vector<T, C> out;

    for (int i = 0; i < length; ++i)
      out.v[i] = v[i] + right;

    return out;
  }

  Vector<T, C> operator+(const Vector<T, C> &right) const {
    Vector<T, C> out;

    for (int i = 0; i < length; ++i)
      out.v[i] = v[i] + right.v[i];

    return out;
  }

  Vector<T, C> operator/(const T right) const {
    Vector<T, C> out;

    for (int i = 0; i < length; ++i)
      out.v[i] = v[i] / right;

    return out;
  }

  Vector<T, C> operator/(const Vector<T, C> &right) const {
    Vector<T, C> out;

    for (int i = 0; i < length; ++i)
      out.v[i] = v[i] / right.v[i];

    return out;
  }

  bool operator==(const T right) {
    for (int i = 0; i < length; ++i)
      if (v[i] != right)
        return false;

    return true;
  }

  T len_squared() const {
    T out = 0;
    for (int i = 0; i < length; ++i) {
      out += v[i] * v[i];
    }

    return out;
  }
};

template <typename T> class vector2 : public Vector<T, 2> {
public:
  vector2() {
    v[0] = 0;
    v[1] = 0;
  }

  vector2(T a, T b) {
    v[0] = a;
    v[1] = b;
  }
};

template <typename T> class vector3 : public Vector<T, 3> {
public:
  vector3() {
    for (int i = 0; i < 3; ++i)
      v[i] = 0;
  }

  vector3(T p) {
    for (int i = 0; i < 3; ++i)
      v[i] = p;
  }

  vector3(T x, T y, T z) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
  }

  T length() const { return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
};

template <typename T> class vector4 : public Vector<T, 4> {
public:
  vector4() {
    for (int i = 0; i < 4; ++i)
      v[i] = 0;
  }

  vector4(T p) {
    for (int i = 0; i < 4; ++i)
      v[i] = p;
  }

  vector4(T x, T y, T z, T w) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
  }

  vector4(vector3<T> &in, T w) {
    v[0] = in.v[0];
    v[1] = in.v[1];
    v[2] = in.v[2];
    v[3] = w;
  }
};

template <typename T> class vector5 : public Vector<T, 5> {
public:
  vector5() {
    for (int i = 0; i < 5; ++i)
      v[i] = 0;
  }

  vector5(T p) {
    for (int i = 0; i < 5; ++i)
      v[i] = p;
  }

  vector5(T x, T y, T z, T w, T u) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
    v[4] = u;
  }

  vector5(vector4<T> &in, T w) {
    v[0] = in.v[0];
    v[1] = in.v[1];
    v[2] = in.v[2];
    v[3] = in.v[3];
    v[4] = w;
  }
};

template <typename T> class vector8 : public Vector<T, 8> {
public:
  vector8() {
    for (int i = 0; i < 8; ++i)
      v[i] = 0;
  }

  vector8(T p) {
    for (int i = 0; i < 8; ++i)
      v[i] = p;
  }

  vector8(T x, T y, T z, T w, T u, T a, T b, T c) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
    v[4] = u;
    v[5] = a;
    v[6] = b;
    v[7] = c;
  }

  vector8(vector4<T> &in, T w) {
    v[0] = in.v[0];
    v[1] = in.v[1];
    v[2] = in.v[2];
    v[3] = in.v[3];
    v[4] = w;
    v[5] = w;
    v[6] = w;
    v[7] = w;
  }
};

typedef vector2<double> vec2;
typedef vector2<int> vec2i;
typedef vector3<double> vec3;
typedef vector3<int> vec3i;
typedef vector4<double> vec4;
typedef vector4<int> vec4i;
typedef vector5<double> vec5;
typedef vector5<int> vec5i;
typedef vector8<double> vec8;
typedef vector8<int> vec8i;

// Compute distance to move based on
double compute_units(double ups, double millis, double fps) {
  if (millis == 0 || fps == 0) // avoid armageddon.
    return 0.0f;

  return (ups / fps);

  // We must assume that the delta has a resolution of about 16ms per frame.
  // That way we know how large each delta value should be to constitute a
  // frame.
  // 1 second divided by current fps gives you the ideal delta per frame.
  double ideal_delta = 1000.0 / fps;
  double ideal_ups = ups / fps;

  double c = millis / ideal_delta;
  return ideal_ups * c;
}

} // namespace math

#endif // _MATH_HPP