import SwiftUI

/// Controls for nixie (commands parsed in nixie.ino cmd_parse()).
struct NixieView: View {
    let connection: DeviceConnection

    @State private var brightness: Double
    @State private var neonBrightness: Double
    @State private var neonMode: Int
    @State private var twentyFourHour: Bool
    @State private var showSeconds: Bool

    // set-neon: tuning (firmware defaults from options_t::to_defaults)
    @State private var neonGamma: Bool
    @State private var neonSteps: Double
    @State private var neonStepSize: Double
    @State private var neonTimeStep: Double
    @State private var neonOff: Double

    // set-ap: anti-poison schedule
    @State private var apHour: Int
    @State private var apMinute: Int
    @State private var apDuration: Int

    init(connection: DeviceConnection) {
        self.connection = connection
        let r = { (key: String, def: Double) in connection.remembered(key, default: def) }
        _brightness = State(initialValue: r("B", 192))
        _neonBrightness = State(initialValue: r("NB", 192))
        _neonMode = State(initialValue: Int(r("NM", 1)))
        _twentyFourHour = State(initialValue: r("24", 0) != 0)
        _showSeconds = State(initialValue: r("SS", 0) != 0)
        _neonGamma = State(initialValue: r("neon.gamma", 0) != 0)
        _neonSteps = State(initialValue: r("neon.steps", 100))
        _neonStepSize = State(initialValue: r("neon.stepsize", 1))
        _neonTimeStep = State(initialValue: r("neon.timestep", 10))
        _neonOff = State(initialValue: r("neon.off", 1))
        _apHour = State(initialValue: Int(r("ap.hour", 5)))
        _apMinute = State(initialValue: Int(r("ap.min", 13)))
        _apDuration = State(initialValue: Int(r("ap.ms", 600)))
    }

    var body: some View {
        Form {
            ConnectionSection(connection: connection)

            Section {
                NixiePreview(twentyFourHour: twentyFourHour, showSeconds: showSeconds, neonMode: neonMode)
                    .frame(maxWidth: .infinity)
                    .listRowBackground(Color.black)
            } footer: {
                Text("Preview at the current US Pacific time.")
            }

            Group {
                timeSection
                displaySection
                brightnessSection
                neonSection
                antiPoisonSection
            }
            .disabled(!connection.isReady)
        }
    }

    private var timeSection: some View {
        Section {
            Button {
                connection.send("set-time:\(DeviceTime.iso8601(Date()))")
            } label: {
                Label("Sync to iPhone Time", systemImage: "arrow.triangle.2.circlepath")
            }
            StepButtons(title: "Hours", steps: [-1, 1]) { connection.send("H:\($0)") }
            StepButtons(title: "Minutes", steps: [-1, 1]) { connection.send("M:\($0)") }
            StepButtons(title: "Seconds", steps: [-10, -1, 1, 10]) { connection.send("S:\($0)") }
        } header: {
            Text("Time")
        } footer: {
            Text("Sync sends US Pacific local time. The step buttons nudge the clock's current time.")
        }
    }

    private var displaySection: some View {
        Section {
            Toggle("24-Hour Time", isOn: sending($twentyFourHour, key: "24"))
            Toggle("Show Seconds", isOn: sending($showSeconds, key: "SS"))
            Button {
                connection.send("ZS:1")
            } label: {
                Label("Next Screen", systemImage: "rectangle.2.swap")
            }
        } header: {
            Text("Display")
        } footer: {
            Text("Next Screen cycles Time → Date → Year → Anti-poison. Date and year show for 3 seconds.")
        }
    }

    private var brightnessSection: some View {
        Section {
            CommandSlider(title: "Nixie Tubes", systemImage: "sun.max",
                          interval: connection.kind.sliderInterval, value: $brightness) { v in
                connection.send("B:\(v)")
                connection.remember("B", Double(v))
            }
            CommandSlider(title: "Neon Lamps", systemImage: "circle.grid.2x1.fill",
                          interval: connection.kind.sliderInterval, value: $neonBrightness) { v in
                connection.send("NB:\(v)")
                connection.remember("NB", Double(v))
            }
        } header: {
            Text("Brightness")
        } footer: {
            Text("Saved in the clock's RTC and restored at power-up.")
        }
    }

    private var neonSection: some View {
        Section {
            Picker("Neon Mode", selection: Binding(get: { neonMode }, set: { m in
                neonMode = m
                connection.send("NM:\(m)")
                connection.remember("NM", Double(m))
            })) {
                Text("Alternate").tag(0)
                Text("Steady").tag(1)
                Text("Blink").tag(2)
                Text("Off").tag(3)
            }
            .pickerStyle(.segmented)

            DisclosureGroup("Fade Tuning") {
                Toggle("Gamma Correction", isOn: $neonGamma)
                tuningSlider("Fade Steps", value: $neonSteps, range: 1...255)
                tuningSlider("Step Size", value: $neonStepSize, range: 1...50)
                tuningSlider("Step Interval (ms)", value: $neonTimeStep, range: 1...100)
                tuningSlider("Off Level", value: $neonOff, range: 0...255)
                Button("Apply Fade Settings", action: applyNeon)
                Button("Restore Defaults") {
                    neonGamma = false; neonSteps = 100; neonStepSize = 1; neonTimeStep = 10; neonOff = 1
                    applyNeon()
                }
            }
        } header: {
            Text("Neon Lamps")
        } footer: {
            Text("Each second the lamps start at Neon brightness and dim by Step Size every Step Interval, for up to Fade Steps steps. Alternate and Blink fade; Steady holds.")
        }
    }

