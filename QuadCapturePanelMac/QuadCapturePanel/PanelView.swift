import SwiftUI

struct PanelView: View {
    @StateObject private var model = PanelModel()

    var body: some View {
        let state = model.drawState
        GeometryReader { geo in
            let layout = PanelLayout(size: geo.size)
            ZStack(alignment: .topLeading) {
                Canvas { context, size in
                    var ctx = context
                    PanelCanvas.draw(&ctx, in: size, state: state)
                }
                .background(PanelCanvas.metalDark)

                ForEach(PanelLayout.knobs, id: \.id) { knob in
                    KnobHandle(
                        value: model.binding(for: knob.id),
                        frame: layout.viewRect(for: knob.hit))
                }
            }
        }
        .onAppear {
            // Adiar IO de áudio para a janela pintar primeiro.
            DispatchQueue.main.async {
                model.start()
            }
        }
        .onDisappear { model.stop() }
    }
}

// MARK: - Layout / hit targets (coordenadas do design 1080×640)

struct KnobSpec {
    let id: KnobID
    /// Centro e raio no espaço de design (para desenho + hit).
    let cx: CGFloat
    let cy: CGFloat
    let radius: CGFloat
    var hit: CGRect {
        CGRect(x: cx - radius - 10, y: cy - radius - 10, width: (radius + 10) * 2, height: (radius + 10) * 2)
    }
}

struct PanelLayout {
    let size: CGSize
    var scale: CGFloat { min(size.width / PanelCanvas.designWidth, size.height / PanelCanvas.designHeight) }
    var origin: CGPoint {
        CGPoint(
            x: (size.width - PanelCanvas.designWidth * scale) / 2,
            y: (size.height - PanelCanvas.designHeight * scale) / 2)
    }

    func viewRect(for design: CGRect) -> CGRect {
        CGRect(
            x: origin.x + design.minX * scale,
            y: origin.y + design.minY * scale,
            width: design.width * scale,
            height: design.height * scale)
    }

    /// Posições dos knobs alinhadas ao Canvas.
    static let knobs: [KnobSpec] = [
        // PREAMP SENS — espelha leitura da placa (build ≥36); arraste só local por agora.
        KnobSpec(id: .sens1, cx: 8 + 72, cy: 64 + 40 + 124, radius: 22),
        KnobSpec(id: .sens2, cx: 8 + 72, cy: 64 + 286 + 124, radius: 22),
        // COMP 1
        KnobSpec(id: .comp1Gate, cx: 292 + 36 + 0 * 76, cy: 104 + 164, radius: 18),
        KnobSpec(id: .comp1Threshold, cx: 292 + 36 + 1 * 76, cy: 104 + 164, radius: 18),
        KnobSpec(id: .comp1Ratio, cx: 292 + 36 + 2 * 76, cy: 104 + 164, radius: 18),
        KnobSpec(id: .comp1Attack, cx: 292 + 36 + 3 * 76, cy: 104 + 164, radius: 18),
        KnobSpec(id: .comp1Release, cx: 292 + 36 + 4 * 76, cy: 104 + 164, radius: 18),
        KnobSpec(id: .comp1Gain, cx: 292 + 36 + 5 * 76, cy: 104 + 164, radius: 18),
        // COMP 2
        KnobSpec(id: .comp2Gate, cx: 292 + 36 + 0 * 76, cy: 350 + 164, radius: 18),
        KnobSpec(id: .comp2Threshold, cx: 292 + 36 + 1 * 76, cy: 350 + 164, radius: 18),
        KnobSpec(id: .comp2Ratio, cx: 292 + 36 + 2 * 76, cy: 350 + 164, radius: 18),
        KnobSpec(id: .comp2Attack, cx: 292 + 36 + 3 * 76, cy: 350 + 164, radius: 18),
        KnobSpec(id: .comp2Release, cx: 292 + 36 + 4 * 76, cy: 350 + 164, radius: 18),
        KnobSpec(id: .comp2Gain, cx: 292 + 36 + 5 * 76, cy: 350 + 164, radius: 18),
        // MIXER
        KnobSpec(id: .mixOutput, cx: 796 + 276 / 2, cy: 64 + 268, radius: 22),
        KnobSpec(id: .mixInput1, cx: 796 + 276 / 2, cy: 64 + 348, radius: 22),
        KnobSpec(id: .mixInput2, cx: 796 + 276 / 2, cy: 64 + 430, radius: 22),
        KnobSpec(id: .mixCoax, cx: 796 + 276 / 2, cy: 64 + 500, radius: 18),
    ]
}

