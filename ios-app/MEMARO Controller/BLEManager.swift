import CoreBluetooth
import Foundation

@MainActor
final class BLEManager: NSObject, ObservableObject {

    // MARK: - UUIDs — must match ESP32 firmware exactly
    static let serviceUUID  = CBUUID(string: "4FAFC201-1FB5-459E-8FCC-C5C9C331914B")
    static let controlUUID  = CBUUID(string: "BEB5483E-36E1-4688-B7F5-EA07361B26A8")
    static let feedbackUUID = CBUUID(string: "1C95D5E3-D8F5-4D7F-8B9C-2B7A5C3F1234")

    // MARK: - Published state
    @Published var isConnected:          Bool           = false
    @Published var isScanning:           Bool           = false
    @Published var peripherals:          [CBPeripheral] = []
    @Published var rpm:                  Int            = 0
    @Published var currentA:             Double         = 0.0
    @Published var connectedDeviceName:  String?        = nil

    // MARK: - Private — BLE objects
    private var central:                CBCentralManager!
    private var connectedPeripheral:    CBPeripheral?
    private var controlCharacteristic:  CBCharacteristic?
    private var feedbackCharacteristic: CBCharacteristic?

    // MARK: - Private — 50 ms command timer
    private var pendingX:     Int8   = 0
    private var pendingY:     Int8   = 0
    private var pendingSpeed: UInt8  = 0
    private var commandTimer: Timer?

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: .main)
    }

    // MARK: - Public interface

    func startScan() {
        guard central.state == .poweredOn else { return }
        peripherals = []
        isScanning  = true
        central.scanForPeripherals(withServices: [BLEManager.serviceUUID])
    }

    func stopScan() {
        central.stopScan()
        isScanning = false
    }

    func connect(_ peripheral: CBPeripheral) {
        stopScan()
        connectedPeripheral = peripheral
        peripheral.delegate = self
        central.connect(peripheral)
    }

    func disconnect() {
        guard let p = connectedPeripheral else { return }
        central.cancelPeripheralConnection(p)
    }

    func updateCommand(x: Int8, y: Int8, speed: UInt8) {
        pendingX = x; pendingY = y; pendingSpeed = speed
    }

    // MARK: - Private

    func sendCommand(x: Int8, y: Int8, speed: UInt8) {
        guard let char       = controlCharacteristic,
              let peripheral = connectedPeripheral else { return }
        let payload = Data([UInt8(bitPattern: x), UInt8(bitPattern: y), speed])
        peripheral.writeValue(payload, for: char, type: .withoutResponse)
    }

    private func startCommandTimer() {
        commandTimer?.invalidate()
        commandTimer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { [weak self] _ in
            Task { @MainActor [weak self] in
                guard let self, self.isConnected else { return }
                self.sendCommand(x: self.pendingX, y: self.pendingY, speed: self.pendingSpeed)
            }
        }
    }

    private func stopCommandTimer() {
        commandTimer?.invalidate()
        commandTimer = nil
        pendingX = 0; pendingY = 0; pendingSpeed = 0
    }
}

// MARK: - CBCentralManagerDelegate
extension BLEManager: @preconcurrency CBCentralManagerDelegate {

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn { startScan() }
    }

    func centralManager(_ central: CBCentralManager,
                        didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any],
                        rssi RSSI: NSNumber) {
        guard !peripherals.contains(where: { $0.identifier == peripheral.identifier }) else { return }
        peripherals.append(peripheral)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        connectedDeviceName = peripheral.name
        peripheral.discoverServices([BLEManager.serviceUUID])
    }

    func centralManager(_ central: CBCentralManager,
                        didDisconnectPeripheral peripheral: CBPeripheral,
                        error: Error?) {
        stopCommandTimer()             // stop timer before touching any state
        controlCharacteristic  = nil   // 1
        feedbackCharacteristic = nil   // 2
        isConnected            = false // 3
        connectedDeviceName    = nil
        connectedPeripheral    = nil
        startScan()                    // 4
    }
}

// MARK: - CBPeripheralDelegate
extension BLEManager: @preconcurrency CBPeripheralDelegate {

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: {
            $0.uuid == BLEManager.serviceUUID
        }) else { return }
        peripheral.discoverCharacteristics(
            [BLEManager.controlUUID, BLEManager.feedbackUUID], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didDiscoverCharacteristicsFor service: CBService,
                    error: Error?) {
        for char in service.characteristics ?? [] {
            switch char.uuid {
            case BLEManager.controlUUID:
                controlCharacteristic = char
            case BLEManager.feedbackUUID:
                feedbackCharacteristic = char
                peripheral.setNotifyValue(true, for: char)
            default:
                break
            }
        }
        if controlCharacteristic != nil && feedbackCharacteristic != nil {
            isConnected = true
            startCommandTimer()
        }
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didUpdateValueFor characteristic: CBCharacteristic,
                    error: Error?) {
        guard characteristic.uuid == BLEManager.feedbackUUID,
              let data = characteristic.value,
              data.count >= 4 else { return }
        rpm      = Int((UInt16(data[0]) << 8) | UInt16(data[1]))
        currentA = Double((UInt16(data[2]) << 8) | UInt16(data[3])) / 1000.0
    }
}
