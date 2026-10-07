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
        var mix1Text: String = ""
        var mix2Text: String = ""
        var mix3Text: String = ""
        var comp1Text: [String] = ["", "", "", "", "", ""]
        var comp2Text: [String] = ["", "", "", "", "", ""]
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
        var mixerOut1: CGFloat = 0
        var mixerOut1Peak: CGFloat = 0
        var mixerOut2: CGFloat = 0
        var mixerOut2Peak: CGFloat = 0
        var sampleRateText: String = "—"
        var webURL: String = ""
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
    private var sensRefresh: DispatchWorkItem?
    private var mixerQueued = [-1, -1, -1]
    private var mixerInFlight = [false, false, false]
    private var sensEpoch = [0, 0]
    private var sensBaseline = [-1, -1]
    private var sensAccepted = [false, false]
    private var gateQueuedStep = [-1, -1]
    private var gateInFlight = [false, false]
    private var thresholdQueuedStep = [-1, -1]
    private var thresholdInFlight = [false, false]
    private var ratioQueuedStep = [-1, -1]
    private var ratioInFlight = [false, false]
    private var attackQueuedStep = [-1, -1]
    private var attackInFlight = [false, false]
    private var releaseQueuedStep = [-1, -1]
    private var releaseInFlight = [false, false]
    private var gainQueuedStep = [-1, -1]
    private var gainInFlight = [false, false]
    private var sawConnected = false
    private var stateRequested = false
    private var switchGen = 0
    @Published var comp1 = CompStrip()
    @Published var comp2 = CompStrip()
    @Published var mixOutput: Double = 0.7
    @Published var mixInput1: Double = 0.65
    @Published var mixInput2: Double = 0.65
    @Published var mixCoax: Double = 0.4
    @Published private var mixerDragIndex: Int?
    @Published private var compDrag: KnobID?
    @Published var levels = InputLevelMonitor.Snapshot()
    @Published var webURL = ""

    private let monitor = InputLevelMonitor()
    private let webServer = PanelWebServer()
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
        s.mix1Text = mixerDragIndex == 0 ? MixerLevel.display(normalized: mixOutput) : ""
        s.mix2Text = mixerDragIndex == 1 ? MixerLevel.display(normalized: mixInput1) : ""
        s.mix3Text = mixerDragIndex == 2 ? MixerLevel.display(normalized: mixInput2) : ""
        s.comp1Text = CompReadout.row(comp1, drag: compDrag, channel: 0)
        s.comp2Text = CompReadout.row(comp2, drag: compDrag, channel: 1)
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
        s.mixerOut1 = mixerOutput(0, peak: false)
        s.mixerOut1Peak = mixerOutput(0, peak: true)
        s.mixerOut2 = mixerOutput(1, peak: false)
        s.mixerOut2Peak = mixerOutput(1, peak: true)
        s.sampleRateText = UA55Device.label(for: levels.sampleRateHz)
        s.webURL = webURL
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

    /// O BYPASS 1 da tela é o canal 0 da placa. 00 liga o botão e 01 desliga.
    /// Com o LINK ligado, o outro BYPASS recebe o mesmo estado.
    func toggleBypass(channel: Int) {
        guard channel == 0 || channel == 1, !loCutBusy else { return }
        let turningOn = channel == 0 ? !comp1.bypass : !comp2.bypass
        let deviceChannel = UInt8(channel)
        let follow = linkOn
        let otherChannel = UInt8(channel == 0 ? 1 : 0)
        loCutBusy = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendBypass(channel: deviceChannel, on: !turningOn)
            let followError: String? = (error == nil && follow)
                ? LoCutMIDI.sendBypass(channel: otherChannel, on: !turningOn)
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
                if follow {
                    self.comp1.bypass = turningOn
                    self.comp2.bypass = turningOn
                }
                self.loCutStatus = ""
                self.loCutBusy = false
            }
        }
    }

    /// Um único LINK. Depois do envio, a placa devolve o estado de todos os botões.
    func toggleLink() {
        guard !loCutBusy else { return }
        let turningOn = !linkOn
        loCutBusy = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendLink(on: turningOn)
            Task { @MainActor in
                guard let self else { return }
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

    /// O número ao lado do knob do compressor aparece só enquanto o usuário segura o giro.
    func setCompDrag(_ id: KnobID, active: Bool) {
        if active {
            compDrag = id
        } else if compDrag == id {
            compDrag = nil
        }
    }

    /// O número ao lado do knob aparece só enquanto o usuário segura o giro.
    func setMixerDrag(_ index: Int, active: Bool) {
        if active {
            mixerDragIndex = index
        } else if mixerDragIndex == index {
            mixerDragIndex = nil
        }
    }

    /// O knob de cima é INPUT 1, o do meio é INPUT 2 e o de baixo é COAX. Só o giro do usuário envia.
    func userSetMixer(index: Int, normalized: Double) {
        guard index >= 0, index <= 2 else { return }
        let clamped = min(1, max(0, normalized))
        switch index {
        case 0: mixOutput = clamped
        case 1: mixInput1 = clamped
        default: mixInput2 = clamped
        }
        let position = MixerLevel.position(from: clamped)
        if mixerQueued[index] == position { return }
        mixerQueued[index] = position
        pumpMixer(index)
    }

    private func pumpMixer(_ index: Int) {
        guard !mixerInFlight[index] else { return }
        let position = mixerQueued[index]
        guard position >= 0 else { return }
        mixerInFlight[index] = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendMixer(index: UInt8(index), position: position)
            Task { @MainActor in
                guard let self else { return }
                self.mixerInFlight[index] = false
                if let error {
                    self.loCutStatus = error
                } else if self.mixerQueued[index] == position {
                    self.loCutStatus = ""
                }
                if self.mixerQueued[index] != position {
                    self.pumpMixer(index)
                }
            }
        }
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

    /// A placa liga o AUTO-SENS ao receber o SENS. A leitura começa quando o giro para.
    private func scheduleButtonRefreshAfterSens() {
        sensRefresh?.cancel()
        let work = DispatchWorkItem { [weak self] in
            guard let self else { return }
            if self.sensInFlight[0] || self.sensInFlight[1] {
                self.scheduleButtonRefreshAfterSens()
                return
            }
            self.refreshButtonsFromBoard()
        }
        sensRefresh = work
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.35, execute: work)
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
                    self.scheduleButtonRefreshAfterSens()
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
        webServer.start { [weak self] command in
            Task { @MainActor in
                self?.applyWeb(command)
            }
        }
        webURL = webServer.advertisedURL()
        cancellable = Timer.scheduledTimer(withTimeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
            Task { @MainActor in
                guard let self else { return }
                self.levels = self.monitor.snapshot
                self.noteBoard(self.levels.connected)
                self.applySetupSwitches()
                if self.webServer.hasClients {
                    self.webServer.broadcast(self.webSnapshotData())
                }
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
        requestBoardButtons(includeMixer: true)
    }

    /// O mesmo pedido da abertura: bloco de 59 bytes, os dois BYPASS e o LINK.
    private func requestBoardButtons(includeMixer: Bool = false) {
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.requestState()
            let bypassError = error == nil ? LoCutMIDI.requestBypass(channel: 0) : nil
            let bypassError2 = error == nil ? LoCutMIDI.requestBypass(channel: 1) : nil
            let linkError = error == nil ? LoCutMIDI.requestLink() : nil
            let gateError = error == nil ? LoCutMIDI.requestGate(channel: 0) : nil
            let gateError2 = error == nil ? LoCutMIDI.requestGate(channel: 1) : nil
            var mixerError: String?
            if includeMixer, error == nil {
                for index in UInt8(0)...2 {
                    if let failed = LoCutMIDI.requestMixer(index: index) {
                        mixerError = failed
                        break
                    }
                }
            }
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
                } else if let mixerError {
                    self.loCutStatus = mixerError
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

    /// A leitura da conexão posiciona o knob. Não devolve a amplitude para a placa.
    private func applyLoadedMixer(_ index: Int, _ position: Int?) {
        guard let position, mixerDragIndex != index, !mixerInFlight[index] else { return }
        let normalized = Double(position) / 1024.0
        switch index {
        case 0: mixOutput = normalized
        case 1: mixInput1 = normalized
        default: mixInput2 = normalized
        }
        mixerQueued[index] = position
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
        applyLoadedMixer(0, shown.mix1)
        applyLoadedMixer(1, shown.mix2)
        applyLoadedMixer(2, shown.mix3)
        let device = HardwareGain.deviceText()
        if device == "on" || device == "off" {
            autoSensText = device
        }
    }

    func stop() {
        sensRefresh?.cancel()
        cancellable?.invalidate()
        cancellable = nil
        webServer.stop()
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
        if key == \.threshold {
            if write1 { queueThreshold(channel: 0, normalized: clamped) }
            if write2 { queueThreshold(channel: 1, normalized: clamped) }
            return
        }
        if key == \.ratio {
            if write1 { queueRatio(channel: 0, normalized: clamped) }
            if write2 { queueRatio(channel: 1, normalized: clamped) }
            return
        }
        if key == \.attack {
            if write1 { queueAttack(channel: 0, normalized: clamped) }
            if write2 { queueAttack(channel: 1, normalized: clamped) }
            return
        }
        if key == \.release {
            if write1 { queueRelease(channel: 0, normalized: clamped) }
            if write2 { queueRelease(channel: 1, normalized: clamped) }
            return
        }
        if key == \.gain {
            if write1 { queueGain(channel: 0, normalized: clamped) }
            if write2 { queueGain(channel: 1, normalized: clamped) }
            return
        }
        if write1 { comp1[keyPath: key] = clamped }
        if write2 { comp2[keyPath: key] = clamped }
    }

    /// Faixa 1 envia o canal 0. O passo 0 é -50 dB; 50 é 0 dB.
    private func queueThreshold(channel: Int, normalized: Double) {
        let step = min(50, max(0, Int((normalized * 50).rounded())))
        if channel == 0 {
            comp1.threshold = normalized
        } else {
            comp2.threshold = normalized
        }
        guard thresholdQueuedStep[channel] != step else { return }
        thresholdQueuedStep[channel] = step
        pumpThreshold(channel)
    }

    private func pumpThreshold(_ channel: Int) {
        guard !thresholdInFlight[channel] else { return }
        let step = thresholdQueuedStep[channel]
        guard step >= 0 else { return }
        thresholdInFlight[channel] = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendThreshold(channel: UInt8(channel), step: UInt8(step))
            Task { @MainActor in
                guard let self else { return }
                self.thresholdInFlight[channel] = false
                if let error {
                    self.loCutStatus = error
                } else if self.thresholdQueuedStep[channel] == step {
                    self.loCutStatus = ""
                }
                if self.thresholdQueuedStep[channel] != step {
                    self.pumpThreshold(channel)
                }
            }
        }
    }

    /// Faixa 1 envia o canal 0. O passo 0 é 1:1.0; 8 é 1:INF.
    private func queueRatio(channel: Int, normalized: Double) {
        let step = min(8, max(0, Int((normalized * 8).rounded())))
        if channel == 0 {
            comp1.ratio = normalized
        } else {
            comp2.ratio = normalized
        }
        guard ratioQueuedStep[channel] != step else { return }
        ratioQueuedStep[channel] = step
        pumpRatio(channel)
    }

    private func pumpRatio(_ channel: Int) {
        guard !ratioInFlight[channel] else { return }
        let step = ratioQueuedStep[channel]
        guard step >= 0 else { return }
        ratioInFlight[channel] = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendRatio(channel: UInt8(channel), step: UInt8(step))
            Task { @MainActor in
                guard let self else { return }
                self.ratioInFlight[channel] = false
                if let error {
                    self.loCutStatus = error
                } else if self.ratioQueuedStep[channel] == step {
                    self.loCutStatus = ""
                }
                if self.ratioQueuedStep[channel] != step {
                    self.pumpRatio(channel)
                }
            }
        }
    }

    /// Faixa 1 envia o canal 0. O passo 0 é 0,2 ms; 25 é 100 ms.
    private func queueAttack(channel: Int, normalized: Double) {
        let step = min(25, max(0, Int((normalized * 25).rounded())))
        if channel == 0 {
            comp1.attack = normalized
        } else {
            comp2.attack = normalized
        }
        guard attackQueuedStep[channel] != step else { return }
        attackQueuedStep[channel] = step
        pumpAttack(channel)
    }

    private func pumpAttack(_ channel: Int) {
        guard !attackInFlight[channel] else { return }
        let step = attackQueuedStep[channel]
        guard step >= 0 else { return }
        attackInFlight[channel] = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendAttack(channel: UInt8(channel), step: UInt8(step))
            Task { @MainActor in
                guard let self else { return }
                self.attackInFlight[channel] = false
                if let error {
                    self.loCutStatus = error
                } else if self.attackQueuedStep[channel] == step {
                    self.loCutStatus = ""
                }
                if self.attackQueuedStep[channel] != step {
                    self.pumpAttack(channel)
                }
            }
        }
    }

    /// Faixa 1 envia o canal 0. O passo 0 é 10 ms; 45 é 500 ms.
    private func queueRelease(channel: Int, normalized: Double) {
        let step = min(45, max(0, Int((normalized * 45).rounded())))
        if channel == 0 {
            comp1.release = normalized
        } else {
            comp2.release = normalized
        }
        guard releaseQueuedStep[channel] != step else { return }
        releaseQueuedStep[channel] = step
        pumpRelease(channel)
    }

    private func pumpRelease(_ channel: Int) {
        guard !releaseInFlight[channel] else { return }
        let step = releaseQueuedStep[channel]
        guard step >= 0 else { return }
        releaseInFlight[channel] = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendRelease(channel: UInt8(channel), step: UInt8(step))
            Task { @MainActor in
                guard let self else { return }
                self.releaseInFlight[channel] = false
                if let error {
                    self.loCutStatus = error
                } else if self.releaseQueuedStep[channel] == step {
                    self.loCutStatus = ""
                }
                if self.releaseQueuedStep[channel] != step {
                    self.pumpRelease(channel)
                }
            }
        }
    }

    /// Faixa 1 envia o canal 0. O passo 0 é -50 dB; 50 é 0 dB; 74 é +24 dB.
    private func queueGain(channel: Int, normalized: Double) {
        let step = min(74, max(0, Int((normalized * 74).rounded())))
        if channel == 0 {
            comp1.gain = normalized
        } else {
            comp2.gain = normalized
        }
        guard gainQueuedStep[channel] != step else { return }
        gainQueuedStep[channel] = step
        pumpGain(channel)
    }

    private func pumpGain(_ channel: Int) {
        guard !gainInFlight[channel] else { return }
        let step = gainQueuedStep[channel]
        guard step >= 0 else { return }
        gainInFlight[channel] = true
        PanelWork.queue.async { [weak self] in
            let error = LoCutMIDI.sendGain(channel: UInt8(channel), step: UInt8(step))
            Task { @MainActor in
                guard let self else { return }
                self.gainInFlight[channel] = false
                if let error {
                    self.loCutStatus = error
                } else if self.gainQueuedStep[channel] == step {
                    self.loCutStatus = ""
                }
                if self.gainQueuedStep[channel] != step {
                    self.pumpGain(channel)
                }
            }
        }
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

    /// Output 1 segue a entrada 1. Output 2 segue a entrada 2.
    private func mixerOutput(_ channel: Int, peak: Bool) -> CGFloat {
        let input = peak ? preampPeak(channel) : preampLevel(channel)
        let knob: Double = channel == 0 ? mixInput1 : mixInput2
        let coaxIndex = 2 + channel
        let source = peak ? levels.peaks : levels.levels
        let coax = (source.count > coaxIndex ? source[coaxIndex] : 0) * CGFloat(mixCoax)
        let mixed = max(input * CGFloat(knob), coax)
        return min(1, mixed * CGFloat(0.5 + mixOutput * 0.7))
    }

    /// A página web usa os mesmos métodos do painel. Abrir a página não envia SysEx.
    private func applyWeb(_ command: [String: Any]) {
        switch command["op"] as? String {
        case "loCut"?:
            if let channel = webInt(command["channel"]) { toggleLoCut(channel: channel) }
        case "phase"?:
            if let channel = webInt(command["channel"]) { togglePhase(channel: channel) }
        case "autoSens"?:
            pressAutoSens()
        case "bypass"?:
            if let channel = webInt(command["channel"]) { toggleBypass(channel: channel) }
        case "link"?:
            toggleLink()
        case "sens"?:
            if let channel = webInt(command["channel"]), let value = webDouble(command["value"]) {
                userSetSens(channel: channel, normalized: value)
            }
        case "mixer"?:
            guard let index = webInt(command["index"]) else { return }
            if let active = command["active"] as? Bool {
                setMixerDrag(index, active: active)
            }
            if let value = webDouble(command["value"]) {
                userSetMixer(index: index, normalized: value)
            }
        case "comp"?:
            guard let channel = webInt(command["channel"]),
                  let name = command["knob"] as? String,
                  let key = compKey(name) else { return }
            if let active = command["active"] as? Bool, let id = compDragID(channel: channel, knob: name) {
                setCompDrag(id, active: active)
            }
            if let value = webDouble(command["value"]) {
                setCompKnob(channel: channel, key, value)
            }
        case "rate"?:
            if let hz = webDouble(command["hz"]) { setSampleRate(hz) }
        default:
            break
        }
    }

    private func webSnapshotData() -> Data {
        let state = drawState
        func strip(
            _ item: CompStrip, _ text: [String], _ gr: CGFloat, _ out: CGFloat, _ peak: CGFloat
        ) -> [String: Any] {
            [
                "bypass": item.bypass,
                "gate": item.gate,
                "threshold": item.threshold,
                "ratio": item.ratio,
                "attack": item.attack,
                "release": item.release,
                "gain": item.gain,
                "text": text,
                "gr": Double(gr),
                "out": Double(out),
                "outPeak": Double(peak),
            ]
        }
        let object: [String: Any] = [
            "settled": levels.settled,
            "connected": state.connected,
            "status": state.status,
            "sens": [state.sens1, state.sens2],
            "sensText": [state.sens1Text, state.sens2Text],
            "autoSens": state.autoSensText,
            "loCut": [state.loCut1, state.loCut2],
            "phase": [state.phase1, state.phase2],
            "link": state.linkOn,
            "error": state.loCutStatus,
            "comp": [
                strip(state.comp1, state.comp1Text, state.gr1, state.compOut1, state.compOut1Peak),
                strip(state.comp2, state.comp2Text, state.gr2, state.compOut2, state.compOut2Peak),
            ],
            "mix": [state.mixOutput, state.mixInput1, state.mixInput2],
            "mixText": [state.mix1Text, state.mix2Text, state.mix3Text],
            "pre": [
                ["level": Double(state.pre1), "peak": Double(state.pre1Peak)],
                ["level": Double(state.pre2), "peak": Double(state.pre2Peak)],
            ],
            "mixerOut": [
                ["level": Double(state.mixerOut1), "peak": Double(state.mixerOut1Peak)],
                ["level": Double(state.mixerOut2), "peak": Double(state.mixerOut2Peak)],
            ],
            "sampleRate": state.sampleRateText,
            "sampleRateHz": levels.sampleRateHz,
        ]
        return (try? JSONSerialization.data(withJSONObject: object)) ?? Data("{}".utf8)
    }

    private func compKey(_ name: String) -> WritableKeyPath<CompStrip, Double>? {
        switch name {
        case "gate": return \.gate
        case "threshold": return \.threshold
        case "ratio": return \.ratio
        case "attack": return \.attack
        case "release": return \.release
        case "gain": return \.gain
        default: return nil
        }
    }

    private func compDragID(channel: Int, knob: String) -> KnobID? {
        switch (channel, knob) {
        case (0, "gate"): return .comp1Gate
        case (0, "threshold"): return .comp1Threshold
        case (0, "ratio"): return .comp1Ratio
        case (0, "attack"): return .comp1Attack
        case (0, "release"): return .comp1Release
        case (0, "gain"): return .comp1Gain
        case (1, "gate"): return .comp2Gate
        case (1, "threshold"): return .comp2Threshold
        case (1, "ratio"): return .comp2Ratio
        case (1, "attack"): return .comp2Attack
        case (1, "release"): return .comp2Release
        case (1, "gain"): return .comp2Gain
        default: return nil
        }
    }

    private func webInt(_ value: Any?) -> Int? {
        (value as? NSNumber)?.intValue
    }

    private func webDouble(_ value: Any?) -> Double? {
        (value as? NSNumber)?.doubleValue
    }
}