    private var antiPoisonSection: some View {
        Section {
            Picker("Hour", selection: $apHour) {
                Text("Every Hour").tag(24)
                ForEach(0..<24, id: \.self) { Text(String(format: "%02d", $0)).tag($0) }
            }
            Picker("Minute", selection: $apMinute) {
                Text("Every Minute").tag(60)
                ForEach(0..<60, id: \.self) { Text(String(format: ":%02d", $0)).tag($0) }
            }
            Stepper(value: $apDuration, in: 100...10_000, step: 100) {
                LabeledContent("Duration", value: "\(apDuration) ms")
            }
            Button("Apply Schedule") {
                connection.send("set-ap:\(apHour) \(apMinute) \(apDuration)")
                connection.remember("ap.hour", Double(apHour))
                connection.remember("ap.min", Double(apMinute))
                connection.remember("ap.ms", Double(apDuration))
            }
        } header: {
            Text("Anti-Poison")
        } footer: {
            Text("Cycles all digits to prevent cathode poisoning. Default is 05:13 for 600 ms. Fade tuning and this schedule reset to defaults when the clock restarts.")
        }
    }

    private func tuningSlider(_ title: String, value: Binding<Double>, range: ClosedRange<Double>) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            LabeledContent(title, value: "\(Int(value.wrappedValue))")
            Slider(value: value, in: range, step: 1).accessibilityLabel(title)
        }
    }

    private func applyNeon() {
        let args = [neonGamma ? 1 : 0, Int(neonBrightness), Int(neonSteps), Int(neonStepSize),
                    Int(neonTimeStep), Int(neonOff), neonMode]
        connection.send("set-neon:" + args.map(String.init).joined(separator: " "))
        connection.remember("neon.gamma", neonGamma ? 1 : 0)
        connection.remember("neon.steps", neonSteps)
        connection.remember("neon.stepsize", neonStepSize)
        connection.remember("neon.timestep", neonTimeStep)
        connection.remember("neon.off", neonOff)
    }

    /// A toggle binding that sends "KEY:0|1" when the user flips it.
    private func sending(_ binding: Binding<Bool>, key: String) -> Binding<Bool> {
        Binding(get: { binding.wrappedValue }, set: { on in
            binding.wrappedValue = on
            connection.send("\(key):\(on ? 1 : 0)")
            connection.remember(key, on ? 1 : 0)
        })
    }
}

/// Four glowing digits plus the two neon lamps, following update_nixie_time().
struct NixiePreview: View {
    let twentyFourHour: Bool
    let showSeconds: Bool
    let neonMode: Int

    private let glow = Color(red: 1.0, green: 0.55, blue: 0.15)

    var body: some View {
        TimelineView(.periodic(from: .now, by: 1)) { context in
            let c = DeviceTime.calendar.dateComponents([.hour, .minute, .second], from: context.date)
            let h24 = c.hour ?? 0
            let h = twentyFourHour ? h24 : (h24 % 12 == 0 ? 12 : h24 % 12)
            let low = showSeconds ? (c.second ?? 0) : (c.minute ?? 0)
            let pm = !twentyFourHour && h24 >= 12
            let odd = (c.second ?? 0) % 2 == 1

            HStack(spacing: 6) {
                // AM/PM dot is on the left-most tube, lit even when its digit is blanked.
                digit(h >= 10 ? "\(h / 10)" : nil, dot: pm)
                digit("\(h % 10)")
                VStack(spacing: 14) {
                    lamp(on: lampOn(index: 0, odd: odd))
                    lamp(on: lampOn(index: 1, odd: odd))
                }
                .padding(.horizontal, 4)
                digit("\(low / 10)")
                digit("\(low % 10)")
            }
            .padding(.vertical, 12)
            .accessibilityElement()
            .accessibilityLabel("Nixie display preview")
            .accessibilityValue(String(format: "%d:%02d", h, low))
        }
    }

    private func lampOn(index: Int, odd: Bool) -> Bool {
        switch neonMode {
        case 0: index == 0 ? odd : !odd
        case 1: true
        case 2: odd
        default: false
        }
    }

    private func digit(_ text: String?, dot: Bool = false) -> some View {
        ZStack(alignment: .bottomTrailing) {
            RoundedRectangle(cornerRadius: 22)
                .fill(.white.opacity(0.06))
                .overlay(RoundedRectangle(cornerRadius: 22).stroke(.white.opacity(0.15)))
            Text(text ?? "8")
                .font(.system(size: 54, weight: .light, design: .serif))
                .foregroundStyle(text == nil ? .white.opacity(0.05) : glow)
                .shadow(color: text == nil ? .clear : glow, radius: 8)
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            Circle()
                .fill(dot ? glow : .white.opacity(0.08))
                .shadow(color: dot ? glow : .clear, radius: 4)
                .frame(width: 7, height: 7)
                .padding(8)
        }
        .frame(width: 54, height: 92)
    }

    private func lamp(on: Bool) -> some View {
        Capsule()
            .fill(on ? glow.opacity(0.9) : .white.opacity(0.08))
            .shadow(color: on ? glow : .clear, radius: 5)
            .frame(width: 9, height: 14)
            .animation(.easeOut(duration: 0.4), value: on)
    }
}
