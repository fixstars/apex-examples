# ADR-0006 — Examples release contract (lockstep gate + per-example docs archive)

Date: 2026-08-04
Status: Accepted

## Context

`apex-examples` ships as a public companion to the `apex-marketplace` plugins.
Two seams were left open by the prior ADRs and the update policy:

1. **Enforcement & verification (the lockstep question).** ADR-0001 accepted the
   L2 hard gate (a plugin tag `<pkg>-v<semver>` push to the marketplace is rejected
   unless the matching examples tag exists on `apex-examples`). But
   ADR-0002 (the role of the 5 trigger flags — was "In consideration", lean B)
   and ADR-0004 (the skill-runs guarantee — was "In consideration", genuinely
   open) left two questions unanswered: *who* detects that an MR affects an
   example, and *who* verifies the skill still runs on the new plugin tag.
2. **Docs baking (the serve question).** `apex-marketplace-serve` bakes
   marketplace package READMEs and tool docs into its MkDocs site at build time
   (serve ADR-001 / ADR-003), but `tutorials/examples.md` is an explicit
   placeholder ("公開後に各例 README への自動焼き込みへ切替予定"). How the serve
   docs build consumes `apex-examples` content was undefined.

This ADR ratifies both, and is the single decision document. It ratifies
ADR-0002 (option B, minus the auto-tag fast-path) and ADR-0004 (option E1 —
author attests) by reference.

The scope of this ADR is `apex-examples`. The serve-side wiring that *consumes*
the docs archive defined here is a downstream consequence implemented in the
serve repo; this ADR defines the **contract** the serve repo depends on.

## Decision 1 — Lockstep enforcement & verification

### 1.1 L1 — nudge, not a block

When a marketplace MR changes a plugin's `version` field (in
`packages/<pkg>/apm.yml`), a bot posts a comment on the MR:

- naming the affected `apex-examples/<example>/` directory (or directories, for
  multi-module examples like `tensorrt/` which has 4 modules each pinning the
  same plugin);
- stating: "verify the example is still compatible with the new version; if
  the walkthrough output / interview wording / CLI flags / skill workflow
  changed (per the trigger checklist in UPDATE-POLICY.md §3), update the
  example and create the matching `<pkg>-v<new-semver>` tag on `apex-examples`";
- linking to the trigger checklist and the affected example's hub README.

The MR is **not blocked** — the plugin code can advance for internal testing
without waiting on the example. The bot only flags.

### 1.2 L2 — hard gate (already accepted, ADR-0001)

Pushing the git tag `<pkg>-v<semver>` to the marketplace is **rejected** by a
pre-receive hook / protected-tag rule unless the **same** `<pkg>-v<semver>` tag
exists on `apex-examples`. This is ADR-0001 (Accepted) — this ADR affirms it as
the binding gate and fixes the "who creates the tag" sub-decision: **the human
author creates it manually.** There is no bot auto-tag fast-path.

The examples tag is the lockstep receipt. The L2 gate proves "someone made a
paired commit on the examples side for this version." It does **not** prove
the walkthrough was correctly updated — that is Decision 1.3.

### 1.3 Verification — author attests (ADR-0004 E1)

The plugin author attests in the paired `apex-examples` MR that they ran the
example's skill locally against the new plugin tag and the walkthrough holds
(expected output, interview flow, planted bug still surfaces at the expected
location). No CI executes the walkthrough. This is ADR-0004 option E1 — chosen
for the reasons ADR-0004 already lists: zero CI cost, the author has the local
GPU environment to actually run it, fastest. The "nightmare risk" (author
attests but nobody ran it) is accepted as the cost of avoiding GPU CI, which
is infeasible in CI for the GPU examples (`tensorrt`, `nsys-profiler`).