enum KnobID: Hashable {
    case sens1, sens2
    case comp1Gate, comp1Threshold, comp1Ratio, comp1Attack, comp1Release, comp1Gain
    case comp2Gate, comp2Threshold, comp2Ratio, comp2Attack, comp2Release, comp2Gain
    case mixOutput, mixInput1, mixInput2

    var mixerIndex: Int? {
        switch self {
        case .mixOutput: return 0
        case .mixInput1: return 1
        case .mixInput2: return 2
        default: return nil
        }
    }

    var isCompressor: Bool {
        switch self {
        case .comp1Gate, .comp1Threshold, .comp1Ratio, .comp1Attack, .comp1Release, .comp1Gain,
             .comp2Gate, .comp2Threshold, .comp2Ratio, .comp2Attack, .comp2Release, .comp2Gain:
            return true
        default:
            return false
        }
    }
}

/// Valor mostrado à direita do knob do compressor. Usa o mesmo passo que o envio.
enum CompReadout {
    private static let ratio = ["1:1.0", "1:1.2", "1:1.5", "1:2.0", "1:2.8", "1:4.0", "1:8.0", "1:16", "1:INF"]
    private static let attack = [0.2, 0.5, 1.0, 2.0, 4.0, 6.0, 8.0, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100]
    private static let release = [10, 12, 15, 18, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380, 400, 420, 440, 460, 480, 500]