struct KnobHandle: View {
    @Binding var value: Double
    let frame: CGRect
    @State private var dragStartValue: Double?

    var body: some View {
        Circle()
            .fill(Color.clear)
            .contentShape(Circle())
            .frame(width: frame.width, height: frame.height)
            .position(x: frame.midX, y: frame.midY)
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { drag in
                        if dragStartValue == nil {
                            dragStartValue = value
                        }
                        let start = dragStartValue ?? value
                        // Arrastar para cima aumenta; para a direita também.
                        let delta = Double((-drag.translation.height + drag.translation.width) / 140.0)
                        value = min(1, max(0, start + delta))
                    }
                    .onEnded { _ in
                        dragStartValue = nil
                    }
            )
            .help("Arraste para ajustar")
    }
}

extension PanelModel {
    func binding(for id: KnobID) -> Binding<Double> {
        switch id {
        case .sens1: return Binding(get: { self.sens1 }, set: { self.sens1 = $0 })
        case .sens2: return Binding(get: { self.sens2 }, set: { self.sens2 = $0 })
        case .comp1Gate: return Binding(get: { self.comp1.gate }, set: { self.comp1.gate = $0 })
        case .comp1Threshold: return Binding(get: { self.comp1.threshold }, set: { self.comp1.threshold = $0 })
        case .comp1Ratio: return Binding(get: { self.comp1.ratio }, set: { self.comp1.ratio = $0 })
        case .comp1Attack: return Binding(get: { self.comp1.attack }, set: { self.comp1.attack = $0 })
        case .comp1Release: return Binding(get: { self.comp1.release }, set: { self.comp1.release = $0 })
        case .comp1Gain: return Binding(get: { self.comp1.gain }, set: { self.comp1.gain = $0 })
        case .comp2Gate: return Binding(get: { self.comp2.gate }, set: { self.comp2.gate = $0 })
        case .comp2Threshold: return Binding(get: { self.comp2.threshold }, set: { self.comp2.threshold = $0 })
        case .comp2Ratio: return Binding(get: { self.comp2.ratio }, set: { self.comp2.ratio = $0 })
        case .comp2Attack: return Binding(get: { self.comp2.attack }, set: { self.comp2.attack = $0 })
        case .comp2Release: return Binding(get: { self.comp2.release }, set: { self.comp2.release = $0 })
        case .comp2Gain: return Binding(get: { self.comp2.gain }, set: { self.comp2.gain = $0 })
        case .mixOutput: return Binding(get: { self.mixOutput }, set: { self.mixOutput = $0 })
        case .mixInput1: return Binding(get: { self.mixInput1 }, set: { self.mixInput1 = $0 })
        case .mixInput2: return Binding(get: { self.mixInput2 }, set: { self.mixInput2 = $0 })
        case .mixCoax: return Binding(get: { self.mixCoax }, set: { self.mixCoax = $0 })
        }
    }
}

// MARK: - Canvas

enum PanelCanvas {
    static let designWidth: CGFloat = 1080
    static let designHeight: CGFloat = 640

    static let metal = Color(hex: 0x3A3A3A)
    static let metalDark = Color(hex: 0x2A2A2A)
    static let ink = Color(hex: 0xF2F2F2)
    static let label = Color(hex: 0xD8D8D8)
    static let pink = Color(hex: 0xF0B0C4)
    static let pinkEdge = Color(hex: 0xC48498)
    static let grayButton = Color(hex: 0xD9D4CC)
    static let grayEdge = Color(hex: 0xA8A39C)
    static let lcdGreen = Color(hex: 0x39F06A)
    static let clock = Color(hex: 0x5CFFF0)
    static let tick = Color(hex: 0xF3E27A)

