// Copyright (c) 2026 Fixstars Corporation
// SPDX-License-Identifier: MIT

#include "evaluate_pose_alignment.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <utility>
#include <cmath>

static Vector3 randomUnitVector(std::mt19937& gen) {
    std::normal_distribution<double> nd(0.0, 1.0);
    Vector3 v(nd(gen), nd(gen), nd(gen));
    double len = v.length();
    if (len < 1e-10) return Vector3(1.0, 0.0, 0.0);
    return v * (1.0 / len);
}

// p0: sensor-frame measurement = q_true^-1 * target + noise
// p1: world-frame target      = target + noise
// After applying q_true to p0, it aligns with p1
static std::pair<Vector3, Vector3> makeParallelPair(std::mt19937& gen, const Quaternion& q_true) {
    Vector3 target = randomUnitVector(gen);
    std::uniform_real_distribution<double> noise(-0.03, 0.03);
    std::uniform_real_distribution<double> scale(0.8, 1.5);
    double s0 = scale(gen);
    double s1 = scale(gen);
    Vector3 p0 = q_true.conjugate().rotate_vector(target * s0) + Vector3(noise(gen), noise(gen), noise(gen));
    Vector3 p1 = target * s1 + Vector3(noise(gen), noise(gen), noise(gen));
    return {p0, p1};
}

static std::pair<Vector3, Vector3> makeOppositePair(std::mt19937& gen, const Quaternion& q_true) {
    Vector3 target = randomUnitVector(gen);
    std::uniform_real_distribution<double> noise(-0.03, 0.03);
    std::uniform_real_distribution<double> scale(0.8, 1.5);
    double s0 = scale(gen);
    double s1 = scale(gen);
    Vector3 p0 = q_true.conjugate().rotate_vector(target * s0) + Vector3(noise(gen), noise(gen), noise(gen));
    Vector3 p1 = target * (-s1) + Vector3(noise(gen), noise(gen), noise(gen));
    return {p0, p1};
}

static std::pair<Vector3, Vector3> makeOrthogonalPair(std::mt19937& gen, const Quaternion& q_true) {
    Vector3 target = randomUnitVector(gen);
    Vector3 arbitrary(1.0, 0.0, 0.0);
    if (std::abs(target.x) > 0.9) arbitrary = Vector3(0.0, 1.0, 0.0);
    Vector3 orth = Vector3(
        target.y * arbitrary.z - target.z * arbitrary.y,
        target.z * arbitrary.x - target.x * arbitrary.z,
        target.x * arbitrary.y - target.y * arbitrary.x
    );
    double orth_len = orth.length();
    if (orth_len < 1e-10) orth = Vector3(0.0, 1.0, 0.0);
    else orth = orth * (1.0 / orth_len);

    std::uniform_real_distribution<double> noise(-0.03, 0.03);
    std::uniform_real_distribution<double> scale(0.8, 1.5);
    double s0 = scale(gen);
    double s1 = scale(gen);
    Vector3 p0 = q_true.conjugate().rotate_vector(orth * s0) + Vector3(noise(gen), noise(gen), noise(gen));
    Vector3 p1 = target * s1 + Vector3(noise(gen), noise(gen), noise(gen));
    return {p0, p1};
}

static std::pair<Vector3, Vector3> makeRandomPair(std::mt19937& gen) {
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    Vector3 p0(dist(gen), dist(gen), dist(gen));
    Vector3 p1(dist(gen), dist(gen), dist(gen));
    return {p0, p1};
}

static std::pair<Vector3, Vector3> makeSmallMagnitudePair(std::mt19937& gen, const Quaternion& /*q_true*/) {
    std::uniform_real_distribution<double> dist(-0.2, 0.2);
    Vector3 p0(dist(gen), dist(gen), dist(gen));
    Vector3 p1 = randomUnitVector(gen) * 0.5 + Vector3(dist(gen), dist(gen), dist(gen));
    return {p0, p1};
}