    static func row(_ strip: PanelModel.CompStrip, drag: KnobID?, channel: Int) -> [String] {
        let ids: [KnobID] = channel == 0
            ? [.comp1Gate, .comp1Threshold, .comp1Ratio, .comp1Attack, .comp1Release, .comp1Gain]
            : [.comp2Gate, .comp2Threshold, .comp2Ratio, .comp2Attack, .comp2Release, .comp2Gain]
        let values = [strip.gate, strip.threshold, strip.ratio, strip.attack, strip.release, strip.gain]
        return zip(ids, values).map { id, value in
            drag == id ? text(id, value) : ""
        }
    }

    private static func text(_ id: KnobID, _ normalized: Double) -> String {
        switch id {
        case .comp1Gate, .comp2Gate:
            let step = step(normalized, max: 50)
            return step == 0 ? "-INF" : String(format: "%.1f", Double(step - 70))
        case .comp1Threshold, .comp2Threshold:
            return decibels(step(normalized, max: 50) - 50)
        case .comp1Ratio, .comp2Ratio:
            return ratio[step(normalized, max: 8)]
        case .comp1Attack, .comp2Attack:
            return milliseconds(attack[step(normalized, max: 25)])
        case .comp1Release, .comp2Release:
            return "\(release[step(normalized, max: 45)]) ms"
        case .comp1Gain, .comp2Gain:
            return decibels(step(normalized, max: 74) - 50)
        default:
            return ""
        }
    }

    private static func step(_ normalized: Double, max upper: Int) -> Int {
        min(upper, max(0, Int((min(1, max(0, normalized)) * Double(upper)).rounded())))
    }

    private static func decibels(_ value: Int) -> String {
        if value > 0 { return String(format: "+%.1f", Double(value)) }
        if value == 0 { return "0.0" }
        return String(format: "%.1f", Double(value))
    }

    private static func milliseconds(_ value: Double) -> String {
        if value < 10 { return String(format: "%.1f ms", value) }
        return String(format: "%.0f ms", value)
    }
}
