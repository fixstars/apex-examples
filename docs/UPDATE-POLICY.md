# Update Policy for `apex-examples`

This document defines the official policy for how and when `apex-examples` is
updated in response to changes in `apex-marketplace`, and how the examples
repo relates to `apex-marketplace-serve`.

It is a **maintainer-facing** document. Three decisions (lockstep enforcement,
examples tagging, serve-floor policy) are left open with provisional
recommendations; they are marked *provisional* and must be ratified by the
APEX team before they become binding.

---

## 1. Context — the three-repo relationship

```
apex-examples          (public)    downstream consumer; open-sourced to GitHub
    ^
    |  installs plugins via apm
    |
apex-marketplace       (private)  plugin source; each plugin is tagged
    |                             <pkg>-v<semver> (e.g. perf-v0.1.0)
    |  clones main + mirrors all tags into a frozen image at build time
    v
apex-marketplace-serve (released) snapshot distributor; exposes the tags
                                 that existed at its build time only
```

The plugin **git tag** (`<pkg>-v<semver>`) is the real contract between an
example and a plugin. `apex-marketplace-serve` does not version the contract —
it merely mirrors plugin tags into a frozen image. A serve image v1.0.0
therefore exposes exactly the plugin tags that existed when that image was
built; it never updates unless rebuilt.

Users install plugins via `apm install` run inside an example directory,
which reads the example's `apm.yml` and installs the plugin tag recorded in
its `ref` field (section 6). This is the per-example version pin: each
example records the tag its walkthrough was validated against. "Always
latest" (section 2) means `main` always carries the latest *validated* tag,
not that users float to the newest tag on serve. The examples repo's
coupling to serve is **zero**; its coupling to the marketplace is **explicit
and event-driven** (defined in section 3).

---

## 2. "Always latest" model

- **User side:** clone `apex-examples/main`, `cd` into an example dir, and run
  `apm install` (which reads the example's `apm.yml` and installs the validated
  plugin tag recorded in its `ref`). "Always latest" means `main` always
  carries the latest **validated** tag — when a plugin change passes the
  trigger checklist (section 3), the `ref` in the affected example's
  `apm.yml` is bumped forward in the paired MR. The user gets "always latest
  validated," not "always newest tag on serve." There is no version juggling
  and no tags to pick.
- **Maintainer side:** `apex-examples/main` is kept in lockstep with
  `apex-marketplace/main`. The examples repo is effectively a downstream
  consumer and gated test fixture of the marketplace. It updates *only* when a
  plugin change alters an example's end-user experience (section 3), and that
  update lands **with** the plugin change, not after it.

`main` is the only supported line. There is one piece of user-facing version
information: a serve-floor recorded in `COMPATIBILITY.md` (section 10). A user
on an older serve image who clones latest examples either updates serve
(`docker compose pull && docker compose up -d`) or checks out an older
examples commit (if examples tags exist — see section 8).

---

## 3. Trigger checklist — when an example MUST be updated

An example update is required when a plugin change causes any of the
following (the "end-user experience" surface):

