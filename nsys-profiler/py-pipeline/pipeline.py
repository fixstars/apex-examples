# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: MIT

#!/usr/bin/env python3
"""
py_pipeline — a Python + CUDA workload that is deliberately NOT a clean nn.Module tree.

It's a pipeline of plain functions (preprocess -> infer -> postprocess) that call torch ops
functionally. There are NO user-defined nn.Module subclasses, so the profiler's Layer-1 module
hooks (auto_instrument walking named_modules()) would find nothing useful here. This is the
"functional / backbone" case: the right NVTX path is FUNCTION-LEVEL annotation (wrap these
functions), which is exactly the new nvtx-inject capability we want to test.

It also has:
  * a warmup phase then a steady-state loop (tests capture-window: where does steady state start?)
  * NO cudaProfilerApi markers and NO NVTX (tests "agent adds them")
  * a pageable H2D copy each step, a flood of small ops, and a host-side sync — so the analyzer
    has something to find afterwards.

Run:
  python pipeline.py --steps 30 --warmup 5
"""

import argparse
import time
import torch


def preprocess(x_cpu, device):
    # pageable H2D copy (no pin) + a few small elementwise ops — function with no module identity
    x = x_cpu.to(device)                      # H2D transfer
    x = x / 255.0
    x = (x - 0.5) * 2.0
    return x


def infer(x, weight):
    # the "compute": a matmul plus a flood of tiny elementwise ops (launch-bound on purpose)
    h = x @ weight
    for _ in range(40):                       # tiny-kernel flood
        h = torch.sin(h) * 1.0001 + 0.0
    return h


def postprocess(h):
    # host-side reduction that forces a device->host sync every step
    s = h.sum()
    return float(s.item())                    # blocking .item() sync


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--steps", type=int, default=30)
    ap.add_argument("--warmup", type=int, default=5)
    ap.add_argument("--batch", type=int, default=512)
    ap.add_argument("--dim", type=int, default=2048)
    args = ap.parse_args()

    device = "cuda"
    weight = torch.randn(args.dim, args.dim, device=device)
    # pageable CPU input (intentionally not pinned)
    x_cpu = torch.randn(args.batch, args.dim)

    times = []
    for step in range(args.warmup + args.steps):
        t0 = time.perf_counter()
        x = preprocess(x_cpu, device)
        h = infer(x, weight)
        _ = postprocess(h)
        torch.cuda.synchronize()
        dt = time.perf_counter() - t0
        if step >= args.warmup:
            times.append(dt)

    times.sort()
    med = times[len(times) // 2]
    print(f"[py_pipeline] steady steps={len(times)} median_step={med*1e3:.2f} ms "
          f"throughput={args.batch/med:.0f} samples/s")


if __name__ == "__main__":
    main()