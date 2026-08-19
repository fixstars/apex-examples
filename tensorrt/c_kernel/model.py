# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: Apache-2.0

"""Module C — custom CUDA kernel.

One custom CUDA op mirroring deformable's hard shape with trivial math: a
scatter-add  out[i] = sum_j mask[i,j] * x[j]  done with atomicAdd into a
zero-initialized output, taking an int64 index input. Deliberately shaped to
reproduce the two real plugin bugs:
  1. int64 -> int32 bridge (TRT passes ints as int64).
  2. all-zeros output when the impl self-allocates + copies instead of writing
     directly into the TRT-provided outputs[0].

The .cu is compiled at runtime via torch.utils.cpp_extension.load_inline, so it
genuinely exercises both bugs. Requires nvcc + a CUDA GPU.

Run eager (needs CUDA):
    python c_kernel.py
"""
import torch
from torch import nn

_CUDA_SRC = r"""
#include <torch/extension.h>

__global__ void scatter_add_kernel(
    const float* x, const float* mask, const long* idx,
    float* out, int N, int M, int D) {
  // out[i, :] += mask[i, j] * x[ idx[j], : ]   summed over j
  int t = blockIdx.x * blockDim.x + threadIdx.x;
  if (t >= N * M) return;
  int i = t / M;
  int j = t % M;
  long src = idx[j];               // int64 index in, on purpose
  float w = mask[i * M + j];
  for (int d = 0; d < D; ++d) {
    atomicAdd(&out[i * D + d], w * x[src * D + d]);
  }
}

torch::Tensor scatter_add_cuda(torch::Tensor x, torch::Tensor mask, torch::Tensor idx) {
  int N = mask.size(0);
  int M = mask.size(1);
  int D = x.size(1);
  auto out = torch::zeros({N, D}, x.options());   // zero-initialized output
  int threads = 256;
  int blocks = (N * M + threads - 1) / threads;
  scatter_add_kernel<<<blocks, threads>>>(
      x.data_ptr<float>(), mask.data_ptr<float>(), idx.data_ptr<long>(),
      out.data_ptr<float>(), N, M, D);
  return out;
}
"""

_CPP_SRC = "torch::Tensor scatter_add_cuda(torch::Tensor x, torch::Tensor mask, torch::Tensor idx);"

_ext = None


def _load_ext():
    global _ext
    if _ext is None:
        from torch.utils.cpp_extension import load_inline
        _ext = load_inline(
            name="scatter_add_ext",
            cpp_sources=_CPP_SRC,
            cuda_sources=_CUDA_SRC,
            functions=["scatter_add_cuda"],
            verbose=False,
        )
    return _ext


class ScatterAdd(nn.Module):
    def forward(self, x, mask, idx):
        return _load_ext().scatter_add_cuda(x, mask, idx)


def build_model():
    # No learnable params here, but seed for consistency with the other modules.
    torch.manual_seed(0)
    return ScatterAdd().eval().cuda()


def make_inputs(seed=0):
    g = torch.Generator().manual_seed(seed)
    N, M, D, S = 16, 4, 8, 32  # rows, neighbors, feat dim, source count
    x = torch.randn(S, D, generator=g).cuda()
    mask = torch.rand(N, M, generator=g).cuda()
    idx = torch.randint(0, S, (M,), generator=g, dtype=torch.int64).cuda()  # int64 on purpose
    return (x, mask, idx)


if __name__ == "__main__":
    model = build_model()
    x, mask, idx = make_inputs()
    out = model(x, mask, idx)
    print("c_kernel output:", tuple(out.shape), "sum=", float(out.sum()))