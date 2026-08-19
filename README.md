# APEX Examples

Demo target workloads for the APEX agent plugins. Each directory is a small,
self-contained project with deliberately planted problems for the corresponding
APEX skill to find and fix — use them to try the skills on a known target.

## Prerequisites

APEX — the `apm` CLI and the skill marketplace — is **distributed separately**
(it is not part of this repository). Register the marketplace endpoint provided
with your APEX distribution once:

```bash
apm marketplace add https://__APEX_HOST__:4430/apex/marketplace.git
```

Then install the plugin for the example you want to try (per example README):

```bash
apm install --target opencode <plugin>@apex-marketplace   # or: --target claude
```

## Examples

| Example | APEX plugin | What it exercises | Stack |
|---|---|---|---|
| [`perf/`](perf/) | `perf` | CPU profiling skills: planted cache-stride, branch-miss and I/O-stall bottlenecks (`imgpipe`) | C / OpenMP |
| [`vectorize/`](vectorize/) | `vectorize` | SIMD vectorization workflow on a 3D pose-alignment benchmark (x86_64 + aarch64 cross-build) | C++ / CMake |
| [`tensorrt/`](tensorrt/) | `tensorrt` | TensorRT conversion, one module per lesson: happy path, dynamic shapes, custom CUDA kernel, stateful refactor | PyTorch |
| [`nsys-profiler/`](nsys-profiler/) | `nsys-profiler` | GPU profiling fixtures (3): function-level NVTX, Layer-1 module hooks, C++ capture markers + manual NVTX | CUDA C++ / PyTorch |
| [`nsys-analyzer/`](nsys-analyzer/) | `nsys-analyzer` | GPU bottleneck analysis on nsys SQLite exports (pre-captured profiles to be published) | nsys `.sqlite` |

Note: walkthroughs inside each example are currently in Japanese.

## License

MIT — see [LICENSE](LICENSE). Copyright (c) 2026 Fixstars Corporation.
