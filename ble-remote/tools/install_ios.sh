#!/bin/bash
# Builds the iOS/iPadOS Release app and installs + launches it on a connected device.
# Usage: tools/install_ios.sh [device name, UDID or substring]
# With no argument, uses the only available physical device, or lists them if there are several.
set -euo pipefail

cd "$(dirname "$0")/.."
BUNDLE_ID="com.wwarner.BLERemote"
APP="build/Release-iphoneos/BLERemote.app"
JSON="$(mktemp)"
trap 'rm -f "$JSON"' EXIT

xcrun devicectl list devices --json-output "$JSON" >/dev/null
# Lines of "UDID<TAB>name" for available physical iPhones/iPads.
DEVICES="$(/usr/bin/python3 -I - "$JSON" <<'PY'
import json, sys
for d in json.load(open(sys.argv[1]))["result"]["devices"]:
    hw, cp = d.get("hardwareProperties", {}), d.get("connectionProperties", {})
    if hw.get("reality") == "physical" and hw.get("platform") == "iOS" \
            and cp.get("tunnelState") != "unavailable":
        print(f'{hw["udid"]}\t{d["deviceProperties"]["name"]}')
PY
)"

if [ -n "${1:-}" ]; then
    MATCH="$(printf '%s\n' "$DEVICES" | grep -iF -- "$1" || true)"
else
    MATCH="$DEVICES"
fi
COUNT="$(printf '%s' "$MATCH" | grep -c . || true)"
if [ "$COUNT" -ne 1 ]; then
    echo "Expected exactly one matching device, found $COUNT. Available:" >&2
    printf '%s\n' "$DEVICES" | sed 's/^/  /' >&2
    echo "Pass a name or UDID: tools/install_ios.sh \"iPad\"" >&2
    exit 1
fi
UDID="${MATCH%%$'\t'*}"
echo "Target: ${MATCH#*$'\t'} ($UDID)"

xcodebuild -project BLERemote.xcodeproj -target BLERemote -sdk iphoneos \
    -configuration Release -allowProvisioningUpdates build

xcrun devicectl device install app --device "$UDID" "$APP"
xcrun devicectl device process launch --device "$UDID" --terminate-existing "$BUNDLE_ID" \
    || echo "Installed, but launch failed (unlock the device and open the app manually)." >&2
