#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

if git rev-parse ${VERSION} -- > /dev/null 2>&1; then
    echo "Tag ${VERSION} already exists. Not creating a release."
    exit 0
fi

git tag ${VERSION}
git pull
git push origin ${VERSION}

gh release create ${VERSION} \
    "../game-${PLATFORM}.zip" \
    --title "${VERSION}" \
    --notes "Release ${VERSION}, view the [patch notes](https://teamcomtress.com/feed/#patches) for details." \
    --verify-tag \
    --fail-on-no-commits
