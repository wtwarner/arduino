#!/bin/bash
# Builds the macOS Release app (signed with the project's development team) and
# installs it to /Applications. Usage: tools/install_mac.sh [destination-dir]
set -euo pipefail

cd "$(dirname "$0")/.."
DEST="${1:-/Applications}"
APP="build/Release/BLERemote.app"

xcodebuild -project BLERemote.xcodeproj -target BLERemote -sdk macosx \
    -configuration Release build

[ -d "$APP" ] || { echo "Build did not produce $APP" >&2; exit 1; }

# Quit a running copy so the replacement isn't in use.
osascript -e 'tell application id "com.wwarner.BLERemote" to quit' 2>/dev/null || true

rm -rf "$DEST/BLERemote.app"
cp -R "$APP" "$DEST/"
echo "Installed $DEST/BLERemote.app"
