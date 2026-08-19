# ADR-0004 — Guarantee that the skill runs on the new plugin tag

Date: 2026-08-03
Status: Accepted — ratified by ADR-0006 (option E1, author attests)

> **Ratified 2026-08-04 by [ADR-0006](adr-0006-examples-release-contract.md).**
> The plugin author attests in the paired `apex-examples` MR that they ran
> the skill locally against the new plugin tag and the walkthrough holds
> (option E1). No CI executes the walkthrough. The CPU-only graduation to a
> hard CI gate (option E2, for `perf` only) is left open as a future
> option. The original options and reasoning are retained below for
> history.

## Context

The version bump ([ADR-0003](adr-0003-example-version-bump.md)) is just a
label. The guarantee that the example still works on the new plugin tag is
a separate question — the "nightmare" risk: the bot bumps and says
"validated," but nobody ran the skill, and the first user hits a broken
example.

This is the same cost-vs-guarantee shape as UPDATE-POLICY.md section 7
(lockstep enforcement). The release gate
([ADR-0001](adr-0001-release-gate.md)) is the *release-prevention* layer
("examples tag exists" — someone made a paired commit); this ADR is the
*content-correctness* layer (the skill actually runs). They are
complementary: the gate is cheap and preventive; the guarantee is
expensive and verifies content. "Examples tag exists" proves someone
made a paired commit, not that the walkthrough was correctly updated.

## Options

### E1. Author attests

The plugin author attests in the paired MR that they ran the skill
locally against the new plugin tag and it works. No CI execution.

- **Pro:** zero CI cost; the author has the local environment (GPU, tools)
  to actually run it; fastest.
- **Con:** relies on author honesty/diligence — the "nightmare" risk is
  exactly this; no audit trail beyond a checkbox; silent breakage if the
  author guesses wrong.

### E2. Bot runs the walkthrough in CI (full for CPU, smoke for GPU)

The paired-MR bot runs the example's walkthrough against the new plugin
tag in CI; the version only bumps if the run succeeds. For CPU examples
(`perf`) the full walkthrough runs. For GPU examples
(`nsys-profiler`, `tensorrt`) a smoke-check runs (skill loads,
first interview step works, the planted bug is still detected at the
expected location) without the full walkthrough.

- **Pro:** strongest guarantee; catches breakage *before* the bump;
  machine-verified, not author-attested.
- **Con:** GPU CI is expensive and may be infeasible (needs CUDA drivers,
  nvcc, a real GPU); the smoke-check is partial — it doesn't catch
  breakage deeper in the walkthrough (e.g. step 4 breaks but steps 1-3
  work); slow; high maintenance.

### E3. Periodic manual sweep, no per-MR gate

The version bumps by diff analysis at MR time with no execution
guarantee. A periodic (e.g. monthly) manual sweep runs all examples
against the latest plugins on a real GPU box and flags breakage.
Breakage between sweeps is a bug the user reports.

- **Pro:** zero per-MR CI cost; spreads the validation effort; feasible
  for GPU (a real GPU box, once a month, is achievable); decouples the
  bump from the validation (fast merges, periodic verification).
- **Con:** user-facing risk between sweeps — a user who clones `main`
  right after a bad bump and before the next sweep hits a broken skill;
  breakage is discovered late; the "validated" claim at bump time is
  weak.

### E4. Community signal

No automated guarantee. The version bumps by diff analysis. If a user
hits a broken skill, they file an issue; we fix forward. The README
explicitly says "examples are validated on a best-effort basis; if you
hit a break, please file an issue."

- **Pro:** zero cost; honest about what we can guarantee; appropriate for
  an open-source repo where the team can't promise enterprise-grade
  validation on a free artifact.
- **Con:** users are the test suite (bad UX for the first user to hit
  it); no proactive guarantee; reputation risk for an open-source
  release.

### E5. Defer / fold into section 7

Don't pin the guarantee mechanism now. Document the question and leave
it provisional alongside UPDATE-POLICY.md section 7 (lockstep
enforcement).

- **Pro:** avoids premature commitment; the decision is genuinely the
  same shape as section 7; the policy doc already has a "provisional"
  pattern for exactly this.
- **Con:** leaves a known gap; the "nightmare" risk is unaddressed until
  ratification; a reader can't tell what guarantee the "validated" label
  carries today.

## Decision

Provisional lean: none — genuinely open.

**E5** (defer) may be the honest interim. Not binding until ratified by
the APEX team.

## Consequences

This ADR is the content-correctness layer complementing the release gate's
release-prevention layer. Whatever option is ratified:

- The release gate (ADR-0001) remains the cheap preventive layer.
- This ADR determines how much we *prove* (vs. assert) that the example
  works on the new plugin tag.
- The cost axis is CPU-vs-GPU CI feasibility — the same axis as
  UPDATE-POLICY.md section 7 option C. A hybrid (E2 for CPU examples, E1
  or E3 for GPU examples) is a possible ratification outcome.
