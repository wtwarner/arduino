import CoreBluetooth
import SwiftUI

/// The supported device types and the BLE transport + command framing each one uses.
enum DeviceKind: String, Codable, CaseIterable, Identifiable {
    case iv25Clock
    case nixie
    case riverTable

    var id: String { rawValue }

    var displayName: String {
        switch self {
        case .iv25Clock: "IV25 Clock"
        case .nixie: "Nixie"
        case .riverTable: "River Table"
        }
    }

    var symbol: String {
        switch self {
        case .iv25Clock: "clock.circle"
        case .nixie: "flame"
        case .riverTable: "water.waves"
        }
    }

    var tint: Color {
        switch self {
        case .iv25Clock: .teal
        case .nixie: .orange
        case .riverTable: .blue
        }
    }

    // HM-10 / HC-08 transparent UART bridge (iv25-clock/bt.ino, shift_bt/bt.ino).
    static let uartService = CBUUID(string: "FFE0")
    static let uartCharacteristic = CBUUID(string: "FFE1")
    // ArduinoBLE peripheral (nixie/ble.ino).
    static let nixieService = CBUUID(string: "903B13CE-EADB-482F-B8FA-A5A2ECFFFFB4")
    static let nixieCharacteristic = CBUUID(string: "2BDE")

    var serviceUUID: CBUUID { self == .nixie ? Self.nixieService : Self.uartService }
    var characteristicUUID: CBUUID { self == .nixie ? Self.nixieCharacteristic : Self.uartCharacteristic }

    var protocolSummary: String {
        switch self {
        case .iv25Clock: "HM-10/HC-08 UART · [cmd args]"
        case .nixie: "ArduinoBLE · CMD:value"
        case .riverTable: "HM-10 UART · [cmd arg]"
        }
    }

    /// Wraps a command in the framing the firmware's parser expects.
    func frame(_ command: String) -> String {
        switch self {
        case .iv25Clock, .riverTable: "[\(command)]"  // btCmdParser collects text between [ and ]
        case .nixie: command                          // one characteristic write == one command
        }
    }

    /// Minimum spacing between commands while a slider is being dragged.
    var sliderInterval: TimeInterval {
        switch self {
        case .iv25Clock: 0.1   // 9600 baud UART, parser consumes one char per loop()
        case .nixie: 0.05
        case .riverTable: 0.25 // every command rewrites EEPROM
        }
    }

    /// Commands understood by the firmware, for the console's quick-insert menu.
    var commandReference: [(syntax: String, help: String)] {
        switch self {
        case .iv25Clock: [
            ("b <0-255>", "VFD tube brightness"),
            ("nb <0-255>", "Accent LED brightness"),
            ("st YYYY-MM-DDTHH:MM:SS", "Set time (US Pacific local)"),
            ("tm <0-3>", "Test mode (0 = clock)"),
            ("gt", "Print time to USB serial"),
        ]
        case .nixie: [
            ("B:<0-255>", "Nixie brightness"),
            ("NB:<0-255>", "Neon lamp brightness"),
            ("NM:<0-3>", "Neon mode"),
            ("24:<0|1>", "24-hour format"),
            ("SS:<0|1>", "Show seconds"),
            ("H:<±n>", "Adjust hours"),
            ("M:<±n>", "Adjust minutes"),
            ("S:<±n>", "Adjust seconds"),
            ("ZS:1", "Next display (date/year/anti-poison)"),
            ("set-time:YYYY-MM-DDTHH:MM:SS", "Set time (US Pacific local)"),
            ("set-neon:<gamma> <start> <steps> <stepsize> <timestep> <off> <mode>", "Neon fade tuning"),
            ("set-ap:<hour> <min> <ms>", "Anti-poison schedule"),
        ]
        case .riverTable: [
            ("ledbright <0-255>", "Table LED brightness"),
            ("neobright <0-255>", "NeoPixel brightness"),
            ("ledmode <0-2>", "LED mode"),
            ("stest <1-4>", "Test LED: 1 −1, 2 +1, 3 −10, 4 +10"),
        ]
        }
    }

    /// Best guess from advertisement data. IV25 Clock and River Table share the same
    /// UART module, so an unrecognized FFE0 device returns nil and the user picks.
    static func guess(name: String?, services: [CBUUID]) -> DeviceKind? {
        let n = (name ?? "").lowercased()
        if services.contains(nixieService) || n.contains("nixie") { return .nixie }
        if n == "clock" || n.contains("iv25") || n.contains("iv-25") { return .iv25Clock }
        if n.contains("river") || n.contains("table") || n.contains("shift") { return .riverTable }
        return nil
    }

    static func advertisesKnownService(_ services: [CBUUID]) -> Bool {
        services.contains(uartService) || services.contains(nixieService)
    }
}

/// Both clocks convert "local" time to UTC with hard-coded US Pacific DST rules,
/// so times are always sent (and previewed) in Pacific time.
enum DeviceTime {
    static let zone = TimeZone(identifier: "America/Los_Angeles")!

    static var calendar: Calendar {
        var c = Calendar(identifier: .gregorian)
        c.timeZone = zone
        return c
    }

    static func iso8601(_ date: Date) -> String {
        let f = DateFormatter()
        f.locale = Locale(identifier: "en_US_POSIX")
        f.timeZone = zone
        f.dateFormat = "yyyy-MM-dd'T'HH:mm:ss"
        return f.string(from: date)
    }
}
