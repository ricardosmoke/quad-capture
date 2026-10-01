import AudioToolbox
import CoreAudio
import Foundation
import SwiftUI

/// Motor Core Audio fora do MainActor (callback de entrada é tempo-real).
final class LevelCaptureEngine: @unchecked Sendable {
    static let maxChannels = 6

    private(set) var audioUnit: AudioUnit?
    private var callbackContext: CallbackContext?
    private let lock = NSLock()
    private var pending = [Float](repeating: 0, count: maxChannels)
    private(set) var activeChannels = 2

    func consumePeaks() -> [Float] {
        lock.lock()
        defer { lock.unlock() }
        let result = pending
        for i in pending.indices { pending[i] = 0 }
        return result
    }

    func notePeaks(_ peaks: [Float]) {
        lock.lock()
        for i in 0..<min(peaks.count, pending.count) {
            pending[i] = max(pending[i], peaks[i])
        }
        lock.unlock()
    }

    func stop() {
        if let unit = audioUnit {
            PanelLog.measure("AudioOutputUnitStop") {
                AudioOutputUnitStop(unit)
                AudioUnitUninitialize(unit)
                AudioComponentInstanceDispose(unit)
            }
        }
        audioUnit = nil
        callbackContext = nil
    }

    func start(deviceID: AudioDeviceID) throws {
        stop()

        var desc = AudioComponentDescription(
            componentType: kAudioUnitType_Output,
            componentSubType: kAudioUnitSubType_HALOutput,
            componentManufacturer: kAudioUnitManufacturer_Apple,
            componentFlags: 0,
            componentFlagsMask: 0)
        guard let component = AudioComponentFindNext(nil, &desc) else {
            throw LevelMonitorError.noHAL
        }

        var unit: AudioUnit?
        var status = PanelLog.measure("AudioComponentInstanceNew") {
            AudioComponentInstanceNew(component, &unit)
        }
        guard status == noErr, let unit else { throw LevelMonitorError.osStatus(status) }

        var enable: UInt32 = 1
        var disable: UInt32 = 0
        status = PanelLog.measure("EnableIO input") {
            AudioUnitSetProperty(
                unit, kAudioOutputUnitProperty_EnableIO,
                kAudioUnitScope_Input, 1, &enable, UInt32(MemoryLayout<UInt32>.size))
        }
        guard status == noErr else { throw LevelMonitorError.osStatus(status) }
        status = PanelLog.measure("EnableIO output off") {
            AudioUnitSetProperty(
                unit, kAudioOutputUnitProperty_EnableIO,
                kAudioUnitScope_Output, 0, &disable, UInt32(MemoryLayout<UInt32>.size))
        }
        guard status == noErr else { throw LevelMonitorError.osStatus(status) }

        var device = deviceID
        status = PanelLog.measure("CurrentDevice \(deviceID)") {
            AudioUnitSetProperty(
                unit, kAudioOutputUnitProperty_CurrentDevice,
                kAudioUnitScope_Global, 0, &device, UInt32(MemoryLayout<AudioDeviceID>.size))
        }
        guard status == noErr else { throw LevelMonitorError.osStatus(status) }

        var asbd = AudioStreamBasicDescription()
        var size = UInt32(MemoryLayout<AudioStreamBasicDescription>.size)
        status = PanelLog.measure("Get StreamFormat") {
            AudioUnitGetProperty(
                unit, kAudioUnitProperty_StreamFormat,
                kAudioUnitScope_Input, 1, &asbd, &size)
        }
        guard status == noErr else { throw LevelMonitorError.osStatus(status) }

        let channelCount = max(1, min(Self.maxChannels, Int(asbd.mChannelsPerFrame)))
        activeChannels = channelCount
        asbd.mFormatID = kAudioFormatLinearPCM
        asbd.mFormatFlags = kAudioFormatFlagIsFloat
            | kAudioFormatFlagIsPacked
            | kAudioFormatFlagIsNonInterleaved
        asbd.mBitsPerChannel = 32
        asbd.mBytesPerFrame = 4
        asbd.mFramesPerPacket = 1
        asbd.mBytesPerPacket = 4
        asbd.mChannelsPerFrame = UInt32(channelCount)

        status = PanelLog.measure("Set StreamFormat ch=\(channelCount)") {
            AudioUnitSetProperty(
                unit, kAudioUnitProperty_StreamFormat,
                kAudioUnitScope_Output, 1, &asbd,
                UInt32(MemoryLayout<AudioStreamBasicDescription>.size))
        }
        guard status == noErr else { throw LevelMonitorError.osStatus(status) }

        let context = CallbackContext(engine: self, channelCount: channelCount)
        callbackContext = context

        var callback = AURenderCallbackStruct(
            inputProc: inputRenderCallback,
            inputProcRefCon: Unmanaged.passUnretained(context).toOpaque())
        status = PanelLog.measure("SetInputCallback") {
            AudioUnitSetProperty(
                unit, kAudioOutputUnitProperty_SetInputCallback,
                kAudioUnitScope_Global, 0, &callback,
                UInt32(MemoryLayout<AURenderCallbackStruct>.size))
        }
        guard status == noErr else { throw LevelMonitorError.osStatus(status) }

        status = PanelLog.measure("AudioUnitInitialize") {
            AudioUnitInitialize(unit)
        }
        guard status == noErr else { throw LevelMonitorError.osStatus(status) }
        status = PanelLog.measure("AudioOutputUnitStart") {
            AudioOutputUnitStart(unit)
        }
        guard status == noErr else { throw LevelMonitorError.osStatus(status) }

        audioUnit = unit
    }

