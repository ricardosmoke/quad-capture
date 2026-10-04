import Foundation
import IOKit

/// SENS 1 e 2 lidos da placa. O DT1 manda duas contagens por dB (108 → 54).
/// A dext guarda o byte cru. A tela mostra o resultado da divisão.
/// A leitura é um user client, fora do Core Audio: a propriedade de áudio travava o HAL.
enum HardwareGain {
    static let sensMinDb: Double = 0
    static let sensMaxDb: Double = 54

    /// 255 fica 255: esse canal ainda não chegou. Acima de 108 trava em 54 dB.
    static func db(fromDeviceByte byte: Int) -> Int {
        guard (0...127).contains(byte) else { return byte }
        return min(Int(sensMaxDb), byte / 2)
    }

    static func db(fromNormalized value: Double) -> Int {
        let clamped = min(1, max(0, value))
        return Int((clamped * sensMaxDb).rounded())
    }

    static func normalized(fromDb db: Int) -> Double {
        Double(min(Int(sensMaxDb), max(Int(sensMinDb), db))) / sensMaxDb
    }

    /// nil se o driver novo ainda não está aberto. 255 num canal = sem leitura.
    static func readSens() -> (Int, Int)? {
        SensFeed.shared.read()
    }

    /// "on", "off" ou "—" se o botão AUTO SENS da placa ainda não falou.
    static func deviceText() -> String {
        SensFeed.shared.deviceText()
    }

    /// O clique da tela inverte o desenho. A leitura seguinte da placa pode corrigir.
    static func rememberAutoSens(_ state: String) {
        SensFeed.shared.remember(state)
    }

    static func start() {
        SensFeed.shared.start()
    }

    static func stop() {
        SensFeed.shared.stop()
    }
}

/// O user client do dext exige um entitlement que este app ainda não tem
/// (`IOServiceOpen` volta `0xe00002e2`). O ganho então sai do `log stream`
/// da dext, que grava cada mudança do knob. O arquivo do OSLogStore atrasa
/// e não mostra o giro na hora.
private final class SensFeed: @unchecked Sendable {
    static let shared = SensFeed()

    private let queue = DispatchQueue(label: "dev.ua55.panel.sens")
    private let lock = NSLock()
    private var connect: io_connect_t = 0
    private var left = 255
    private var right = 255
    private var device = "—"
    private var stream: Process?
    private var leader: Int32 = -1
    private var reader: DispatchSourceRead?
    private var pending = Data()
    private var started = false

    func start() {
        queue.async { [weak self] in
            guard let self, !self.started else { return }
            self.started = true
            if self.openUserClient() {
                return
            }
            self.startLog()
        }
    }

    func stop() {
        queue.async { [weak self] in
            guard let self else { return }
            self.reader?.cancel()
            self.reader = nil
            self.stream?.terminate()
            self.stream = nil
            self.leader = -1
            self.pending.removeAll()
            if self.connect != 0 {
                IOServiceClose(self.connect)
                self.connect = 0
            }
            self.started = false
        }
    }

    func read() -> (Int, Int)? {
        if connect != 0 {
            return readUserClient()
        }
        lock.lock()
        let pair = (left, right)
        lock.unlock()
        return pair
    }

    func deviceText() -> String {
        lock.lock()
        let text = device
        lock.unlock()
        return text
    }

    func remember(_ state: String) {
        lock.lock()
        device = state
        lock.unlock()
    }

    private func openUserClient() -> Bool {
        let matching = IOServiceMatching("IOUserService") as NSMutableDictionary
        matching["IOUserServerName"] = "dev.ua55.UA55DiagnosticApp.driver"
        let service = IOServiceGetMatchingService(kIOMainPortDefault, matching)
        guard service != 0 else {
            PanelLog.write("sens service not found")
            return false
        }
        defer { IOObjectRelease(service) }
        var opened: io_connect_t = 0
        let status = IOServiceOpen(service, mach_task_self_, 0x55, &opened)
        if status != KERN_SUCCESS {
            PanelLog.write(String(format: "sens IOServiceOpen 0x%08x — usando o log do driver", status))
            return false
        }
        connect = opened
        PanelLog.write("sens user client open")
        return true
    }

    private func readUserClient() -> (Int, Int)? {
        var output = [UInt64](repeating: 0, count: 1)
        var count: UInt32 = 1
        let status = IOConnectCallScalarMethod(connect, 0, nil, 0, &output, &count)
        if status != KERN_SUCCESS {
            PanelLog.write(String(format: "sens IOConnect 0x%08x", status))
            return nil
        }
        return (
            HardwareGain.db(fromDeviceByte: Int(output[0] & 0xff)),
            HardwareGain.db(fromDeviceByte: Int((output[0] >> 8) & 0xff))
        )
    }

