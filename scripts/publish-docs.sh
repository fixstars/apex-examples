#!/usr/bin/env bash
# scripts/publish-docs.sh — build & publish a per-example docs archive.
#
# Implements the docs-archive contract of ADR-0006 §2.3. Triggered by CI on
# `<pkg>-v<semver>` tag push (see .gitlab-ci.yml). Produces ONE archive per
# tag, and uploads it to the apex-examples GitLab project's generic package
# registry as `docs-<pkg>@<ver>` so apex-marketplace-serve can fetch+bake it
# at build time (reusing the existing gitlab-generic tool-docs fetcher).
#
# Two archive shapes (ADR-0006 §2.2 / §2.7):
#
# **Multi-page** (example has a docs/ dir):
#   docs-<pkg>-<ver>.tar.gz
#   ├── index.md      ← <example>/README.md (the hub, becomes the landing page)
#   ├── <page>.md     ← <example>/docs/<page>.md (each walkthrough sub-page)
#   ├── ...
#   ├── img/          ← <example>/img/ if present (shared by all pages)
#   └── .pages        ← auto-generated: title + arrange (pages in docs/ order)
#
# **Single-page** (no docs/ dir):
#   docs-<pkg>-<ver>.tar.gz
#   ├── index.md      ← <example>/README.md, verbatim
#   ├── img/          ← <example>/img/ if present
#   └── .pages        ← auto-generated: title only
#
# The `.pages` file is ALWAYS generated at publish time from the actual
# docs/ folder structure — it is never hand-authored in the repo. awesome-pages
# `arrange:` lists the sub-pages in sorted order so the left nav is
# deterministic.
#
# Tag → example mapping mirrors the apex-marketplace plugin tag convention
# (`<pkg>-v<semver>`, the tag_pattern strategy). Non-semver tags (e.g. a
# hypothetical `perf-v1-autonomous`) are ignored — return 0 so CI doesn't
# fail on unrelated tags.
#
# Auth: $CI_JOB_TOKEN (set by GitLab CI for every job). No extra PAT needed.
# Re-running on the same tag overwrites the package version (idempotent),
# matching serve's release.sh generic-package behavior.
#
# Usage (CI invokes this; can also run locally for testing):
#   bash scripts/publish-docs.sh <tag>
#
# Env (CI):
#   CI_API_V4_URL, CI_PROJECT_ID, CI_JOB_TOKEN  — provided by GitLab Runner.
# Env (local dry-run — no upload):
#   PUBLISH_DOCS_DRY_RUN=1  skip the upload, just build the archive and
#                           print its path + contents.

set -euo pipefail

# ── Args & env ────────────────────────────────────────────────────────────────

TAG="${1:-}"
if [[ -z "$TAG" ]]; then
  echo "ERROR: tag argument required. Usage: bash scripts/publish-docs.sh <tag>" >&2
  echo "  e.g. bash scripts/publish-docs.sh perf-v0.1.0" >&2
  exit 1
fi

# Tag → (pkg, ver, example_dir). Only strict semver tags <pkg>-v<X.Y.Z> are
# published; non-semver tags are a no-op (exit 0) so CI on an unrelated tag
# doesn't fail.
if [[ "$TAG" =~ ^(perf|vectorize|tensorrt|nsys-profiler|nsys-analyzer)-v([0-9]+\.[0-9]+\.[0-9]+)$ ]]; then
  PKG="${BASH_REMATCH[1]}"
  VER="${BASH_REMATCH[2]}"
  EXAMPLE_DIR="${PKG}"   # example dir name == pkg name for all current examples
else
  echo "INFO: tag '$TAG' is not an example docs tag (<pkg>-v<semver>) — skipping."
  exit 0
fi

# Resolve repo root from script location so the script works from a tag checkout.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "==> publish-docs.sh"
echo "    tag      : ${TAG}"
echo "    pkg      : ${PKG}"
echo "    version  : ${VER}"
echo "    example  : ${EXAMPLE_DIR}/"
echo "    repo root: ${REPO_ROOT}"

# ── Sanity: example hub README must exist (contract precondition) ────────────
README="${REPO_ROOT}/${EXAMPLE_DIR}/README.md"
if [[ ! -f "$README" ]]; then
  echo "ERROR: ${EXAMPLE_DIR}/README.md not found at tag ${TAG}" >&2
  echo "  Every example must have a hub README (ADR-0006 §2.2). Aborting." >&2
  exit 1
fi

# ── Assemble the archive (ADR-0006 §2.2) ──────────────────────────────────────
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# index.md = hub README, verbatim. No content transform at publish time — the
# serve side owns any deploy-time rewriting (host-token substitution, dead
# relative-link flattening, canonical install admonition). Mirrors how
# marketplace package READMEs are baked (serve gen-docs.sh write_pkg_readme).
cp "$README" "${WORK}/index.md"

