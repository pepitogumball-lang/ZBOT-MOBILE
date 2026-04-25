#!/usr/bin/env bash
# scripts/git-sync.sh
#
# Sync the current working tree to origin/main on GitHub.
#
# Workflow:
#   1. Inspect working tree (git status / git log origin/main..HEAD)
#   2. If there are changes, stage everything and create one commit.
#      A clean tree with no unpushed commits is a successful no-op.
#   3. Push to origin/main using the GITHUB_PERSONAL_ACCESS_TOKEN secret
#      embedded in a one-shot push URL (token is never persisted to disk
#      or written into .git/config).
#   4. Verify the branch is up to date with origin/main.
#
# Hard guarantees:
#   - Only the `main` branch is pushed.
#   - No --force / --force-with-lease, no rebases, no resets.
#   - Empty commits are never created.
#   - The PAT is never echoed to stdout/stderr; all output is scrubbed.
#
# Usage:
#   GITHUB_PERSONAL_ACCESS_TOKEN=... ./scripts/git-sync.sh ["commit message"]

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_DIR"

REMOTE_NAME="origin"
BRANCH="main"
REMOTE_HOST_PATH="github.com/pepitogumball-lang/ZBOT-MOBILE.git"
DEFAULT_MSG="${1:-chore: sync working tree}"

# --- Preconditions -----------------------------------------------------------

if [[ -z "${GITHUB_PERSONAL_ACCESS_TOKEN:-}" ]]; then
  echo "[git-sync] ERROR: GITHUB_PERSONAL_ACCESS_TOKEN is not set." >&2
  exit 2
fi

CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD)"
if [[ "$CURRENT_BRANCH" != "$BRANCH" ]]; then
  echo "[git-sync] ERROR: refusing to sync; on branch '$CURRENT_BRANCH', expected '$BRANCH'." >&2
  exit 3
fi

# --- 1. Inspect --------------------------------------------------------------

echo "[git-sync] === git status ==="
git status

echo "[git-sync] === unpushed commits (origin/$BRANCH..HEAD) ==="
git fetch "$REMOTE_NAME" "$BRANCH" --quiet || true
UNPUSHED="$(git log --oneline "$REMOTE_NAME/$BRANCH..HEAD" || true)"
echo "${UNPUSHED:-<none>}"

# --- 2. Stage & commit (only if there's something to commit) -----------------

DIRTY=0
if ! git diff --quiet || ! git diff --cached --quiet || \
   [[ -n "$(git ls-files --others --exclude-standard)" ]]; then
  DIRTY=1
fi

if [[ "$DIRTY" -eq 1 ]]; then
  echo "[git-sync] staging all changes"
  git add -A

  if git diff --cached --quiet; then
    echo "[git-sync] nothing to commit after staging (skipping commit)"
  else
    echo "[git-sync] committing: $DEFAULT_MSG"
    git commit -m "$DEFAULT_MSG"
  fi
else
  echo "[git-sync] working tree clean; no commit needed"
fi

# --- 3. Push (no-op if already up to date) -----------------------------------

if [[ -z "$(git log --oneline "$REMOTE_NAME/$BRANCH..HEAD" || true)" ]]; then
  echo "[git-sync] no commits to push; branch already up to date"
else
  echo "[git-sync] pushing to $REMOTE_NAME/$BRANCH"
  PUSH_URL="https://x-access-token:${GITHUB_PERSONAL_ACCESS_TOKEN}@${REMOTE_HOST_PATH}"
  # Push via an inline URL so the token is never persisted in .git/config.
  # Scrub the token from any output just in case git ever echoes the URL.
  GIT_TERMINAL_PROMPT=0 GIT_ASKPASS=/bin/true \
    git push "$PUSH_URL" "$BRANCH" 2>&1 \
    | sed "s|${GITHUB_PERSONAL_ACCESS_TOKEN}|***REDACTED***|g"
  unset PUSH_URL
fi

# --- 4. Verify ---------------------------------------------------------------

git fetch "$REMOTE_NAME" "$BRANCH" --quiet || true
echo "[git-sync] === post-push status ==="
git status
echo "[git-sync] === recent commits ==="
git log --oneline -3
echo "[git-sync] done"
