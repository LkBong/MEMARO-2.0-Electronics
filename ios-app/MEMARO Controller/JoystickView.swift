import SwiftUI

struct JoystickView: View {
    let diameter: CGFloat
    let onUpdate:  (Int8, Int8) -> Void   // called every drag change
    let onRelease: () -> Void             // called on drag end — caller must send speed 0

    @State private var thumbOffset: CGSize = .zero

    private var radius:        CGFloat { diameter / 2 }
    private var thumbDiameter: CGFloat { diameter * 0.22 }

    var body: some View {
        ZStack {
            // Outer track
            Circle()
                .stroke(Color.white.opacity(0.25), lineWidth: 1.5)
                .frame(width: diameter, height: diameter)

            // Inner reference ring
            Circle()
                .stroke(Color.white.opacity(0.1), lineWidth: 1)
                .frame(width: diameter * 0.5, height: diameter * 0.5)

            // Thumb
            Circle()
                .fill(Color.white.opacity(0.9))
                .frame(width: thumbDiameter, height: thumbDiameter)
                .offset(thumbOffset)
                .shadow(color: .black.opacity(0.35), radius: 6, x: 0, y: 3)
        }
        .frame(width: diameter, height: diameter)
        .contentShape(Circle())
        .gesture(
            DragGesture(minimumDistance: 0)
                .onChanged { value in
                    let t = value.translation
                    let distance = sqrt(t.width * t.width + t.height * t.height)

                    // Clamp to radius
                    let clamped: CGSize
                    if distance > radius {
                        let scale = radius / distance
                        clamped = CGSize(width: t.width * scale, height: t.height * scale)
                    } else {
                        clamped = t
                    }
                    thumbOffset = clamped

                    // Normalise to [-1, 1]; invert Y so up = positive
                    let nx =  Double(clamped.width)  / Double(radius)
                    let ny = -Double(clamped.height) / Double(radius)

                    // Dead zone: 5% of radius
                    if sqrt(nx * nx + ny * ny) < 0.05 {
                        onUpdate(0, 0)
                    } else {
                        let x = Int8(clamping: Int((nx * 100).rounded()))
                        let y = Int8(clamping: Int((ny * 100).rounded()))
                        onUpdate(x, y)
                    }
                }
                .onEnded { _ in
                    withAnimation(.easeOut(duration: 0.1)) { thumbOffset = .zero }
                    onRelease()
                }
        )
    }
}