    static func draw(_ context: inout GraphicsContext, in size: CGSize, state: PanelModel.DrawState) {
        let scale = min(size.width / designWidth, size.height / designHeight)
        context.translateBy(
            x: (size.width - designWidth * scale) / 2,
            y: (size.height - designHeight * scale) / 2)
        context.scaleBy(x: scale, y: scale)

        menu(&context, state: state)
        header(&context)
        preamp(&context, 8, 64, 268, 520, state: state)
        compressor(&context, 282, 64, 508, 520, state: state)
        mixer(&context, 796, 64, 276, 520, state: state)
        footer(&context, state: state)
    }

    private static func menu(_ context: inout GraphicsContext, state: PanelModel.DrawState) {
        fill(&context, rect(0, 0, designWidth, 26), Color(hex: 0xF3F3F3))
        strokeLine(&context, 0, 26, designWidth, 26, Color(hex: 0xC8C8C8), 1)
        text(&context, "Driver", 14, 0, 70, 26, 13, Color(hex: 0x222222), bold: false, left: true)
        text(&context, "Dispositivo(V)", 78, 0, 140, 26, 13, Color(hex: 0x222222), bold: false, left: true)
        let statusColor = state.connected ? Color(hex: 0x1A7A32) : Color(hex: 0xA02020)
        text(&context, state.status, 240, 0, 820, 26, 12, statusColor, bold: false, left: true)
    }

    private static func header(_ context: inout GraphicsContext) {
        fill(&context, rect(0, 26, designWidth, 36), Color(hex: 0x323232))
        var plate = Path()
        plate.move(to: point(742, 28))
        plate.addLine(to: point(786, 60))
        plate.addLine(to: point(1072, 60))
        plate.addLine(to: point(1072, 28))
        plate.closeSubpath()
        context.fill(plate, with: .color(Color(hex: 0x4E4E4E)))
        context.stroke(plate, with: .color(Color(hex: 0x8A8A8A)), lineWidth: 1)
        text(&context, "QUAD-CAPTURE", 800, 28, 260, 32, 18, ink, bold: true)
    }

    private static func preamp(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat,
        state: PanelModel.DrawState
    ) {
        frame(&context, x, y, w, h)
        title(&context, "PREAMP", x, y + 6, w)
        channel(
            &context, x + 8, y + 40, "1", state.sens1Text, PanelModel.knobAngle(state.sens1),
            level: state.pre1, peak: state.pre1Peak)
        button(&context, x + 168, y + 250, 86, 28, "AUTO SENS", gray: true)
        channel(
            &context, x + 8, y + 286, "2", state.sens2Text, PanelModel.knobAngle(state.sens2),
            level: state.pre2, peak: state.pre2Peak)
    }

    private static func channel(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat,
        _ number: String, _ sens: String, _ sensAngle: CGFloat,
        level: CGFloat, peak: CGFloat
    ) {
        text(&context, number, x, y + 36, 28, 40, 28, ink, bold: true)
        button(&context, x + 36, y + 8, 72, 30, "LO-CUT", gray: false)
        button(&context, x + 36, y + 44, 72, 26, "PHASE", gray: false)
        meter(&context, x + 196, y + 4, 118, level, peak: peak, showClip: true)
        text(&context, "SENS", x + 36, y + 78, 72, 16, 10, label, bold: false)
        knob(&context, x + 72, y + 124, 22, sensAngle)
        lcd(&context, x + 40, y + 156, 68, 26, sens)
    }

    private static func compressor(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat,
        state: PanelModel.DrawState
    ) {
        frame(&context, x, y, w, h)
        title(&context, "COMPRESSOR", x, y + 6, w)
        compStrip(&context, x + 10, y + 40, strip: state.comp1, gr: state.gr1, out: state.compOut1, outPeak: state.compOut1Peak)
        button(&context, x + w / 2 - 36, y + 248, 72, 28, "LINK", gray: false)
        compStrip(&context, x + 10, y + 286, strip: state.comp2, gr: state.gr2, out: state.compOut2, outPeak: state.compOut2Peak)
    }