Rationale for not adopting a hard CI gate (ADR-0004 E2) now: GPU CI is
expensive and may be infeasible (needs CUDA drivers, nvcc, a real GPU); a
smoke-check is partial (step 4 can break while steps 1-3 work). The CPU-only
example (`perf`) is the only candidate where a full CI walkthrough is
bearable, and graduating `perf` to a CI gate is left as a future option
(per ADR-0004's "hybrid E2-for-CPU, E1/E3-for-GPU is a possible ratification
outcome") — not part of this decision.

### 1.4 What the bot does NOT do

- It does **not** block the MR (L1 is a nudge, ADR-0001 option C).
- It does **not** auto-create the examples tag (the author creates it
  manually — this rejects the "fast path" clause of ADR-0002's provisional
  lean B). Rationale: the author must run the skill and decide whether the
  walkthrough needs editing before tagging; an auto-tag would assert
  "validated" without the validation having happened.
- It does **not** run the skill in CI (verification is author attestation,
  Decision 1.3).
- It does **not** decide whether a trigger fired — detection is heuristic
  assist; the author is the judge of whether the walkthrough actually needs
  editing.

### 1.5 Tag format

The examples tag mirrors the marketplace plugin tag exactly: `<pkg>-v<semver>`
(e.g. `perf-v0.1.0`, `tensorrt-v0.1.0`, `nsys-profiler-v0.1.0`). This is the
existing convention (enforced by `docs/lint-examples.sh` rule 3 on the `ref:`
field). The L2 gate compares tags by exact string equality — no version-range
matching at the gate. Per the policy (§6), the `ref:` in each example's
`apm.yml` is the **validated** tag; floating semver ranges (`^0.1.0`) in
`ref:` are rejected by this ADR because they defeat the validated-tag
guarantee (a user could receive an unvalidated `0.1.5`).

## Decision 2 — Per-example docs archive (release-artifact contract)

`apex-marketplace-serve` bakes each example's docs into the
`tutorials/examples/<example>/` section of its MkDocs site by **fetching a
published Markdown archive from the GitLab project registry of
`apex-examples`** at build time. This is the same shape as the tool-docs
contract (serve ADR-003, `gitlab-generic` source, `convert-tool-docs.py`):
a clean artifact boundary, immutable verified content, offline-capable on
AIStation (no runtime internet).

### 2.1 One archive per example, versioned by the plugin tag

Each example publishes its docs as a **separate** generic package to the
`apex-examples` GitLab project registry, named `docs-<pkg>`, versioned
`<semver>` to mirror the plugin tag `<pkg>-v<semver>`:

| Example dir | Generic package | Archive path | Trigger tag |
|---|---|---|---|
| `perf/` | `docs-perf` | `packages/generic/docs-perf/<ver>/archive.tar.gz` | `perf-v<ver>` |
| `vectorize/` | `docs-vectorize` | `packages/generic/docs-vectorize/<ver>/archive.tar.gz` | `vectorize-v<ver>` |
| `tensorrt/` | `docs-tensorrt` | `packages/generic/docs-tensorrt/<ver>/archive.tar.gz` | `tensorrt-v<ver>` |
| `nsys-profiler/` | `docs-nsys-profiler` | `packages/generic/docs-nsys-profiler/<ver>/archive.tar.gz` | `nsys-profiler-v<ver>` |
| `nsys-analyzer/` | `docs-nsys-analyzer` | `packages/generic/docs-nsys-analyzer/<ver>/archive.tar.gz` | `nsys-analyzer-v<ver>` |

Each archive contains **only that example's** hub README plus its local
assets. One tag builds docs for only that example — a `perf-v0.1.1` tag does
not rebuild or republish `docs-tensorrt`.

### 2.2 Archive contents (the build contract)

The archive is a `.tar.gz` with a fixed, minimal layout. The build script
(§2.3) assembles it from the repo at the tagged commit. Two shapes are
supported:

**Single-page (default — examples without a `docs/` directory):**

```
docs-<pkg>-<ver>.tar.gz
├── index.md          ← the example's hub README, renamed
├── img/              ← local image assets referenced by the README (if any)
│   └── *.png
└── .pages            ← awesome-pages section title (nav label = example name)
```

**Multi-page (examples that author a `docs/` directory — see §2.7):**

```
docs-<pkg>-<ver>.tar.gz
└── (the example's docs/ tree, verbatim)
    ├── index.md
    ├── <page>.md
    ├── ...
    ├── img/
    └── .pages        ← with `title:` + `arrange:` for nav order
```

The serve fetcher (`convert-tool-docs.py:284-294`) already prefers `docs/`
in the archive, then falls back to the archive root — so both shapes are
consumed by the existing `gitlab-generic` code path with no serve-side
changes. `bake_mkdocs_tree` copies every `.md` and `.pages` verbatim,
preserving sub-directory structure (verified at
`convert-tool-docs.py:195-226`).

Rules common to both shapes:

1. **`index.md`** is the example's landing page (hub README for
   single-page; `docs/index.md` for multi-page), verbatim. No content
   transformation at publish time — the serve side owns any deploy-time
   rewriting (§2.4).
