import SwiftUI

/// Controls for iv25-clock (commands parsed in iv25-clock.ino loop()).
struct IV25ClockView: View {
    let connection: DeviceConnection
    @State private var brightness: Double
    @State private var ledBrightness: Double
    @State private var testMode: Int
    @State private var customTime = Date()

    init(connection: DeviceConnection) {
        self.connection = connection
        _brightness = State(initialValue: connection.remembered("b", default: 255))
        _ledBrightness = State(initialValue: connection.remembered("nb", default: 128))
        _testMode = State(initialValue: Int(connection.remembered("tm", default: 0)))
    }

    var body: some View {
        Form {
            ConnectionSection(connection: connection)

            Section {
                IV25FacePreview()
                    .frame(height: 260)
                    .frame(maxWidth: .infinity)
                    .listRowBackground(Color.black)
            } footer: {
                Text("How the clock face looks at the current US Pacific time.")
            }

            Group {
                Section {
                    Button { sendTime(Date()) } label: {
                        Label("Sync to iPhone Time", systemImage: "arrow.triangle.2.circlepath")
                    }
                    DatePicker("Custom", selection: $customTime)
                    Button("Set Custom Time") { sendTime(customTime) }
                } header: {
                    Text("Time")
                } footer: {
                    Text("Sent as US Pacific local time; the clock applies its built-in PST/PDT rules.")
                }

                Section {
                    CommandSlider(title: "VFD Tubes", systemImage: "sun.max",
                                  interval: connection.kind.sliderInterval, value: $brightness) { v in
                        connection.send("b \(v)")
                        connection.remember("b", Double(v))
                    }
                    CommandSlider(title: "Accent LEDs", systemImage: "light.ribbon",
                                  interval: connection.kind.sliderInterval, value: $ledBrightness) { v in
                        connection.send("nb \(v)")
                        connection.remember("nb", Double(v))
                    }
                } header: {
                    Text("Brightness")
                } footer: {
                    Text("Saved in the clock's RTC and restored at power-up.")
                }

                Section {
                    Picker("Mode", selection: Binding(get: { testMode }, set: { m in
                        testMode = m
                        connection.send("tm \(m)")
                        connection.remember("tm", Double(m))
                    })) {
                        Text("Clock").tag(0)
                        Text("Chase").tag(1)
                        Text("Tube Walk").tag(2)
                        Text("Pattern").tag(3)
                    }
                    .pickerStyle(.segmented)
                } header: {
                    Text("Display Mode")
                } footer: {
                    Text(modeDescription)
                }

                Section {
                    Button { connection.send("gt") } label: {
                        Label("Print Time to USB Serial", systemImage: "cable.connector")
                    }
                } header: {
                    Text("Diagnostics")
                } footer: {
                    Text("The clock prints the time on its USB serial port only; nothing comes back over Bluetooth.")
                }
            }
            .disabled(!connection.isReady)
        }
    }

    private var modeDescription: String {
        switch testMode {
        case 1: "Test: alternating single segments chase across all 24 tubes."
        case 2: "Test: lights each tube in turn to check wiring."
        case 3: "Test: animated pattern on the first tubes."
        default: "Normal clock display."
        }
    }

    private func sendTime(_ date: Date) {
        connection.send("st \(DeviceTime.iso8601(date))")
    }
}

/// Mirrors time_to_vfd(): 12 positions, each an inner (lo) and outer (hi) 7-segment tube.
/// Hour lights the inner tube; minutes/5 lights both tubes; minutes%5 partially fills the
/// next inner tube; other inner tubes blink one segment each second.
struct IV25FacePreview: View {
    private let glow = Color(red: 0.35, green: 1.0, blue: 0.85)

    var body: some View {
        TimelineView(.periodic(from: .now, by: 1)) { context in
            let t = DeviceTime.calendar.dateComponents([.hour, .minute, .second], from: context.date)
            Canvas { gc, size in
                draw(in: &gc, size: size, hour: (t.hour ?? 0) % 12, minute: t.minute ?? 0, second: t.second ?? 0)
            }
            .accessibilityLabel("Clock face preview")
            .accessibilityValue(Text(context.date, format: .dateTime.hour().minute()))
        }
    }

    private func draw(in gc: inout GraphicsContext, size: CGSize, hour: Int, minute: Int, second: Int) {
        let center = CGPoint(x: size.width / 2, y: size.height / 2)
        let radius = min(size.width, size.height) / 2 - 6
        let m5 = minute / 5
        let next = (m5 + 1) % 12
        let dot = radius * 0.032

        for pos in 0..<12 {
            var lo = 0
            var hi = 0
            if pos == hour { lo = 0x7F }
            if pos == m5 { lo = 0x7F; hi = 0x7F }
            if pos == next { lo |= (1 << (minute % 5)) - 1 }
            if pos != hour && pos != m5 && pos != next && second % 2 == 1 { lo |= 1 }

            let angle = Double(pos) / 12 * 2 * .pi - .pi / 2
            let dir = CGPoint(x: cos(angle), y: sin(angle))
            for (mask, start) in [(lo, 0.30), (hi, 0.64)] {
                for seg in 0..<7 {
                    let r = radius * (start + Double(seg) * 0.05)
                    let p = CGPoint(x: center.x + dir.x * r, y: center.y + dir.y * r)
                    let rect = CGRect(x: p.x - dot, y: p.y - dot, width: dot * 2, height: dot * 2)
                    let lit = mask & (1 << seg) != 0
                    if lit {
                        var g = gc
                        g.addFilter(.shadow(color: glow, radius: 4))
                        g.fill(Path(ellipseIn: rect), with: .color(glow))
                    } else {
                        gc.fill(Path(ellipseIn: rect), with: .color(.white.opacity(0.08)))
                    }
                }
            }
        }
    }
}
