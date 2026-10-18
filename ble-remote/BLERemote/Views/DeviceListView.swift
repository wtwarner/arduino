import SwiftUI

/// Root screen: paired devices on the left/top, the selected device's controls in the detail.
struct DeviceListView: View {
    @Environment(DeviceStore.self) private var store
    @Environment(BLEManager.self) private var ble
    @State private var selection: UUID?
    @State private var showScanner = false
    @State private var editing: SavedDevice?

    var body: some View {
        NavigationSplitView {
            List(selection: $selection) {
                if let problem = ble.bluetoothProblem {
                    Section {
                        Label(problem, systemImage: "exclamationmark.triangle")
                            .foregroundStyle(.orange)
                            .font(.callout)
                    }
                }
                if !store.devices.isEmpty {
                    Section("My Devices") {
                        ForEach(store.devices) { device in
                            NavigationLink(value: device.id) {
                                DeviceRow(device: device, connection: ble.connections[device.id])
                            }
                            .swipeActions {
                                Button("Forget", role: .destructive) { forget(device) }
                                Button("Edit") { editing = device }.tint(.indigo)
                            }
                            .contextMenu { menu(for: device) }
                        }
                    }
                }
            }
            .overlay {
                if store.devices.isEmpty {
                    ContentUnavailableView {
                        Label("No Devices", systemImage: "antenna.radiowaves.left.and.right")
                    } description: {
                        Text("Scan for nearby Bluetooth LE devices and pair an IV25 Clock, Nixie or River Table.")
                    } actions: {
                        Button("Scan for Devices") { showScanner = true }
                            .buttonStyle(.borderedProminent)
                    }
                }
            }
            .navigationTitle("BLE Remote")
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button { showScanner = true } label: {
                        Label("Add Device", systemImage: "plus")
                    }
                }
            }
        } detail: {
            if let id = selection, let connection = ble.connections[id] {
                DeviceDetailView(connection: connection, selection: $selection)
            } else {
                ContentUnavailableView("Select a Device", systemImage: "dot.radiowaves.left.and.right",
                                       description: Text("Choose a paired device to control it."))
            }
        }
        .sheet(isPresented: $showScanner) {
            ScannerView { id in selection = id }
        }
        .sheet(item: $editing) { device in
            EditDeviceView(device: device)
        }
    }

    @ViewBuilder
    private func menu(for device: SavedDevice) -> some View {
        if let c = ble.connections[device.id] {
            if c.wantsConnection {
                Button("Disconnect", systemImage: "bolt.horizontal.circle") { ble.disconnect(c) }
            } else {
                Button("Connect", systemImage: "bolt.horizontal.circle.fill") { ble.connect(c) }
            }
        }
        Button("Edit", systemImage: "pencil") { editing = device }
        Button("Forget", systemImage: "trash", role: .destructive) { forget(device) }
    }

    private func forget(_ device: SavedDevice) {
        if selection == device.id { selection = nil }
        ble.forget(device.id)
    }
}

private struct DeviceRow: View {
    let device: SavedDevice
    let connection: DeviceConnection?

    var body: some View {
        HStack(spacing: 12) {
            DeviceIcon(kind: device.kind)
            VStack(alignment: .leading, spacing: 2) {
                Text(device.name).font(.headline)
                Text(device.kind.displayName + (device.isDemo ? " · Demo" : ""))
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            if let connection {
                HStack(spacing: 6) {
                    Text(connection.linkState.label)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    StatusDot(state: connection.linkState)
                }
            }
        }
        .padding(.vertical, 2)
    }
}

struct EditDeviceView: View {
    @Environment(BLEManager.self) private var ble
    @Environment(\.dismiss) private var dismiss
    let device: SavedDevice
    @State private var name: String
    @State private var kind: DeviceKind

    init(device: SavedDevice) {
        self.device = device
        _name = State(initialValue: device.name)
        _kind = State(initialValue: device.kind)
    }

    var body: some View {
        NavigationStack {
            Form {
                Section("Name") {
                    TextField("Name", text: $name)
                }
                DeviceKindPicker(kind: $kind)
            }
            .navigationTitle("Edit Device")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) { Button("Cancel") { dismiss() } }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Save") {
                        var d = device
                        d.name = name.trimmingCharacters(in: .whitespaces)
                        d.kind = kind
                        ble.update(d)
                        dismiss()
                    }
                    .disabled(name.trimmingCharacters(in: .whitespaces).isEmpty)
                }
            }
        }
    }
}

struct DeviceKindPicker: View {
    @Binding var kind: DeviceKind
    var footer: String? = nil

    var body: some View {
        Section {
            Picker("Device Type", selection: $kind) {
                ForEach(DeviceKind.allCases) { k in
                    HStack(spacing: 12) {
                        DeviceIcon(kind: k, size: 28)
                        VStack(alignment: .leading) {
                            Text(k.displayName)
                            Text(k.protocolSummary).font(.caption).foregroundStyle(.secondary)
                        }
                    }
                    .tag(k)
                }
            }
            .pickerStyle(.inline)
            .labelsHidden()
        } header: {
            Text("Device Type")
        } footer: {
            if let footer { Text(footer) }
        }
    }
}
