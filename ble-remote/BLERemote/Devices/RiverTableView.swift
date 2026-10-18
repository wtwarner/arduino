import SwiftUI

/// Controls for shift_bt (commands parsed in shift_bt.ino loop()). The table streams
/// its state, so controls follow what the device reports.
struct RiverTableView: View {
    let connection: DeviceConnection

    @State private var ledBrightness: Double
    @State private var neoBrightness: Double
    @State private var mode: Int
    @State private var lastModeChange = Date.distantPast

    private static let ledCount = 56 // 7 shift registers × 8 (shift.ino NUM_LEDS)

    init(connection: DeviceConnection) {
        self.connection = connection
        _ledBrightness = State(initialValue: connection.remembered("ledbright", default: 128))
        _neoBrightness = State(initialValue: connection.remembered("neobright", default: 128))
        _mode = State(initialValue: Int(connection.remembered("ledmode", default: 1)))
    }

    private func reported(_ key: String) -> Int? { connection.status[key].flatMap { Int($0) } }

    private var reportedMode: Int? {
        (0...2).first { reported("modeled\($0)") == 1 }
    }

    var body: some View {
        Form {
            ConnectionSection(connection: connection)

            Section("Live Status") {
                TimelineView(.periodic(from: .now, by: 1)) { context in
                    let fresh = connection.lastStatusDate.map { context.date.timeIntervalSince($0) < 2 } ?? false
                    HStack {
                        Circle()
                            .fill(reported("led") == 1 && fresh ? Color.green : Color.gray.opacity(0.3))
                            .frame(width: 12, height: 12)
                            .animation(.easeInOut(duration: 0.15), value: reported("led"))
                        Text(fresh ? "Receiving heartbeat" : "No status from table")
                            .foregroundStyle(fresh ? .primary : .secondary)
                        Spacer()
                        if fresh, let m = reportedMode {
                            Text(modeName(m)).foregroundStyle(.secondary)
                        }
                    }
                    .accessibilityElement(children: .combine)
                }
            }

            Group {
                Section {
                    CommandSlider(title: "Table LEDs", systemImage: "sun.max",
                                  interval: connection.kind.sliderInterval,
                                  reported: reported("ledpwm"), value: $ledBrightness) { v in
                        connection.send("ledbright \(v)")
                        connection.remember("ledbright", Double(v))
                    }
                    CommandSlider(title: "NeoPixels", systemImage: "light.ribbon",
                                  interval: connection.kind.sliderInterval,
                                  reported: reported("neopwm"), value: $neoBrightness) { v in
                        connection.send("neobright \(v)")
                        connection.remember("neobright", Double(v))
                    }
                } header: {
                    Text("Brightness")
                } footer: {
                    Text("Saved in the table's EEPROM.")
                }

                Section {
                    Picker("Mode", selection: Binding(get: { mode }, set: { m in
                        mode = m
                        lastModeChange = Date()
                        connection.send("ledmode \(m)")
                        connection.remember("ledmode", Double(m))
                    })) {
                        ForEach(0...2, id: \.self) { Text(modeName($0)).tag($0) }
                    }
                    .pickerStyle(.segmented)
                } header: {
                    Text("LED Mode")
                } footer: {
                    Text(modeDescription(mode))
                }

                if mode == 2 {
                    testSection
                }
            }
            .disabled(!connection.isReady)
        }
        .onChange(of: reportedMode) { _, new in
            guard let new, Date().timeIntervalSince(lastModeChange) > 1 else { return }
            mode = new
        }
        .animation(.default, value: mode)
    }

    private var testSection: some View {
        Section {
            let index = reported("stest")
            LEDGrid(count: Self.ledCount, lit: index)
                .padding(.vertical, 6)
            HStack {
                Text(index.map { $0 < Self.ledCount ? "LED \($0)" : "LED \($0) (past end; all off)" } ?? "LED –")
                    .monospacedDigit()
                Spacer()
            }
            HStack {
                // shift_bt.ino "stest" pad codes: 1 −1, 2 +1, 3 −10, 4 +10
                testButton("−10", systemImage: "backward.fill", code: 3)
                testButton("−1", systemImage: "chevron.left", code: 1)
                testButton("+1", systemImage: "chevron.right", code: 2)
                testButton("+10", systemImage: "forward.fill", code: 4)
            }
            .buttonStyle(.bordered)
        } header: {
            Text("LED Test")
        } footer: {
            Text("Lights one LED at a time along the shift-register chain to find wiring faults.")
        }
    }

    private func testButton(_ title: String, systemImage: String, code: Int) -> some View {
        Button { connection.send("stest \(code)") } label: {
            Label(title, systemImage: systemImage)
                .labelStyle(.iconOnly)
                .frame(maxWidth: .infinity)
        }
        .accessibilityLabel("Test LED \(title)")
    }

    private func modeName(_ m: Int) -> String {
        switch m {
        case 0: "Wave"
        case 1: "Twinkle"
        default: "LED Test"
        }
    }

    private func modeDescription(_ m: Int) -> String {
        switch m {
        case 0: "A sine wave of brightness rolls across the five PWM channels."
        case 1: "LEDs flicker randomly, like light on water."
        default: "Single-LED test mode."
        }
    }
}

private struct LEDGrid: View {
    let count: Int
    let lit: Int?

    var body: some View {
        let columns = Array(repeating: GridItem(.flexible(), spacing: 6), count: 8)
        LazyVGrid(columns: columns, spacing: 6) {
            ForEach(0..<count, id: \.self) { i in
                Circle()
                    .fill(i == lit ? Color.yellow : Color.secondary.opacity(0.2))
                    .shadow(color: i == lit ? .yellow : .clear, radius: 4)
                    .aspectRatio(1, contentMode: .fit)
                    .overlay {
                        if i % 8 == 0 {
                            Text("\(i)").font(.system(size: 8)).foregroundStyle(.secondary)
                        }
                    }
            }
        }
        .accessibilityElement()
        .accessibilityLabel("LED test position")
        .accessibilityValue(lit.map { "LED \($0)" } ?? "unknown")
    }
}
