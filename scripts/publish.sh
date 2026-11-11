#!/usr/bin/env bash
# Uploads the source code to GitHub.
#
#   scripts/publish.sh                      commit everything and push to main
#   scripts/publish.sh "Linux port, v2.0"   ... with your own commit message
#
# Options (environment variables):
#   NIIX_REMOTE   repository URL  (default: https://github.com/techniixdotcom/niix-torrent.git)
#   NIIX_BRANCH   branch to push  (default: main)
#
# Works from a fresh unzip as well as from an existing clone. When the folder
# is not a git repository yet, it is connected to the GitHub repository first
# and your files are committed ON TOP of the history that is already there -
# nothing on GitHub is overwritten and no force-push is used.
#
# Build output and downloaded dependencies (build-*, dist, vendor/...) are
# excluded by .gitignore. Signing in to GitHub is handled by git itself: the
# first push opens the browser / asks for a token, or uses your SSH key if
# NIIX_REMOTE is an SSH address (git@github.com:techniixdotcom/niix-torrent.git).
set -euo pipefail

REMOTE="${NIIX_REMOTE:-https://github.com/techniixdotcom/niix-torrent.git}"
BRANCH="${NIIX_BRANCH:-main}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="$(head -n1 "$ROOT/VERSION" | tr -d '[:space:]')"
MESSAGE="${1:-NiiX Torrent $VERSION}"

cd "$ROOT"

# Never hand output to a pager (less may not be installed, and the script
# should not stop to wait for a key press).
export GIT_PAGER=cat PAGER=cat

if ! command -v git >/dev/null 2>&1; then
    # shellcheck source=scripts/prereqs.sh
    source "$ROOT/scripts/prereqs.sh"
    echo "git is not installed - installing it"
    _install_packages native
fi

# git needs to know who is committing.
if [ -z "$(git config user.name || true)" ] || [ -z "$(git config user.email || true)" ]; then
    echo "error: git does not know your name and e-mail yet. Set them once with:" >&2
    echo "         git config --global user.name  \"Your Name\"" >&2
    echo "         git config --global user.email \"you@example.com\"" >&2
    exit 1
fi

if [ ! -d .git ]; then
    echo "[git] connecting this folder to $REMOTE"
    git init --quiet --initial-branch="$BRANCH"
    git remote add origin "$REMOTE"

    if git fetch --quiet origin "$BRANCH" 2>/dev/null; then
        # Adopt the existing history without touching the files in this
        # folder: they become the next commit on top of it.
        git reset --quiet --mixed FETCH_HEAD
        git branch --quiet --set-upstream-to="origin/$BRANCH" "$BRANCH" 2>/dev/null || true
        echo "[git] continuing from the existing history on GitHub"
    else
        echo "[git] the repository has no '$BRANCH' branch yet - this will be its first commit"
    fi
else
    if git remote get-url origin >/dev/null 2>&1; then
        git remote set-url origin "$REMOTE"
    else
        git remote add origin "$REMOTE"
    fi

    current="$(git rev-parse --abbrev-ref HEAD)"
    if [ "$current" != "$BRANCH" ]; then
        echo "error: you are on branch '$current' but publishing '$BRANCH'." >&2
        echo "       Switch with 'git switch $BRANCH' or run:  NIIX_BRANCH=$current scripts/publish.sh" >&2
        exit 1
    fi
fi

git add --all

# Keep the shell scripts executable for Linux checkouts, also when this
# commit is made on Windows.
git update-index --chmod=+x build.sh scripts/bootstrap.sh scripts/prereqs.sh scripts/publish.sh

if git diff --cached --quiet; then
    echo "[git] no changes to commit"
else
    echo "[git] changes in this upload:"
    git diff --cached --shortstat
    git commit --quiet -m "$MESSAGE"
    echo "[git] committed: $MESSAGE"
fi

echo "[git] pushing to $REMOTE ($BRANCH)"
if ! git push --set-upstream origin "$BRANCH"; then
    echo >&2
    echo "error: the push was rejected. If GitHub has commits you do not have here, run" >&2
    echo "         git pull --rebase origin $BRANCH" >&2
    echo "       and then this script again. If it is a sign-in problem, check that your" >&2
    echo "       account may write to the repository." >&2
    exit 1
fi

WEB="${REMOTE%.git}"
WEB="${WEB/#git@github.com:/https://github.com/}"

echo
echo "Done: $WEB"
echo "Create the release at: $WEB/releases/new"
