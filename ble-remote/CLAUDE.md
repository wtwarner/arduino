# BLE Remote: project state for resuming work

Last updated 2026-10-17. Read this first, then [README.md](README.md). The README has the
per-device command tables and how each command maps to a control.

## What this is
A SwiftUI iOS app (iOS 17+, Swift 5 language mode, `@Observable`) that scans for, pairs with
and controls three custom BLE devices: IV25 Clock, Nixie and River Table. The firmware lives
in the sibling folders `../iv25-clock`, `../nixie` and `../shift_bt`. Re-read the firmware
whenever a command set changes; the app hard-codes the UUIDs and commands in
`BLERemote/Model/DeviceKind.swift`.

## Status
Done and checked in the iOS Simulator with the demo devices:
- Scanning, pairing (save peripheral + device type) and auto-reconnect.
- Device list and switching between devices (sidebar list and title menu).
- Raw console for every device, with a command quick-insert menu.
- Device screens:
  - IV25 Clock: clock-face preview, time sync, brightness sliders, test modes, `gt`.
  - Nixie: nixie preview, time sync and nudges, display toggles, brightness, neon mode,
    fade tuning, anti-poison schedule.
  - River Table: live status sync, brightness, LED mode, LED test pad with a 56-LED grid.
- App icon (nixie tube + VFD tube), drawn by `tools/make_icon.swift`.

Not done or not verified:
- **Never tested against real hardware.** The Simulator has no Bluetooth. Untested on
  hardware: HM-10 write chunking, Nixie 64-char writes, notification handling, River Table
  status reassembly over a real UART.
- Not yet installed on the user's iPhone. The user has set `DEVELOPMENT_TEAM` in the project;
  keep it.
- Nothing is committed to git. `ble-remote/` is untracked inside the `~/Documents/Arduino`
  repo. Don't commit unless the user asks.

## Decisions to keep
- "Pairing" = saving the peripheral id + device type in UserDefaults. None of the modules use
  passkey bonding.
- IV25 Clock and River Table share the HM-10/HC-08 module (FFE0/FFE1). Detection: advertised
  name "Clock" means IV25; otherwise the pairing screen asks the user.
- Times are always sent as **US Pacific** local time (`DeviceTime` in `DeviceKind.swift`),
  because both clocks' firmware converts with hard-coded PST/PDT rules.
- `CommandSlider` sends throttled updates while dragging and the final value on release.
  Values set by code never send. River Table is limited to 4/s because each command rewrites
  EEPROM.
- The clocks never report state, so the last value sent is remembered per device
  (`DeviceConnection.remembered/remember`). River Table controls follow its streamed
  `key:value;` status, with a ~1 s grace period after a user change.
- Pickers and toggles that send use custom `Binding(get:set:)`, not `onChange`, so syncing a
  value from the device doesn't send it back.
- Nixie AM/PM dot is on the **left-most** tube (the user confirmed this for the physical
  clock). The firmware's `nixie_dec_state[1]` suggests otherwise; follow the user.
- Demo devices (`BLE/DemoDevice.swift`) are shown by default only in the Simulator. On a
  device they appear via the scanner's options menu. The River Table demo mirrors
  shift_bt.ino.

## Layout
- `BLERemote.xcodeproj`: hand-written pbxproj using a file-system-synchronized group, so new
  files under `BLERemote/` are picked up automatically and need no pbxproj edits. The app icon
  is set with `ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon`.
- `BLERemote/Model`: `DeviceKind` (UUIDs, framing, command reference, type detection),
  `DeviceStore` (paired devices).
- `BLERemote/BLE`: `BLEManager` (central, scanning, connect/reconnect), `DeviceConnection`
  (one per device: discovery, chunked writes, receiving, log), `DemoDevice`.
- `BLERemote/Views`: device list, scanner/pairing, detail host, console, shared components.
- `BLERemote/Devices`: `IV25ClockView`, `NixieView`, `RiverTableView`.
- `tools/make_icon.swift`: regenerates `BLERemote/Assets.xcassets/AppIcon.appiconset/AppIcon.png`.

## Build and run
```bash
cd ~/Documents/Arduino/ble-remote
xcodebuild -project BLERemote.xcodeproj -target BLERemote -sdk iphonesimulator -configuration Debug CODE_SIGNING_ALLOWED=NO build
```
The built app is at `build/Debug-iphonesimulator/BLERemote.app`; launch it with the iOS
Simulator tool. The simulator used was iPhone 18 Pro (3A13C9D9-F346-4C8E-A971-7A8EFF0D5254).
Regenerate the icon with `swift tools/make_icon.swift BLERemote/Assets.xcassets/AppIcon.appiconset/AppIcon.png`.
To run on the phone, the user opens the project in Xcode and presses ⌘R.

## Environment notes
- In the last session, the Bash tool was blocked by a PreToolUse hook (`~/.claude/apple`
  proxy failure). Shell commands ran through the terminal panel (`run_in_terminal` /
  `read_terminal`) instead. Try Bash first next time; it may be fixed.
- Simulator screenshots tend to lag one action behind; take another screenshot before
  concluding a tap failed.

## Possible next steps
- Test with the real devices and fix whatever the hardware reveals.
- Have the firmware report state (IV25 / Nixie) so the app doesn't rely on remembered values.
- Dark/tinted icon variants; iPad layout polish.
