import SwiftUI

struct PairCandidate: Identifiable, Hashable {
    let id: UUID
    let advertisedName: String?
    let guessedKind: DeviceKind?
    let isDemo: Bool
}

/// Scans for nearby BLE peripherals and pairs one as a given device type.
struct ScannerView: View {
    @Environment(BLEManager.self) private var ble
    @Environment(DeviceStore.self) private var store
    @Environment(\.dismiss) private var dismiss
    var onPaired: (UUID) -> Void

    @State private var showAll = false
    #if targetEnvironment(simulator)
    @State private var showDemos = true
    #else
    @State private var showDemos = false
    #endif
    @State private var pairing: PairCandidate?

    private var visible: [DiscoveredPeripheral] {
        ble.discovered.values
            .filter { showAll || $0.isLikelyMatch }
            .sorted {
                if ($0.guessedKind != nil) != ($1.guessedKind != nil) { return $0.guessedKind != nil }
                return ($0.name ?? "~") < ($1.name ?? "~")
            }
    }

    var body: some View {
        NavigationStack {
            List {
                if let problem = ble.bluetoothProblem {
                    Section {
                        Label(problem, systemImage: "exclamationmark.triangle").foregroundStyle(.orange)
                    }
                }
                Section {
                    if visible.isEmpty {
                        Text(ble.isScanning ? "Searching…" : "No devices found.")
                            .foregroundStyle(.secondary)
                    }
                    ForEach(visible) { p in
                        let paired = store.contains(p.id)
                        Button {
                            pairing = PairCandidate(id: p.id, advertisedName: p.name,
                                                    guessedKind: p.guessedKind, isDemo: false)
                        } label: {
                            ScanRow(peripheral: p, paired: paired)
                        }
                        .disabled(paired)
                    }
                } header: {
                    HStack {
                        Text("Nearby")
                        Spacer()
                        if ble.isScanning { ProgressView() }
                    }
                } footer: {
                    Text(showAll
                         ? "Showing every advertising BLE device."
                         : "Showing devices with a recognized name or that advertise the HM-10/HC-08 serial service (FFE0) or the Nixie service.")
                }

                if showDemos {
                    Section {
                        ForEach(DeviceKind.allCases) { k in
                            Button {
                                pairing = PairCandidate(id: UUID(), advertisedName: "Demo \(k.displayName)",
                                                        guessedKind: k, isDemo: true)
                            } label: {
                                HStack(spacing: 12) {
                                    DeviceIcon(kind: k, size: 30)
                                    Text("Demo \(k.displayName)").foregroundStyle(.primary)
                                }
                            }
                        }
                    } header: {
                        Text("Demo Devices")
                    } footer: {
                        Text("Simulated devices for trying the controls without hardware.")
                    }
                }
            }
            .navigationTitle("Add Device")
            .inlineNavigationTitle()
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") { dismiss() }
                }
                ToolbarItem(placement: .primaryAction) {
                    Menu {
                        Toggle("Show All Devices", isOn: $showAll)
                        Toggle("Show Demo Devices", isOn: $showDemos)
                        Button(ble.isScanning ? "Stop Scanning" : "Scan Again") {
                            if ble.isScanning { ble.stopScan() } else { ble.startScan() }
                        }
                    } label: {
                        Label("Options", systemImage: "line.3.horizontal.decrease.circle")
                    }
                }
            }
            .navigationDestination(item: $pairing) { candidate in
                PairDeviceView(candidate: candidate) { device in
                    ble.pair(device)
                    onPaired(device.id)
                    dismiss()
                }
            }
            .onAppear { ble.startScan() }
            .onChange(of: ble.state) { ble.startScan() }
            .onDisappear { ble.stopScan() }
        }
    }
}

private struct ScanRow: View {
    let peripheral: DiscoveredPeripheral
    let paired: Bool

    var body: some View {
        HStack(spacing: 12) {
            if let kind = peripheral.guessedKind {
                DeviceIcon(kind: kind, size: 30)
            } else {
                Image(systemName: "questionmark.square.dashed")
                    .font(.title2)
                    .frame(width: 30, height: 30)
                    .foregroundStyle(.secondary)
            }
            VStack(alignment: .leading, spacing: 2) {
                Text(peripheral.name ?? "Unnamed device").foregroundStyle(.primary)
                Text(subtitle).font(.caption).foregroundStyle(.secondary)
            }
            Spacer()
            if paired {
                Image(systemName: "checkmark.circle.fill").foregroundStyle(.green)
                    .accessibilityLabel("Already paired")
            }
            Image(systemName: "cellularbars", variableValue: signal)
                .foregroundStyle(.secondary)
                .accessibilityLabel("Signal \(peripheral.rssi) dBm")
        }
    }

    private var subtitle: String {
        if let kind = peripheral.guessedKind { return kind.displayName }
        if peripheral.services.contains(DeviceKind.uartService) { return "Serial module (FFE0): choose type" }
        return peripheral.id.uuidString.prefix(8) + "…"
    }

    private var signal: Double {
        // -100 dBm → 0, -50 dBm → 1
        min(1, max(0, Double(peripheral.rssi + 100) / 50))
    }
}

struct PairDeviceView: View {
    let candidate: PairCandidate
    let onPair: (SavedDevice) -> Void
    @State private var name: String
    @State private var kind: DeviceKind

    init(candidate: PairCandidate, onPair: @escaping (SavedDevice) -> Void) {
        self.candidate = candidate
        self.onPair = onPair
        let kind = candidate.guessedKind ?? .riverTable
        _kind = State(initialValue: kind)
        _name = State(initialValue: candidate.advertisedName ?? kind.displayName)
    }

    private var trimmed: String { name.trimmingCharacters(in: .whitespaces) }

    var body: some View {
        Form {
            Section("Name") {
                TextField("Name", text: $name)
            }
            DeviceKindPicker(kind: $kind, footer: candidate.guessedKind == nil
                ? "IV25 Clock and River Table use the same HM-10/HC-08 serial module, so pick which one this is."
                : "Detected from the advertised name or service.")
            if !candidate.isDemo {
                Section {
                    LabeledContent("Identifier", value: String(candidate.id.uuidString.prefix(8)) + "…")
                } footer: {
                    Text("These modules don't use a passcode; the app remembers the device and reconnects whenever it's in range.")
                }
            }
        }
        .navigationTitle("Pair Device")
        .inlineNavigationTitle()
        .toolbar {
            ToolbarItem(placement: .confirmationAction) {
                Button("Pair") {
                    onPair(SavedDevice(id: candidate.id, name: trimmed, kind: kind, isDemo: candidate.isDemo))
                }
                .disabled(trimmed.isEmpty)
            }
        }
    }
}
