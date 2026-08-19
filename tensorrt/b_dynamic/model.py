# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: Apache-2.0

"""Module B — dynamic-shape module.

A single attention block whose key/value length grows 1 -> K during a warmup,
then stabilizes at K. The SAME module is called at many shapes, mimicking
SparseDrive's instance-queue filling. Naive conversion over all frames trips the
dynamic-shape path; the fix is to convert using only steady-state (step >= K)
frames so shapes are consistent.

Run eager:
    python b_dynamic.py
"""
import torch
from torch import nn

D = 64
K = 8  # steady-state key/value length


class Attn(nn.Module):
    def __init__(self):
        super().__init__()
        self.attn = nn.MultiheadAttention(D, num_heads=4, batch_first=True)

    def forward(self, query, kv):
        # query: (B, 1, D)   kv: (B, L, D)  with L = min(step, K)
        out, _ = self.attn(query, kv, kv)
        return out


def build_model():
    # Deterministic weights so any process builds the identical model
    # (cross-process eager-vs-TRT comparison needs the same weights).
    torch.manual_seed(0)
    return Attn().eval()


def make_inputs(seed=0):
    """One full warmup-then-steady sequence of (query, kv) pairs.

    Steps 1..K grow the kv length; the rest stay at K. A frame is "steady-state"
    when its kv length == K.
    """
    g = torch.Generator().manual_seed(seed)
    frames = []
    for step in range(1, 2 * K + 1):
        L = min(step, K)
        query = torch.randn(1, 1, D, generator=g)
        kv = torch.randn(1, L, D, generator=g)
        frames.append((query, kv))
    return frames


def is_steady(frame):
    _, kv = frame
    return kv.shape[1] == K


if __name__ == "__main__":
    model = build_model()
    frames = make_inputs()
    with torch.no_grad():
        for i, (q, kv) in enumerate(frames):
            out = model(q, kv)
            tag = "steady" if is_steady((q, kv)) else "warmup"
            print(f"frame {i:2d} kv_len={kv.shape[1]:2d} [{tag}] -> {tuple(out.shape)}")