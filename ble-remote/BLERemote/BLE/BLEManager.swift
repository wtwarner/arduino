import CoreBluetooth
import Observation

struct DiscoveredPeripheral: Identifiable {
    let id: UUID
    var name: String?
    var rssi: Int
    var services: [CBUUID]
    var lastSeen: Date

    var guessedKind: DeviceKind? { DeviceKind.guess(name: name, services: services) }
    var isLikelyMatch: Bool { guessedKind != nil || DeviceKind.advertisesKnownService(services) }
}

/// Owns the CBCentralManager: scanning, and one DeviceConnection per paired device.
@Observable
final class BLEManager: NSObject {
    private(set) var state: CBManagerState = .unknown
    private(set) var isScanning = false
    private(set) var discovered: [UUID: DiscoveredPeripheral] = [:]
    private(set) var connections: [UUID: DeviceConnection] = [:]

    @ObservationIgnored let store: DeviceStore
    @ObservationIgnored private var central: CBCentralManager!
    @ObservationIgnored private var peripherals: [UUID: CBPeripheral] = [:]

    init(store: DeviceStore) {
        self.store = store
        super.init()
        central = CBCentralManager(delegate: self, queue: .main)
        for device in store.devices {
            let c = DeviceConnection(device: device)
            connections[device.id] = c
            connect(c) // real devices connect once Bluetooth reports poweredOn
        }
    }

    var bluetoothProblem: String? {
        switch state {
        case .poweredOff: "Bluetooth is turned off."
        case .unauthorized: "Bluetooth access is denied. Allow it in Settings › Privacy & Security › Bluetooth."
        case .unsupported: "Bluetooth LE isn't available here. Demo devices still work."
        default: nil
        }
    }

    // MARK: Paired devices

    func pair(_ device: SavedDevice) {
        store.upsert(device)
        let c = connections[device.id] ?? DeviceConnection(device: device)
        c.device = device
        connections[device.id] = c
        connect(c)
    }

    /// Saves a rename or type change; a type change needs fresh service discovery.
    func update(_ device: SavedDevice) {
        guard let c = connections[device.id] else { return }
        let kindChanged = c.device.kind != device.kind
        store.upsert(device)
        if kindChanged {
            disconnect(c)
            c.device = device
            connect(c)
        } else {
            c.device = device
        }
    }

    func forget(_ id: UUID) {
        if let c = connections[id] { disconnect(c) }
        connections[id] = nil
        store.remove(id)
    }

    func connect(_ c: DeviceConnection) {
        c.wantsConnection = true
        if c.device.isDemo {
            c.startDemo()
            return
        }
        guard state == .poweredOn else { return }
        guard let p = peripheral(for: c.device.id) else {
            c.linkState = .notFound
            return
        }
        c.attach(p)
        c.linkState = .connecting
        // A pending connect never times out; iOS completes it when the device comes into range.
        central.connect(p)
    }

    func disconnect(_ c: DeviceConnection) {
        c.wantsConnection = false
        if c.device.isDemo {
            c.stopDemo()
            return
        }
        if let p = c.peripheral { central.cancelPeripheralConnection(p) }
        c.linkState = .disconnected
    }

    private func peripheral(for id: UUID) -> CBPeripheral? {
        if let p = peripherals[id] { return p }
        if let p = central.retrievePeripherals(withIdentifiers: [id]).first {
            peripherals[id] = p
            return p
        }
        return nil
    }

    // MARK: Scanning

    func startScan() {
        guard state == .poweredOn, !isScanning else { return }
        discovered.removeAll()
        central.scanForPeripherals(withServices: nil,
                                   options: [CBCentralManagerScanOptionAllowDuplicatesKey: true])
        isScanning = true
    }

    func stopScan() {
        guard isScanning else { return }
        central.stopScan()
        isScanning = false
    }
}

extension BLEManager: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        state = central.state
        if central.state == .poweredOn {
            for c in connections.values where c.wantsConnection && !c.device.isDemo {
                connect(c)
            }
        } else {
            isScanning = false
            for c in connections.values where !c.device.isDemo {
                c.didDisconnect(error: nil)
            }
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        let rssi = RSSI.intValue
        guard rssi != 127 else { return } // 127 = unavailable
        let id = peripheral.identifier
        peripherals[id] = peripheral
        let name = advertisementData[CBAdvertisementDataLocalNameKey] as? String ?? peripheral.name
        let services = advertisementData[CBAdvertisementDataServiceUUIDsKey] as? [CBUUID] ?? []
        let now = Date()

        if var d = discovered[id] {
            // Duplicates arrive many times a second; only publish meaningful changes.
            let renamed = d.name == nil && name != nil
            guard renamed || abs(d.rssi - rssi) >= 4 || now.timeIntervalSince(d.lastSeen) > 2 else { return }
            d.rssi = rssi
            d.lastSeen = now
            if let name { d.name = name }
            if !services.isEmpty { d.services = services }
            discovered[id] = d
        } else {
            discovered[id] = DiscoveredPeripheral(id: id, name: name, rssi: rssi, services: services, lastSeen: now)
        }
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        connections[peripheral.identifier]?.didConnect()
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        guard let c = connections[peripheral.identifier] else { return }
        c.didDisconnect(error: error)
        if c.wantsConnection {
            DispatchQueue.main.asyncAfter(deadline: .now() + 2) { [weak self] in
                guard let self, c.wantsConnection else { return }
                self.central.connect(peripheral)
            }
        }
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        guard let c = connections[peripheral.identifier] else { return }
        c.didDisconnect(error: error)
        if c.wantsConnection {
            central.connect(peripheral) // reconnect automatically when back in range
        }
    }
}