    final class CallbackContext {
        let engine: LevelCaptureEngine
        let channelCount: Int
        init(engine: LevelCaptureEngine, channelCount: Int) {
            self.engine = engine
            self.channelCount = channelCount
        }
    }
}

enum LevelMonitorError: LocalizedError {
    case noHAL
    case osStatus(OSStatus)
    var errorDescription: String? {
        switch self {
        case .noHAL: return "HAL Output não disponível"
        case .osStatus(let s): return String(format: "OSStatus %d", s)
        }
    }
}

@MainActor
final class InputLevelMonitor: ObservableObject {
    struct Snapshot: Equatable, Sendable {
        var levels: [CGFloat] = Array(repeating: 0, count: 6)
        var peaks: [CGFloat] = Array(repeating: 0, count: 6)
        var connected: Bool = false
        var status: String = "Procurando QUAD-CAPTURE…"
        var sampleRateHz: Double = 0

        var channel1: CGFloat { levels[0] }
        var channel2: CGFloat { levels[1] }
        var peak1: CGFloat { peaks[0] }
        var peak2: CGFloat { peaks[1] }
    }

    @Published private(set) var snapshot = Snapshot()

    private let engine = LevelCaptureEngine()
    private var uiTimer: Timer?
    private var scanTimer: Timer?
    private var hold = [Float](repeating: 0, count: LevelCaptureEngine.maxChannels)
    private var running = false
    private var openInFlight = false

    func start() {
        guard !running else { return }
        running = true
        scheduleOpen(reason: "start")
        uiTimer = Timer.scheduledTimer(withTimeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
            Task { @MainActor in self?.tick() }
        }
        scanTimer = Timer.scheduledTimer(withTimeInterval: 2.0, repeats: true) { [weak self] _ in
            Task { @MainActor in
                self?.ensureRunning()
                self?.refreshSampleRate()
            }
        }
    }

