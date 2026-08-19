// Copyright (c) 2026 Fixstars Corporation
// SPDX-License-Identifier: MIT

// saxpy.cu — minimal CUDA C++ workload for testing the profiler's C++ paths.
//
// Deliberately has NO cudaProfilerStart/Stop markers and NO NVTX ranges, so the profiler
// interview can exercise:
//   * capture-window (C++): insert cudaProfilerStart()/Stop() (+ a sync before stop) around the
//     steady-state loop, OR use --delay/--duration with no edit.
//   * nvtx-inject (C++): suggest manual nvtxRangePush/Pop snippets (no auto-injection for C++).
//
// Structure mirrors the Python example: a warmup phase, then a steady-state loop with a H2D
// copy + a kernel each step. Build needs nvcc (CUDA toolkit).
//
// Build:
//   nvcc -O2 -o saxpy saxpy.cu
// Run:
//   ./saxpy 30 5        # 30 steady steps, 5 warmup

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>
// For the cudaProfilerApi capture path the profiler would add:  #include <cuda_profiler_api.h>

__global__ void saxpy(int n, float a, const float* x, float* y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) y[i] = a * x[i] + y[i];
}

int main(int argc, char** argv) {
    int steady = argc > 1 ? atoi(argv[1]) : 30;
    int warmup = argc > 2 ? atoi(argv[2]) : 5;

    const int N = 1 << 22;             // ~4M elements
    const size_t bytes = N * sizeof(float);

    float *h_x = (float*)malloc(bytes); // pageable host memory (intentionally not pinned)
    float *h_y = (float*)malloc(bytes);
    for (int i = 0; i < N; ++i) { h_x[i] = 1.0f; h_y[i] = 2.0f; }

    float *d_x, *d_y;
    cudaMalloc(&d_x, bytes);
    cudaMalloc(&d_y, bytes);
    cudaMemcpy(d_y, h_y, bytes, cudaMemcpyHostToDevice);

    int threads = 256, blocks = (N + threads - 1) / threads;

    // --- profiler would wrap the steady-state loop below ---
    // capture-window (cudaProfilerApi): cudaProfilerStart() here, after warmup.
    for (int step = 0; step < warmup + steady; ++step) {
        // pageable H2D copy each step (transfer signal fodder)
        cudaMemcpy(d_x, h_x, bytes, cudaMemcpyHostToDevice);
        // a small flood of kernel launches (launch-bound fodder)
        for (int k = 0; k < 20; ++k) {
            saxpy<<<blocks, threads>>>(N, 2.0f, d_x, d_y);
        }
        cudaDeviceSynchronize();       // host sync each step
    }
    // capture-window: cudaDeviceSynchronize(); cudaProfilerStop();  // sync BEFORE stop

    cudaMemcpy(h_y, d_y, bytes, cudaMemcpyDeviceToHost);
    printf("[saxpy] done: y[0]=%f (steady=%d warmup=%d)\n", h_y[0], steady, warmup);

    cudaFree(d_x); cudaFree(d_y); free(h_x); free(h_y);
    return 0;
}