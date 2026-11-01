import SwiftUI

/// A 0–255 style slider that sends commands while dragging (rate-limited) and
/// always sends the final value on release. Programmatic changes never send.
struct CommandSlider: View {
    let title: String
    let systemImage: String
    var range: ClosedRange<Double> = 0...255
    var step: Double = 1
    let interval: TimeInterval
    /// Value last reported by the device; adopted while the user isn't dragging.
    var reported: Int? = nil
    @Binding var value: Double
    let send: (Int) -> Void

    @State private var isEditing = false
    @State private var lastSend = Date.distantPast
    @State private var lastSentValue: Int?

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Label(title, systemImage: systemImage)
                Spacer()
                Text("\(Int(value.rounded()))")
                    .monospacedDigit()
                    .foregroundStyle(.secondary)
            }
            Slider(value: $value, in: range, step: step) { editing in
                isEditing = editing
                if !editing { commit(force: true) }
            }
            .accessibilityLabel(title)
        }
        .onChange(of: value) {
            if isEditing { commit(force: false) }
        }
        .onChange(of: reported) { _, new in
            // Brief grace period so a stale status report doesn't snap the slider back.
            guard let new, !isEditing, Date().timeIntervalSince(lastSend) > 1 else { return }
            lastSentValue = new
            value = Double(new)
        }
    }

    private func commit(force: Bool) {
        let v = Int(value.rounded())
        guard v != lastSentValue else { return }
        guard force || Date().timeIntervalSince(lastSend) >= interval else { return }
        lastSend = Date()
        lastSentValue = v
        send(v)
    }
}

struct StatusDot: View {
    let state: LinkState

    var body: some View {
        Circle()
            .fill(color)
            .frame(width: 10, height: 10)
            .accessibilityHidden(true)
    }

    private var color: Color {
        switch state {
        case .ready: .green
        case .connecting, .discovering: .yellow
        case .failed, .notFound: .red
        case .disconnected: .gray
        }
    }
}

struct DeviceIcon: View {
    let kind: DeviceKind
    var size: CGFloat = 36

    var body: some View {
        Image(systemName: kind.symbol)
            .font(.system(size: size * 0.5, weight: .semibold))
            .foregroundStyle(.white)
            .frame(width: size, height: size)
            .background(kind.tint.gradient, in: RoundedRectangle(cornerRadius: size * 0.25))
            .accessibilityHidden(true)
    }
}

/// First section of every device screen: link status and connect/disconnect.
struct ConnectionSection: View {
    @Environment(BLEManager.self) private var ble
    let connection: DeviceConnection

    var body: some View {
        Section {
            HStack(spacing: 12) {
                DeviceIcon(kind: connection.kind)
                VStack(alignment: .leading, spacing: 2) {
                    HStack(spacing: 6) {
                        StatusDot(state: connection.linkState)
                        Text(connection.linkState.label).font(.headline)
                    }
                    Text(detail)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .lineLimit(2)
                }
                Spacer()
                if connection.wantsConnection {
                    Button("Disconnect") { ble.disconnect(connection) }
                        .buttonStyle(.bordered)
                } else {
                    Button("Connect") { ble.connect(connection) }
                        .buttonStyle(.borderedProminent)
                }
            }
        }
    }

    private var detail: String {
        switch connection.linkState {
        case .failed(let reason): return reason
        case .notFound: return "iOS no longer knows this device. Forget it and pair again."
        case .connecting: return "Waiting for the device to come into range."
        default:
            return connection.kind.displayName + (connection.device.isDemo ? " · Demo" : "")
        }
    }
}

/// A row of small step buttons, e.g. "−1  +1" for nudging the time.
struct StepButtons: View {
    let title: String
    let steps: [Int]
    let action: (Int) -> Void

    var body: some View {
        HStack {
            Text(title)
            Spacer()
            ForEach(steps, id: \.self) { s in
                Button(s > 0 ? "+\(s)" : "−\(-s)") { action(s) }
                    .buttonStyle(.bordered)
                    .monospacedDigit()
                    .accessibilityLabel("\(title) \(s > 0 ? "plus" : "minus") \(abs(s))")
            }
        }
    }
}

extension View {
    /// Inline navigation title on iOS; a no-op on macOS, which has no such modifier.
    func inlineNavigationTitle() -> some View {
        #if os(iOS)
        navigationBarTitleDisplayMode(.inline)
        #else
        self
        #endif
    }

    /// Disables auto-capitalization on iOS; a no-op on macOS.
    func noAutocapitalization() -> some View {
        #if os(iOS)
        textInputAutocapitalization(.never)
        #else
        self
        #endif
    }
}
