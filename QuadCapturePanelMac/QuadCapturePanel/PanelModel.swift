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

    func start() {
        PanelLog.write("PanelModel.start")
        monitor.start()
        HardwareGain.start()
        cancellable = Timer.scheduledTimer(withTimeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
            Task { @MainActor in
                guard let self else { return }
                self.levels = self.monitor.snapshot
                guard !self.sensBusy else { return }
                self.sensBusy = true
                PanelWork.queue.async { [weak self] in
                    let reading = PanelLog.measure("readSens") { HardwareGain.readSens() }
                    let device = HardwareGain.deviceText()
                    Task { @MainActor in
                        guard let self else { return }
                        self.sensBusy = false
                        self.applyHardwareSens(reading)
                        if device != self.autoSensText {
                            self.autoSensText = device
                        }
                    }
                }
            }
        }
    }

    func stop() {
        cancellable?.invalidate()
        cancellable = nil
        HardwareGain.stop()
        monitor.stop()
    }

    private func applyHardwareSens(_ reading: (Int, Int)?) {
        guard let reading else { return }
        if reading.0 <= Int(HardwareGain.sensMaxDb) {
            sens1 = HardwareGain.normalized(fromDb: reading.0)
            sens1Known = true
        }
        if reading.1 <= Int(HardwareGain.sensMaxDb) {
            sens2 = HardwareGain.normalized(fromDb: reading.1)
            sens2Known = true
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