2. **`img/`** is included only when the example has a local `img/`
   directory (today: only `vectorize/img/`). Images are referenced from
   the README by relative path (`./img/do-simd.png`); the archive
   preserves that path so the relative links resolve in the baked MkDocs
   tree. For multi-page, `img/` lives inside `docs/` and is shared by all
   pages.
3. **`.pages`** carries the awesome-pages `title:` (the example's
   display name in the left nav) and, for multi-page, the `arrange:`
   list controlling page order (same convention as serve's
   `docs/release/.pages`).
4. **Nothing else.** No `apm.yml`, no source code, no module sub-folders,
   no `BUILD.md` (vectorize's `demo/README.md` is agent-readable build
   commands, not a walkthrough — it is not part of the published docs).
   The anti-cheat design (agent runs in the module sub-folder, told not
   to read the hub walkthrough) is preserved: the walkthrough the user
   reads on the serve site is the hub README (single-page) or the
   `docs/` tree (multi-page), and the module sub-folders' source is not
   published as docs.

### 2.3 Publish pipeline (apex-examples side)

A new `.gitlab-ci.yml` (the repo has no CI today) defines a single job,
`docs:publish`, that runs **only on `<pkg>-v<semver>` tag push** (the same
tags that the L2 gate keys on). The job:

1. Determines `<pkg>` and `<ver>` by parsing the pushed tag against
   `^(perf|vectorize|tensorrt|nsys-profiler|nsys-analyzer)-v(\d+\.\d+\.\d+)$`
   (the same regex `docs/lint-examples.sh` rule 3 uses for the `ref:` field;
   non-semver tags like a hypothetical `perf-v1-autonomous` are ignored —
   mirroring serve's `gen-docs.sh` semver-only rule).
2. Assembles the archive per §2.2 from the repo at `git checkout <tag>`.
   The shape depends on whether the example authors a `docs/` directory
   (see §2.7):
   - **Multi-page** (`<example>/docs/` exists): `tar -C <example>/docs -czf
     docs-<pkg>-<ver>.tar.gz .` — the `docs/` tree ships verbatim (all
     `.md` + `.pages` + `img/`). No content transform.
   - **Single-page** (no `docs/`): `cp <example>/README.md index.md`; if
     `<example>/img/` exists, `cp -r <example>/img/ img/`;
     `printf 'title: %s\n' <example> > .pages`;
     `tar czf docs-<pkg>-<ver>.tar.gz index.md .pages [img]`.
3. Uploads to the project's generic package registry:
   ```
   curl --header "JOB-TOKEN: $CI_JOB_TOKEN" \
     --upload-file docs-<pkg>-<ver>.tar.gz \
     "$CI_API_V4_URL/projects/$CI_PROJECT_ID/packages/generic/docs-<pkg>/<ver>/archive.tar.gz"
   ```
   Uses the built-in `$CI_JOB_TOKEN` (no extra PAT). Re-running on the same
   tag overwrites the package version (idempotent, matches serve's release.sh
   generic-package behavior).

The publish job is the **only** new CI in `apex-examples`. It is intentionally
a publish step, not a build step — the archive is a repackaging of files
already in the tagged commit, so there is no separate "docs build" to run
(MkDocs build happens on the serve side; the archive carries Markdown, not
HTML — mirroring serve ADR-003's "Markdown ソースを generic package として公開"
contract).

### 2.4 Consume pipeline (apex-marketplace-serve side — downstream consequence)

The serve repo implements the fetch+bake. This ADR defines the contract; the
serve-side wiring is sketched here so the contract is unambiguous, but the
implementation lives in the serve repo. The shape mirrors the existing
tool-docs pipeline (`gen-tool-docs.py` + `convert-tool-docs.py` + a committed
map), reusing the `gitlab-generic` fetcher:

1. **Committed map** (like `services/mkdocs/tool-docs.map.json`), e.g.
   `services/mkdocs/example-docs.map.json`, keying `<example>` →
   `{ nav_title, marketplace_package }`. With 5 fixed examples this is small
   and stable; full auto-discovery is unnecessary (unlike tools, where the set
   is open).
2. **Fetch step** — a new `gen-example-docs.py` (or an extension of
   `convert-tool-docs.py`) reads the map, fetches each
   `packages/generic/docs-<pkg>/<ver>/archive.tar.gz` from the
   `apex-examples` project registry using `GITLAB_TOKEN` (same BuildKit secret
   as tools), and unpacks to `tutorials/examples/<example>/`.
3. **Versioning** — mirrors the packages/tools convention (serve ADR-005):
   latest `<ver>` at the item root `tutorials/examples/<example>/index.md`,
   older `<ver>` under `tutorials/examples/<example>/<ver>/` with
   `.pages: hide: true`; per-item `versions.json` + the existing `versions.js`
   dropdown. Version selection honors serve's `DOCS_VERSIONS` env (`all` or
   `N` newest) to bound image size, identical to packages/tools.