    /// Para a captura, pede a taxa escolhida ao Core Audio e reabre os meters.
    /// O dext faz a troca USB quando o HAL chama a mudança de sample rate.
    func setSampleRate(_ hz: Double) {
        guard running, !openInFlight else { return }
        if snapshot.sampleRateHz > 0 && UA55Device.bucket(snapshot.sampleRateHz) == UA55Device.bucket(hz) {
            return
        }
        openInFlight = true
        let engine = engine
        PanelWork.queue.async {
            let result = Self.switchRateOffMain(engine, hz: hz)
            Task { @MainActor [weak self] in
                guard let self else { return }
                self.openInFlight = false
                guard self.running else { return }
                self.apply(result)
            }
        }
    }

    func stop() {
        running = false
        uiTimer?.invalidate()
        uiTimer = nil
        scanTimer?.invalidate()
        scanTimer = nil
        let engine = engine
        PanelWork.queue.async {
            PanelLog.measure("engine.stop [panel]") { engine.stop() }
        }
        snapshot = Snapshot(status: "Parado")
    }

    private func ensureRunning() {
        guard running, !openInFlight, engine.audioUnit == nil else { return }
        scheduleOpen(reason: "rescan")
    }

    private func scheduleOpen(reason: String) {
        guard !openInFlight else { return }
        openInFlight = true
        let engine = engine
        PanelWork.queue.async {
            let result = Self.openOffMain(engine, reason: reason)
            Task { @MainActor [weak self] in
                guard let self else { return }
                self.openInFlight = false
                guard self.running else { return }
                self.apply(result)
            }
        }
    }

    private func apply(_ result: OpenResult) {
        snapshot.connected = result.connected
        snapshot.status = result.status
        snapshot.sampleRateHz = result.sampleRateHz
    }

    /// Acompanha uma troca feita fora do painel (Audio MIDI Setup) e reabre
    /// a captura quando o número de canais muda, como em 192 kHz.
    private func refreshSampleRate() {
        guard running, !openInFlight else { return }
        let known = snapshot.sampleRateHz
        let engine = engine
        PanelWork.queue.async {
            guard let id = UA55Device.find() else { return }
            let hz = UA55Device.nominalRate(id) ?? 0
            let changed = known > 0 && UA55Device.bucket(hz) != UA55Device.bucket(known)
            var reopened: OpenResult?
            if changed {
                reopened = Self.openOffMain(engine, reason: "rate-watch")
            }
            Task { @MainActor [weak self] in
                guard let self, self.running, !self.openInFlight else { return }
                if let reopened {
                    self.apply(reopened)
                } else if hz > 0 {
                    self.snapshot.sampleRateHz = hz
                }
            }
        }
    }

    private struct OpenResult: Sendable {
        var connected: Bool
        var status: String
        var sampleRateHz: Double
    }

    private nonisolated static func switchRateOffMain(_ engine: LevelCaptureEngine, hz: Double) -> OpenResult {
        PanelLog.measure("engine.stop [rate]") { engine.stop() }
        guard let id = UA55Device.find() else {
            return OpenResult(connected: false, status: "QUAD-CAPTURE não encontrada", sampleRateHz: 0)
        }
        let current = UA55Device.nominalRate(id) ?? 0
        let target = UA55Device.bucket(hz)
        let status = PanelLog.measure("set sample rate \(Int(target))") {
            UA55Device.setNominalRate(id, hz: target)
        }
        PanelLog.write("sample rate \(Int(current)) -> \(Int(target)) status \(status)")
        var opened = openOffMain(engine, reason: "rate")
        if status != noErr {
            opened.status = "Falha ao trocar taxa (\(status))"
        }
        return opened
    }

