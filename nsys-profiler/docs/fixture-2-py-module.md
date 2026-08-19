# Fixture 2 — py-module (PyTorch, user nn.Module tree)

[← セットアップとインストールは README](../README.md)

PyTorch workload built from **user-defined `nn.Module` subclasses** (`Stem` / `Block` /
`Head` / `TinyNet`). Exercises the profiler's **Layer-1 module-hooks** NVTX path
(`auto_instrument` attaches named ranges `stem` / `blocks.0..2` / `head`).

Planted bottlenecks: pageable H2D each step, a tiny-op flood **inside `Block`**
(localizes to `blocks.N`), a per-step `.item()` sync.

## Run

```bash
cd py-module
python model.py --steps 30 --warmup 5
```

Requires torch + CUDA. Flags: `--steps N` (steady steps), `--warmup N`, `--batch N`,
`--dim N`. Prints the steady-state median step time so you can sanity-check it runs
before profiling.

## Profiler interview — what it asks & what answers to try

This fixture is built to make the profiler's **Layer-1 module-hooks** branch the
"right" answer. The interview order (from the profiler's DESIGN.md):
**context-detect → Q-target → Q-nvtx → Q-trace → Q-gpu → Q-distributed → Q-output**.

Launch command:
```
python model.py --steps 30 --warmup 5     (cwd: py-module/)
```

| Step | Question | Answer to try | Why / what it tests |
|---|---|---|---|
| context-detect | "You seem to be profiling …?" | confirm: a small PyTorch nn.Module net | should detect PyTorch + that it has user modules |
| Q-target | Python / **PyTorch** / C++ / generic | **PyTorch** | enables model-aware NVTX (Layers 0/1/2) |
| Q-nvtx | already have / agent adds / none | **agent adds them** | invokes nvtx-inject |
| └ which NVTX strategy | Layer 0 / **module hooks (L1)** / phase ranges / function annotation | **module hooks (Layer 1)** | the key test: `TinyNet` is a tree of user classes (`Stem`/`Block`/`Head`), so `auto_instrument` attaches named ranges `stem`/`blocks.0..2`/`head`. The agent adds the 2 lines after `model = TinyNet(...)` (or via env gate) and sets `APEX_NSYS_NVTX_MODULES=1` |
| └ depth sub-question | value / unlimited / agent-recommended | **unlimited** (shallow model) | tests the depth knob |
| Q-capture | cudaProfilerApi / delay-duration | **cudaProfilerApi** | torch marker insertion around the steady loop (note: model uses `inference_mode`, no backward — Layer-1 forward hooks cover it) |
| Q-trace | minimal / cpu-sampling / **rich** | **rich** | nvtx trace needed |
| Q-gpu | which device | **0** | — |
| Q-distributed | (skipped) | — | single process |
| Q-output | dir / base | `prof/py_module` | — |

**Expected planted findings:** pageable H2D each step, a tiny-op flood **inside
`Block`** (so the analyzer should localize it to `blocks.N`), a per-step `.item()`
sync. This is the fixture where module-scoped attribution (`--filter-nvtx=blocks.1`,
or `nvtx-timeline --root blocks.1`) should work.

**Contrast check:** running the **function annotation** strategy here is *also* valid
but less natural — confirm the agent recommends module hooks for this fixture and
function annotation for `py-pipeline`.

## What "passing" looks like

1. context-detect makes a reasonable, non-over-claiming guess.
2. Q-target routes correctly (PyTorch with user modules → module hooks available).
3. nvtx-inject, when chosen, edits the **right** thing reversibly: `model.py` gets
   the 2 hook-registration lines after `model = TinyNet(...)`.
4. capture-window inserts language-correct markers (torch) and the assembled
   `nsys profile` command carries the matching flag.
5. `run_nsys.sh` produces a `.nsys-rep` + `.sqlite` and the `=== PROFILE RESULT ===`
   block.
6. (end-to-end) the analyzer's `nvtx-timeline` can anchor on the injected module
   ranges (`stem`/`blocks.0..2`/`head`); `--filter-nvtx=blocks.1` localizes to
   `Block`.
