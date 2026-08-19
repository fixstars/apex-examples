// Copyright (c) 2026 Fixstars Corporation
// SPDX-License-Identifier: Apache-2.0

#include "evaluate_pose_alignment.hpp"
#include <algorithm>
#include <cmath>

// 原点付近は無視！
double TOO_SHORT_LENGTH_THRESHOLD = 0.1;

std::vector<double> EvaluatePoseAlignmentValues(Quaternion& q, InputData& inputs)
{
    std::vector<double> values;
    if (inputs.num_data == 0) {
        return values;
    }

    Matrix3 R = q.normalized().to_rotation_matrix();

    for (std::size_t i = 0; i < inputs.num_data; ++i) {
        const Vector3& p0 = inputs.point0[i];
        const Vector3& p1 = inputs.point1[i];

        if (p0.length() <= TOO_SHORT_LENGTH_THRESHOLD) { continue; }        
        Vector3 rotated_p0 = R * p0;
        const double len_rp0 = rotated_p0.length();
        const double len_p1 = p1.length();

        if (len_rp0 <= TOO_SHORT_LENGTH_THRESHOLD || len_p1 <= TOO_SHORT_LENGTH_THRESHOLD) { continue; }

        double cos_theta = rotated_p0.dot(p1) / (len_rp0 * len_p1);
        cos_theta = std::max(-1.0, std::min(1.0, cos_theta));

        values.push_back(cos_theta);
    }

    return values;
}

PoseAlignmentMetrics EvaluatePoseAlignmentMetrics(Quaternion& q, InputData& inputs)
{
    PoseAlignmentMetrics metrics;
    std::vector<double> values = EvaluatePoseAlignmentValues(q, inputs);
    const std::size_t valid_count = values.size();

    if (valid_count == 0) {
        return metrics;
    }

    double sum = 0.0;
    double sum_sq = 0.0;
    double current_min = values[0];
    double current_max = values[0];

    for (double v : values) {
        sum += v;
        sum_sq += v * v;
        if (v < current_min) current_min = v;
        if (v > current_max) current_max = v;
    }

    metrics.min = current_min;
    metrics.max = current_max;
    metrics.mean = sum / static_cast<double>(valid_count);
    metrics.count = valid_count;

    if (valid_count > 1) {
        double mean_of_squares = sum_sq / static_cast<double>(valid_count);
        double variance = mean_of_squares - metrics.mean * metrics.mean;
        if (variance < 0.0 && variance > -1e-12) {
            variance = 0.0;
        }
        metrics.variance = variance;
    }

    return metrics;
}

Quaternion FindOptimalRotation(
    InputData& inputs,
    int max_iterations,
    double learning_rate,
    double epsilon,
    double convergence_tol)
{
    Quaternion q(1.0, 0.0, 0.0, 0.0);

    auto compute_loss = [&](Quaternion& q_test) -> double {
        return EvaluatePoseAlignmentMetrics(q_test, inputs).mean;
    };

    double prev_loss = compute_loss(q);

    for (int iter = 0; iter < max_iterations; ++iter) {
        // Central differences for each component
        Quaternion q_plus;
        Quaternion q_minus;
        // w component
        q_plus  = Quaternion(q.w + epsilon, q.x, q.y, q.z);
        q_minus = Quaternion(q.w - epsilon, q.x, q.y, q.z);
        double grad_w = (compute_loss(q_plus) - compute_loss(q_minus)) / (2.0 * epsilon);
        // x component
        q_plus  = Quaternion(q.w, q.x + epsilon, q.y, q.z);
        q_minus = Quaternion(q.w, q.x - epsilon, q.y, q.z);
        double grad_x = (compute_loss(q_plus) - compute_loss(q_minus)) / (2.0 * epsilon);
        // y component
        q_plus  = Quaternion(q.w, q.x, q.y + epsilon, q.z);
        q_minus = Quaternion(q.w, q.x, q.y - epsilon, q.z);
        double grad_y = (compute_loss(q_plus) - compute_loss(q_minus)) / (2.0 * epsilon);
        // z component
        q_plus  = Quaternion(q.w, q.x, q.y, q.z + epsilon);
        q_minus = Quaternion(q.w, q.x, q.y, q.z - epsilon);
        double grad_z = (compute_loss(q_plus) - compute_loss(q_minus)) / (2.0 * epsilon);
        // Gradient ascent step (maximize mean cosine)
        q = Quaternion(
            q.w + learning_rate * grad_w,
            q.x + learning_rate * grad_x,
            q.y + learning_rate * grad_y,
            q.z + learning_rate * grad_z
        );
        // Normalize to maintain unit quaternion
        q = q.normalized();
        double curr_loss = compute_loss(q);
        if (std::abs(curr_loss - prev_loss) < convergence_tol) {
            break;
        }
        prev_loss = curr_loss;
    }

    return q;
}