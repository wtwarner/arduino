import SwiftUI

/// Raw traffic log plus a field for sending any command (framing is added automatically).
struct ConsoleView: View {
    let connection: DeviceConnection
    @Environment(\.dismiss) private var dismiss
    @State private var text = ""
    @FocusState private var focused: Bool

    var body: some View {
        NavigationStack {
            ScrollViewReader { proxy in
                List(connection.log) { entry in
                    HStack(alignment: .firstTextBaseline, spacing: 8) {
                        Text(entry.date, format: .dateTime.hour().minute().second())
                            .font(.caption2.monospacedDigit())
                            .foregroundStyle(.secondary)
                        Image(systemName: icon(entry.direction))
                            .font(.caption)
                            .foregroundStyle(color(entry.direction))
                        Text(entry.text)
                            .font(.system(.callout, design: .monospaced))
                            .foregroundStyle(entry.direction == .error ? .red : .primary)
                            .textSelection(.enabled)
                    }
                    .id(entry.id)
                    .listRowSeparator(.hidden)
                }
                .listStyle(.plain)
                .overlay {
                    if connection.log.isEmpty {
                        ContentUnavailableView("No Traffic", systemImage: "terminal",
                                               description: Text("Commands sent and data received appear here."))
                    }
                }
                .onChange(of: connection.log.last?.id) {
                    if let id = connection.log.last?.id {
                        withAnimation { proxy.scrollTo(id, anchor: .bottom) }
                    }
                }
            }
            .safeAreaInset(edge: .bottom) {
                HStack {
                    Menu {
                        ForEach(connection.kind.commandReference, id: \.syntax) { cmd in
                            Button {
                                text = template(cmd.syntax)
                                focused = true
                            } label: {
                                Text(cmd.syntax)
                                Text(cmd.help)
                            }
                        }
                    } label: {
                        Image(systemName: "list.bullet.rectangle")
                            .font(.title2)
                    }
                    .accessibilityLabel("Command reference")

                    TextField(placeholder, text: $text)
                        .textFieldStyle(.roundedBorder)
                        .font(.system(.body, design: .monospaced))
                        .autocorrectionDisabled()
                        .textInputAutocapitalization(.never)
                        .focused($focused)
                        .submitLabel(.send)
                        .onSubmit(send)
                    Button(action: send) {
                        Image(systemName: "arrow.up.circle.fill").font(.title)
                    }
                    .disabled(text.trimmingCharacters(in: .whitespaces).isEmpty || !connection.isReady)
                    .accessibilityLabel("Send")
                }
                .padding()
                .background(.bar)
            }
            .navigationTitle("\(connection.device.name) Console")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Clear") { connection.clearLog() }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Done") { dismiss() }
                }
            }
        }
    }

    private var placeholder: String {
        switch connection.kind {
        case .iv25Clock: "b 200   ([ ] added)"
        case .nixie: "B:200"
        case .riverTable: "ledmode 1   ([ ] added)"
        }
    }

    /// "b <0-255>" → "b ", "ZS:1" → "ZS:1"
    private func template(_ syntax: String) -> String {
        if let i = syntax.firstIndex(of: "<") { return String(syntax[..<i]) }
        if let i = syntax.firstIndex(of: ":"), syntax.contains("YYYY") { return String(syntax[...i]) }
        if syntax.hasPrefix("st ") { return "st " }
        return syntax
    }

    private func send() {
        let cmd = text.trimmingCharacters(in: .whitespaces)
        guard !cmd.isEmpty else { return }
        connection.send(cmd)
        text = ""
    }

    private func icon(_ d: LogEntry.Direction) -> String {
        switch d {
        case .tx: "arrow.up"
        case .rx: "arrow.down"
        case .info: "info.circle"
        case .error: "exclamationmark.triangle"
        }
    }

    private func color(_ d: LogEntry.Direction) -> Color {
        switch d {
        case .tx: .blue
        case .rx: .green
        case .info: .secondary
        case .error: .red
        }
    }
}
