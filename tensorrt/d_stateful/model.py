# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: Apache-2.0

"""Module D — refactor-required module (referential transparency).

A stateful module that is NOT a pure DAG: holds self.state, branches on
`state is None` (first call), and mutates it across calls. This is the exact
NoTransparencyClass from the tensorrt-refactor skill's referential-transparency.md.
Direct ONNX export fails / is non-deterministic (internal state + branch). The
fix is to lift state to explicit I/O (TransparencyClass below), zero-init the
first frame, so it becomes a pure DAG that exports cleanly.

Run eager:
    python d_stateful.py
"""
import torch
from torch import nn


class NoTransparencyClass(nn.Module):
    """Not exportable: internal state + first-call branch."""

    def __init__(self):
        super().__init__()
        self.fc = nn.Linear(256, 256)
        self.state = None

    def forward(self, x):
        if self.state is None:
            self.state = x
        else:
            x = self.state + x
            self.state = x
        return self.fc(x)


class TransparencyClass(nn.Module):
    """Refactored: state is explicit I/O -> pure DAG, exportable."""

    def __init__(self):
        super().__init__()
        self.fc = nn.Linear(256, 256)

    def forward(self, x, state):
        y = state + x
        return self.fc(y), y


def build_model():
    # Deterministic weights so any process builds the identical model
    # (cross-process eager-vs-TRT comparison needs the same weights).
    torch.manual_seed(0)
    return NoTransparencyClass().eval()


def make_inputs(seed=0):
    g = torch.Generator().manual_seed(seed)
    return (torch.randn(256, 256, generator=g),)


if __name__ == "__main__":
    # Equivalence check: refactored module with zero-init state matches the
    # stateful one's first call (the lesson the refactor skill teaches).
    no_t = NoTransparencyClass().eval()
    t = TransparencyClass().eval()
    t.fc = no_t.fc  # share weights for the comparison only
    x = make_inputs()[0]
    with torch.no_grad():
        a = no_t(x)
        b = t(x, torch.zeros(256, 256))[0]
    print("d_stateful equivalence (first frame):", bool(torch.allclose(a, b)))