4. **Host token substitution** — the example READMEs (and `docs/` pages)
   use the `__APEX_HOST__` token (standardized to match serve's convention:
   `https://__APEX_HOST__:4430/apex/marketplace.git`). Serve's existing
   `40-apex-host.sh` deploy-time rewrite substitutes `__APEX_HOST__` →
   `$CERT_HOST` across the baked docs tree, so examples are covered with
   **zero new logic** — the same mechanism that already runs for
   packages/tools. The `<apex-examples-repo-url>` placeholder (the user's
   clone URL for this public repo) is left as-is; it is a user-facing
   instruction, not a deploy-time host substitution.
5. **Table injection** — `tutorials/examples.md` (the current placeholder)
   gets a `<!-- EXAMPLES_TABLE -->` marker, and `gen-docs.sh` injects a
   generated table of the discovered examples (name + one-line description,
   derived from each `index.md`'s first prose line — the same `describe()`
   helper `gen-docs.sh` already uses for packages). This mirrors the existing
   `<!-- PACKAGE_TABLE -->` injection.

### 2.5 Why per-example archives (not one repo-wide archive)

- **Per-example versioning is the point.** Each example pins its own plugin
  tag and is released independently — `perf-v0.1.1` should not force a
  `docs-tensorrt` republish. A single repo-wide `docs@<ver>` archive would
  version the whole repo (e.g. `0.1.0`), collapsing the per-plugin dropdown
  to one "all examples at version X" selector, which loses the
  packages/tools UX the serve site already presents.
- **Parity with tools.** Tools are "1 project = 1 docs site." Examples are
  "1 example = 1 archive" — the closest analogue given examples live in one
  repo. The serve fetcher reuses the `gitlab-generic` code path with no new
  source type.
- **Idempotent per-tag.** Re-pushing (or re-running CI on) `perf-v0.1.1`
  overwrites only `docs-perf@0.1.1`; other examples' archives are untouched.

### 2.6 Why a release-artifact contract (not clone-and-bake, not federation)

- **vs. clone-and-bake (option 2A):** cloning `apex-examples --tags` in the
  serve Dockerfile would work (the tags exist), but it couples the serve
  build to the `apex-examples` **source structure** — a folder rename (which
  the repo just did: `perf-plugin/`→`perf/`) would silently break the serve
  build. The artifact contract isolates: `apex-examples` changes its layout,
  republishes the archive, and serve is unaffected as long as the archive
  layout (§2.2) is stable.
- **vs. federation / link-to-GitHub (option 2B):** AIStation is air-gapped at
  runtime — the whole point of serve is baking content in at build time.
  Runtime links to `github.com/.../apex-examples` would 404 on AIStation.
- **vs. source vendoring (option 2D):** the all-in-1 hub README pattern means
  each example is one README, not a deep tree; vendoring collapses to the
  archive contract anyway, and the contract is the cleaner boundary.

