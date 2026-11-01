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
        // Grouped (inset, rounded) sections on every platform; macOS defaults to a flat
        // columns style that ignores row backgrounds and has no side margins.
        .formStyle(.grouped)
        #if os(macOS)
        .frame(maxWidth: 640)
        .frame(maxWidth: .infinity)
        #endif
        // Fresh control state when switching devices or changing a device's type.
        .id("\(connection.device.id)-\(connection.kind.rawValue)")
        .navigationTitle(connection.device.name)
        .inlineNavigationTitle()
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
