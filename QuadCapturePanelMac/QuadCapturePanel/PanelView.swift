import SwiftUI

struct PanelView: View {
    @StateObject private var model = PanelModel()
    @State private var rateMenuOpen = false

    var body: some View {
        let state = model.drawState
        GeometryReader { geo in
            let layout = PanelLayout(size: geo.size)
            let rateFrame = layout.viewRect(for: PanelLayout.sampleRateHit)
            ZStack(alignment: .topLeading) {
                Canvas { context, size in
                    var ctx = context
                    PanelCanvas.draw(&ctx, in: size, state: state)
                }
                .background(PanelCanvas.metalDark)

                ForEach(PanelLayout.knobs.filter(\.interactive), id: \.id) { knob in
                    KnobHandle(
                        value: model.binding(for: knob.id),
                        frame: layout.viewRect(for: knob.hit),
                        onActive: { active in
                            if let index = knob.id.mixerIndex {
                                model.setMixerDrag(index, active: active)
                            } else if knob.id.isCompressor {
                                model.setCompDrag(knob.id, active: active)
                            }
                        })
                }

                if rateMenuOpen {
                    Color.clear
                        .contentShape(Rectangle())
                        .frame(width: geo.size.width, height: geo.size.height)
                        .onTapGesture { rateMenuOpen = false }
                    SampleRateMenu(
                        anchor: rateFrame,
                        currentHz: model.levels.sampleRateHz
                    ) { hz in
                        rateMenuOpen = false
                        model.setSampleRate(hz)
                    }
                }

                SampleRateButton(frame: rateFrame) {
                    rateMenuOpen.toggle()
                }

                ForEach(0..<2, id: \.self) { channel in
                    LoCutButton(frame: layout.viewRect(for: PanelLayout.loCutHit(channel: channel))) {
                        model.toggleLoCut(channel: channel)
                    }
                    PhaseButton(frame: layout.viewRect(for: PanelLayout.phaseHit(channel: channel))) {
                        model.togglePhase(channel: channel)
                    }
                }

                PhaseButton(frame: layout.viewRect(for: PanelLayout.autoSensHit), helpText: "Ligar ou desligar o AUTO-SENS") {
                    model.pressAutoSens()
                }

                PhaseButton(frame: layout.viewRect(for: PanelLayout.linkHit), helpText: "Ligar ou desligar o LINK") {
                    model.toggleLink()
                }

                ForEach(0..<2, id: \.self) { channel in
                    PhaseButton(
                        frame: layout.viewRect(for: PanelLayout.bypassHit(channel: channel)),
                        helpText: "Ligar ou desligar o BYPASS"
                    ) {
                        model.toggleBypass(channel: channel)
                    }
                }

                if model.levels.settled && !state.connected {
                    DisconnectedCover()
                        .frame(width: geo.size.width, height: geo.size.height)
                }
            }
        }
        .onChange(of: state.connected) { connected in
            if !connected {
                rateMenuOpen = false
            }
        }
        .onAppear {
            PanelLog.write("window appear pid=\(ProcessInfo.processInfo.processIdentifier)")
            DispatchQueue.main.async {
                model.start()
            }
        }
        .onDisappear {
            PanelLog.write("window disappear")
            model.stop()
        }
    }
}

// MARK: - Layout / hit targets (coordenadas do design 866×592)

struct KnobSpec {
    let id: KnobID
    /// Centro e raio no espaço de design (para desenho + hit).
    let cx: CGFloat
    let cy: CGFloat
    let radius: CGFloat
    var interactive: Bool = true
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

    /// Caixa SAMPLE RATE no rodapé (design 866×592).
    static let sampleRateHit = CGRect(x: 150, y: 563, width: 110, height: 24)

    /// LO-CUT dentro de `channel`: (x+36, y+8, 72, 30). Canal 0 fica em y=78, canal 1 em y=356.
    static func loCutHit(channel: Int) -> CGRect {
        let channelX: CGFloat = 16
        let channelY: CGFloat = channel == 0 ? 78 : 356
        return CGRect(x: channelX + 36, y: channelY + 8, width: 72, height: 30)
    }