1. A walkthrough's **expected output** differs — numeric values, table shape,
   top function or top source-line attribution (e.g. "56.35%
   `transform._omp_fn.0`" changing).
2. An **interview question's wording** or the set of answer choices changes.
3. A **skill name** or slash-command is renamed, removed, or added
   (e.g. `/perf-hotspot` renamed, or a new skill appears in the workflow).
4. A **CLI flag** or skill argument that the walkthrough tells the user to
   pass changes.
5. A **planted bug's location** or the code path that surfaces it shifts
   (rare; only when example source also moves).
6. The **skill workflow ordering** changes — which skill calls which, or the
   orchestrator's decision tree.

Patch-level plugin fixes that touch none of the above are
**install-compatible** and need no example update. The walkthrough continues
to hold against the patch because the end-user-visible behavior is unchanged.
When an update fires, the concrete mechanism that records it is a bump to the
`ref` field in the affected example's `apm.yml` (section 6).

---

## 4. Authorship (decided)

**The plugin author who changed the behavior writes the paired example
update.** They have the most context on what shifted. If the author is unsure
which example is affected, they hand off to the examples maintainer, but the
trigger detection (section 7) is expected to surface the affected example
automatically.

---

## 5. Update workflow (the "how and when")

1. Plugin author opens an MR in `apex-marketplace`.
2. If the diff hits any checklist item (section 3) for an example, the author
   opens a **paired MR** in `apex-examples` updating that example's
   walkthrough and/or source.
3. The author bumps the `ref` field in the affected example's `apm.yml` to
   the new validated plugin tag (section 6).
4. Both MRs land in the **same release window**. The plugin tag is **not
   cut** until the example is updated — this is the lockstep gate.
5. `COMPATIBILITY.md` (section 10) "main" row is updated with the new plugin
   tag and, if the change raises the serve floor, the new minimum serve
   version (per the policy ratified in section 9).
6. Users pulling `main` after this point get the matching example + plugin
   automatically.

---

## 6. Per-example `apm.yml` convention

Each example directory contains an `apm.yml` that is the **machine-readable
record** of which plugin tag the example's walkthrough was validated against.
It is the per-example contract; `COMPATIBILITY.md` (section 10) is the
human-readable summary derived from it.

### Fields

| Field | Value |
|---|---|
| `name` | example's own name (e.g. `tensorrt-demo`) |
| `version` | example's own version (semver) |
| `description` | short description |
| `author` | `APEX team` |
| `license` | `MIT` (examples are MIT; marketplace plugins are `LicenseRef-Fixstars-Proprietary`) |
| `devDependencies.apm[]` | one entry per plugin the example depends on |
| `.git` | marketplace URL placeholder `https://<your-marketplace-host>/apex/marketplace.git` |
| `.path` | `packages/<pkg>` |
| `.ref` | the validated plugin tag, e.g. `tensorrt-v0.1.0` |
| `mcp` | `[]` |
| `includes` | `auto` |

### Install path

Users clone `main`, `cd` into the example dir, and run `apm install` (which
reads `apm.yml` and installs the pinned `ref`). The unpinned form
`apm install <pkg>@apex-marketplace` is **not** the documented install path
under this model — README install snippets in each example should point users
to run `apm install` in the example dir, not the unpinned form.

### Template (tensorrt)

```yaml
name: tensorrt-demo
version: 1.0.0
description: tensorrt package demo project
author: APEX team
license: MIT
devDependencies:
  apm:
    - git: https://<your-marketplace-host>/apex/marketplace.git
      path: packages/tensorrt
      ref: tensorrt-v0.1.0
  mcp: []
includes: auto
```

### What `ref` means

The `ref` is the tag the walkthrough was last validated against. A newer
plugin tag that exists on serve but has not passed the trigger checklist
(section 3) is **not** referenced by any example until a paired MR bumps the
`ref`. Users therefore never receive an unvalidated tag through this path.

### Placeholder host

The `git:` URL stays as `https://<your-marketplace-host>/apex/marketplace.git`
(users replace it with their serve URL via `apm marketplace add`). The `ref`
is host-independent (it is a git tag), so pinning works regardless of which
serve instance the user points at.

### Multi-module examples: per-module `apm.yml`

For multi-module examples (`tensorrt/`, `nsys-profiler/`), the `apm.yml` lives at
the **module** level (one per module/fixture), not one per top-level directory. Each
module depends on exactly one plugin, so the `ref` pin — and, under ADR-0003 option
D4, the mirrored `version` — are unambiguous:

| Module | Plugin dep |
|---|---|
| `tensorrt/a_backbone/` | `tensorrt` |
| `tensorrt/b_dynamic/` | `tensorrt` |
| `tensorrt/c_kernel/` | `tensorrt` |
| `tensorrt/d_stateful/` | `tensorrt` |
| `nsys-profiler/py-pipeline/` | `nsys-profiler` |
| `nsys-profiler/py-module/` | `nsys-profiler` |
| `nsys-profiler/cpp-saxpy/` | `nsys-profiler` |

Both `tensorrt/` and `nsys-profiler/` use the **all-in-1 hub README** pattern: a
single `README.md` at the top level holds Installation, Usage (with skill
sub-sections), and a per-module walkthrough section. The module sub-folders hold
only the source + `apm.yml` (no per-module `README.md`). The agent runs in the
module sub-folder and is instructed not to read the hub walkthrough, preserving
the anti-cheat design.

Single-project examples (`perf/`, `vectorize/`, `nsys-analyzer/`) keep one
`apm.yml` at the top level, as in the template above. `nsys-analyzer/` has no
fixtures yet (pre-captured profiles to be published).

### README structure: single hub, required sections

Every example hub `README.md` (the docs-merge target) must contain `## Installation`
and `## Usage` as main sections. Skills with multiple use cases get sub-sections
under Usage. Multi-module examples (`tensorrt/`, `nsys-profiler/`) embed the
per-module walkthroughs as further `##` sections in the same hub README (all-in-1
pattern) — module sub-folders have no `README.md` of their own. The agent runs in
the module sub-folder and is told not to read the hub walkthrough, preserving the
anti-cheat design. `vectorize/demo/README.md` is an exception (agent-readable build
commands only; the walkthrough is the denied `README.md`).

These conventions are enforced by [`docs/lint-examples.sh`](lint-examples.sh),
which checks: README section presence, `apm.yml` existence per leaf dir, `apm.yml`
`ref` format (matches `<package-name>-v<semver>` derived from the `path:` field),
and absence of old folder names. The lint is offline-only — it does NOT verify the
ref tag exists on the remote (that's the L2 release gate's job, per ADR-0001, to
avoid the chicken-and-egg at paired-MR time).

---

## 7. Decision A — Lockstep enforcement

*Decision: **B — bot-nudge**, ratified 2026-08-04 by
[ADR-0006](adr/adr-0006-examples-release-contract.md). Clarification: the
bot flags + assists but does **not** auto-create the examples tag — the
human author does. See ADR-0006 §1.1–1.4 for the full decision.*

How is the "plugin MR must update the example" rule enforced?

### A. Process-only
CODEOWNERS on example-touched skill files + an MR template checklist
("Did your change affect any example walkthrough?").

- **Pros:** zero infra; fast; no bot maintenance.
- **Cons:** relies on human memory; silent drift if forgotten; no detection
  of indirect changes (a refactor that alters output without touching
  walkthrough strings).

### B. Bot-nudge (provisional recommendation)
A bot scans the plugin MR diff for skill names, interview strings, and CLI
flags referenced in any example's walkthrough, then posts a comment: "this MR
likely affects `perf/` and `tensorrt/` — open a paired
`apex-examples` MR." Detection is automated; the fix is still human. No CI
gate.

- **Pros:** automated detection; cheap; no CI infrastructure required;
  surfaces indirect changes a human reviewer might miss.
- **Cons:** heuristic — false positives (noise) and false negatives (a
  semantic change not caught by string match). Needs a small script and bot
  wiring (e.g. a GitLab webhook or CI job that diffs against the examples
  repo's walkthrough files).

### C. Hard CI gate
The marketplace MR is blocked by CI that runs the affected example's
walkthrough against the new plugin and diffs the expected outputs.

- **Pros:** strongest guarantee; catches semantic drift the bot misses.
- **Cons:** expensive — perf needs CPU, nsys/tensorrt need CUDA in CI; may
  be infeasible for GPU examples; slow; high maintenance.

*Provisional recommendation: **B**. Can graduate to **C** for the CPU-only
examples (perf) later, where the cost is bearable and the guarantee valuable.*

---

## 8. Decision B — Examples tagging

*Provisional recommendation: **C — tags only on serve-floor bumps**. To be
ratified.*

Does the examples repo itself get git tags?

### A. No tags — pure main
`main` is the only line; users always clone `main`.

- **Pros:** simplest; matches the "always latest" promise exactly; zero
  ceremony.
- **Cons:** a user who can't or won't update serve has no easy rollback to a
  matching examples snapshot.

### B. Checkpoint tags on each update
Cut an examples git tag (e.g. `examples-v2026.02` or `examples-v1.0.0`)
each time `main` is updated for a plugin change.

- **Pros:** gives a recovery point for users pinned to an old serve image.
- **Cons:** tags proliferate for routine walkthrough-refresh updates that
  don't actually break old serve; signal-to-noise is low.

### C. Tags only on serve-floor bumps (provisional recommendation)
Tag examples only when a plugin change raises the serve floor (a breaking
change, per section 9). Routine walkthrough refreshes float on `main`
untagged.

- **Pros:** tags carry meaning (each one = "you need at least this serve
  version"); few tags; clean signal.
- **Cons:** requires knowing when the floor actually moves (depends on
  section 9 decision); slightly more thought per release.

*Provisional recommendation: **C** — tags carry signal, not churn. Pairs
naturally with the adaptive serve-floor policy in section 9 (a tag is cut
exactly when the floor moves).*

---

## 9. Decision C — Serve-floor / lag policy

*Provisional recommendation: **C — adaptive**. To be ratified.*

How is the serve floor (the minimum `apex-marketplace-serve` image version
required to run `examples/main`) decided? This encodes the intuition that
"it depends on what version breaks things."

### A. Strict — latest examples needs latest serve
Floor = the serve version current at the time of any plugin change.
`COMPATIBILITY.md` simply says "≥ latest serve."

- **Pros:** simplest mental model; never a compatibility surprise.
- **Cons:** forces every user to update serve on every breaking example
  change, even if the change did not need a newer serve feature.

### B. Lenient — one-version backward compat
`main` must work with serve *N* and *N-1*. The author verifies backward
compatibility each update; the floor only moves on a major change.

- **Pros:** lenient for slow-to-update customers.
- **Cons:** verification burden each time; may be impossible if a plugin
  tag genuinely only exists in serve *N* (i.e. the tag was cut after serve
  *N-1* was built).

### C. Adaptive — floor moves only when a plugin tag requires it (provisional recommendation)
Floor = the serve version that **first shipped the plugin tag** the example
now relies on. If the plugin change is a patch on an existing tag's
`major.minor` (already present in the current floor's image), the floor
does not move. Determined by querying "which serve image first contained
tag X" (a single GitLab/mirror lookup, or a small script against serve
release notes), or by attempting `apm install` against a reference older
serve image.

- **Pros:** the floor moves only when it genuinely must; minimal user
  disruption; directly matches "depends on what version breaks things."
- **Cons:** needs the "first-shipped-in-serve" lookup (cheap — the
  information is in serve release notes or derivable from the mirror); slightly
  more bookkeeping than strict.

### D. No floor tracked
If an old serve breaks the example, it is reported as a bug and we fix the
example or tell the user to update.

- **Pros:** zero bookkeeping.
- **Cons:** riskiest; silent breakage for slow customers; no proactive
  contract.

*Provisional recommendation: **C** — directly encodes the "depends on what
version breaks things" intuition. Pairs with section 8 option **C** (an
examples tag is cut exactly when the floor moves, making tags meaningful).*

---

## 10. `COMPATIBILITY.md` role (independent of decisions above)

Regardless of which options in sections 7–9 are ratified, a root-level
`COMPATIBILITY.md` (public) records the contract surface:

- The **trigger checklist** (section 3) — so open-source contributors and
  users can see what constitutes an example-affecting change.
- The **current serve floor** — the minimum `apex-marketplace-serve`
  version required to run `examples/main`, per the ratified section 9
  policy.
- A **pointer to the latest paired MR** that updated `main` (a short log
  of "examples update X ↔ marketplace MR Y").
- The per-example `apm.yml` `ref` values (section 6) are the
  machine-readable source of truth. `COMPATIBILITY.md` is the human-readable
  summary and **can be generated from the refs** — when all examples carry
  `apm.yml` refs, the compatibility table is derivable rather than
  hand-maintained.

This file is the public contract; the policy document you are reading
governs *how* it gets maintained. Its contents are minimal and
human-readable; it is not a machine-consumed lockfile.

---

## 11. Open questions

The following three decisions were **provisional** pending APEX team
ratification. **Lockstep enforcement is now ratified by [ADR-0006](adr/adr-0006-examples-release-contract.md)
(2026-08-04)** — option B (bot-nudge), with the clarification that the bot
flags + assists but does **not** auto-create the examples tag (the human
author does). The other two (examples tagging §8, serve-floor policy §9)
remain provisional:

| Decision | Section | Status |
|---|---|---|
| Lockstep enforcement | 7 | **Ratified — ADR-0006** (B — bot-nudge, no auto-tag fast-path) |
| Examples tagging | 8 | **C — tags only on serve-floor bumps** (provisional) |
| Serve-floor policy | 9 | **C — adaptive** (provisional) |

Until ratified, the two remaining provisional recommendations are the
working assumption. This document is a living reference; update it when a
decision is ratified, and record the ratification date and deciding party in
the section header.

Additional decisions are recorded as ADRs (Architecture Decision Records)
under [`docs/adr/`](adr/):

| ADR | Status | Title |
|---|---|---|
| [ADR-0001](adr/adr-0001-release-gate.md) | Accepted | Release gate: hybrid nudge at L1, hard gate at L2 tag push |
| [ADR-0002](adr/adr-0002-trigger-flags-role.md) | Accepted | Role of the 5 trigger flags when the gate is on version/tag |
| [ADR-0003](adr/adr-0003-example-version-bump.md) | In consideration | Example's own `version` field bump strategy |
| [ADR-0004](adr/adr-0004-skill-runs-guarantee.md) | Accepted | Guarantee that the skill runs on the new plugin tag |
| [ADR-0005](adr/adr-0005-verification-flow.md) | In consideration | User verification flow documentation |
| [ADR-0006](adr/adr-0006-examples-release-contract.md) | Accepted | Examples release contract (lockstep gate + per-example docs archive) |

ADR-0006 ratifies ADR-0002 (option B, minus the auto-tag fast-path) and
ADR-0004 (option E1, author attests). ADRs with status "In consideration" are
not binding until ratified.

---

## Change log

| Date | Change |
|---|---|
| 2026-08-03 | Initial draft. Three decisions left provisional pending APEX team ratification. |
| 2026-08-03 | Added section 6: per-example `apm.yml` convention (Model A — pins validated tag, users install via `apm.yml`). Renumbered subsequent sections. |
| 2026-08-03 | Added section 6 multi-fixture note (apm.yml at sub-folder level for nsys-examples and tensorrt-example). |
| 2026-08-03 | Moved release gate and open decisions (trigger-flags role, example version bump, skill-runs guarantee, verification flow) to ADRs under `docs/adr/`. Section 11 updated with an ADR index. |
| 2026-08-03 | Renamed example folders (dropped `-example`/`-plugin`/`-examples` suffixes): `perf-plugin/`→`perf/`, `vectorize-example/`→`vectorize/`, `tensorrt-example/`→`tensorrt/`. Split `nsys-examples/` into 3 top-level examples (`nsys-py-pipeline/`, `nsys-py-module/`, `nsys-cpp-saxpy/`). Section 6 multi-fixture table updated to reflect new structure (nsys fixtures are now top-level; only `tensorrt/` remains multi-module). |
| 2026-08-03 | Re-grouped nsys to mirror marketplace packages: moved 3 nsys fixtures under `nsys-profiler/` as sub-folders (py-pipeline, py-module, cpp-saxpy); created `nsys-analyzer/` with placeholder profiles. Both nsys-profiler and tensorrt now use the all-in-1 hub README pattern (single README with Installation/Usage/walkthroughs; module sub-folders have no README). Added `apm.yml` to all leaf examples (perf, 4 tensorrt modules, 3 nsys fixtures) + fixed vectorize ref. Added `docs/lint-examples.sh` enforcing: README sections (Installation, Usage), apm.yml existence, apm.yml ref format (matches `<pkg>-v<semver>`), no old folder names. Section 6 updated with README-structure criterion + lint reference. |
| 2026-08-04 | Ratified Lockstep enforcement (§7) via ADR-0006 (option B — bot-nudge, no auto-tag fast-path). ADR-0006 also ratifies ADR-0002 (trigger-flags role) and ADR-0004 (skill-runs guarantee — E1, author attests), and defines the per-example docs archive contract (release-artifact, `docs-<pkg>@<ver>`) that `apex-marketplace-serve` consumes for the tutorials section. ADR index (§11) and `docs/adr/README.md` updated; ADR-0002/0004 status flipped to Accepted. |
