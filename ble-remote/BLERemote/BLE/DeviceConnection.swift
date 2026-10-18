import CoreBluetooth
import Observation

struct LogEntry: Identifiable {
    enum Direction { case tx, rx, info, error }
    let id = UUID()
    let date = Date()
    let direction: Direction
    let text: String
}

enum LinkState: Equatable {
    case disconnected
    case notFound
    case connecting
    case discovering
    case failed(String)
    case ready

    var label: String {
        switch self {
        case .disconnected: "Disconnected"
        case .notFound: "Not found"
        case .connecting: "Connecting…"
        case .discovering: "Discovering…"
        case .failed: "Incompatible"
        case .ready: "Connected"
        }
    }
}

/// The link to one paired device: service discovery, framed writes, and received data.
@Observable
final class DeviceConnection: NSObject {
    var device: SavedDevice
    var linkState: LinkState = .disconnected
    var wantsConnection = false
    private(set) var log: [LogEntry] = []
    /// Latest `key:value;` fields streamed by the device (River Table).
    private(set) var status: [String: String] = [:]
    private(set) var lastStatusDate: Date?

    @ObservationIgnored private(set) var peripheral: CBPeripheral?
    @ObservationIgnored private var characteristic: CBCharacteristic?
    @ObservationIgnored private var rxBuffer = ""
    @ObservationIgnored private var writeQueue: [Data] = []
    @ObservationIgnored private var demo: DemoDevice?

    init(device: SavedDevice) {
        self.device = device
        super.init()
    }

    var kind: DeviceKind { device.kind }
    var isReady: Bool { linkState == .ready }

    // MARK: Sending

    func send(_ command: String) {
        guard isReady else {
            append(.error, "Not connected; dropped \(command)")
            return
        }
        let framed = kind.frame(command)
        append(.tx, framed)

        if let demo {
            demo.receive(command)
            return
        }
        guard let peripheral, let characteristic, let data = framed.data(using: .utf8) else { return }
        let type: CBCharacteristicWriteType =
            characteristic.properties.contains(.writeWithoutResponse) ? .withoutResponse : .withResponse
        let chunk = max(20, peripheral.maximumWriteValueLength(for: type))
        var offset = 0
        while offset < data.count {
            writeQueue.append(data.subdata(in: offset..<min(offset + chunk, data.count)))
            offset += chunk
        }
        flushWrites()
    }

    private func flushWrites() {
        guard let peripheral, let characteristic else {
            writeQueue.removeAll()
            return
        }
        let withoutResponse = characteristic.properties.contains(.writeWithoutResponse)
        while !writeQueue.isEmpty {
            // Resumes from peripheralIsReady(toSendWriteWithoutResponse:).
            if withoutResponse && !peripheral.canSendWriteWithoutResponse { return }
            peripheral.writeValue(writeQueue.removeFirst(), for: characteristic,
                                  type: withoutResponse ? .withoutResponse : .withResponse)
        }
    }

    // MARK: Receiving

    func receive(_ text: String) {
        guard kind == .riverTable else {
            append(.rx, text)
            return
        }
        // shift_bt.ino streams "led:1;ledpwm:128;..." four times a second; the UART
        // bridge may split or merge those writes, so reassemble on ';'.
        rxBuffer += text
        while let end = rxBuffer.firstIndex(of: ";") {
            let message = String(rxBuffer[..<end])
            rxBuffer.removeSubrange(...end)
            if let colon = message.firstIndex(of: ":") {
                let key = message[..<colon].trimmingCharacters(in: .whitespacesAndNewlines)
                let value = message[message.index(after: colon)...].trimmingCharacters(in: .whitespacesAndNewlines)
                if status[key] != value { status[key] = value }
            } else if !message.isEmpty {
                append(.rx, message)
            }
        }
        if rxBuffer.count > 256 {
            append(.rx, rxBuffer)
            rxBuffer = ""
        }
        if lastStatusDate.map({ Date().timeIntervalSince($0) > 0.5 }) ?? true {
            lastStatusDate = Date()
        }
    }

    // MARK: Lifecycle (driven by BLEManager)

    func attach(_ p: CBPeripheral) {
        peripheral = p
        p.delegate = self
    }

    func didConnect() {
        linkState = .discovering
        append(.info, "Connected; discovering \(kind.displayName) service")
        peripheral?.discoverServices([kind.serviceUUID])
    }

    func didDisconnect(error: Error?) {
        characteristic = nil
        writeQueue.removeAll()
        rxBuffer = ""
        if linkState == .ready || linkState == .discovering {
            if let error {
                append(.error, "Disconnected: \(error.localizedDescription)")
            } else {
                append(.info, "Disconnected")
            }
        }
        linkState = wantsConnection ? .connecting : .disconnected
    }

    func startDemo() {
        guard demo == nil else { return }
        demo = DemoDevice(kind: kind) { [weak self] text in self?.receive(text) }
        linkState = .ready
        append(.info, "Demo \(kind.displayName) connected")
    }

    func stopDemo() {
        demo?.stop()
        demo = nil
        status = [:]
        linkState = .disconnected
        append(.info, "Demo disconnected")
    }

    // MARK: Log

    func clearLog() { log.removeAll() }

    private func append(_ direction: LogEntry.Direction, _ text: String) {
        log.append(LogEntry(direction: direction, text: text))
        if log.count > 500 { log.removeFirst(log.count - 500) }
    }

    private func fail(_ reason: String) {
        append(.error, reason)
        linkState = .failed(reason)
    }

    // MARK: Remembered control values
    // The clocks don't report state back, so the app remembers what it last sent.

    func remembered(_ key: String, default value: Double) -> Double {
        UserDefaults.standard.object(forKey: prefKey(key)) as? Double ?? value
    }

    func remember(_ key: String, _ value: Double) {
        UserDefaults.standard.set(value, forKey: prefKey(key))
    }

    private func prefKey(_ key: String) -> String { "device.\(device.id.uuidString).\(key)" }
}

extension DeviceConnection: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: { $0.uuid == kind.serviceUUID }) else {
            fail("Service \(kind.serviceUUID.uuidString) not found. Is this really a \(kind.displayName)?")
            return
        }
        peripheral.discoverCharacteristics([kind.characteristicUUID], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let c = service.characteristics?.first(where: { $0.uuid == kind.characteristicUUID }) else {
            fail("Characteristic \(kind.characteristicUUID.uuidString) not found.")
            return
        }
        characteristic = c
        if c.properties.contains(.notify) || c.properties.contains(.indicate) {
            peripheral.setNotifyValue(true, for: c)
        }
        linkState = .ready
        append(.info, "Ready")
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard let data = characteristic.value, !data.isEmpty else { return }
        receive(String(decoding: data, as: UTF8.self))
    }

    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        if let error { append(.error, "Write failed: \(error.localizedDescription)") }
    }

    func peripheralIsReady(toSendWriteWithoutResponse peripheral: CBPeripheral) {
        flushWrites()
    }
}