    private func startLog() {
        seedFromShow()
        startStream()
    }

    private func seedFromShow() {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/log")
        process.arguments = [
            "show", "--last", "10m", "--style", "compact",
            "--predicate", #"eventMessage CONTAINS "[UA55] sens" OR eventMessage CONTAINS "[UA55] autosens" OR eventMessage CONTAINS "[UA55] dt1""#
        ]
        let pipe = Pipe()
        process.standardOutput = pipe
        process.standardError = FileHandle.nullDevice
        do {
            try process.run()
        } catch {
            PanelLog.write("sens show failed \(error.localizedDescription)")
            return
        }
        process.waitUntilExit()
        let data = pipe.fileHandleForReading.readDataToEndOfFile()
        guard let text = String(data: data, encoding: .utf8) else { return }
        var applied = 0
        for line in text.split(whereSeparator: \.isNewline) {
            let text = String(line)
            _ = applyDevice(text)
            if apply(text) {
                applied += 1
            }
        }
        lock.lock()
        let shownLeft = left
        let shownRight = right
        lock.unlock()
        PanelLog.write("sens show seed aplicadas=\(applied) sens1=\(shownLeft) sens2=\(shownRight)")
    }

    private func startStream() {
        var primary: Int32 = -1
        var replica: Int32 = -1
        guard openpty(&primary, &replica, nil, nil, nil) == 0 else {
            PanelLog.write("sens pty failed")
            return
        }
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/log")
        process.arguments = [
            "stream", "--style", "compact",
            "--predicate", #"eventMessage CONTAINS "[UA55] sens" OR eventMessage CONTAINS "[UA55] autosens" OR eventMessage CONTAINS "[UA55] dt1""#
        ]
        let output = FileHandle(fileDescriptor: replica, closeOnDealloc: false)
        process.standardOutput = output
        process.standardError = output
        process.terminationHandler = { proc in
            PanelLog.write("sens stream ended \(proc.terminationStatus)")
        }
        do {
            try process.run()
        } catch {
            PanelLog.write("sens stream failed \(error.localizedDescription)")
            close(primary)
            close(replica)
            return
        }
        close(replica)
        leader = primary
        stream = process
        let source = DispatchSource.makeReadSource(fileDescriptor: primary, queue: queue)
        source.setEventHandler { [weak self] in
            self?.readStream()
        }
        source.setCancelHandler {
            close(primary)
        }
        source.resume()
        reader = source
        PanelLog.write("sens stream started")
    }

    private func readStream() {
        var buffer = [UInt8](repeating: 0, count: 4096)
        let count = Darwin.read(leader, &buffer, buffer.count)
        if count <= 0 { return }
        pending.append(buffer, count: count)
        while let newline = pending.firstIndex(of: 0x0A) {
            let line = pending.prefix(upTo: newline)
            pending.removeSubrange(...newline)
            guard let text = String(data: line, encoding: .utf8) else { continue }
            if applyDevice(text) {
                lock.lock()
                let shown = device
                lock.unlock()
                PanelLog.write("device \(shown)")
            }
            if apply(text) {
                lock.lock()
                let shownLeft = left
                let shownRight = right
                lock.unlock()
                PanelLog.write("sens stream sens1=\(shownLeft) sens2=\(shownRight)")
            }
        }
    }

    /// true quando o AUTO SENS da placa muda. "on" / "off".
    private func applyDevice(_ message: String) -> Bool {
        let state: String
        if message.contains("[UA55] autosens on") {
            state = "on"
        } else if message.contains("[UA55] autosens off") {
            state = "off"
        } else if message.contains("[UA55] dt1 ") {
            guard let range = message.range(of: "[UA55] dt1 ") else { return false }
            let hex = message[range.upperBound...].split(whereSeparator: \.isWhitespace).first.map(String.init) ?? ""
            switch hex {
            case "0002010201", "0002010302":
                state = "on"
            case "0002010202", "0002010300":
                state = "off"
            default:
                return false
            }
        } else {
            return false
        }
        lock.lock()
        defer { lock.unlock() }
        if device == state {
            return false
        }
        device = state
        return true
    }

    /// true quando o texto muda um canal.
    private func apply(_ message: String) -> Bool {
        guard let marker = message.range(of: "[UA55] sens ") else { return false }
        let parts = message[marker.upperBound...].split(separator: " ")
        guard parts.count >= 3, let channel = Int(parts[0]), let raw = Int(parts[2]) else {
            return false
        }
        let db = HardwareGain.db(fromDeviceByte: raw)
        guard db <= Int(HardwareGain.sensMaxDb) else { return false }
        lock.lock()
        defer { lock.unlock() }
        if channel == 1, left != db {
            left = db
            return true
        }
        if channel == 2, right != db {
            right = db
            return true
        }
        return false
    }
}
