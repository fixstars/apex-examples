// Copyright (c) 2026 Fixstars Corporation
// SPDX-License-Identifier: MIT

#ifndef EVALUATE_POSE_ALIGNMENT_HPP
#define EVALUATE_POSE_ALIGNMENT_HPP

#include "mathlib.hpp"
#include <cstddef>
#include <vector>

struct InputData {
    std::vector<Vector3> point0;
    std::vector<Vector3> point1;
    std::size_t num_data;
};

struct PoseAlignmentMetrics {
    double max = 0.0;
    double min = 0.0;
    double mean = 0.0;
    double variance = 0.0;
    std::size_t count = 0;
};

PoseAlignmentMetrics EvaluatePoseAlignmentMetrics(Quaternion& q, InputData& inputs);
std::vector<double> EvaluatePoseAlignmentValues(Quaternion& q, InputData& inputs);

Quaternion FindOptimalRotation(
    InputData& inputs,
    int max_iterations = 1000,
    double learning_rate = 0.5,
    double epsilon = 1e-5,
    double convergence_tol = 1e-7);

#endif // EVALUATE_POSE_ALIGNMENT_HPP