static InputData generateInputData(std::mt19937& gen, const Quaternion& q_true)
{
    std::uniform_int_distribution<int> total_dist(99000, 101000);
    std::size_t num_data = static_cast<std::size_t>(total_dist(gen));

    InputData inputs;
    inputs.num_data = num_data;
    inputs.point0.reserve(num_data);
    inputs.point1.reserve(num_data);

    std::uniform_int_distribution<int> shift(-200, 200);
    long c1 = static_cast<long>(num_data * 0.80) + shift(gen);
    long c2 = static_cast<long>(num_data * 0.05) + shift(gen);
    long c3 = static_cast<long>(num_data * 0.05) + shift(gen);
    long c4 = static_cast<long>(num_data * 0.05) + shift(gen);
    long c5 = static_cast<long>(num_data * 0.05) + shift(gen);

    long sum = c1 + c2 + c3 + c4 + c5;
    c1 += static_cast<long>(num_data) - sum;

    if (c1 < 0) c1 = 0;
    if (c2 < 0) c2 = 0;
    if (c3 < 0) c3 = 0;
    if (c4 < 0) c4 = 0;
    if (c5 < 0) c5 = 0;

    // Cluster 1: parallel (cos ≈ 0.9 ~ 1.0 with q_true)
    for (long i = 0; i < c1; ++i) {
        auto [p0, p1] = makeParallelPair(gen, q_true);
        inputs.point0.push_back(p0);
        inputs.point1.push_back(p1);
    }
    // Cluster 2: opposite (cos ≈ -1.0 ~ -0.9 with q_true)
    for (long i = 0; i < c2; ++i) {
        auto [p0, p1] = makeOppositePair(gen, q_true);
        inputs.point0.push_back(p0);
        inputs.point1.push_back(p1);
    }
    // Cluster 3: orthogonal (cos ≈ 0 with q_true)
    for (long i = 0; i < c3; ++i) {
        auto [p0, p1] = makeOrthogonalPair(gen, q_true);
        inputs.point0.push_back(p0);
        inputs.point1.push_back(p1);
    }
    // Cluster 4: random
    for (long i = 0; i < c4; ++i) {
        auto [p0, p1] = makeRandomPair(gen);
        inputs.point0.push_back(p0);
        inputs.point1.push_back(p1);
    }
    // Cluster 5: small magnitude (filtered by threshold)
    for (long i = 0; i < c5; ++i) {
        auto [p0, p1] = makeSmallMagnitudePair(gen, q_true);
        inputs.point0.push_back(p0);
        inputs.point1.push_back(p1);
    }

    // Shuffle data pairs to remove cluster ordering bias
    for (std::size_t i = num_data - 1; i > 0; --i) {
        std::uniform_int_distribution<std::size_t> dist(0, i);
        std::size_t j = dist(gen);
        std::swap(inputs.point0[i], inputs.point0[j]);
        std::swap(inputs.point1[i], inputs.point1[j]);
    }

    return inputs;
}

static double quaternionAngularDifference(const Quaternion& a, const Quaternion& b) {
    double dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    // q and -q represent the same rotation
    dot = std::abs(dot);
    if (dot > 1.0) dot = 1.0;
    return 2.0 * std::acos(dot); // radians
}

