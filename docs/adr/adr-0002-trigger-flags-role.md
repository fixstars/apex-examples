# ADR-0002 — Role of the 5 trigger flags when the gate is on version/tag

Date: 2026-08-03
Status: Accepted — ratified by ADR-0006 (option B, minus the auto-tag fast-path)

> **Ratified 2026-08-04 by [ADR-0006](adr-0006-examples-release-contract.md).**
> The bot flags the affected example + posts an assist at L1 (not a block);
> the human author creates the examples tag manually — **no auto-tag
> fast-path.** The original options and reasoning are retained below for
> history.

## Context

[ADR-0001](adr-0001-release-gate.md) decided the gate fires on version
bump + tag existence. The 5 trigger flags (UPDATE-POLICY.md section 3 —
expected output, interview wording, skill name, CLI flag, workflow
ordering) were originally the gate condition. With the gate on version/tag,
their role is open.

The simplification question: gate purely on version bump (drop the flags),
or keep the flags in some role? The concern with dropping them is that
every release — including internal-only changes — gates. The concern with
keeping them as the gate condition is the maintenance pain of per-change
judgment and the false-negative risk of bot detection.

## Options

### A. Drop the flags

Gate purely on version bump. Every release requires an examples tag.
Re-validation tags (walkthrough unchanged) are expected to be cheap.

- **Pro:** simplest; no detection judgment; uniform rule.
- **Con:** every release gates, including internal-only changes that bumped
  version for reasons unrelated to end-user-visible behavior. The cost is
  paid only when the walkthrough actually changed (which is when you wanted
  the gate anyway), *if* re-validation tags stay cheap.

### B. Keep flags as fast-path + assist (provisional lean)

Gate on version bump. Separately, the bot runs trigger detection and uses
it to:

- (a) auto-create the examples tag when no trigger fires (fast path — the
  author does zero work for internal-only releases), and
- (b) post the "assist" comment naming the file + line + what shifted when a
  trigger fires.

The flags *route* (automation) rather than *gate* (judgment).

- **Pro:** recovers the fast path that "drop" loses; the assist turns
  "figure out what changed" into "apply this diff"; keeps the detection
  machinery as automation, not as a judgment the gate makes.
- **Con:** keeps detection machinery; false-negative risk in detection — a
  semantic change the bot misses still gates (since the gate is on version
  bump), but the assist is not generated, so the author does manual diff
  work.

### C. Keep flags as the gate condition

Gate fires only when both version bumped AND a trigger flag hit.
Internal-only version bumps don't gate.

- **Pro:** fewer gates; internal-only version bumps don't block.
- **Con:** keeps detection judgment and its false-negative risk — a
  semantic change the bot misses ships without an example update.

## Decision

Provisional lean: **B**.

Not binding until ratified by the APEX team.

## Consequences

If **B** is ratified:

- The bot needs trigger-detection wiring (scan plugin MR diff for skill
  names / interview strings / CLI flags referenced in example walkthroughs).
- The fast-path auto-tag needs a rule for when the bot can safely create
  the examples tag without author work (answer: when no trigger fires — the
  walkthrough is unchanged, so re-validation is a `ref` bump + smoke check,
  not a content edit).
- The assist needs a format (file + line + before/after) that turns the
  author's work into an "apply this diff" task.
- The "who creates the examples tag" sub-decision resolves to: the bot on
  the fast path (no trigger), the author on the slow path (trigger fires).
- The "escape valves for release frequency" sub-decision resolves to: the
  fast path *is* the escape valve — internal-only releases cost the author
  nothing.
