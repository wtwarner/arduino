# BLE Remote

SwiftUI iOS app (iOS 17+) that scans for, pairs with and controls three custom BLE devices.
Open `BLERemote.xcodeproj`, set your Team under Signing & Capabilities, and run on an iPhone.
The iOS Simulator has no Bluetooth; use the **Demo Devices** in the Add Device sheet there.

## App structure

| Area | File |
|---|---|
| Device types, UUIDs, framing, command reference | `BLERemote/Model/DeviceKind.swift` |
| Paired-device persistence | `BLERemote/Model/DeviceStore.swift` |
| Scanning, connect / auto-reconnect | `BLERemote/BLE/BLEManager.swift` |
| Per-device link: discovery, chunked writes, status parsing | `BLERemote/BLE/DeviceConnection.swift` |
| Simulated devices | `BLERemote/BLE/DemoDevice.swift` |
| Device list, scanner/pairing, console | `BLERemote/Views/` |
| Per-device control screens | `BLERemote/Devices/` |

"Pairing" means saving the peripheral and its device type. None of the modules need a
passkey; the app reconnects to paired devices whenever they are in range. IV25 Clock and
River Table use the same HM-10/HC-08 module, so the pairing screen asks which one it is
unless the advertised name identifies it ("Clock" → IV25 Clock).

Both clocks convert local time with hard-coded US Pacific rules, so the app always sends
times in Pacific time regardless of the phone's time zone.

## Command sets (from the firmware)

### IV25 Clock — `iv25-clock/`
HM-10/HC-08 UART, service `FFE0`, characteristic `FFE1`. Framed as `[cmd args]` (`btCmdParser`).
No replies over BT.

| Command | Effect | UI |
|---|---|---|
| `b <0-255>` | VFD brightness (saved in RTC) | VFD Tubes slider |
| `nb <0-255>` | NeoPixel brightness (saved in RTC) | Accent LEDs slider |
| `st YYYY-MM-DDTHH:MM:SS` | Set local time | Sync / Set Custom Time |
| `tm <0-3>` | Test mode (0 clock, 1 chase, 2 tube walk, 3 pattern) | Display Mode picker |
| `gt` | Print time on USB serial | Diagnostics |

### Nixie — `nixie/`
ArduinoBLE, service `903B13CE-EADB-482F-B8FA-A5A2ECFFFFB4`, characteristic `2BDE`
(write-without-response + notify, 64 chars). One write per command, `CMD:value` (`cmd_parse()`).

| Command | Effect | UI |
|---|---|---|
| `B:<0-255>` | Nixie brightness (saved) | Nixie Tubes slider |
| `NB:<0-255>` | Neon brightness (saved) | Neon Lamps slider |
| `NM:<0-3>` | Neon mode: alternate / steady / blink / off | Neon Mode picker |
| `24:<0\|1>` | 24-hour format | toggle |
| `SS:<0\|1>` | Show seconds instead of minutes | toggle |
| `H:` `M:` `S:` `<±n>` | Nudge time | step buttons |
| `ZS:1` | Cycle Time → Date → Year → Anti-poison | Next Screen |
| `set-time:YYYY-MM-DDTHH:MM:SS` | Set local time | Sync to iPhone Time |
| `set-neon:g start steps stepsize timestep off mode` | Neon fade tuning | Fade Tuning |
| `set-ap:hour min ms` | Anti-poison schedule (24/60 = every) | Anti-Poison |

### River Table — `shift_bt/`
HM-10 UART, `FFE0`/`FFE1`, framed `[cmd arg]`. Streams `key:value;` status every 250 ms:
`led`, `ledpwm`, `neopwm`, `stest`, `modeled0..2`; the UI follows these values.

| Command | Effect | UI |
|---|---|---|
| `ledbright <0-255>` | Shift-register LED PWM | Table LEDs slider |
| `neobright <0-255>` | NeoPixel brightness | NeoPixels slider |
| `ledmode <0-2>` | Wave / Twinkle / LED Test | LED Mode picker |
| `stest <1-4>` | Test LED −1 / +1 / −10 / +10 | LED Test pad + 56-LED grid |

Every River Table command triggers `EEPROM.put`, so sliders send at most 4×/s while dragging.
