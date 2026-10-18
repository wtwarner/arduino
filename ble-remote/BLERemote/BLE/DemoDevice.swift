import Foundation

/// Stand-in for real hardware so the UI can be exercised without a device
/// (the iOS Simulator has no Bluetooth). The River Table demo mirrors
/// shift_bt.ino: it applies commands and streams status like the firmware.
final class DemoDevice {
    private let kind: DeviceKind
    private let output: (String) -> Void
    private var timer: Timer?

    private var led = 0
    private var ledPwm = 128
    private var neoPwm = 128
    private var testValue = 0
    private var mode = 1

    init(kind: DeviceKind, output: @escaping (String) -> Void) {
        self.kind = kind
        self.output = output
        if kind == .riverTable {
            timer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: true) { [weak self] _ in
                self?.tick()
            }
        }
    }

    func stop() {
        timer?.invalidate()
        timer = nil
    }

    func receive(_ command: String) {
        guard kind == .riverTable else { return }
        let parts = command.split(separator: " ", maxSplits: 1)
        guard let name = parts.first else { return }
        let arg = parts.count > 1 ? Int(parts[1].trimmingCharacters(in: .whitespaces)) ?? 0 : 0
        switch name {
        case "ledbright": ledPwm = arg & 0xFF
        case "neobright": neoPwm = arg & 0xFF
        case "ledmode": mode = arg & 0xFF
        case "stest":
            switch arg {
            case 1: testValue = max(testValue - 1, 0)
            case 2: testValue = (testValue + 1) & 0xFF
            case 3: testValue = max(testValue - 10, 0)
            case 4: testValue = (testValue + 10) & 0xFF
            default: break
            }
        default: break
        }
    }

    private func tick() {
        led = 1 - led
        let s = "led:\(led);ledpwm:\(ledPwm);neopwm:\(neoPwm);stest:\(testValue);"
            + "modeled0:\(mode == 0 ? 1 : 0);modeled1:\(mode == 1 ? 1 : 0);modeled2:\(mode == 2 ? 1 : 0);"
        // Deliver in uneven pieces, as an HM-10 would, to exercise reassembly.
        let split = s.index(s.startIndex, offsetBy: 17)
        output(String(s[..<split]))
        output(String(s[split...]))
    }
}
