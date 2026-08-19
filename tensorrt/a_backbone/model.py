# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: MIT

"""Module A — clean backbone (happy path).

A stock conv backbone: pure conv/bn/relu, static input, no custom op, no state,
no dynamic shape. This is what "convertible with zero troubleshooting" looks like.

Run eager:
    python a_backbone.py
"""
import torch
import torchvision


def build_model():
    # Seed before construction so every process (convert vs --skip-conversion)
    # builds bit-identical weights. resnet18() has random init by default, so
    # without this the convert-process model and a fresh eager reference would be
    # two different random networks — making eager-vs-TRT comparison meaningless.
    torch.manual_seed(0)
    return torchvision.models.resnet18().eval()


def make_inputs(seed=0):
    g = torch.Generator().manual_seed(seed)
    return (torch.randn(1, 3, 224, 224, generator=g),)


if __name__ == "__main__":
    model = build_model()
    inputs = make_inputs()
    with torch.no_grad():
        out = model(*inputs)
    print("a_backbone output:", tuple(out.shape), "sum=", float(out.sum()))