import SwiftUI

/// Hosts the device-specific control screen, with a title menu to switch devices.
struct DeviceDetailView: View {
    @Environment(DeviceStore.self) private var store
    let connection: DeviceConnection
    @Binding var selection: UUID?
    @State private var showConsole = false

    var body: some View {
        Group {
            switch connection.kind {
            case .iv25Clock: IV25ClockView(connection: connection)
            case .nixie: NixieView(connection: connection)
            case .riverTable: RiverTableView(connection: connection)
            }
        }
        // Fresh control state when switching devices or changing a device's type.
        .id("\(connection.device.id)-\(connection.kind.rawValue)")
        .navigationTitle(connection.device.name)
        .navigationBarTitleDisplayMode(.inline)
        .toolbarTitleMenu {
            ForEach(store.devices) { d in
                Button { selection = d.id } label: {
                    Label(d.name, systemImage: d.id == connection.device.id ? "checkmark" : d.kind.symbol)
                }
            }
        }
        .toolbar {
            ToolbarItem(placement: .primaryAction) {
                Button { showConsole = true } label: {
                    Label("Console", systemImage: "terminal")
                }
            }
        }
        .sheet(isPresented: $showConsole) {
            ConsoleView(connection: connection)
        }
    }
}
