import SwiftUI

struct ContentView: View {
    @EnvironmentObject var ble: BLEManager

    @State private var speed:  Double = 0.0
    @State private var lastX:  Int8   = 0
    @State private var lastY:  Int8   = 0
    @State private var showScanSheet = false

    var body: some View {
        GeometryReader { geo in
            HStack(spacing: 0) {
                leftColumn(geo: geo)
                Divider()
                centreColumn
                Divider()
                rightColumn(geo: geo)
            }
        }
        .preferredColorScheme(.dark)
        .background(Color(.systemBackground))
        .sheet(isPresented: $showScanSheet) { scanSheet }
        .onChange(of: ble.isConnected) { connected in
            if connected { showScanSheet = false }
        }
    }

    // MARK: - Left: joystick

    private func leftColumn(geo: GeometryProxy) -> some View {
        let diameter = min(geo.size.width * 0.35, geo.size.height * 0.85)
        return ZStack {
            JoystickView(diameter: diameter) { x, y in
                lastX = x; lastY = y
                ble.updateCommand(x: x, y: y, speed: UInt8((speed * 255).rounded()))
            } onRelease: {
                lastX = 0; lastY = 0
                ble.updateCommand(x: 0, y: 0, speed: 0)
            }
        }
        .frame(width: geo.size.width * 0.35)
        .frame(maxHeight: .infinity)
    }

    // MARK: - Centre: telemetry + BLE controls

    private var centreColumn: some View {
        VStack(spacing: 0) {
            telemetryRow
                .padding(.vertical, 20)
            Divider()
            bleControls
                .padding(.vertical, 16)
        }
        .padding(.horizontal, 20)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }

    private var telemetryRow: some View {
        HStack(spacing: 48) {
            telemetryCell(
                value: "\(ble.rpm)",
                unit: "RPM",
                active: ble.isConnected
            )
            telemetryCell(
                value: String(format: "%.3f", ble.currentA),
                unit: "A",
                active: ble.isConnected
            )
        }
    }

    private func telemetryCell(value: String, unit: String, active: Bool) -> some View {
        VStack(spacing: 3) {
            Text(value)
                .font(.system(size: 36, weight: .semibold, design: .monospaced))
                .foregroundColor(active ? .primary : .secondary)
                .monospacedDigit()
            Text(unit)
                .font(.caption)
                .foregroundColor(.secondary)
        }
    }

    @ViewBuilder
    private var bleControls: some View {
        if ble.isConnected {
            VStack(spacing: 10) {
                Label(ble.connectedDeviceName ?? "MEMARO",
                      systemImage: "antenna.radiowaves.left.and.right")
                    .font(.caption)
                    .foregroundColor(.green)
                Button("Disconnect") { ble.disconnect() }
                    .buttonStyle(.bordered)
                    .tint(.red)
                    .controlSize(.small)
            }
        } else {
            VStack(spacing: 10) {
                HStack(spacing: 6) {
                    if ble.isScanning { ProgressView().scaleEffect(0.75) }
                    Text(ble.isScanning ? "Scanning…" : "Idle")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                Button("Connect") { showScanSheet = true }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
            }
        }
    }

    // MARK: - Right: vertical speed slider

    private func rightColumn(geo: GeometryProxy) -> some View {
        let trackLength = geo.size.height * 0.62
        return VStack(spacing: 8) {
            Text("\(Int(speed * 100))%")
                .font(.system(size: 13, weight: .medium, design: .monospaced))
            Slider(value: $speed, in: 0...1)
                .frame(width: trackLength)
                .rotationEffect(.degrees(-90))
                .frame(width: 44, height: trackLength)
                .onChange(of: speed) { _ in
                    ble.updateCommand(x: lastX, y: lastY, speed: UInt8((speed * 255).rounded()))
                }
            Text("SPD")
                .font(.caption2)
                .foregroundColor(.secondary)
        }
        .frame(width: 80)
        .frame(maxHeight: .infinity)
    }

    // MARK: - Scan sheet

    private var scanSheet: some View {
        NavigationView {
            Group {
                if ble.peripherals.isEmpty {
                    VStack(spacing: 12) {
                        if ble.isScanning { ProgressView() }
                        Text("Searching for MEMARO…")
                            .foregroundColor(.secondary)
                            .font(.subheadline)
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                } else {
                    List(ble.peripherals, id: \.identifier) { peripheral in
                        Button {
                            ble.connect(peripheral)
                        } label: {
                            HStack {
                                Text(peripheral.name ?? "Unknown")
                                Spacer()
                                Image(systemName: "chevron.right")
                                    .foregroundColor(.secondary)
                                    .font(.caption)
                            }
                        }
                        .foregroundColor(.primary)
                    }
                }
            }
            .navigationTitle("Select Rover")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") { showScanSheet = false }
                }
            }
        }
        .presentationDetents([.medium])
        .onAppear { ble.startScan() }
        .onDisappear { ble.stopScan() }
    }
}

#Preview {
    ContentView()
        .environmentObject(BLEManager())
        .previewInterfaceOrientation(.landscapeLeft)
}