    /// PHASE dentro de `channel`: (x+36, y+44, 72, 26).
    static func phaseHit(channel: Int) -> CGRect {
        let channelX: CGFloat = 16
        let channelY: CGFloat = channel == 0 ? 78 : 356
        return CGRect(x: channelX + 36, y: channelY + 44, width: 72, height: 26)
    }

    /// LINK entre as faixas do compressor: (220, 286, 70, 32).
    static let linkHit = CGRect(x: 220, y: 286, width: 70, height: 32)

    /// BYPASS dentro de `compStrip`: (x, y+18, 70, 32). Faixa 1 em (220, 78), faixa 2 em (220, 324).
    static func bypassHit(channel: Int) -> CGRect {
        let stripY: CGFloat = channel == 0 ? 78 : 324
        return CGRect(x: 220, y: stripY + 18, width: 70, height: 32)
    }

    /// AUTO SENS entre os dois canais: x+44, 72×40, centralizado no vão.
    static let autoSensHit: CGRect = {
        let x: CGFloat = 8 + 44
        let channel1Y: CGFloat = 38 + 40
        let channel2Y: CGFloat = 38 + 318
        let gapTop = channel1Y + 182
        let gapBottom = channel2Y + 4
        let buttonH: CGFloat = 40
        let y = gapTop + (gapBottom - gapTop - buttonH) / 2
        return CGRect(x: x, y: y, width: 72, height: buttonH)
    }()

    /// Posições dos knobs alinhadas ao Canvas.
    static let knobs: [KnobSpec] = [
        // SENS acompanha o knob físico e também envia o passo quando o usuário arrasta.
        KnobSpec(id: .sens1, cx: 8 + 72, cy: 38 + 40 + 124, radius: 22),
        KnobSpec(id: .sens2, cx: 8 + 72, cy: 38 + 318 + 124, radius: 22),
        // COMP 1
        KnobSpec(id: .comp1Gate, cx: 220 + 58 + 0 * 62, cy: 78 + 180, radius: 18),
        KnobSpec(id: .comp1Threshold, cx: 220 + 58 + 1 * 62, cy: 78 + 180, radius: 18),
        KnobSpec(id: .comp1Ratio, cx: 220 + 58 + 2 * 62, cy: 78 + 180, radius: 18),
        KnobSpec(id: .comp1Attack, cx: 220 + 58 + 3 * 62, cy: 78 + 180, radius: 18),
        KnobSpec(id: .comp1Release, cx: 220 + 58 + 4 * 62, cy: 78 + 180, radius: 18),
        KnobSpec(id: .comp1Gain, cx: 220 + 58 + 5 * 62, cy: 78 + 180, radius: 18),
        // COMP 2
        KnobSpec(id: .comp2Gate, cx: 220 + 58 + 0 * 62, cy: 324 + 180, radius: 18),
        KnobSpec(id: .comp2Threshold, cx: 220 + 58 + 1 * 62, cy: 324 + 180, radius: 18),
        KnobSpec(id: .comp2Ratio, cx: 220 + 58 + 2 * 62, cy: 324 + 180, radius: 18),
        KnobSpec(id: .comp2Attack, cx: 220 + 58 + 3 * 62, cy: 324 + 180, radius: 18),
        KnobSpec(id: .comp2Release, cx: 220 + 58 + 4 * 62, cy: 324 + 180, radius: 18),
        KnobSpec(id: .comp2Gain, cx: 220 + 58 + 5 * 62, cy: 324 + 180, radius: 18),
        // MIXER
        KnobSpec(id: .mixOutput, cx: 662 + 196 / 2, cy: 38 + 268, radius: 22),
        KnobSpec(id: .mixInput1, cx: 662 + 196 / 2, cy: 38 + 348, radius: 22),
        KnobSpec(id: .mixInput2, cx: 662 + 196 / 2, cy: 38 + 430, radius: 22),
    ]
}

struct KnobHandle: View {
    @Binding var value: Double
    let frame: CGRect
    var onActive: ((Bool) -> Void)?
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
                            onActive?(true)
                        }
                        let start = dragStartValue ?? value
                        // Arrastar para cima aumenta; para a direita também.
                        let delta = Double((-drag.translation.height + drag.translation.width) / 140.0)
                        value = min(1, max(0, start + delta))
                    }
                    .onEnded { _ in
                        dragStartValue = nil
                        onActive?(false)
                    }
            )
            .help("Arraste para ajustar")
    }
}

