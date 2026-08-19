// Copyright (c) 2026 Fixstars Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef MATHLIB_HPP
#define MATHLIB_HPP

struct Vector3 {
    double x, y, z;

    Vector3();
    Vector3(double x_, double y_, double z_);

    double dot(const Vector3& other) const;
    double length() const;

    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(double s) const;

    friend Vector3 operator*(double s, const Vector3& v);
};

struct Matrix3 {
    double m[3][3];

    Matrix3();
    Matrix3(double m00, double m01, double m02,
            double m10, double m11, double m12,
            double m20, double m21, double m22);

    Vector3 operator*(const Vector3& v) const;
    Matrix3 transpose() const;
    Matrix3 operator*(const Matrix3& other) const;
};

struct Quaternion {
    double w, x, y, z;

    Quaternion();
    Quaternion(double w_, double x_, double y_, double z_);

    Quaternion normalized() const;
    Quaternion conjugate() const;
    Quaternion operator*(const Quaternion& other) const;

    Vector3 rotate_vector(const Vector3& v) const;
    Matrix3 to_rotation_matrix() const;
};

#endif // MATHLIB_HPP