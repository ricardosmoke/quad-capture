import Foundation
import SwiftUI

/// Estado do painel: knobs interativos + meters derivados do áudio ao vivo.
@MainActor
final class PanelModel: ObservableObject {
    struct CompStrip: Equatable, Sendable {
        var bypass: Bool = false
        var gate: Double = 0.15
        var threshold: Double = 0.55
        var ratio: Double = 0.35
        var attack: Double = 0.25
        var release: Double = 0.45
        var gain: Double = 0.5
    }

    /// Snapshot puro para o Canvas (evita isolamento MainActor no draw).
    struct DrawState: Equatable, Sendable {
        var status: String = "Procurando…"
        var connected: Bool = false
        var sens1: Double = 0.72
        var sens2: Double = 0.55
        var sens1Text: String = "—"
        var sens2Text: String = "—"
        var autoSensText: String = "—"
        var loCut1: Bool = false
        var loCut2: Bool = false
        var phase1: Bool = false
        var phase2: Bool = false
        var loCutStatus: String = ""
        var linkOn: Bool = false
        var comp1 = CompStrip()
        var comp2 = CompStrip()
        var mixOutput: Double = 0.7
        var mixInput1: Double = 0.65
        var mixInput2: Double = 0.65
        var pre1: CGFloat = 0
        var pre1Peak: CGFloat = 0
        var pre2: CGFloat = 0
        var pre2Peak: CGFloat = 0
        var gr1: CGFloat = 0.02
        var gr2: CGFloat = 0.02
        var compOut1: CGFloat = 0
        var compOut1Peak: CGFloat = 0
        var compOut2: CGFloat = 0
        var compOut2Peak: CGFloat = 0
        var mixerOut: CGFloat = 0
        var mixerOutPeak: CGFloat = 0
        var sampleRateText: String = "—"
    }

    @Published var sens1: Double = 0
    @Published var sens2: Double = 0
    private var sens1Known = false
    private var sens2Known = false
    @Published var autoSensText: String = "—"
    @Published var loCut1 = false
    @Published var loCut2 = false
    @Published var phase1 = false
    @Published var phase2 = false
    @Published var loCutStatus = ""
    @Published var linkOn = false
    private var loCutBusy = false
    private var autoSensEpoch = 0
    private var sensHold = [false, false]
    private var sensQueuedStep = [-1, -1]
    private var sensInFlight = [false, false]
    private var sensEpoch = [0, 0]
    private var sensBaseline = [-1, -1]
    private var sensAccepted = [false, false]
    private var gateQueuedStep = [-1, -1]
    private var gateInFlight = [false, false]
    private var sawConnected = false
    private var stateRequested = false
    private var switchGen = 0
    @Published var comp1 = CompStrip()
    @Published var comp2 = CompStrip()
    @Published var mixOutput: Double = 0.7
    @Published var mixInput1: Double = 0.65
    @Published var mixInput2: Double = 0.65
    @Published var mixCoax: Double = 0.4
    @Published var levels = InputLevelMonitor.Snapshot()

    private let monitor = InputLevelMonitor()
    private var cancellable: Timer?
    private var sensBusy = false

    var drawState: DrawState {
        var s = DrawState()
        s.status = levels.status
        s.connected = levels.connected
        s.sens1 = sens1
        s.sens2 = sens2
        s.sens1Text = sens1Known ? String(HardwareGain.db(fromNormalized: sens1)) : "—"
        s.sens2Text = sens2Known ? String(HardwareGain.db(fromNormalized: sens2)) : "—"
        s.autoSensText = autoSensText
        s.loCut1 = loCut1
        s.loCut2 = loCut2
        s.phase1 = phase1
        s.phase2 = phase2
        s.loCutStatus = loCutStatus
        s.linkOn = linkOn
        s.comp1 = comp1
        s.comp2 = comp2
        s.mixOutput = mixOutput
        s.mixInput1 = mixInput1
        s.mixInput2 = mixInput2
        s.pre1 = preampLevel(0)
        s.pre1Peak = preampPeak(0)
        s.pre2 = preampLevel(1)
        s.pre2Peak = preampPeak(1)
        s.gr1 = gainReduction(comp1, inputLevel: s.pre1)
        s.gr2 = gainReduction(comp2, inputLevel: s.pre2)
        s.compOut1 = compOutLevel(comp1, channel: 0)
        s.compOut1Peak = compOutPeak(comp1, channel: 0)
        s.compOut2 = compOutLevel(comp2, channel: 1)
        s.compOut2Peak = compOutPeak(comp2, channel: 1)
        s.mixerOut = mixerOutputLevel
        s.mixerOutPeak = mixerOutputPeak
        s.sampleRateText = UA55Device.label(for: levels.sampleRateHz)
        return s
    }