struct PhaseButton: View {
    let frame: CGRect
    var helpText: String = "Inverter a fase"
    let action: () -> Void

    var body: some View {
        Color.clear
            .contentShape(Rectangle())
            .frame(width: frame.width, height: frame.height)
            .position(x: frame.midX, y: frame.midY)
            .onTapGesture(perform: action)
            .help(helpText)
    }
}

struct LoCutButton: View {
    let frame: CGRect
    let action: () -> Void

    var body: some View {
        Color.clear
            .contentShape(Rectangle())
            .frame(width: frame.width, height: frame.height)
            .position(x: frame.midX, y: frame.midY)
            .onTapGesture(perform: action)
            .help("Ligar ou desligar o LO-CUT")
    }
}

struct SampleRateButton: View {
    let frame: CGRect
    let action: () -> Void

    var body: some View {
        Color.clear
            .contentShape(Rectangle())
            .frame(width: frame.width, height: frame.height)
            .position(x: frame.midX, y: frame.midY)
            .onTapGesture(perform: action)
            .help("Escolher sample rate")
    }
}

private struct DisconnectedCover: View {
    var body: some View {
        ZStack {
            Color.black.opacity(0.72)
            Text("A placa está desconectada")
                .font(.system(size: 22, weight: .semibold))
                .foregroundStyle(PanelCanvas.ink)
                .padding(.horizontal, 28)
                .padding(.vertical, 18)
                .background(
                    RoundedRectangle(cornerRadius: 6)
                        .fill(PanelCanvas.metal)
                        .overlay(
                            RoundedRectangle(cornerRadius: 6)
                                .stroke(Color(hex: 0x1A1A1A), lineWidth: 2)))
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .contentShape(Rectangle())
    }
}

struct SampleRateMenu: View {
    let anchor: CGRect
    let currentHz: Double
    let onSelect: (Double) -> Void

    var body: some View {
        let rowH = anchor.height
        let menuH = rowH * CGFloat(UA55Device.rates.count)
        let font = max(11, 12 * rowH / 24)
        VStack(spacing: 0) {
            ForEach(UA55Device.rates, id: \.self) { hz in
                let selected = currentHz > 0 && UA55Device.bucket(currentHz) == hz
                Button {
                    onSelect(hz)
                } label: {
                    Text(UA55Device.label(for: hz))
                        .font(.system(size: font, weight: .semibold))
                        .foregroundStyle(selected ? PanelCanvas.clock : PanelCanvas.ink)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .padding(.horizontal, 8)
                        .frame(height: rowH)
                        .background(selected ? Color(hex: 0x2A2A2A) : Color(hex: 0x111111))
                }
                .buttonStyle(.plain)
            }
        }
        .frame(width: anchor.width, height: menuH)
        .overlay(
            RoundedRectangle(cornerRadius: 3)
                .stroke(Color(hex: 0x666666), lineWidth: 1))
        .position(x: anchor.midX, y: anchor.minY - 4 - menuH / 2)
    }
}

extension PanelModel {
    func binding(for id: KnobID) -> Binding<Double> {
        switch id {
        case .sens1: return Binding(get: { self.sens1 }, set: { self.userSetSens(channel: 0, normalized: $0) })
        case .sens2: return Binding(get: { self.sens2 }, set: { self.userSetSens(channel: 1, normalized: $0) })
        case .comp1Gate: return Binding(get: { self.comp1.gate }, set: { self.setCompKnob(channel: 0, \.gate, $0) })
        case .comp1Threshold: return Binding(get: { self.comp1.threshold }, set: { self.setCompKnob(channel: 0, \.threshold, $0) })
        case .comp1Ratio: return Binding(get: { self.comp1.ratio }, set: { self.setCompKnob(channel: 0, \.ratio, $0) })
        case .comp1Attack: return Binding(get: { self.comp1.attack }, set: { self.setCompKnob(channel: 0, \.attack, $0) })
        case .comp1Release: return Binding(get: { self.comp1.release }, set: { self.setCompKnob(channel: 0, \.release, $0) })
        case .comp1Gain: return Binding(get: { self.comp1.gain }, set: { self.setCompKnob(channel: 0, \.gain, $0) })
        case .comp2Gate: return Binding(get: { self.comp2.gate }, set: { self.setCompKnob(channel: 1, \.gate, $0) })
        case .comp2Threshold: return Binding(get: { self.comp2.threshold }, set: { self.setCompKnob(channel: 1, \.threshold, $0) })
        case .comp2Ratio: return Binding(get: { self.comp2.ratio }, set: { self.setCompKnob(channel: 1, \.ratio, $0) })
        case .comp2Attack: return Binding(get: { self.comp2.attack }, set: { self.setCompKnob(channel: 1, \.attack, $0) })
        case .comp2Release: return Binding(get: { self.comp2.release }, set: { self.setCompKnob(channel: 1, \.release, $0) })
        case .comp2Gain: return Binding(get: { self.comp2.gain }, set: { self.setCompKnob(channel: 1, \.gain, $0) })
        case .mixOutput: return Binding(get: { self.mixOutput }, set: { self.userSetMixer(index: 0, normalized: $0) })
        case .mixInput1: return Binding(get: { self.mixInput1 }, set: { self.userSetMixer(index: 1, normalized: $0) })
        case .mixInput2: return Binding(get: { self.mixInput2 }, set: { self.userSetMixer(index: 2, normalized: $0) })
        }
    }
}

// MARK: - Canvas

enum PanelCanvas {
    static let designWidth: CGFloat = 866
    static let designHeight: CGFloat = 592

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

