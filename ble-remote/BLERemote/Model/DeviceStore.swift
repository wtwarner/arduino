import Foundation
import Observation

/// A device the user has paired with the app.
struct SavedDevice: Codable, Identifiable, Hashable {
    /// CBPeripheral.identifier, or a random id for demo devices.
    var id: UUID
    var name: String
    var kind: DeviceKind
    var isDemo = false
}

/// Persists the paired-device list in UserDefaults.
@Observable
final class DeviceStore {
    private(set) var devices: [SavedDevice] = []

    @ObservationIgnored private let key = "savedDevices"

    init() {
        if let data = UserDefaults.standard.data(forKey: key),
           let saved = try? JSONDecoder().decode([SavedDevice].self, from: data) {
            devices = saved
        }
    }

    func contains(_ id: UUID) -> Bool { devices.contains { $0.id == id } }

    func upsert(_ device: SavedDevice) {
        if let i = devices.firstIndex(where: { $0.id == device.id }) {
            devices[i] = device
        } else {
            devices.append(device)
        }
        save()
    }

    func remove(_ id: UUID) {
        devices.removeAll { $0.id == id }
        save()
    }

    private func save() {
        if let data = try? JSONEncoder().encode(devices) {
            UserDefaults.standard.set(data, forKey: key)
        }
    }
}
