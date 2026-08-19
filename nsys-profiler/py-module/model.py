# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: MIT

#!/usr/bin/env python3
"""
py_module — a PyTorch workload built from USER-DEFINED nn.Module subclasses.

This is the case where the profiler's Layer-1 module hooks (auto_instrument walking
named_modules()) are the RIGHT NVTX path: the model is a tree of the user's own classes
(Stem, Block, Head, TinyNet), so the hooks attach named ranges (stem / blocks.0 / head) the
analyzer can scope to. (Contrast with ../py_pipeline, which is functional and needs
function-level annotation instead.)

Deliberately:
  * built from user classes (so module hooks find something), NOT a bare torchvision model
  * warmup then steady-state loop (capture-window: where does steady state start?)
  * NO cudaProfilerApi markers, NO NVTX (tests "agent adds them" → Layer 1)
  * planted issues for the analyzer: pageable H2D each step, a tiny-op flood inside Block,
    a per-step .item() sync.

Run:
  python model.py --steps 30 --warmup 5
"""

import argparse
import time
import torch
import torch.nn as nn


class Stem(nn.Module):
    def __init__(self, dim):
        super().__init__()
        self.fc = nn.Linear(dim, dim)

    def forward(self, x):
        return torch.relu(self.fc(x))


class Block(nn.Module):
    def __init__(self, dim):
        super().__init__()
        self.fc = nn.Linear(dim, dim)

    def forward(self, x):
        h = self.fc(x)
        for _ in range(20):                  # tiny-kernel flood inside a named module
            h = torch.sin(h) * 1.0001
        return h


class Head(nn.Module):
    def __init__(self, dim, classes):
        super().__init__()
        self.fc = nn.Linear(dim, classes)

    def forward(self, x):
        return self.fc(x)


class TinyNet(nn.Module):
    def __init__(self, dim=2048, classes=1000, n_blocks=3):
        super().__init__()
        self.stem = Stem(dim)
        self.blocks = nn.ModuleList([Block(dim) for _ in range(n_blocks)])
        self.head = Head(dim, classes)

    def forward(self, x):
        x = self.stem(x)
        for blk in self.blocks:
            x = blk(x)
        return self.head(x)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--steps", type=int, default=30)
    ap.add_argument("--warmup", type=int, default=5)
    ap.add_argument("--batch", type=int, default=512)
    ap.add_argument("--dim", type=int, default=2048)
    args = ap.parse_args()

    device = "cuda"
    model = TinyNet(dim=args.dim).to(device).eval()
    # NOTE: this is where the profiler's Layer-1 injection would add (env-gated):
    #   from nvtx_instrument import auto_instrument
    #   handles = auto_instrument(model)     # no-op unless APEX_NSYS_NVTX_MODULES=1
    x_cpu = torch.randn(args.batch, args.dim)   # pageable CPU input (not pinned)

    times = []
    with torch.inference_mode():
        for step in range(args.warmup + args.steps):
            t0 = time.perf_counter()
            x = x_cpu.to(device)                 # pageable H2D
            out = model(x)
            _ = float(out.sum().item())          # per-step blocking sync
            torch.cuda.synchronize()
            dt = time.perf_counter() - t0
            if step >= args.warmup:
                times.append(dt)

    times.sort()
    med = times[len(times) // 2]
    print(f"[py_module] steady steps={len(times)} median_step={med*1e3:.2f} ms "
          f"throughput={args.batch/med:.0f} samples/s")


if __name__ == "__main__":
    main()