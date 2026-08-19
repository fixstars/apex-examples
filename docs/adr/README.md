# ADR Index

Architecture Decision Records for `apex-examples` release policy.

| ADR | Status | Title |
|---|---|---|
| [ADR-0001](adr-0001-release-gate.md) | Accepted | Release gate: hybrid nudge at L1, hard gate at L2 tag push |
| [ADR-0002](adr-0002-trigger-flags-role.md) | Accepted | Role of the 5 trigger flags when the gate is on version/tag |
| [ADR-0003](adr-0003-example-version-bump.md) | In consideration | Example's own `version` field bump strategy |
| [ADR-0004](adr-0004-skill-runs-guarantee.md) | Accepted | Guarantee that the skill runs on the new plugin tag |
| [ADR-0005](adr-0005-verification-flow.md) | In consideration | User verification flow documentation |
| [ADR-0006](adr-0006-examples-release-contract.md) | Accepted | Examples release contract (lockstep gate + per-example docs archive) |

ADR-0006 ratifies ADR-0002 (option B, minus the auto-tag fast-path) and
ADR-0004 (option E1, author attests). See ADR-0006 for the full decision.

## Status convention

- **Accepted** — decided and binding. Recorded in `UPDATE-POLICY.md` as a decided section.
- **In consideration** — open, with a provisional lean. Options and reasoning captured here for future ratification. Not binding until the APEX team ratifies.
- **Superseded** — replaced by a later ADR. Kept for history.

## ADR template

```
# ADR-NNNN — Title

Date: YYYY-MM-DD
Status: Accepted | In consideration | Superseded

## Context
(why this decision is needed)

## Options

### A. ...
- Pro: ...
- Con: ...

## Decision
(what was decided, or "provisional lean: X" if in consideration)

## Consequences
(what follows from the decision)
```