### 2.7 Archive page structure (single-page vs. multi-page)

The archive contract (§2.2) supports two shapes: a single-page archive
(hub README → `index.md`) for short examples, and a multi-page archive
(an authored `docs/` sub-tree) for long, multi-section examples. The
choice is per-example and detected at publish time by the presence of a
`<example>/docs/` directory. Both shapes are consumed by the same
`gitlab-generic` fetcher — no serve-side change is needed for either.

**Why multi-page exists:** `tensorrt` (413 lines, 4 module walkthroughs),
`nsys-profiler` (321 lines, 3 fixtures), and `perf` (330 lines, a
cache-miss walkthrough + an orchestrator-mode section with 2 planted
bottlenecks) are long single pages. The multi-page shape lets the serve
site present a left-nav with sub-pages under each example, matching the
packages/tools UX, instead of one long scroll with in-page anchors only.

**What goes in `docs/` (Option C — walkthroughs split out):**

- `docs/index.md` — the example's overview + Installation + Usage +
  skill list + a links block pointing into the sub-pages. This keeps
  the air-gapped AIStation user self-contained (they see install
  instructions on the serve site without going to GitHub).
- `docs/<page>.md` — one page per major walkthrough section (e.g. one
  per module for `tensorrt`, one per fixture for `nsys-profiler`).
