#!/usr/bin/env bash
#
# Run script within the directory
BIN_DIR=$(dirname "$(readlink -fn "$0")")
cd "${BIN_DIR}" || exit 2

set -e

source ./shared.sh

git fetch --tags origin

if git rev-parse ${VERSION} -- > /dev/null 2>&1; then
    #echo "::warning Tag ${VERSION} already exists. Not creating a release."
    IS_PRERELEASE=$(gh release view ${VERSION} --json "isPrerelease" --template "{{.isPrerelease}}")
    if [ "${IS_PRERELEASE}" == "true" ]; then
        gh release upload --clobber ${VERSION} "../game-${PLATFORM}.zip" || true
    else
        gh release upload ${VERSION} "../game-${PLATFORM}.zip" || true
    fi
else
    printf "Release ${VERSION}\n\n[Download](https://teamcomtress.com/)\n[Patch Notes](https://teamcomtress.com/feed/#patches)" > "notes.txt"
    git tag ${VERSION}
    git pull
    git push origin ${VERSION}
    gh release create ${VERSION} \
        "../game-${PLATFORM}.zip" \
        --title "${VERSION}" \
        -F "notes.txt" \
        --prerelease \
        --verify-tag \
        --fail-on-no-commits
    rm notes.txt
fi