# img/ included only when the example has a local img/ dir (today: vectorize).
# Relative image paths in the README (./img/x.png) resolve in the baked tree.
# For multi-page, img/ is shared by all pages (lives at the archive root).
if [[ -d "${REPO_ROOT}/${EXAMPLE_DIR}/img" ]]; then
  cp -r "${REPO_ROOT}/${EXAMPLE_DIR}/img" "${WORK}/img"
  echo "    img/     : included ($(find "${WORK}/img" -type f | wc -l) file(s))"
else
  echo "    img/     : none"
fi

# Multi-page: copy docs/*.md as sub-pages alongside index.md.
DOCS_DIR="${REPO_ROOT}/${EXAMPLE_DIR}/docs"
if [[ -d "$DOCS_DIR" ]]; then
  PAGES=()
  for f in "${DOCS_DIR}"/*.md; do
    [[ -f "$f" ]] || continue
    name="$(basename "$f")"
    cp "$f" "${WORK}/${name}"
    PAGES+=("$name")
  done
  # Sort the page list for deterministic nav order.
  IFS=$'\n' sorted=($(sort <<<"${PAGES[*]}")); unset IFS
  PAGES=("${sorted[@]}")
  echo "    pages    : ${#PAGES[@]} sub-page(s): ${PAGES[*]}"
else
  PAGES=()
  echo "    pages    : single-page (no docs/ dir)"
fi

# .pages is ALWAYS generated at publish time from the actual docs/ structure.
# awesome-pages `title:` = the example's nav label; `arrange:` lists sub-pages
# in sorted order so the left nav is deterministic. Never hand-authored.
{
  printf 'title: %s\n' "$PKG"
  if [[ ${#PAGES[@]} -gt 0 ]]; then
    printf 'arrange:\n'
    for p in "${PAGES[@]}"; do
      printf '  - %s\n' "$p"
    done
  fi
} > "${WORK}/.pages"

ARCHIVE_NAME="docs-${PKG}-${VER}.tar.gz"
ARCHIVE_PATH="${WORK}/${ARCHIVE_NAME}"
# Archive everything in WORK except the archive itself (created in WORK; tar
# would otherwise include it). `--exclude` keeps the command simple and avoids
# a second temp dir.
tar -C "$WORK" -czf "$ARCHIVE_PATH" --exclude="./${ARCHIVE_NAME}" .

echo "    archive  : ${ARCHIVE_NAME} ($(du -h "$ARCHIVE_PATH" | cut -f1))"
echo "    contents :"
tar -tzf "$ARCHIVE_PATH" | sed 's/^/      /'

# ── Upload to the GitLab generic package registry (ADR-0006 §2.3) ─────────────
# Path: packages/generic/docs-<pkg>/<ver>/archive.tar.gz
# Auth: $CI_JOB_TOKEN (built-in for GitLab CI jobs). Re-running on the same
# tag overwrites the package version (idempotent).
if [[ "${PUBLISH_DOCS_DRY_RUN:-0}" == "1" ]]; then
  echo ""
  echo "==> DRY RUN — skipping upload. Archive at: ${ARCHIVE_PATH}"
  exit 0
fi

# CI env vars. Provide fallbacks for local (non-CI) runs so the error is
# clear rather than a curl against an empty URL.
API="${CI_API_V4_URL:-}"
PROJECT_ID="${CI_PROJECT_ID:-}"
TOKEN="${CI_JOB_TOKEN:-}"

if [[ -z "$API" || -z "$PROJECT_ID" || -z "$TOKEN" ]]; then
  echo "ERROR: missing GitLab CI env vars (CI_API_V4_URL, CI_PROJECT_ID, CI_JOB_TOKEN)." >&2
  echo "  This script is meant to run in GitLab CI. For a local build-only" >&2
  echo "  test, set PUBLISH_DOCS_DRY_RUN=1." >&2
  exit 1
fi

UPLOAD_URL="${API}/projects/${PROJECT_ID}/packages/generic/docs-${PKG}/${VER}/archive.tar.gz"
echo "    upload to: ${UPLOAD_URL}"

HTTP_CODE="$(curl -sS --fail-with-body \
  --header "JOB-TOKEN: ${TOKEN}" \
  --upload-file "$ARCHIVE_PATH" \
  --write-out '%{http_code}' \
  "$UPLOAD_URL" 2>&1)"

echo "    response : HTTP ${HTTP_CODE}"
echo "==> Done. Published docs-${PKG}@${VER}"
