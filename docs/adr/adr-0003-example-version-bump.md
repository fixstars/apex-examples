# ADR-0003 — Example's own `version` field bump strategy

Date: 2026-08-03
Status: In consideration

## Context

The example's `apm.yml` has a `version` field (e.g. `vectorize`
has `version: 1.0.0`). When a paired MR updates the example for a plugin
change, does the example's own version bump, and who decides? The
maintenance concern is the human judgment per bump.

This is coupled with [ADR-0004](adr-0004-skill-runs-guarantee.md): a
version bump is just a label unless there's a guarantee behind it. The
bump-type decision (this ADR) and the runs-validation decision
(ADR-0004) are separable, but the bump only means something if ADR-0004
is settled.

## Options

### D1. Automated bump in paired-MR CI

A bot decides bump type by diff analysis: new skill name / new CLI flag →
**minor**; removed/renamed skill → **major**; expected-output refresh →
**patch**. Human reviews but does not make the bump judgment.

- **Pro:** no human bump judgment; semver signal preserved; the bump is
  auditable (the diff that triggered it is in the MR).
- **Con:** bot can't verify the skill runs (that's ADR-0004); requires bot
  infra; bump-type heuristic may misjudge (e.g. a "new skill" that's
  actually a breaking change because it replaces an old one).

### D2. Auto date-stamp (e.g. `2026.08.03`), no semver

Bot sets `version` to the paired-MR merge date. No major/minor/patch
judgment ever.

- **Pro:** zero judgment; trivially automatable; user sees the date and
  knows the revision.
- **Con:** loses semver signal — user can't tell from the version whether
  the change was breaking; date versions don't compose with semver-range
  install paths (the marketplace object form's `version: "~2.1.0"` becomes
  meaningless for the example).

### D3. No `version` field — use the git commit SHA

Remove `version` from `apm.yml`. The example's identity is the git commit
the user cloned; users compare to a "latest validated commit" line in
`COMPATIBILITY.md`.

- **Pro:** truly zero ceremony; git SHA is the real identity of what the
  user checked out anyway; no version field to drift.
- **Con:** no machine-readable version in `apm.yml`; user must do a manual
  comparison against `COMPATIBILITY.md`; `apm view --lock` won't show an
  example version.

### D4. Mirror the plugin tag version (provisional lean)

When the paired MR bumps `ref` to `tensorrt-v0.2.0`, the example's
`version` becomes `0.2.0`. One source of truth — the plugin tag.

- **Pro:** automatic (no independent judgment); self-documenting — the
  version *is* the pairing statement (user sees `0.2.0` and knows it pairs
  with `tensorrt-v0.2.0`); the plugin's semver already encodes
  major/minor/patch and the example inherits it for free; works for every
  example in this repo because each sub-folder depends on exactly one
  plugin (see UPDATE-POLICY.md section 6 "Multi-fixture examples" —
  `nsys-profiler/` and `tensorrt/` have one `apm.yml` per
  sub-folder, each depending on one plugin).
- **Con:** a walkthrough *content* change that doesn't correspond to a
  plugin tag bump doesn't move the version. Arguably fine: such a change
  doesn't alter "which plugin this example works with," so the version
  (which encodes the pairing, not the walkthrough revision) shouldn't
  move. A user who wants the exact walkthrough revision uses the git
  commit SHA, not the example version.

### D5. Bump only on serve-floor bumps

Example `version` bumps only when a plugin change raises the serve floor
(aligns with UPDATE-POLICY.md section 8 option C tagging). Routine
walkthrough refreshes don't bump the example version.

- **Pro:** example version maps 1:1 to the serve-floor contract.
- **Con:** a user on `main` between floor bumps can't tell from the version
  whether the walkthrough was refreshed.

## Decision

Provisional lean: **D4**.

Not binding until ratified by the APEX team.

## Consequences

If **D4** is ratified:

- Requires the section 6 structural correction (apm.yml at sub-folder level
  for multi-fixture examples) — already captured in UPDATE-POLICY.md.
- A walkthrough content change without a plugin tag bump doesn't move the
  version. Users who want the exact walkthrough revision use `git rev-parse
  HEAD`, not the example version. This is a documentation point — the
  version encodes the pairing, not the walkthrough revision.
- The verification flow ([ADR-0005](adr-0005-verification-flow.md)) becomes
  clean: `apm view --lock` shows example `version: 0.2.0` and plugin
  `resolved_ref: tensorrt-v0.2.0` — they match, so the pairing is confirmed
  *by construction*.
