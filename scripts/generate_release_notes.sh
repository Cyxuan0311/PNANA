#!/bin/bash

# Generate release notes script
# Usage: ./generate_release_notes.sh <VERSION> [PREV_VERSION]

set -e

VERSION=$1
PREV_VERSION=${2:-"latest"}
DATE=$(date +%Y-%m-%d)
USERNAME=${GITHUB_REPOSITORY_OWNER:-"your-username"}

if [ -z "$VERSION" ]; then
    echo "Error: Please provide version number"
    echo "Usage: $0 <VERSION> [PREV_VERSION]"
    exit 1
fi

# Create temporary release notes file
TEMP_RELEASE_FILE="release_notes_${VERSION}.md"

# Replace placeholders in template
sed -e "s/{VERSION}/${VERSION}/g" \
    -e "s/{DATE}/${DATE}/g" \
    -e "s/{USERNAME}/${USERNAME}/g" \
    -e "s/{PREV_VERSION}/${PREV_VERSION}/g" \
    -e "s|{PREV_RELEASE_URL}|https://github.com/${USERNAME}/pnana/releases/tag/v${PREV_VERSION}|g" \
    RELEASE.md > "${TEMP_RELEASE_FILE}"

echo "Release notes generated: ${TEMP_RELEASE_FILE}"
echo "Please edit this file, add actual version update content, then use for GitHub Release"

# If in GitHub Actions environment, output file path
if [ -n "$GITHUB_ACTIONS" ]; then
    echo "release_notes_file=${TEMP_RELEASE_FILE}" >> $GITHUB_OUTPUT
fi