    private nonisolated static func openOffMain(_ engine: LevelCaptureEngine, reason: String) -> OpenResult {
        PanelLog.measure("engine.stop [\(reason)]") { engine.stop() }
        guard let id = PanelLog.measure("find device [\(reason)]", { UA55Device.find() }) else {
            return OpenResult(connected: false, status: "QUAD-CAPTURE não encontrada", sampleRateHz: 0)
        }
        let hz = UA55Device.nominalRate(id) ?? 0
        do {
            try PanelLog.measure("engine.start id=\(id) [\(reason)]") {
                try engine.start(deviceID: id)
            }
            let name = UA55Device.name(id) ?? "UA-55"
            PanelLog.write("input open ok \(name) \(Int(hz)) Hz")
            return OpenResult(connected: true, status: "Entrada ao vivo · \(name)", sampleRateHz: hz)
        } catch {
            PanelLog.write("input open failed \(error.localizedDescription)")
            return OpenResult(
                connected: false,
                status: "Falha ao abrir entrada: \(error.localizedDescription)",
                sampleRateHz: hz)
        }
    }

    private func tick() {
        let peaks = engine.consumePeaks()
        var next = snapshot
        for i in 0..<LevelCaptureEngine.maxChannels {
            let peak = i < peaks.count ? peaks[i] : 0
            hold[i] = max(peak, hold[i] * 0.92)
            next.levels[i] = Self.level(fromPeak: peak)
            next.peaks[i] = Self.level(fromPeak: hold[i])
        }
        if next != snapshot {
            snapshot = next
        }
    }

    static func level(fromPeak peak: Float) -> CGFloat {
        guard peak > 0.000_001 else { return 0 }
        let db = 20 * log10(peak)
        return CGFloat(min(1, max(0, (db + 48) / 48)))
    }
}

enum UA55Device {
    static func find() -> AudioDeviceID? {
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDevices,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        var dataSize: UInt32 = 0
        guard AudioObjectGetPropertyDataSize(
            AudioObjectID(kAudioObjectSystemObject), &address, 0, nil, &dataSize) == noErr
        else { return nil }

        let count = Int(dataSize) / MemoryLayout<AudioDeviceID>.size
        var devices = [AudioDeviceID](repeating: 0, count: count)
        guard AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject), &address, 0, nil, &dataSize, &devices) == noErr
        else { return nil }

        for device in devices {
            let deviceName = name(device) ?? ""
            let deviceUID = uid(device) ?? ""
            guard inputChannels(device) > 0 else { continue }
            if deviceUID == "QUAD-CAPTURE-UA-55"
                || deviceName.localizedCaseInsensitiveContains("QUAD-CAPTURE")
                || deviceName.localizedCaseInsensitiveContains("UA-55") {
                return device
            }
        }
        return nil
    }

    static func name(_ device: AudioDeviceID) -> String? {
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioObjectPropertyName,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        var cfName: Unmanaged<CFString>?
        var size = UInt32(MemoryLayout<Unmanaged<CFString>?>.size)
        guard AudioObjectGetPropertyData(device, &address, 0, nil, &size, &cfName) == noErr,
              let cfName else { return nil }
        return cfName.takeUnretainedValue() as String
    }

    static func uid(_ device: AudioDeviceID) -> String? {
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyDeviceUID,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        var cfUID: Unmanaged<CFString>?
        var size = UInt32(MemoryLayout<Unmanaged<CFString>?>.size)
        guard AudioObjectGetPropertyData(device, &address, 0, nil, &size, &cfUID) == noErr,
              let cfUID else { return nil }
        return cfUID.takeUnretainedValue() as String
    }

    static func inputChannels(_ device: AudioDeviceID) -> Int {
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyStreamConfiguration,
            mScope: kAudioObjectPropertyScopeInput,
            mElement: kAudioObjectPropertyElementMain)
        var size: UInt32 = 0
        guard AudioObjectGetPropertyDataSize(device, &address, 0, nil, &size) == noErr, size > 0 else {
            return 0
        }
        let raw = UnsafeMutableRawPointer.allocate(
            byteCount: Int(size),
            alignment: MemoryLayout<AudioBufferList>.alignment)
        defer { raw.deallocate() }
        guard AudioObjectGetPropertyData(device, &address, 0, nil, &size, raw) == noErr else {
            return 0
        }
        let list = raw.assumingMemoryBound(to: AudioBufferList.self)
        return UnsafeMutableAudioBufferListPointer(list).reduce(0) { $0 + Int($1.mNumberChannels) }
    }

    static let rates: [Double] = [44100, 48000, 96000, 192000]

    static func bucket(_ hz: Double) -> Double {
        rates.min(by: { abs($0 - hz) < abs($1 - hz) }) ?? rates[0]
    }

    static func label(for hz: Double) -> String {
        guard hz > 0 else { return "—" }
        switch bucket(hz) {
        case 44100: return "44.1 kHz"
        case 48000: return "48 kHz"
        case 96000: return "96 kHz"
        case 192000: return "192 kHz"
        default: return "—"
        }
    }

    static func nominalRate(_ device: AudioDeviceID) -> Double? {
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyNominalSampleRate,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        var rate: Float64 = 0
        var size = UInt32(MemoryLayout<Float64>.size)
        guard AudioObjectGetPropertyData(device, &address, 0, nil, &size, &rate) == noErr else {
            return nil
        }
        return rate
    }

    /// Mesma propriedade que o Audio MIDI Setup escreve. O dext recebe
    /// HandleChangeSampleRate e faz a troca no USB.
    static func setNominalRate(_ device: AudioDeviceID, hz: Double) -> OSStatus {
        var address = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyNominalSampleRate,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        var rate = Float64(hz)
        let size = UInt32(MemoryLayout<Float64>.size)
        return AudioObjectSetPropertyData(device, &address, 0, nil, size, &rate)
    }
}

