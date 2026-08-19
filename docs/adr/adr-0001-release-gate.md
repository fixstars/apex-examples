# ADR-0001 — Release gate: hybrid nudge at L1, hard gate at L2 tag push

Date: 2026-08-03
Status: Accepted

## Context

Plugin releases in `apex-marketplace` have two levels:

| Level | Where | What it means |
|---|---|---|
| **L1 — internal** | `packages/<name>/apm.yml` `version` field, changed in the MR | "we intend to release X.Y.Z" — a declaration, not a release |
| **L2 — external** | git tag `<pkg>-v<version>`, pushed later | **the release act** — this is what `apex-marketplace-serve` mirrors and what users install |

Confirmed in `apex-marketplace/AGENTS.md`: the version lives in
`packages/<name>/apm.yml`; the tag `<name>-v<version>` is granted at release
time. `apm pack --check-versions` verifies L1 consistency (the tag renders
uniquely); it does not push the tag. L1→L2 is a separate, deliberate step.

The "always latest" model (UPDATE-POLICY.md section 2) requires that every
released plugin tag has a matching validated example. Without a gate, a
plugin tag can be pushed without the example being updated, and the first
user to install the new tag hits a walkthrough that doesn't match.

The gate must fire somewhere. Three placements were considered.

## Options

### A. Block the MR (gate L1)

Bot blocks the marketplace MR from merging until the examples tag exists.

- **Pro:** `main` is always fully releasable — every `version` on main has a
  matching examples tag, so any future tag push is safe by construction.
- **Con:** a plugin fix that should land fast is held hostage to the examples
  walkthrough work, which may take days (GPU walkthrough). The *code* can't
  advance even though only the *release* needs the example. Couples
  code-merge cadence to release cadence.

### B. Block the tag push only (gate L2, no nudge)

MR merges freely; the git tag push is rejected (pre-receive hook /
protected-tag rule) unless the examples tag exists. No bot comment at MR
time.

- **Pro:** decouples code-merge cadence from release cadence. The plugin fix
  lands immediately; only the *release* waits for the example. The gate is
  on the *thing that matters* (the released artifact), not the *thing that
  doesn't* (main's internal state).
- **Con:** the author gets no help at MR time. An author who merges without
  thinking about examples hits the hard gate at tag time with no
  preparation.

### C. Hybrid: nudge at L1, hard gate at L2 (chosen)

MR merges freely with a bot **nudge** — "you bumped `perf` to 0.2.0 —
examples tag `perf-example-v0.2.0` must exist on `apex-examples` before you
push `perf-v0.2.0`; here's what likely needs updating: …" (the nudge
carries the "assist"). The MR is **not** blocked. The hard gate fires at
L2: the tag push is rejected unless the matching examples tag exists.

- **Pro:** combines B's decoupling with an assist at the moment the author
  has context and time (MR time). The later tag push is not a surprise.
- **Con:** slightly more bot machinery (two checks — nudge at L1, gate at L2
  — not one). The nudge can be ignored (it's not a block), so an author who
  merges without reading it hits the hard gate at tag time.

## Decision

**Gate C — hybrid.**

- At L1 (the MR): nudge, no block.
- At L2 (the tag push): hard gate — push rejected unless the matching
  examples tag exists on `apex-examples`.

## Consequences

- `main` can carry "unreleased versions" — a version in `apm.yml` with no
  tag yet. Discipline required: tag promptly after merge; do not let
  versions stack. If two version bumps stack on main before either is
  tagged, cut the older one first (the newer one's example work is expected
  to cover both, since a walkthrough is validated against the latest plugin
  state).
- The gate proves "someone made a paired commit." It does **not** prove the
  walkthrough was *correctly* updated (a trivial/empty-commit tag can
  satisfy the gate under deadline pressure). The gate is the
  **release-prevention** layer. The **content-correctness** layer —
  proving the skill actually runs on the new plugin tag — is
  [ADR-0004](adr-0004-skill-runs-guarantee.md). They are complementary.
- Two sub-decisions are open and captured as ADRs:
  - Who creates the examples tag, and escape valves for release frequency
    → folded into [ADR-0002](adr-0002-trigger-flags-role.md), since the
    answers depend on the trigger-flags role.
