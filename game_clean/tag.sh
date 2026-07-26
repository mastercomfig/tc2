#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

if [ "${GITHUB_REF_TYPE:-}" = "tag" ] && [ -n "${GITHUB_REF_NAME:-}" ]; then
    VERSION="${GITHUB_REF_NAME}"
fi

if [ -n "${GH_REPO}" ]; then
    RELEASE_REPO="${GH_REPO}"
else
    REPO_OWNER="${GITHUB_REPOSITORY_OWNER:-}"
    if [ -z "${REPO_OWNER}" ]; then
        REMOTE_URL=$(git remote get-url origin 2>/dev/null || echo "")
        REPO_OWNER=$(echo "${REMOTE_URL}" | sed -E 's#.*[:/]([^/]+)/[^/]+(\.git)?$#\1#')
    fi

    if [ "${REPO_OWNER}" = "mastercomfig" ] || [ "${REPO_OWNER}" = "mastercoms" ]; then
        RELEASE_REPO="mastercomfig/tc2"
    else
        RELEASE_REPO=""
    fi
fi

if [ -n "${RELEASE_REPO}" ]; then
    REPO_FLAG=(--repo "${RELEASE_REPO}")
else
    REPO_FLAG=()
fi

git fetch --tags origin

TARGET_COMMIT="${GITHUB_SHA:-$(git rev-parse HEAD 2>/dev/null || echo "${GITHUB_REF_NAME:-tc2-mod}")}"

if IS_PRERELEASE=$(gh release view "${VERSION}" "${REPO_FLAG[@]}" --json "isPrerelease" --template "{{.isPrerelease}}" 2>/dev/null); then
    echo "Release ${VERSION} already exists on GitHub. Uploading asset..."
    if [ "${IS_PRERELEASE}" = "true" ]; then
        gh release upload "${REPO_FLAG[@]}" --clobber "${VERSION}" "../game-${PLATFORM}.zip" || true
    else
        gh release upload "${REPO_FLAG[@]}" "${VERSION}" "../game-${PLATFORM}.zip" || true
    fi
else
    echo "Release ${VERSION} does not exist on GitHub yet. Creating release at commit ${TARGET_COMMIT}..."
    if [ "${GITHUB_REPOSITORY:-}" = "mastercoms/tc2-private" ] || [ "${PRIVATE_RELEASE:-false}" = "true" ]; then
        if [ -z "$(git config user.name 2>/dev/null || true)" ]; then
            git config user.name "github-actions[bot]"
            git config user.email "github-actions[bot]@users.noreply.github.com"
        fi
        PREV_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "")
        REMOTE_URL="https://x-access-token:${GH_TOKEN:-${GITHUB_TOKEN}}@github.com/${RELEASE_REPO:-mastercomfig/tc2}.git"
        git fetch "${REMOTE_URL}" private-release:refs/remotes/origin/private-release || true
        if git rev-parse refs/remotes/origin/private-release >/dev/null 2>&1; then
            git checkout -B private-release refs/remotes/origin/private-release
        else
            git checkout --orphan private-release
        fi
        git commit --allow-empty -m "Release ${VERSION}"
        TARGET_COMMIT=$(git rev-parse HEAD)
        git push "${REMOTE_URL}" private-release
        if [ -n "${PREV_BRANCH}" ] && [ "${PREV_BRANCH}" != "HEAD" ]; then
            git checkout "${PREV_BRANCH}" || true
        fi
    fi
    if ! git rev-parse "${VERSION}" -- > /dev/null 2>&1; then
        git tag "${VERSION}" "${TARGET_COMMIT}"
    fi
    git pull
    git push origin "${VERSION}"

    NOTES="Release ${VERSION}

[Download](https://teamcomtress.com/)
[Patch Notes](https://teamcomtress.com/feed/#patches)"

    gh release create "${VERSION}" \
        "../game-${PLATFORM}.zip" \
        "${REPO_FLAG[@]}" \
        --target "${TARGET_COMMIT}" \
        --title "${VERSION}" \
        --notes "${NOTES}" \
        --prerelease \
        --verify-tag \
        --fail-on-no-commits
fi