        header(&context)
        preamp(&context, 8, 38, 196, 520, state: state)
        compressor(&context, 210, 38, 446, 520, state: state)
        mixer(&context, 662, 38, 196, 520, state: state)
        footer(&context, state: state)
    }

    private static func header(_ context: inout GraphicsContext) {
        fill(&context, rect(0, 0, designWidth, 36), Color(hex: 0x323232))
        var plate = Path()
        plate.move(to: point(528, 2))
        plate.addLine(to: point(572, 34))
        plate.addLine(to: point(858, 34))
        plate.addLine(to: point(858, 2))
        plate.closeSubpath()
        context.fill(plate, with: .color(Color(hex: 0x4E4E4E)))
        context.stroke(plate, with: .color(Color(hex: 0x8A8A8A)), lineWidth: 1)
        text(&context, "QUAD-CAPTURE", 586, 2, 260, 32, 18, ink, bold: true)
    }

    private static func preamp(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat,
        state: PanelModel.DrawState
    ) {
        frame(&context, x, y, w, h)
        title(&context, "PREAMP", x, y + 6, w)
        let channel1Y = y + 40
        let channel2Y = y + 318
        channel(
            &context, x + 8, channel1Y, "1", state.sens1Text, PanelModel.knobAngle(state.sens1),
            level: state.pre1, peak: state.pre1Peak, loCutOn: state.loCut1, phaseOn: state.phase1)
        let buttonW: CGFloat = 72
        let buttonH: CGFloat = 40
        let gapTop = channel1Y + 182
        let gapBottom = channel2Y + 4
        button(
            &context,
            x + 44,
            gapTop + (gapBottom - gapTop - buttonH) / 2,
            buttonW, buttonH,
            "AUTO SENS", gray: state.autoSensText != "on")
        channel(
            &context, x + 8, channel2Y, "2", state.sens2Text, PanelModel.knobAngle(state.sens2),
            level: state.pre2, peak: state.pre2Peak, loCutOn: state.loCut2, phaseOn: state.phase2)
    }

    private static func channel(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat,
        _ number: String, _ sens: String, _ sensAngle: CGFloat,
        level: CGFloat, peak: CGFloat, loCutOn: Bool, phaseOn: Bool
    ) {
        text(&context, number, x, y + 36, 28, 40, 28, ink, bold: true)
        button(&context, x + 36, y + 8, 72, 30, "LO-CUT", gray: !loCutOn)
        button(&context, x + 36, y + 44, 72, 26, "PHASE", gray: !phaseOn)
        meter(&context, x + 118, y + 4, 118, level, peak: peak, showClip: true)
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
        compStrip(&context, x + 10, y + 40, strip: state.comp1, readouts: state.comp1Text, gr: state.gr1, out: state.compOut1, outPeak: state.compOut1Peak)
        button(&context, x + 10, y + 248, 70, 32, "LINK", gray: !state.linkOn)
        compStrip(&context, x + 10, y + 286, strip: state.comp2, readouts: state.comp2Text, gr: state.gr2, out: state.compOut2, outPeak: state.compOut2Peak)
    }