int main() {
    const std::size_t num_iterations = 100;

    std::mt19937 gen(42);
    Quaternion q_true = Quaternion(0.3, 0.7, 0.2, 0.6).normalized();

    InputData inputs = generateInputData(gen, q_true);

    std::vector<InputData> datasets;
    datasets.push_back(inputs);
    for (int i = 1; i < 10; ++i) {
        std::mt19937 dataset_gen(42 + i);
        datasets.push_back(generateInputData(dataset_gen, q_true));
    }

    for (std::size_t i = 0; i < 5; ++i) {
        EvaluatePoseAlignmentMetrics(q_true, datasets[i % 10]);
    }

    auto start = std::chrono::high_resolution_clock::now();

    double max_time_ms = 0.0;
    PoseAlignmentMetrics last_metrics;
    for (std::size_t i = 0; i < num_iterations; ++i) {
        auto iter_start = std::chrono::high_resolution_clock::now();
        last_metrics = EvaluatePoseAlignmentMetrics(q_true, datasets[i % 10]);
        auto iter_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> iter_elapsed = iter_end - iter_start;
        double iter_ms = iter_elapsed.count();
        if (iter_ms > max_time_ms) {
            max_time_ms = iter_ms;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    double total_time_ms = elapsed.count();
    double avg_time_ms = total_time_ms / num_iterations;
    double throughput = (inputs.num_data * num_iterations) / (total_time_ms / 1000.0);

    std::cout << "=== Performance Test Results ===" << std::endl;
    std::cout << "Data points per iteration: " << inputs.num_data << std::endl;
    std::cout << "Iterations: " << num_iterations << std::endl;
    std::cout << "Total time: " << total_time_ms << " ms" << std::endl;
    std::cout << "Average time per iteration: " << avg_time_ms << " ms" << std::endl;
    std::cout << "Worst time per iteration: " << max_time_ms << " ms" << std::endl;
    std::cout << "Throughput: " << throughput << " points/second" << std::endl;
    std::cout << "Valid count (last iteration): " << last_metrics.count << std::endl;
    std::cout << "Min:   " << last_metrics.min << std::endl;
    std::cout << "Max:   " << last_metrics.max << std::endl;
    std::cout << "Mean:  " << last_metrics.mean << std::endl;
    std::cout << "Var:   " << last_metrics.variance << std::endl;

    InputData small_input;
    small_input.num_data = 3;
    small_input.point0 = { Vector3(1.0, 0.0, 0.0), Vector3(0.0, 0.05, 0.0), Vector3(0.0, 1.0, 0.0) };
    small_input.point1 = { Vector3(1.0, 0.0, 0.0), Vector3(0.0, 1.0, 0.0), Vector3(0.0, 0.0, 1.0) };

    auto small_metrics = EvaluatePoseAlignmentMetrics(q_true, small_input);

    std::cout << "\n=== Correctness Check ===" << std::endl;
    std::cout << "point0[1] should be skipped due to length 0.05 < 0.1" << std::endl;
    std::cout << "Valid count: " << small_metrics.count << std::endl;
    std::cout << "Min:      " << small_metrics.min << std::endl;
    std::cout << "Max:      " << small_metrics.max << std::endl;
    std::cout << "Mean:     " << small_metrics.mean << std::endl;
    std::cout << "Variance: " << small_metrics.variance << std::endl;

    std::cout << "\n=== Optimization Test ===" << std::endl;
    std::cout << "True q:     (" << q_true.w << ", " << q_true.x << ", " << q_true.y << ", " << q_true.z << ")" << std::endl;

    auto true_metrics = EvaluatePoseAlignmentMetrics(q_true, inputs);
    std::cout << "True q mean: " << true_metrics.mean << std::endl;

    Quaternion q_identity(1.0, 0.0, 0.0, 0.0);
    auto initial_opt_metrics = EvaluatePoseAlignmentMetrics(q_identity, inputs);
    std::cout << "Initial mean (identity q): " << initial_opt_metrics.mean << std::endl;

    auto optimized_q = FindOptimalRotation(inputs);
    auto optimized_metrics = EvaluatePoseAlignmentMetrics(optimized_q, inputs);

    std::cout << "Optimized q: (" << optimized_q.w << ", " << optimized_q.x << ", " << optimized_q.y << ", " << optimized_q.z << ")" << std::endl;
    std::cout << "Optimized mean: " << optimized_metrics.mean << std::endl;

    double angle_diff = quaternionAngularDifference(q_true, optimized_q);
    std::cout << "Angular diff between true q and optimized q: " << angle_diff << " rad (" << angle_diff * 180.0 / M_PI << " deg)" << std::endl;

    if (optimized_metrics.mean > initial_opt_metrics.mean) {
        std::cout << "SUCCESS: Optimized mean is higher than initial mean." << std::endl;
    } else {
        std::cout << "WARNING: Optimized mean is NOT higher than initial mean." << std::endl;
    }

    return 0;
}