    private static func compStrip(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat,
        strip: PanelModel.CompStrip,
        gr: CGFloat,
        out: CGFloat,
        outPeak: CGFloat
    ) {
        button(&context, x, y + 18, 70, 32, "BYPASS", gray: strip.bypass)
        text(&context, "GR", x + 78, y, 36, 14, 10, label, bold: true)
        meter(&context, x + 84, y + 16, 100, gr, peak: gr, showClip: false)
        graph(&context, x + 132, y + 8, 210, 108, strip: strip)
        meter(&context, x + 430, y + 8, 116, out, peak: outPeak, showClip: true)

        let labels = ["GATE", "THRESHOLD", "RATIO", "ATTACK", "RELEASE", "GAIN"]
        let values = [strip.gate, strip.threshold, strip.ratio, strip.attack, strip.release, strip.gain]
        for index in labels.indices {
            let cx = x + 36 + CGFloat(index) * 76
            text(&context, labels[index], cx - 40, y + 122, 80, 14, 9, label, bold: false)
            knob(&context, cx, y + 164, 18, PanelModel.knobAngle(values[index]))
        }
    }

    private static func mixer(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat,
        state: PanelModel.DrawState
    ) {
        frame(&context, x, y, w, h)
        title(&context, "MIXER", x, y + 6, w)
        text(&context, "OUTPUT 1-2", x, y + 42, w, 16, 12, label, bold: true)
        meter(&context, x + w / 2 + 8, y + 64, 150, state.mixerOut, peak: state.mixerOutPeak, showClip: true)
        knob(&context, x + w / 2, y + 268, 22, PanelModel.knobAngle(state.mixOutput))
        text(&context, "INPUT 1", x, y + 294, w, 16, 12, label, bold: true)
        knob(&context, x + w / 2, y + 348, 22, PanelModel.knobAngle(state.mixInput1))
        text(&context, "INPUT 2", x, y + 374, w, 16, 12, label, bold: true)
        knob(&context, x + w / 2, y + 430, 22, PanelModel.knobAngle(state.mixInput2))
        text(&context, "COAX (3/4)", x, y + 456, w, 16, 12, label, bold: true)
        knob(&context, x + w / 2, y + 500, 18, PanelModel.knobAngle(state.mixCoax))
    }

    private static func footer(_ context: inout GraphicsContext, state: PanelModel.DrawState) {
        fill(&context, rect(0, 590, designWidth, 50), Color(hex: 0x242424))
        strokeLine(&context, 0, 590, designWidth, 590, Color(hex: 0x111111), 1)
        text(&context, "SAMPLE RATE", 16, 598, 130, 34, 13, label, bold: true, left: true)
        fill(&context, rect(150, 604, 110, 26, radius: 3), Color(hex: 0x111111))
        strokeRect(&context, 150, 604, 110, 26, 3, Color(hex: 0x666666), 1)
        text(&context, "44.1 kHz", 154, 604, 80, 26, 12, ink, bold: true, left: true)

        var arrow = Path()
        arrow.move(to: point(236, 614))
        arrow.addLine(to: point(246, 614))
        arrow.addLine(to: point(241, 620))
        arrow.closeSubpath()
        context.fill(arrow, with: .color(ink))

        text(&context, "CLOCK", 290, 598, 70, 34, 13, label, bold: true, left: true)
        text(&context, "INTERNAL", 360, 598, 120, 34, 14, clock, bold: true, left: true)

        let live = state.connected
        fill(&context, circle(980, 617, 5), live ? lcdGreen : Color(hex: 0xFF3A32))
        text(
            &context,
            live ? "IN LIVE" : "OFFLINE",
            990, 598, 70, 34, 11, live ? lcdGreen : Color(hex: 0xFF8A80),
            bold: true, left: true)
    }