private func inputRenderCallback(
    _ inRefCon: UnsafeMutableRawPointer,
    _ ioActionFlags: UnsafeMutablePointer<AudioUnitRenderActionFlags>,
    _ inTimeStamp: UnsafePointer<AudioTimeStamp>,
    _ inBusNumber: UInt32,
    _ inNumberFrames: UInt32,
    _ ioData: UnsafeMutablePointer<AudioBufferList>?
) -> OSStatus {
    let context = Unmanaged<LevelCaptureEngine.CallbackContext>
        .fromOpaque(inRefCon)
        .takeUnretainedValue()
    guard let unit = context.engine.audioUnit else { return noErr }

    let channels = context.channelCount
    let frames = Int(inNumberFrames)
    let ablSize = MemoryLayout<AudioBufferList>.size
        + max(0, channels - 1) * MemoryLayout<AudioBuffer>.size
    let ablRaw = UnsafeMutableRawPointer.allocate(
        byteCount: ablSize,
        alignment: MemoryLayout<AudioBufferList>.alignment)
    defer { ablRaw.deallocate() }
    ablRaw.initializeMemory(as: UInt8.self, repeating: 0, count: ablSize)
    let abl = ablRaw.assumingMemoryBound(to: AudioBufferList.self)
    abl.pointee.mNumberBuffers = UInt32(channels)

    let pointers = UnsafeMutableAudioBufferListPointer(abl)
    for i in 0..<channels {
        let data = UnsafeMutablePointer<Float>.allocate(capacity: frames)
        data.initialize(repeating: 0, count: frames)
        pointers[i] = AudioBuffer(
            mNumberChannels: 1,
            mDataByteSize: UInt32(frames * MemoryLayout<Float>.size),
            mData: UnsafeMutableRawPointer(data))
    }
    defer {
        for i in 0..<channels {
            pointers[i].mData?.assumingMemoryBound(to: Float.self).deallocate()
        }
    }

    let status = AudioUnitRender(
        unit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, abl)
    guard status == noErr else { return status }

    var peaks = [Float](repeating: 0, count: channels)
    for ch in 0..<channels {
        guard let ptr = pointers[ch].mData?.assumingMemoryBound(to: Float.self) else { continue }
        var peak: Float = 0
        for frame in 0..<frames {
            peak = max(peak, abs(ptr[frame]))
        }
        peaks[ch] = peak
    }
    context.engine.notePeaks(peaks)
    return noErr
}
