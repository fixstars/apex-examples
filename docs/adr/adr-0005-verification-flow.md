# ADR-0005 — User verification flow documentation

Date: 2026-08-03
Status: In consideration

## Context

How does the user discover and verify they are running the correct
example version against the correct plugin version? This depends on
[ADR-0003](adr-0003-example-version-bump.md): the verification commands
differ depending on whether the example has a `version` field and what
it means. This ADR is deferred until ADR-0003 settles.

The flow itself uses existing `apm` commands (verified in `apm --help`
and apm docs):

```bash
apm install              # reads apm.yml, installs the pinned ref,
                         # generates apm.lock.yaml (not committed — per-user,
                         # since the marketplace URL is a placeholder host)
apm view --lock          # shows resolved_ref (the plugin tag),
                         # resolved_commit (SHA), version
apm view <pkg> versions  # lists available plugin tags on your serve
apm outdated            # shows you're behind latest (by design)
apm audit                # integrity: installed files match lockfile hashes
```

The lockfile (`apm.lock.yaml`) is the **proof** — it records the exact
plugin tag (`resolved_ref`), commit SHA, and per-file hashes. The lockfile
is **not committed** (the `git:` URL is a per-user placeholder host, so
the lockfile is machine-specific — confirmed in apm docs: "Lockfiles
produced this way are machine-specific -- do not commit them"). So the
lockfile is a *generated* verification artifact, not a committed one.

## Options

### F1. Per-README "Verify your install" block

The policy doc adds a section describing the flow; each example README
gets a short "Verify your install" block with the concrete commands.

- **Pro:** most discoverable for users; concrete commands in each example;
  self-contained — a user in an example dir doesn't need to find the
  policy doc.
- **Con:** README duplication (the same command block in multiple READMEs);
  if the commands change, all READMEs need updating; READMEs get longer.

### F2. Document in policy only, not in READMEs

The policy doc describes the flow; example READMEs keep only the install
command and link to the policy for verification.

- **Pro:** single source of truth; READMEs stay short; one place to
  update.
- **Con:** users who don't read the policy doc miss the verification
  commands; the policy doc is maintainer-facing (per its own header), so
  pointing end-users there is a mismatch.

### F3. Link to apm docs, don't duplicate

The policy doc notes the lockfile is the verification artifact and links
to the authoritative apm docs for the commands.

- **Pro:** least duplication; apm docs are authoritative and kept current
  by the apm project; correct layering.
- **Con:** users have to context-switch to apm docs; the apex-specific
  framing (placeholder host, per-user lockfile not committed, "you're
  intentionally behind latest by design") is lost — the apm docs won't
  say "being behind latest is expected because the example is validated
  against a pin."

## Decision

Provisional lean: **F1** (but this leans on ADR-0003 settling first).

Not binding until ratified by the APEX team.

## Consequences

If **F1** is ratified (and ADR-0003 ratifies D4 — mirror plugin tag
version):

- Each example README gets a "Verify your install" block with the five
  `apm` commands above.
- The verification story is clean: `apm view --lock` shows example
  `version: 0.2.0` and plugin `resolved_ref: tensorrt-v0.2.0` — they
  match, so the pairing is confirmed *by construction* (D4 makes the
  version the pairing statement). The lockfile proves the install; the
  version match proves the pairing.
- If ADR-0003 ratifies D3 (no version field), the verification flow is
  `git rev-parse HEAD` + compare to `COMPATIBILITY.md` — different
  commands, and F1's block would be different.
