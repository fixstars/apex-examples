# Fixture 3 — cpp-saxpy (CUDA C++)

[← セットアップとインストールは README](../README.md)

Raw **CUDA C++** workload. Warmup + steady-state loop with H2D copy + kernel launches
per step. Exercises the profiler's **C++ capture markers** (`cudaProfilerStart/Stop` +
sync-before-stop) and **manual NVTX** (`nvtxRangePush/Pop`) paths. Has no markers/NVTX
by design.

Planted bottlenecks: pageable H2D each step, a 20× kernel launch flood, a per-step
`cudaDeviceSynchronize`.

## Build & run

```bash
cd cpp-saxpy
nvcc -O2 -o saxpy saxpy.cu
./saxpy 30 5            # 30 steady steps, 5 warmup
```

Requires the CUDA toolkit (`nvcc`). Args: `./saxpy <steady_steps> <warmup>`.
Prints the steady-state median step time so you can sanity-check it runs before
profiling.

## Profiler interview — what it asks & what answers to try

This fixture is built to make the profiler's **C++ capture markers + manual NVTX**
branch the "right" answer. The interview order (from the profiler's DESIGN.md):
**context-detect → Q-target → Q-nvtx → Q-trace → Q-gpu → Q-distributed → Q-output**.

Build first (needs nvcc): `nvcc -O2 -o saxpy saxpy.cu`. Launch command:
`./saxpy 30 5`.

| Step | Question | Answer to try | Why / what it tests |
|---|---|---|---|
| context-detect | "You seem to be profiling …?" | confirm: a CUDA C++ saxpy | should detect C++/CUDA, NOT suggest PyTorch paths |
| Q-target | Python / PyTorch / **C++ / CUDA** / generic | **Raw CUDA / C++** | disables auto-injection; routes to C++ markers + manual NVTX |
| Q-nvtx | already have / agent adds / none | **agent adds them** → expect **manual snippet** | C++ can't be auto-injected; agent should hand `nvtxRangePush("infer")/Pop()` snippets + the `#include <nvtx3/nvToolsExt.h>` and link note |
| Q-capture | cudaProfilerApi markers / delay-duration | **cudaProfilerApi** | tests C++ marker insertion: `#include <cuda_profiler_api.h>`, `cudaProfilerStart()` after warmup, **`cudaDeviceSynchronize()` then `cudaProfilerStop()`** (sync before stop) |
| └ alt | delay-duration | **delay 1s / duration 3s** | the no-edit universal path; should work on the unmodified binary |
| Q-trace | minimal / cpu-sampling / rich | **rich** | exercise full trace |
| Q-gpu | which device | **0** | — |
| Q-distributed | (skipped) | — | single process |
| Q-output | dir / base | `prof/saxpy` | — |

**Expected planted findings:** pageable H2D each step, a 20× kernel launch flood, a
per-step `cudaDeviceSynchronize`.

**Relative-path check:** give the launch command as `./saxpy …` from a different cwd
to confirm the profiler's relative-path warning (Q1) fires and asks for an absolute
path / explicit cwd.

## What "passing" looks like

1. context-detect makes a reasonable, non-over-claiming guess.
2. Q-target routes correctly (cpp → manual NVTX only, no auto-injection).
3. nvtx-inject, when chosen, hands **snippets to paste** (not an auto-edit): the
   `nvtxRangePush("infer")/Pop()` calls + the `#include` and link note.
4. capture-window inserts language-correct markers (`cudaProfilerStart()` +
   `cudaDeviceSynchronize()` then `cudaProfilerStop()`) OR picks delay/duration, and
   the assembled `nsys profile` command carries the matching flag.
5. `run_nsys.sh` produces a `.nsys-rep` + `.sqlite` and the `=== PROFILE RESULT ===`
   block.
6. (end-to-end) the analyzer's `nvtx-timeline` can anchor on the injected ranges.