    private static func frame(_ context: inout GraphicsContext, _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat) {
        fill(&context, rect(x, y, w, h, radius: 3), metal)
        strokeRect(&context, x, y, w, h, 3, Color(hex: 0x1A1A1A), 2)
        strokeRect(&context, x + 1, y + 1, w - 2, h - 2, 3, Color(hex: 0x6A6A6A), 1)
    }

    private static func title(_ context: inout GraphicsContext, _ value: String, _ x: CGFloat, _ y: CGFloat, _ w: CGFloat) {
        text(&context, value, x, y, w, 22, 15, ink, bold: true)
        strokeLine(&context, x + 12, y + 24, x + w - 12, y + 24, Color(hex: 0x6E6E6E), 1)
    }

    private static func button(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat,
        _ label: String, gray: Bool
    ) {
        fill(&context, rect(x + 2, y + 2, w, h, radius: 4), Color(hex: 0x1A1A1A))
        fill(&context, rect(x, y, w, h, radius: 4), gray ? grayButton : pink)
        strokeRect(&context, x, y, w, h, 4, gray ? grayEdge : pinkEdge, 1)
        strokeLine(&context, x + 4, y + 2, x + w - 4, y + 2, .white, 1)
        text(&context, label, x, y, w, h, 10, Color(hex: 0x2A2A2A), bold: true)
    }

    private static func knob(_ context: inout GraphicsContext, _ cx: CGFloat, _ cy: CGFloat, _ radius: CGFloat, _ angleDegrees: CGFloat) {
        fill(&context, circle(cx, cy + 2, radius + 3), Color(hex: 0x0E0E0E))
        fill(&context, circle(cx, cy, radius + 3), Color(hex: 0x4A4A4A))
        fill(&context, circle(cx, cy, radius), Color(hex: 0x161616))
        strokeCircle(&context, cx, cy, radius - 4, Color(hex: 0x2E2E2E), 2)

        let rad = (angleDegrees - 90) * .pi / 180
        let x2 = cx + CGFloat(cos(rad)) * (radius - 3)
        let y2 = cy + CGFloat(sin(rad)) * (radius - 3)
        strokeLine(&context, cx, cy, x2, y2, tick, 2)
    }

    private static func meter(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat, _ h: CGFloat,
        _ level: CGFloat, peak: CGFloat, showClip: Bool
    ) {
        let labels = showClip
            ? ["CLIP", "0", "-2", "-6", "-12", "-24", "-48"]
            : ["0", "-2", "-6", "-12", "-24", "-48"]
        let trackX = x + 36
        fill(&context, rect(trackX, y, 12, h, radius: 2), Color(hex: 0x101010))

        let clamped = min(max(level, 0), 1)
        let fillHeight = clamped * (h - 4)
        let fillY = y + h - 2 - fillHeight
        var clipped = context
        clipped.clip(to: Path(CGRect(x: trackX + 2, y: fillY, width: 8, height: fillHeight)))
        let greenTop = y + h * 0.45
        let yellowTop = y + h * 0.18
        fill(&clipped, rect(trackX + 2, y + 2, 8, yellowTop - y), Color(hex: 0xFF3A32))
        fill(&clipped, rect(trackX + 2, yellowTop, 8, greenTop - yellowTop), Color(hex: 0xF0D040))
        fill(&clipped, rect(trackX + 2, greenTop, 8, y + h - greenTop), Color(hex: 0x3DDE4A))

        let peakClamped = min(max(max(peak, level), 0), 1)
        let peakY = y + h - 2 - peakClamped * (h - 4)
        strokeLine(&context, trackX + 1, peakY, trackX + 11, peakY, clock, 2)

        for index in labels.indices {
            let ly = y + CGFloat(index) * ((h - 10) / CGFloat(labels.count - 1))
            text(&context, labels[index], x, ly - 6, 34, 12, 8, label, bold: false)
        }
    }

