# Fixture 1 — py-pipeline (Python, functional)

[← セットアップとインストールは README](../README.md)

Python + CUDA workload built as **plain functions** (`preprocess` / `infer` / `postprocess`) —
no `nn.Module` subclasses. Exercises the profiler's **function-level NVTX annotation** path.

Planted bottlenecks: pageable H2D in `preprocess`, tiny-kernel flood in `infer`
(40× `sin`), per-step `.item()` sync in `postprocess`.

## Run

```bash
cd py-pipeline
python pipeline.py --steps 30 --warmup 5
```

Requires torch + CUDA. Flags: `--steps N` (steady steps), `--warmup N`, `--batch N`,
`--dim N`. Prints the steady-state median step time so you can sanity-check it runs
before profiling.

## Profiler interview — what it asks & what answers to try

This fixture is built to make the profiler's **function-level NVTX annotation** branch
the "right" answer. The interview order (from the profiler's DESIGN.md):
**context-detect → Q-target → Q-nvtx → Q-trace → Q-gpu → Q-distributed → Q-output**.

Launch command to give the profiler:
```
python pipeline.py --steps 30 --warmup 5     (cwd: py-pipeline/)
```

| Step | Question | Answer to try | Why / what it tests |
|---|---|---|---|
| context-detect | "You seem to be profiling …?" | confirm it's this pipeline | reads README/CLAUDE if present; should not over-claim "PyTorch model" since there are no modules |
| Q-target | Python / PyTorch / C++ / generic | **Python + CUDA (PyTorch ops, functional)** | routes NVTX to function-level, not module hooks |
| Q-nvtx | already have / agent adds / none | **agent adds them** | invokes nvtx-inject |
| └ which NVTX strategy | Layer 0 / module hooks / phase ranges / **function annotation** | **function annotation** | the key test: `preprocess`/`infer`/`postprocess` are plain functions; module hooks would find nothing. Agent should read `pipeline.py`, list these 3 functions, offer to wrap them |
| Q-capture | cudaProfilerApi markers / delay-duration | **cudaProfilerApi** | tests Python marker insertion (`torch.cuda.profiler.start()/stop()`) around the steady loop, after `--warmup` |
| Q-trace | minimal / cpu-sampling / rich | **rich** (`cuda,osrt,nvtx -s cpu`) | NVTX needs the nvtx trace; cpu-sampling helps explain host gaps |
| Q-gpu | which device | **0** | sets `CUDA_VISIBLE_DEVICES=0` |
| Q-distributed | (skipped — not torchrun/mpirun) | — | single process |
| Q-output | dir / base | e.g. `prof/py_pipeline` | SQLite export for the analyzer |

**Expected planted findings** (for the analyzer afterwards): pageable H2D in
`preprocess`, a tiny-kernel flood in `infer` (the 40× `sin`), a per-step `.item()`
sync in `postprocess`.

**Alt run to also try:** Q-nvtx = **none / don't touch code** → should fall back to
`--pytorch=autograd-nvtx` (op-level) and state the caveat that it can't attribute to
your functions.

## What "passing" looks like

1. context-detect makes a reasonable, non-over-claiming guess.
2. Q-target routes correctly (function annotation available).
3. nvtx-inject, when chosen, edits the **right** thing reversibly: `pipeline.py` gets
   `nvtx_range` wrappers on the 3 functions.
4. capture-window inserts language-correct markers (torch) OR picks delay/duration,
   and the assembled `nsys profile` command carries the matching flag.
5. `run_nsys.sh` produces a `.nsys-rep` + `.sqlite` and the `=== PROFILE RESULT ===`
   block.
6. (end-to-end) the analyzer's `nvtx-timeline` can anchor on the injected ranges
   (`preprocess`/`infer`/`postprocess`).