- `docs/.pages` — awesome-pages `title:` (the example's nav label) +
  `arrange:` controlling page order, using the same convention as
  serve's `docs/release/.pages`.
- `docs/img/` — local image assets, shared by all pages (when present).

**What stays in the hub README (GitHub-facing):** the hub `<example>/README.md`
keeps `## Installation` and `## Usage` (so `docs/lint-examples.sh` rule 1
passes and GitHub readers see how to run the example) plus a
`## Walkthroughs` section with links into `docs/`. The detailed
walkthrough text **moves** to `docs/`; the hub README becomes a short
landing page that points into `docs/` for the walkthroughs.

**Anti-cheat preservation:** the agent runs in a module sub-folder
(e.g. `tensorrt/a_backbone/`) and is instructed not to read outside its
folder. Moving the walkthroughs from the hub README to `docs/` does not
change this constraint — `docs/` is still outside the module sub-folder.
The instruction wording is updated to name `docs/` explicitly: "don't
read the root README or `docs/` — the walkthroughs there are for the
human, not the agent." The walkthroughs on the serve site are for the
**human reader**; the agent still works from the `model.py` in front of
it. This matches the existing design intent (the original instruction
said "ルート README のモジュール walkthrough を見てなぞるのを防ぐため").

**Which examples get `docs/` (per-example choice):**

| Example | Shape | Why |
|---|---|---|
| `perf/` | multi-page | cache-miss walkthrough + orchestrator-mode bottlenecks (2 planted issues) — distinct sections worth separate pages |
| `tensorrt/` | multi-page | 4 modules, each a self-contained lesson (happy path / dynamic shape / custom kernel / refactor) |
| `nsys-profiler/` | multi-page | 3 fixtures + analyzer-profiles note — each fixture is a distinct NVTX-strategy lesson |
| `vectorize/` | single-page | 189 lines, one SIMD-vectorization workflow — fine as one page |
| `nsys-analyzer/` | single-page | 102 lines, placeholder (no fixtures yet) |

**Why not a splitter (Option B) or a manifest (`.docs.yml`):** a splitter
adds heuristics (code-fence-aware heading detection, anchor generation)
that can break on edge cases, and the in-repo README and published pages
would diverge in shape (README stays one file for GitHub, archive is
multi-page). Authoring `docs/` directly (Option C) is WYSIWYG — the
author writes the pages they want, no transform step. The cost is
content movement (the walkthrough text moves from the README to `docs/`),
but no duplication: the hub README becomes a thin pointer, not a
second copy.

**Why not all 5 examples multi-page:** 3 of 5 examples (`perf` aside —
see table) are short enough that a single page reads fine. Forcing a
`docs/` tree on a 102-line or 189-line example adds structure without
value. The choice is per-example and reversible (adding a `docs/` dir
later switches the example to multi-page; removing it switches back).

## Consequences

- **ADR-0002** (trigger-flags role): ratified as **B minus auto-tag** — the
  bot flags + assists, the human creates the tag. Status flips to Accepted.
- **ADR-0004** (skill-runs guarantee): ratified as **E1** — author attests.
  Status flips to Accepted. The CPU-CI graduation (E2 for `perf` only) is
  explicitly left open as a future option, not foreclosed.
- **New CI in `apex-examples`:** a `.gitlab-ci.yml` with one `docs:publish`
  job on `<pkg>-v<semver>` tag push (§2.3). The repo had no CI before.
- **Serve-side downstream work** (implemented in the serve repo, not here):
  `example-docs.map.json` + a fetch step + `versions.json` per example +
  `<!-- EXAMPLES_TABLE -->` marker in `tutorials/examples.md` + host-token
  alignment (§2.4). This ADR defines the contract; the serve repo implements
  the consumer.
- **Lockstep is now end-to-end decided:** L1 nudge (1.1) → author edits +
  attests (1.3) → author tags `apex-examples` (1.5) → L2 gate checks the tag
  exists (1.2) → plugin tag push succeeds → CI publishes the docs archive
  (2.3) → serve build bakes it (2.4). No provisional gaps remain in the
  release flow.
- **The L2 gate does not verify content correctness** (per ADR-0001) — only
  that a paired examples tag exists. Content correctness is the author's
  attested responsibility (1.3). The gate is the release-prevention layer;
  attestation is the content-correctness layer. They are complementary, as
  ADR-0001 already notes.
- **Tag-mirror discipline:** `apex-examples` must carry a `<pkg>-v<semver>`
  tag for every released plugin tag. The L2 gate enforces this at release
  time; drift is impossible by construction (a plugin tag with no examples
  counterpart cannot be pushed).

## Open follow-ups (not part of this decision)

- **CPU-only CI gate for `perf`** (ADR-0004 E2): a future graduation once the
  bot-nudge value is proven. Not part of this decision.
- **Bot implementation** (1.1): the trigger-detection heuristic (scanning a
  plugin MR diff for skill names / interview strings / CLI flags referenced
  in example walkthroughs) is a script + webhook/CI job in the marketplace
  repo, not in `apex-examples`. This ADR defines the *behavior*; the
  implementation is a marketplace-repo concern.
- **Serve-side consumer wiring** (§2.4): `example-docs.map.json` + a fetch
  step + `versions.json` per example + `<!-- EXAMPLES_TABLE -->` marker in
  `tutorials/examples.md`. This ADR defines the contract; the serve repo
  implements the consumer.

## Cross-references

- [ADR-0001](adr-0001-release-gate.md) — the L2 hard gate (Accepted). This
  ADR affirms it as the binding gate and fixes the "who creates the tag"
  sub-decision (the human author, manually).
- [ADR-0002](adr-0002-trigger-flags-role.md) — ratified by this ADR (option
  B, minus the auto-tag fast-path).
- [ADR-0004](adr-0004-skill-runs-guarantee.md) — ratified by this ADR
  (option E1, author attests).
- serve repo `docs/ADR/001`, `docs/ADR/003`, `docs/ADR/005` — the precedent
  for build-time bake of a registry artifact, the `gitlab-generic` fetch
  contract, and the latest-at-root + dropdown versioning UX that the
  examples archive consumes.
- `docs/UPDATE-POLICY.md` §3 (trigger checklist), §6 (`apm.yml` `ref`
  convention), §10 (`COMPATIBILITY.md`).

## Change log

| Date | Change |
|---|---|
| 2026-08-04 | Initial. Ratifies ADR-0002 (B minus auto-tag) and ADR-0004 (E1); defines the per-example docs archive contract for serve. |
| 2026-08-04 | Widened §2.2 to support both single-page and multi-page archive shapes; added §2.7 (page structure — Option C: authored `docs/` sub-tree for perf/tensorrt/nsys-profiler, single-page for vectorize/nsys-analyzer). Host placeholders standardized to `__APEX_HOST__` (§2.4 §4 updated — no longer a follow-up). |
