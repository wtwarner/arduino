import SwiftUI

@main
struct BLERemoteApp: App {
    @State private var store: DeviceStore
    @State private var ble: BLEManager

    init() {
        let store = DeviceStore()
        _store = State(initialValue: store)
        _ble = State(initialValue: BLEManager(store: store))
    }

    var body: some Scene {
        WindowGroup {
            DeviceListView()
                .environment(store)
                .environment(ble)
        }
    }
}