    private static func graph(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat,
        strip: PanelModel.CompStrip
    ) {
        fill(&context, Path(CGRect(x: x, y: y, width: w, height: h)), Color(hex: 0xC45A18))
        for index in 1..<8 {
            let gx = x + CGFloat(index) * w / 8
            strokeLine(&context, gx, y, gx, y + h, Color(hex: 0xE8A060), 1)
        }
        for index in 1..<5 {
            let gy = y + CGFloat(index) * h / 5
            strokeLine(&context, x, gy, x + w, gy, Color(hex: 0xE8A060), 1)
        }

        // Curva reage a threshold/ratio.
        let thrX = x + 8 + CGFloat(strip.threshold) * (w - 24)
        let kneeY = y + h - 10 - CGFloat(strip.ratio) * (h * 0.55)
        var curve = Path()
        curve.move(to: point(x + 8, y + h - 10))
        curve.addLine(to: point(thrX, y + h - 10 - CGFloat(strip.threshold) * (h - 24)))
        curve.addQuadCurve(
            to: point(x + w - 10, max(y + 10, kneeY)),
            control: point(thrX + (w - thrX) * 0.35, kneeY + 10))
        context.stroke(curve, with: .color(Color(hex: 0xFFF4D8)), lineWidth: 2)
        text(&context, "0", x + w - 16, y + 2, 14, 12, 9, ink, bold: false)
        text(&context, "-60", x + 2, y + h - 14, 28, 12, 8, ink, bold: false, left: true)
    }

    private static func lcd(_ context: inout GraphicsContext, _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat, _ value: String) {
        fill(&context, rect(x, y, w, h, radius: 2), Color(hex: 0x070707))
        strokeRect(&context, x, y, w, h, 2, Color(hex: 0x3A3A3A), 1)
        text(&context, value, x, y, w, h, 14, lcdGreen, bold: true)
    }

    private static func text(
        _ context: inout GraphicsContext,
        _ value: String,
        _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat,
        _ size: CGFloat, _ color: Color, bold: Bool, left: Bool = false
    ) {
        let resolved = context.resolve(
            Text(value)
                .font(.system(size: size, weight: bold ? .semibold : .regular))
                .foregroundColor(color))
        let origin = left
            ? CGPoint(x: x, y: y + h / 2)
            : CGPoint(x: x + w / 2, y: y + h / 2)
        context.draw(resolved, at: origin, anchor: left ? .leading : .center)
    }

    private static func fill(_ context: inout GraphicsContext, _ path: Path, _ color: Color) {
        context.fill(path, with: .color(color))
    }

    private static func strokeLine(
        _ context: inout GraphicsContext,
        _ x1: CGFloat, _ y1: CGFloat, _ x2: CGFloat, _ y2: CGFloat,
        _ color: Color, _ width: CGFloat
    ) {
        var path = Path()
        path.move(to: point(x1, y1))
        path.addLine(to: point(x2, y2))
        context.stroke(path, with: .color(color), lineWidth: width)
    }

    private static func strokeRect(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat,
        _ radius: CGFloat, _ color: Color, _ width: CGFloat
    ) {
        context.stroke(rect(x, y, w, h, radius: radius), with: .color(color), lineWidth: width)
    }

    private static func strokeCircle(
        _ context: inout GraphicsContext,
        _ cx: CGFloat, _ cy: CGFloat, _ radius: CGFloat,
        _ color: Color, _ width: CGFloat
    ) {
        context.stroke(circle(cx, cy, radius), with: .color(color), lineWidth: width)
    }

    private static func rect(_ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat, radius: CGFloat = 0) -> Path {
        Path(roundedRect: CGRect(x: x, y: y, width: w, height: h), cornerRadius: radius)
    }

    private static func circle(_ cx: CGFloat, _ cy: CGFloat, _ radius: CGFloat) -> Path {
        Path(ellipseIn: CGRect(x: cx - radius, y: cy - radius, width: radius * 2, height: radius * 2))
    }

    private static func point(_ x: CGFloat, _ y: CGFloat) -> CGPoint {
        CGPoint(x: x, y: y)
    }
}

extension Color {
    init(hex: UInt32) {
        self.init(
            red: Double((hex >> 16) & 0xFF) / 255,
            green: Double((hex >> 8) & 0xFF) / 255,
            blue: Double(hex & 0xFF) / 255)
    }
}