    private static func compStrip(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat,
        strip: PanelModel.CompStrip,
        readouts: [String],
        gr: CGFloat,
        out: CGFloat,
        outPeak: CGFloat
    ) {
        button(&context, x, y + 18, 70, 32, "BYPASS", gray: !strip.bypass)
        text(&context, "GR", x + 78, y, 36, 14, 10, label, bold: true)
        meter(&context, x + 84, y + 16, 100, gr, peak: gr, showClip: false)
        graph(
            &context, x + 186, y + 8, 104, 104,
            gate: strip.gate, threshold: strip.threshold, ratio: strip.ratio,
            gain: strip.gain, bypassed: strip.bypass)
        meter(&context, x + 368, y + 8, 116, out, peak: outPeak, showClip: true)

        let labels = ["GATE", "THRESHOLD", "RATIO", "ATTACK", "RELEASE", "GAIN"]
        let values = [strip.gate, strip.threshold, strip.ratio, strip.attack, strip.release, strip.gain]
        for index in labels.indices {
            let cx = x + 58 + CGFloat(index) * 62
            text(&context, labels[index], cx - 30, y + 138, 60, 14, 9, label, bold: false)
            knob(&context, cx, y + 180, 18, PanelModel.knobAngle(values[index]))
        }
        for index in labels.indices where index < readouts.count {
            let cx = x + 58 + CGFloat(index) * 62
            mixerGain(&context, readouts[index], cx + 20, y + 172)
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
        meter(
            &context, x + w / 2 - 36, y + 64, 150,
            state.mixerOut1, peak: state.mixerOut1Peak, showClip: true,
            level2: state.mixerOut2, peak2: state.mixerOut2Peak)
        knob(&context, x + w / 2, y + 268, 22, PanelModel.knobAngle(state.mixOutput))
        mixerGain(&context, state.mix1Text, x + w / 2 + 28, y + 260)
        text(&context, "INPUT 1", x, y + 294, w, 16, 12, label, bold: true)
        knob(&context, x + w / 2, y + 348, 22, PanelModel.knobAngle(state.mixInput1))
        mixerGain(&context, state.mix2Text, x + w / 2 + 28, y + 340)
        text(&context, "INPUT 2", x, y + 374, w, 16, 12, label, bold: true)
        knob(&context, x + w / 2, y + 430, 22, PanelModel.knobAngle(state.mixInput2))
        mixerGain(&context, state.mix3Text, x + w / 2 + 28, y + 422)
        text(&context, "COAX (3/4)", x, y + 456, w, 16, 12, label, bold: true)
    }

    private static func mixerGain(
        _ context: inout GraphicsContext,
        _ value: String,
        _ x: CGFloat, _ y: CGFloat
    ) {
        guard !value.isEmpty else { return }
        text(&context, value, x, y, 64, 16, 11, ink, bold: true, left: true)
    }

    private static func footer(_ context: inout GraphicsContext, state: PanelModel.DrawState) {
        let footerY: CGFloat = 558
        fill(&context, rect(0, footerY, designWidth, designHeight - footerY), Color(hex: 0x242424))
        strokeLine(&context, 0, footerY, designWidth, footerY, Color(hex: 0x111111), 1)
        text(&context, "SAMPLE RATE", 16, footerY, 130, 34, 13, label, bold: true, left: true)
        fill(&context, rect(150, footerY + 5, 110, 24, radius: 3), Color(hex: 0x111111))
        strokeRect(&context, 150, footerY + 5, 110, 24, 3, Color(hex: 0x666666), 1)
        text(&context, state.sampleRateText, 154, footerY + 5, 80, 24, 12, ink, bold: true, left: true)

        var arrow = Path()
        arrow.move(to: point(236, footerY + 14))
        arrow.addLine(to: point(246, footerY + 14))
        arrow.addLine(to: point(241, footerY + 20))
        arrow.closeSubpath()
        context.fill(arrow, with: .color(ink))

        text(&context, "CLOCK", 290, footerY, 70, 34, 13, label, bold: true, left: true)
        text(&context, "INTERNAL", 360, footerY, 120, 34, 14, clock, bold: true, left: true)
        if !state.loCutStatus.isEmpty {
            text(&context, state.loCutStatus, 488, footerY, 270, 34, 10, Color(hex: 0xFF8A80), bold: false, left: true)
        }

        let live = state.connected
        fill(&context, circle(766, footerY + 17, 5), live ? lcdGreen : Color(hex: 0xFF3A32))
        text(
            &context,
            live ? "IN LIVE" : "OFFLINE",
            776, footerY, 70, 34, 11, live ? lcdGreen : Color(hex: 0xFF8A80),
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
        _ label: String, caption: String = "", gray: Bool
    ) {
        fill(&context, rect(x + 2, y + 2, w, h, radius: 4), Color(hex: 0x1A1A1A))
        fill(&context, rect(x, y, w, h, radius: 4), gray ? grayButton : pink)
        strokeRect(&context, x, y, w, h, 4, gray ? grayEdge : pinkEdge, 1)
        strokeLine(&context, x + 4, y + 2, x + w - 4, y + 2, .white, 1)
        if caption.isEmpty {
            text(&context, label, x, y, w, h, 10, Color(hex: 0x2A2A2A), bold: true)
        } else {
            text(&context, label, x, y + 2, w, h / 2, 9, Color(hex: 0x2A2A2A), bold: true)
            text(&context, caption, x, y + h / 2 - 2, w, h / 2, 8, Color(hex: 0x2A2A2A), bold: false)
        }
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
        _ level: CGFloat, peak: CGFloat, showClip: Bool,
        level2: CGFloat? = nil, peak2: CGFloat? = nil
    ) {
        let labels = showClip
            ? ["CLIP", "0", "-2", "-6", "-12", "-24", "-48"]
            : ["0", "-2", "-6", "-12", "-24", "-48"]
        let trackX = x + 36
        drawMeterTrack(&context, trackX, y, h, level, peak: peak)
        if let level2 {
            drawMeterTrack(&context, trackX + 16, y, h, level2, peak: peak2 ?? level2)
            text(&context, "1", trackX - 2, y + h, 16, 12, 8, label, bold: true)
            text(&context, "2", trackX + 14, y + h, 16, 12, 8, label, bold: true)
        }

        for index in labels.indices {
            let ly = y + CGFloat(index) * ((h - 10) / CGFloat(labels.count - 1))
            text(&context, labels[index], x, ly - 6, 34, 12, 8, label, bold: false)
        }
    }

    private static func drawMeterTrack(
        _ context: inout GraphicsContext,
        _ trackX: CGFloat, _ y: CGFloat, _ h: CGFloat,
        _ level: CGFloat, peak: CGFloat
    ) {
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
    }

    /// Visor do compressor. O quadrado é só a grade; a escala -60…0 fica
    /// fora dele, embaixo e à direita, como no painel da Roland.
    /// A curva usa o GATE, o THRESHOLD, o RATIO e o GAIN desta faixa.
    private static func graph(
        _ context: inout GraphicsContext,
        _ x: CGFloat, _ y: CGFloat, _ w: CGFloat, _ h: CGFloat,
        gate: Double, threshold: Double, ratio: Double, gain: Double, bypassed: Bool
    ) {
        let plot = min(w, h)
        let marks = ["-60", "-48", "-36", "-24", "-12", "0"]
        let steps = CGFloat(marks.count - 1)
        let gateStep = min(50, max(0, Int((min(1, max(0, gate)) * 50).rounded())))
        let gateDb = -70.0 + Double(gateStep)
        let gateKnee = gateDb > -60
        let thresholdStep = min(50, max(0, Int((min(1, max(0, threshold)) * 50).rounded())))
        let thresholdDb = Double(thresholdStep) - 50
        let ratioStep = min(8, max(0, Int((min(1, max(0, ratio)) * 8).rounded())))
        let ratioDivisor = [1.0, 1.2, 1.5, 2.0, 2.8, 4.0, 8.0, 16.0, 0][ratioStep]
        let gainStep = min(74, max(0, Int((min(1, max(0, gain)) * 74).rounded())))
        let gainDb = Double(gainStep) - 50

        func outputDb(_ input: Double) -> Double {
            if input <= thresholdDb {
                return input + gainDb
            }
            if ratioDivisor == 0 {
                return thresholdDb + gainDb
            }
            return thresholdDb + (input - thresholdDb) / ratioDivisor + gainDb
        }

        func plotX(_ db: Double) -> CGFloat {
            let fraction = min(1, max(0, (db + 60) / 60))
            return x + CGFloat(fraction) * plot
        }

        func plotY(_ db: Double) -> CGFloat {
            let fraction = min(1, max(0, (db + 60) / 60))
            return y + plot - CGFloat(fraction) * plot
        }

        var samples: [(Double, Double)] = []
        if gateKnee {
            samples.append((gateDb, -60))
            samples.append((gateDb, outputDb(gateDb)))
            if gateDb < thresholdDb {
                samples.append((thresholdDb, outputDb(thresholdDb)))
            }
        } else {
            samples.append((-60, outputDb(-60)))
            if thresholdDb > -60 {
                samples.append((thresholdDb, outputDb(thresholdDb)))
            }
        }
        if samples.last?.0 != 0 {
            samples.append((0, outputDb(0)))
        }
        samples = pinnedToGraph(samples)

        let field = Color(hex: bypassed ? 0x8C8882 : 0xE39B45)
        let under = Color(hex: bypassed ? 0x5C5854 : 0xC4621E)
        let grid = Color(hex: bypassed ? 0x3E3C3A : 0x8A3A12)
        let curveColor = Color(hex: bypassed ? 0xC8C4BE : 0xFFF8EC)
        let scale = bypassed ? Color(hex: 0x8A8680) : label

        fill(&context, Path(CGRect(x: x, y: y, width: plot, height: plot)), field)

        var lower = Path()
        lower.move(to: point(plotX(samples[0].0), plotY(samples[0].1)))
        for sample in samples.dropFirst() {
            lower.addLine(to: point(plotX(sample.0), plotY(sample.1)))
        }
        lower.addLine(to: point(x + plot, y + plot))
        lower.addLine(to: point(plotX(samples[0].0), y + plot))
        lower.closeSubpath()
        context.fill(lower, with: .color(under))

        for index in marks.indices {
            let t = CGFloat(index) / steps
            let gx = x + t * plot
            let gy = y + plot - t * plot
            strokeLine(&context, gx, y, gx, y + plot, grid, 1)
            strokeLine(&context, x, gy, x + plot, gy, grid, 1)
            text(&context, marks[index], gx - 13, y + plot + 2, 26, 12, 8, scale, bold: false)
            text(
                &context, marks[index], x + plot + 4, gy - 6, 24, 12, 8, scale,
                bold: false, left: true)
        }

        var curve = Path()
        curve.move(to: point(plotX(samples[0].0), plotY(samples[0].1)))
        for sample in samples.dropFirst() {
            curve.addLine(to: point(plotX(sample.0), plotY(sample.1)))
        }
        context.stroke(curve, with: .color(curveColor), lineWidth: 1.5)
    }

    /// Mantém no quadrado o trecho em que a saída sai de -60…0 dB.
    private static func pinnedToGraph(_ raw: [(Double, Double)]) -> [(Double, Double)] {
        guard var previous = raw.first else { return [] }
        var pinned = [previous]
        for sample in raw.dropFirst() {
            let edges: [Double] = previous.1 <= sample.1 ? [-60, 0] : [0, -60]
            for edge in edges {
                let from = previous.1 - edge
                let to = sample.1 - edge
                if from * to < 0 {
                    let t = (edge - previous.1) / (sample.1 - previous.1)
                    pinned.append((previous.0 + t * (sample.0 - previous.0), edge))
                }
            }
            pinned.append(sample)
            previous = sample
        }
        return pinned
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
