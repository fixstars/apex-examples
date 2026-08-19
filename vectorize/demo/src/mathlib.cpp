// Copyright (c) 2026 Fixstars Corporation
// SPDX-License-Identifier: Apache-2.0

#include "mathlib.hpp"
#include <cmath>

// Vector3
Vector3::Vector3() : x(0.0), y(0.0), z(0.0) {}
Vector3::Vector3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

double Vector3::dot(const Vector3& other) const {
    return x * other.x + y * other.y + z * other.z;
}

double Vector3::length() const {
    return std::sqrt(dot(*this));
}

Vector3 Vector3::operator+(const Vector3& other) const {
    return Vector3(x + other.x, y + other.y, z + other.z);
}

Vector3 Vector3::operator-(const Vector3& other) const {
    return Vector3(x - other.x, y - other.y, z - other.z);
}

Vector3 Vector3::operator*(double s) const {
    return Vector3(x * s, y * s, z * s);
}

Vector3 operator*(double s, const Vector3& v) {
    return v * s;
}

// Matrix3
Matrix3::Matrix3() {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m[i][j] = (i == j) ? 1.0 : 0.0;
}

Matrix3::Matrix3(double m00, double m01, double m02,
                 double m10, double m11, double m12,
                 double m20, double m21, double m22) {
    m[0][0] = m00; m[0][1] = m01; m[0][2] = m02;
    m[1][0] = m10; m[1][1] = m11; m[1][2] = m12;
    m[2][0] = m20; m[2][1] = m21; m[2][2] = m22;
}

Vector3 Matrix3::operator*(const Vector3& v) const {
    return Vector3(
        m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
        m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
        m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
    );
}

Matrix3 Matrix3::transpose() const {
    return Matrix3(
        m[0][0], m[1][0], m[2][0],
        m[0][1], m[1][1], m[2][1],
        m[0][2], m[1][2], m[2][2]
    );
}

Matrix3 Matrix3::operator*(const Matrix3& other) const {
    Matrix3 r;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            r.m[i][j] = 0.0;
            for (int k = 0; k < 3; ++k) {
                r.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return r;
}

// Quaternion
Quaternion::Quaternion() : w(1.0), x(0.0), y(0.0), z(0.0) {}
Quaternion::Quaternion(double w_, double x_, double y_, double z_) : w(w_), x(x_), y(y_), z(z_) {}

Quaternion Quaternion::normalized() const {
    double len_sq = w * w + x * x + y * y + z * z;
    if (len_sq == 0.0) return Quaternion(1.0, 0.0, 0.0, 0.0);
    double inv_len = 1.0 / std::sqrt(len_sq);
    return Quaternion(w * inv_len, x * inv_len, y * inv_len, z * inv_len);
}

Quaternion Quaternion::conjugate() const {
    return Quaternion(w, -x, -y, -z);
}

Quaternion Quaternion::operator*(const Quaternion& other) const {
    return Quaternion(
        w * other.w - x * other.x - y * other.y - z * other.z,
        w * other.x + x * other.w + y * other.z - z * other.y,
        w * other.y - x * other.z + y * other.w + z * other.x,
        w * other.z + x * other.y - y * other.x + z * other.w
    );
}

Vector3 Quaternion::rotate_vector(const Vector3& v) const {
    Quaternion qv(0.0, v.x, v.y, v.z);
    Quaternion result = (*this) * qv * conjugate();
    return Vector3(result.x, result.y, result.z);
}

Matrix3 Quaternion::to_rotation_matrix() const {
    double xx = x * x, yy = y * y, zz = z * z;
    double xy = x * y, xz = x * z, yz = y * z;
    double wx = w * x, wy = w * y, wz = w * z;

    return Matrix3(
        1.0 - 2.0 * (yy + zz), 2.0 * (xy - wz),     2.0 * (xz + wy),
        2.0 * (xy + wz),       1.0 - 2.0 * (xx + zz), 2.0 * (yz - wx),
        2.0 * (xz - wy),       2.0 * (yz + wx),       1.0 - 2.0 * (xx + yy)
    );
}