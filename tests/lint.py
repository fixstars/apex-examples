# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: Apache-2.0

#!/usr/bin/env python3
"""apex-examples CI lint.

Discovers example folders dynamically: every top-level directory whose name
matches a plugin in apex-marketplace is an example. Non-example dirs (docs/,
tests/, scripts/, etc.) are skipped by an exclude list. New examples land
automatically — no hardcoded folder list to update.

Rules (all offline, no network):

  Example structure:
    E1. Every top-level dir is a marketplace plugin name or a known
        non-example dir. Prevents a stray folder that isn't a plugin.
    E2. Every example has a hub README.md with `## Installation` and
        `## Usage` sections; Installation starts with `git clone`.
    E3. No old folder names in tracked files (perf-plugin, vectorize-example,
        etc.).
    E4. No committed `.pages` in any `docs/` dir (publish-docs.sh generates
        .pages at publish time — a committed one would be overwritten).
    E5. `docs/*.md` filenames are lowercase-hyphenated (deterministic nav sort).

  Open-source readiness (no internal hosts leak):
    O1. No internal hosts (repos.fixstars.com, ssh://git@, private IPs) in
        tracked files. This repo goes public on GitHub.
    O2. The marketplace URL must use the `__APEX_URL__` token (a complete
        base URL, no scheme prefix), not a real host or ad-hoc placeholder.
        `<apex-examples-repo-url>` is the only allowed user-facing clone
        placeholder.

  Docs build contract:
    D1. Every `docs/*.md` sub-page links back to the hub README for install
        (one source of truth — no install-instruction copy-paste).
    D2. Any image referenced in a README or docs/*.md resolves to a file in
        the example's `img/` folder (relative path `./img/<name>`).

Exits non-zero on any violation. Run from the repo root:
    python3 tests/lint.py
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

# ── Marketplace plugin names (the authoritative example-folder allowlist) ────
# Sourced from apex-marketplace/packages/. A folder is an example iff its name
# is in this list. Update this list when a new plugin lands in the marketplace.
MARKETPLACE_PLUGINS = {
    "apexsh", "dummy-skill", "gitlab-comments", "gitlab-mr-review", "grill-me",
    "interactive-review", "marketplace-creator", "nsys-analyzer", "nsys-profiler",
    "perf", "refine-code", "refine-spec", "target-setup", "tensorrt", "vectorize",
}

# Non-example top-level dirs (not plugin examples). Anything not in this list
# AND not in MARKETPLACE_PLUGINS is a violation (E1).
NON_EXAMPLE_DIRS = {
    "docs", "tests", "scripts", ".git", ".github", ".gitlab", ".claude", ".opencode",
}

# ── Helpers ───────────────────────────────────────────────────────────────────
errors: list[str] = []


def fail(msg: str) -> None:
    print(f"FAIL  {msg}", file=sys.stderr)
    errors.append(msg)


def note(msg: str) -> None:
    print(f"  {msg}")


def pass_(msg: str) -> None:
    print(f" ok   {msg}")


def git_tracked_files() -> list[str]:
    """List git-tracked files (one per line). Returns [] if not a git repo."""
    import subprocess
    try:
        out = subprocess.check_output(["git", "ls-files"], text=True, stderr=subprocess.DEVNULL)
        return out.splitlines() if out.strip() else []
    except (subprocess.CalledProcessError, FileNotFoundError):
        return []


def extract_install_block(text: str) -> str:
    """Return the lines under `## Installation` up to the next `## ` heading."""
    lines = text.splitlines()
    block: list[str] = []
    in_block = False
    for line in lines:
        if re.match(r"^## Installation\b", line):
            in_block = True
            continue
        if in_block and re.match(r"^## ", line):
            break
        if in_block:
            block.append(line)
    return "\n".join(block)


# ── Discover example dirs ─────────────────────────────────────────────────────
def discover_examples(root: Path) -> tuple[list[str], list[str]]:
    """Return (example_dirs, unknown_dirs). Discovers top-level dirs."""
    examples: list[str] = []
    unknown: list[str] = []
    for entry in sorted(root.iterdir()):
        if not entry.is_dir() or entry.name.startswith("."):
            continue
        if entry.name in MARKETPLACE_PLUGINS:
            examples.append(entry.name)
        elif entry.name in NON_EXAMPLE_DIRS:
            continue
        else:
            unknown.append(entry.name)
    return examples, unknown


# == E1. Top-level dirs are marketplace plugins ================================
def check_e1(examples: list[str], unknown: list[str]) -> None:
    print("== E1. example folders are marketplace plugins ==")
    if unknown:
        fail("unknown top-level dirs (not marketplace plugins, not in exclude list):")
        for d in unknown:
            note(f"{d}/ — add to MARKETPLACE_PLUGINS if it's a new plugin, or NON_EXAMPLE_DIRS if not")
    else:
        pass_("all top-level dirs are marketplace plugins or known non-examples")
    print(f"    examples discovered: {' '.join(examples) if examples else '<none>'}")


# == E2. Hub README sections ===================================================
def check_e2(examples: list[str], root: Path) -> None:
    print("== E2. hub README sections ==")
    found_errors = False
    for ex in examples:
        f = root / ex / "README.md"
        if not f.is_file():
            fail(f"{f}: missing (every example needs a hub README)")
            found_errors = True
            continue
        text = f.read_text(encoding="utf-8")
        if not re.search(r"^## Usage\b", text, re.MULTILINE):
            fail(f"{f}: missing `## Usage` section")
            found_errors = True
        if not re.search(r"^## Installation\b", text, re.MULTILINE):
            fail(f"{f}: missing `## Installation` section")
            found_errors = True
            continue        
    if not found_errors:
        pass_("hub READMEs have Installation + Usage (git clone first)")


# == E3. No old folder names ===================================================
def check_e3(root: Path) -> None:
    print("== E3. no old folder names ==")
    old_names = re.compile(
        r"perf-plugin|vectorize-example|tensorrt-example|nsys-examples"
        r"|nsys-py-pipeline|nsys-py-module|nsys-cpp-saxpy"
    )
    changelog = re.compile(r":\| 2026-\d{2}-\d{2} \|")
    violations: list[str] = []
    for tracked in git_tracked_files():
        if tracked == "tests/lint.py":
            continue
        f = root / tracked
        if not f.is_file():
            continue
        try:
            text = f.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        for i, line in enumerate(text.splitlines(), 1):
            if changelog.search(line):
                continue
            if old_names.search(line):
                violations.append(f"{tracked}:{i}:{line.strip()}")
    if violations:
        fail("old folder names found in tracked files:")
        for v in violations:
            note(v)
    else:
        pass_("no old folder names in tracked files")


# == E4. No committed .pages in docs/ =========================================
def check_e4(examples: list[str], root: Path) -> None:
    print("== E4. no committed .pages in docs/ ==")
    violations: list[str] = []
    for ex in examples:
        p = root / ex / "docs" / ".pages"
        if p.is_file():
            violations.append(str(p))
    if violations:
        fail("committed .pages found in docs/ (publish-docs.sh generates them):")
        for v in violations:
            note(v)
    else:
        pass_("no committed .pages in docs/")


# == E5. docs/*.md filenames lowercase-hyphenated =============================
def check_e5(examples: list[str], root: Path) -> None:
    print("== E5. docs/*.md filenames lowercase-hyphenated ==")
    valid = re.compile(r"^[a-z0-9-]+\.md$")
    violations: list[str] = []
    for ex in examples:
        docs = root / ex / "docs"
        if not docs.is_dir():
            continue
        for f in docs.glob("*.md"):
            if not valid.match(f.name):
                violations.append(str(f))
    if violations:
        fail("docs/*.md filenames must be lowercase-hyphenated (deterministic nav sort):")
        for v in violations:
            note(v)
    else:
        pass_("docs/*.md filenames are lowercase-hyphenated")


# == O1. No internal hosts in tracked files ===================================
def check_o1(root: Path) -> None:
    print("== O1. no internal hosts (open-source readiness) ==")
    # Match internal hostnames and private IPs as standalone tokens, not
    # substrings of version numbers (e.g. 10.13.3.9 in "tensorrt-cu12==10.13.3.9").
    # Require the IP pattern to be word-bounded and not preceded by == or .
    internal = re.compile(
        r"repos\.fixstars\.com"
        r"|ssh://git@"
        r"|fixstates\.com"
        r"|(?:^|[^\w.=])(?:192\.168\.\d|10\.\d+\.\d+\.\d+|172\.(?:1[6-9]|2\d|3[01])\.\d+\.\d+)(?=[^\d])"
    )
    changelog = re.compile(r"^\| 2026-\d{2}-\d{2} \|")
    violations: list[str] = []
    for tracked in git_tracked_files():
        if tracked == "tests/lint.py":
            continue
        f = root / tracked
        if not f.is_file():
            continue
        try:
            text = f.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        for i, line in enumerate(text.splitlines(), 1):
            if changelog.match(line):
                continue
            if internal.search(line):
                violations.append(f"{tracked}:{i}:{line.strip()}")
    if violations:
        fail("internal hosts found in tracked files (this repo is public on GitHub):")
        for v in violations:
            note(v)
    else:
        pass_("no internal hosts in tracked files")


# == O2. __APEX_URL__ token for marketplace; <apex-examples-repo-url> only =====
def check_o2(examples: list[str], root: Path) -> None:
    print("== O2. host token conventions ==")
    bad_marketplace = re.compile(
        r"<marketplace-url"
        r"|<your-marketplace-host"
        r"|__APEX_HOST__"
        r"|<APEX配布物で提供されたマーケットプレイスURL"
        r"|https?://[a-z0-9.-]+:\d+/apex/marketplace"
    )
    bad_clone = re.compile(r"<repo-url|<clone-url|<examples-url|<example-repo-url")
    violations: list[str] = []
    for ex in examples:
        files = [root / ex / "README.md"]
        docs = root / ex / "docs"
        if docs.is_dir():
            files.extend(sorted(docs.glob("*.md")))
        for f in files:
            if not f.is_file():
                continue
            try:
                text = f.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                continue
            for i, line in enumerate(text.splitlines(), 1):
                if bad_marketplace.search(line):
                    violations.append(f"{f}:{i}: bad marketplace placeholder (use __APEX_URL__/apex/marketplace.git)")
                if bad_clone.search(line):
                    violations.append(f"{f}:{i}: bad clone placeholder (use <apex-examples-repo-url>)")
    if violations:
        fail("host token violations:")
        for v in violations:
            note(v)
    else:
        pass_("host tokens conform (marketplace uses __APEX_URL__; clone uses <apex-examples-repo-url>)")




# == D2. Image references resolve to files in img/ =============================
def check_d2(examples: list[str], root: Path) -> None:
    print("== D2. image references resolve to img/ ==")
    img_ref = re.compile(r"!\[[^\]]*\]\(([^)]+\.(?:png|jpg|jpeg|gif|svg))\)")
    violations: list[str] = []
    for ex in examples:
        files = [root / ex / "README.md"]
        docs = root / ex / "docs"
        if docs.is_dir():
            files.extend(sorted(docs.glob("*.md")))
        for f in files:
            if not f.is_file():
                continue
            try:
                text = f.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                continue
            for m in img_ref.finditer(text):
                rel = m.group(1)
                target = (f.parent / rel).resolve()
                if not target.is_file():
                    violations.append(f"{f}: image not found: {rel}")
    if violations:
        fail("image references that don't resolve:")
        for v in violations:
            note(v)
    else:
        pass_("all image references resolve")


# ── main ──────────────────────────────────────────────────────────────────────
def main() -> int:
    root = Path(__file__).resolve().parent.parent
    examples, unknown = discover_examples(root)

    check_e1(examples, unknown)
    check_e2(examples, root)
    check_e3(root)
    check_e4(examples, root)
    check_e5(examples, root)
    check_o1(root)
    check_o2(examples, root)
    check_d2(examples, root)

    print()
    if not errors:
        print("lint: all checks passed")
        return 0
    print(f"lint: {len(errors)} violation(s)", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