    func setSampleRate(_ hz: Double) {
        monitor.setSampleRate(hz)
    }

    /// Canal 0 da tela é o SysEx canal 0; canal 1 da tela é o SysEx canal 1.
    func toggleLoCut(channel: Int) {
        guard channel == 0 || channel == 1, !loCutBusy else { return }
        let turningOn = channel == 0 ? !loCut1 : !loCut2
        loCutBusy = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.send(channel: UInt8(channel), on: turningOn)
            Task { @MainActor in
                guard let self else { return }
                self.finishControl(error) {
                    if channel == 0 {
                        self.loCut1 = turningOn
                    } else {
                        self.loCut2 = turningOn
                    }
                }
            }
        }
    }

    /// Canal 0 da tela é o SysEx canal 0; canal 1 da tela é o SysEx canal 1.
    func togglePhase(channel: Int) {
        guard channel == 0 || channel == 1, !loCutBusy else { return }
        let turningOn = channel == 0 ? !phase1 : !phase2
        loCutBusy = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendPhase(channel: UInt8(channel), on: turningOn)
            Task { @MainActor in
                guard let self else { return }
                self.finishControl(error) {
                    if channel == 0 {
                        self.phase1 = turningOn
                    } else {
                        self.phase2 = turningOn
                    }
                }
            }
        }
    }

    /// Faixa 1 da tela é o canal 1 da placa. 01 é bypass; 00 deixa o compressor ativo.
    /// Com o LINK ligado, o clique na faixa 1 também clica a faixa 2 antes da leitura.
    func toggleBypass(channel: Int) {
        guard channel == 0 || channel == 1, !loCutBusy else { return }
        let turningOn = channel == 0 ? !comp1.bypass : !comp2.bypass
        let deviceChannel: UInt8 = channel == 0 ? 1 : 0
        let alsoBypass2 = channel == 0 && linkOn
        let bypass2On = !comp2.bypass
        loCutBusy = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendBypass(channel: deviceChannel, on: turningOn)
            let followError: String? = (error == nil && alsoBypass2)
                ? LoCutMIDI.sendBypass(channel: 0, on: bypass2On)
                : nil
            Task { @MainActor in
                guard let self else { return }
                if let error {
                    self.loCutBusy = false
                    self.loCutStatus = error
                    return
                }
                if channel == 0 {
                    self.comp1.bypass = turningOn
                } else {
                    self.comp2.bypass = turningOn
                }
                if let followError {
                    self.loCutBusy = false
                    self.loCutStatus = followError
                    return
                }
                if alsoBypass2 {
                    self.comp2.bypass = bypass2On
                }
                self.loCutStatus = ""
                self.loCutBusy = false
                self.refreshButtonsFromBoard()
            }
        }
    }

    /// Um único LINK. 01 liga as duas faixas do compressor.
    /// A placa, ao ligar o LINK, copia o canal 0 para o canal 1. O canal 0 é o BYPASS 2
    /// da tela, então o BYPASS 2 viraria mestre. O BYPASS 1 é o mestre: o 2 recebe o valor
    /// dele antes do LINK, e a leitura só sai depois dos dois envios.
    func toggleLink() {
        guard !loCutBusy else { return }
        let turningOn = !linkOn
        let masterOn = comp1.bypass
        loCutBusy = true
        PanelWork.queue.async { [weak self] in
            let followError: String? = turningOn ? LoCutMIDI.sendBypass(channel: 0, on: masterOn) : nil
            let error = followError == nil ? LoCutMIDI.sendLink(on: turningOn) : nil
            Task { @MainActor in
                guard let self else { return }
                if let followError {
                    self.loCutBusy = false
                    self.loCutStatus = followError
                    return
                }
                if turningOn {
                    self.comp2.bypass = masterOn
                    self.comp2.gate = self.comp1.gate
                    self.comp2.threshold = self.comp1.threshold
                    self.comp2.ratio = self.comp1.ratio
                    self.comp2.attack = self.comp1.attack
                    self.comp2.release = self.comp1.release
                    self.comp2.gain = self.comp1.gain
                }
                if let error {
                    self.loCutBusy = false
                    self.loCutStatus = error
                    return
                }
                self.linkOn = turningOn
                self.loCutStatus = ""
                self.loCutBusy = false
                self.refreshButtonsFromBoard()
            }
        }
    }

    /// Sempre envia o dado 01. A placa liga no primeiro clique e desliga no seguinte.
    func pressAutoSens() {
        guard !loCutBusy else { return }
        loCutBusy = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendAutoSens()
            Task { @MainActor in
                guard let self else { return }
                self.finishControl(error) {
                    self.autoSensEpoch += 1
                    self.autoSensText = self.autoSensText == "on" ? "off" : "on"
                    HardwareGain.rememberAutoSens(self.autoSensText)
                }
            }
        }
    }

    /// O envio terminou. Se a placa aceitou, os botões voltam a ser lidos dela.
    private func finishControl(_ error: String?, apply: () -> Void) {
        loCutBusy = false
        if let error {
            loCutStatus = error
            return
        }
        apply()
        loCutStatus = ""
        refreshButtonsFromBoard()
    }

    /// Arraste do knob. O passo é o dB mostrado vezes 2. A leitura da placa não passa por aqui.
    func userSetSens(channel: Int, normalized: Double) {
        guard channel == 0 || channel == 1 else { return }
        let clamped = min(1, max(0, normalized))
        let db = HardwareGain.db(fromNormalized: clamped)
        let step = min(108, max(0, db * 2))
        if sensQueuedStep[channel] == step {
            if channel == 0 {
                sens1 = clamped
                sens1Known = true
            } else {
                sens2 = clamped
                sens2Known = true
            }
            return
        }
        if !sensHold[channel] {
            sensBaseline[channel] = displayedSensDb(channel)
        }
        sensEpoch[channel] += 1
        sensAccepted[channel] = false
        if channel == 0 {
            sens1 = clamped
            sens1Known = true
        } else {
            sens2 = clamped
            sens2Known = true
        }
        sensQueuedStep[channel] = step
        sensHold[channel] = true
        pumpSens(channel)
    }

    private func displayedSensDb(_ channel: Int) -> Int {
        if channel == 0 {
            return sens1Known ? HardwareGain.db(fromNormalized: sens1) : -1
        }
        return sens2Known ? HardwareGain.db(fromNormalized: sens2) : -1
    }

    private func pumpSens(_ channel: Int) {
        guard !sensInFlight[channel] else { return }
        let step = sensQueuedStep[channel]
        guard step >= 0 else { return }
        sensInFlight[channel] = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendSens(channel: UInt8(channel), step: UInt8(step))
            Task { @MainActor in
                guard let self else { return }
                self.sensInFlight[channel] = false
                if let error {
                    self.sensAccepted[channel] = false
                    self.sensHold[channel] = true
                    self.loCutStatus = error
                } else if self.sensQueuedStep[channel] == step {
                    self.sensAccepted[channel] = true
                    self.loCutStatus = ""
                }
                if self.sensQueuedStep[channel] != step {
                    self.pumpSens(channel)
                }
            }
        }
    }

    func start() {
        PanelLog.write("PanelModel.start")
        showButtonsOff()
        monitor.start()
        HardwareGain.start()
        cancellable = Timer.scheduledTimer(withTimeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
            Task { @MainActor in
                guard let self else { return }
                self.levels = self.monitor.snapshot
                self.noteBoard(self.levels.connected)
                self.applySetupSwitches()
                guard !self.sensBusy else { return }
                self.sensBusy = true
                let epoch = self.autoSensEpoch
                let sensEpoch = self.sensEpoch
                PanelWork.queue.async { [weak self] in
                    let reading = PanelLog.measure("readSens") { HardwareGain.readSens() }
                    let device = HardwareGain.deviceText()
                    Task { @MainActor in
                        guard let self else { return }
                        self.sensBusy = false
                        self.applyHardwareSens(reading, sampledEpoch: sensEpoch)
                        if self.autoSensEpoch == epoch, device != self.autoSensText {
                            self.autoSensText = device
                        }
                    }
                }
            }
        }
    }

    /// Sobe uma vez quando a placa aparece e de novo depois que ela some.
    private func noteBoard(_ connected: Bool) {
        if connected == sawConnected {
            return
        }
        sawConnected = connected
        if !connected {
            stateRequested = false
            return
        }
        guard !stateRequested else { return }
        stateRequested = true
        showButtonsOff()
        requestBoardButtons()
    }

    /// O mesmo pedido da abertura: bloco de 59 bytes, os dois BYPASS e o LINK.
    private func requestBoardButtons() {
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.requestState()
            let bypassError = error == nil ? LoCutMIDI.requestBypass(channel: 0) : nil
            let bypassError2 = error == nil ? LoCutMIDI.requestBypass(channel: 1) : nil
            let linkError = error == nil ? LoCutMIDI.requestLink() : nil
            let gateError = error == nil ? LoCutMIDI.requestGate(channel: 0) : nil
            let gateError2 = error == nil ? LoCutMIDI.requestGate(channel: 1) : nil
            if error == nil {
                HardwareGain.collectState()
            }
            Task { @MainActor in
                guard let self else { return }
                if let error {
                    self.loCutStatus = error
                } else if let bypassError {
                    self.loCutStatus = bypassError
                } else if let bypassError2 {
                    self.loCutStatus = bypassError2
                } else if let linkError {
                    self.loCutStatus = linkError
                } else if let gateError {
                    self.loCutStatus = gateError
                } else if let gateError2 {
                    self.loCutStatus = gateError2
                }
            }
        }
    }

    /// Depois do clique, sem apagar o desenho: a resposta desta leitura substitui o que estiver na tela.
    private func refreshButtonsFromBoard() {
        HardwareGain.armRefresh()
        requestBoardButtons()
    }

    /// Antes da leitura desta conexão, LO-CUT, PHASE, AUTO-SENS, BYPASS e LINK ficam off.
    private func showButtonsOff() {
        autoSensEpoch += 1
        loCut1 = false
        loCut2 = false
        phase1 = false
        phase2 = false
        autoSensText = "off"
        comp1.bypass = false
        comp2.bypass = false
        linkOn = false
        HardwareGain.beginRead()
    }

    /// O DT1 de 59 bytes só escreve o desenho. Não passa pelos envios.
    private func applySetupSwitches() {
        let shown = HardwareGain.preampSwitches()
        guard shown.generation != switchGen, !loCutBusy else { return }
        switchGen = shown.generation
        if let value = shown.loCut1 { loCut1 = value }
        if let value = shown.loCut2 { loCut2 = value }
        if let value = shown.phase1 { phase1 = value }
        if let value = shown.phase2 { phase2 = value }
        if let value = shown.bypass1 { comp1.bypass = value }
        if let value = shown.bypass2 { comp2.bypass = value }
        if let value = shown.link { linkOn = value }
        if let step = shown.gate1 {
            comp1.gate = Double(step) / 50.0
            gateQueuedStep[0] = step
        }
        if let step = shown.gate2 {
            comp2.gate = Double(step) / 50.0
            gateQueuedStep[1] = step
        }
    }

    func stop() {
        cancellable?.invalidate()
        cancellable = nil
        HardwareGain.stop()
        monitor.stop()
    }

    private func applyHardwareSens(_ reading: (Int, Int)?, sampledEpoch: [Int]) {
        guard let reading else { return }
        applyHardwareSensChannel(0, db: reading.0, sampledEpoch: sampledEpoch[0])
        applyHardwareSensChannel(1, db: reading.1, sampledEpoch: sampledEpoch[1])
    }

    private func applyHardwareSensChannel(_ channel: Int, db: Int, sampledEpoch: Int) {
        guard db <= Int(HardwareGain.sensMaxDb) else { return }
        if sampledEpoch != sensEpoch[channel] {
            return
        }
        let step = min(108, max(0, db * 2))
        if sensHold[channel] {
            if step == sensQueuedStep[channel] {
                sensHold[channel] = false
            } else if sensAccepted[channel], db != sensBaseline[channel] {
                sensHold[channel] = false
            } else {
                return
            }
        }
        let normalized = HardwareGain.normalized(fromDb: db)
        if channel == 0 {
            sens1 = normalized
            sens1Known = true
        } else {
            sens2 = normalized
            sens2Known = true
        }
    }

    /// Com o LINK ligado, a faixa 2 usa os mesmos knobs da faixa 1. O BYPASS não entra aqui.
    func setCompKnob(channel: Int, _ key: WritableKeyPath<CompStrip, Double>, _ value: Double) {
        let clamped = min(1, max(0, value))
        let write1 = channel == 0 || linkOn
        let write2 = channel == 1 || linkOn
        if key == \.gate {
            if write1 { queueGate(channel: 0, normalized: clamped) }
            if write2 { queueGate(channel: 1, normalized: clamped) }
            return
        }
        if write1 { comp1[keyPath: key] = clamped }
        if write2 { comp2[keyPath: key] = clamped }
    }

    /// Faixa 1 envia o canal 0. O passo 0 é -INF; 50 é -20 dB.
    private func queueGate(channel: Int, normalized: Double) {
        let step = min(50, max(0, Int((normalized * 50).rounded())))
        if channel == 0 {
            comp1.gate = normalized
        } else {
            comp2.gate = normalized
        }
        guard gateQueuedStep[channel] != step else { return }
        gateQueuedStep[channel] = step
        pumpGate(channel)
    }

    private func pumpGate(_ channel: Int) {
        guard !gateInFlight[channel] else { return }
        let step = gateQueuedStep[channel]
        guard step >= 0 else { return }
        gateInFlight[channel] = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendGate(channel: UInt8(channel), step: UInt8(step))
            Task { @MainActor in
                guard let self else { return }
                self.gateInFlight[channel] = false
                if let error {
                    self.loCutStatus = error
                } else if self.gateQueuedStep[channel] == step {
                    self.loCutStatus = ""
                }
                if self.gateQueuedStep[channel] != step {
                    self.pumpGate(channel)
                }
            }
        }
    }

    nonisolated static func knobAngle(_ value: Double) -> CGFloat {
        CGFloat(value * 270.0 - 135.0)
    }

    func preampLevel(_ channel: Int) -> CGFloat {
        guard channel < levels.levels.count else { return 0 }
        return levels.levels[channel]
    }

    func preampPeak(_ channel: Int) -> CGFloat {
        guard channel < levels.peaks.count else { return 0 }
        return levels.peaks[channel]
    }

    func gainReduction(_ strip: CompStrip, inputLevel: CGFloat) -> CGFloat {
        guard !strip.bypass else { return 0.02 }
        let thr = CGFloat(strip.threshold)
        guard inputLevel > thr else { return 0.02 }
        let ratio = 1.0 + strip.ratio * 11.0
        let over = Double(inputLevel - thr)
        let reduced = over * (1.0 - 1.0 / ratio)
        return CGFloat(min(1, max(0.02, reduced * 1.8)))
    }

    func compOutLevel(_ strip: CompStrip, channel: Int) -> CGFloat {
        let input = preampLevel(channel)
        let gr = gainReduction(strip, inputLevel: input)
        let afterGR = max(0, input - gr * 0.65)
        let makeup = CGFloat(0.7 + strip.gain * 0.6)
        return min(1, afterGR * makeup)
    }

    func compOutPeak(_ strip: CompStrip, channel: Int) -> CGFloat {
        let input = preampPeak(channel)
        let gr = gainReduction(strip, inputLevel: input)
        let afterGR = max(0, input - gr * 0.65)
        let makeup = CGFloat(0.7 + strip.gain * 0.6)
        return min(1, afterGR * makeup)
    }

    var mixerOutputLevel: CGFloat {
        let i1 = preampLevel(0) * CGFloat(mixInput1)
        let i2 = preampLevel(1) * CGFloat(mixInput2)
        let coaxL = (levels.levels.count > 2 ? levels.levels[2] : 0) * CGFloat(mixCoax)
        let coaxR = (levels.levels.count > 3 ? levels.levels[3] : 0) * CGFloat(mixCoax)
        let mixed = max(i1, i2, coaxL, coaxR)
        return min(1, mixed * CGFloat(0.5 + mixOutput * 0.7))
    }

    var mixerOutputPeak: CGFloat {
        let i1 = preampPeak(0) * CGFloat(mixInput1)
        let i2 = preampPeak(1) * CGFloat(mixInput2)
        let coaxL = (levels.peaks.count > 2 ? levels.peaks[2] : 0) * CGFloat(mixCoax)
        let coaxR = (levels.peaks.count > 3 ? levels.peaks[3] : 0) * CGFloat(mixCoax)
        let mixed = max(i1, i2, coaxL, coaxR)
        return min(1, mixed * CGFloat(0.5 + mixOutput * 0.7))
    }
}

enum KnobID: Hashable {
    case sens1, sens2
    case comp1Gate, comp1Threshold, comp1Ratio, comp1Attack, comp1Release, comp1Gain
    case comp2Gate, comp2Threshold, comp2Ratio, comp2Attack, comp2Release, comp2Gain
    case mixOutput, mixInput1, mixInput2
}
