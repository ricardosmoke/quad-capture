import Foundation
import IOKit

/// SENS 1 e 2 lidos da placa. O DT1 manda duas contagens por dB (108 → 54).
/// A dext guarda o byte cru. A tela mostra o resultado da divisão.
/// A leitura é um user client, fora do Core Audio: a propriedade de áudio travava o HAL.
/// Botões do preamp lidos no DT1 de 59 bytes. nil = esse canal ainda não veio.
struct PreampSwitches: Equatable {
    var generation: Int = 0
    var loCut1: Bool?
    var loCut2: Bool?
    var phase1: Bool?
    var phase2: Bool?
    var bypass1: Bool?
    var bypass2: Bool?
    var link: Bool?
}

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

    /// LO-CUT e PHASE do bloco de 59 bytes. SENS desse bloco entra em `readSens`.
    static func preampSwitches() -> PreampSwitches {
        SensFeed.shared.switches()
    }

    /// Tela abrindo ou placa de volta: os botões ficam off até a leitura desta conexão.
    static func beginRead() {
        SensFeed.shared.beginRead()
    }

    /// Depois de um clique: a próxima resposta da placa pode corrigir os botões.
    static func armRefresh() {
        SensFeed.shared.armRefresh()
    }

    /// A resposta do RQ1 pode cair antes do `log stream` existir. Esta busca pega o DT1 desta conexão.
    static func collectState() {
        SensFeed.shared.collectState()
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
    private var loCut: [Bool?] = [nil, nil]
    private var phase: [Bool?] = [nil, nil]
    private var bypass: [Bool?] = [nil, nil]
    private var link: Bool?
    /// A primeira resposta desta conexão, uma por canal. O eco seguinte não copia o outro canal.
    private var bypassSeen = [false, false]
    private var linkSeen = false
    private var setupSeen = false
    private var buttonGen = 0
    private var collectGen = 0
    /// Linhas anteriores a isto são de outra conexão e não acendem botão.
    private var readAfter: Date?
    private let logStamp: DateFormatter = {
        let formatter = DateFormatter()
        formatter.locale = Locale(identifier: "en_US_POSIX")
        formatter.timeZone = .current
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss.SSS"
        return formatter
    }()
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

    func beginRead() {
        lock.lock()
        readAfter = Date()
        loCut = [false, false]
        phase = [false, false]
        bypass = [false, false]
        bypassSeen = [false, false]
        link = false
        linkSeen = false
        setupSeen = false
        device = "off"
        buttonGen += 1
        collectGen += 1
        lock.unlock()
    }

    func armRefresh() {
        lock.lock()
        readAfter = Date()
        loCut = [nil, nil]
        phase = [nil, nil]
        bypass = [nil, nil]
        link = nil
        bypassSeen = [false, false]
        linkSeen = false
        setupSeen = false
        collectGen += 1
        lock.unlock()
    }

    func remember(_ state: String) {
        lock.lock()
        device = state
        lock.unlock()
    }

    func switches() -> PreampSwitches {
        lock.lock()
        defer { lock.unlock() }
        return PreampSwitches(
            generation: buttonGen,
            loCut1: loCut[0], loCut2: loCut[1],
            phase1: phase[0], phase2: phase[1],
            bypass1: bypass[0], bypass2: bypass[1],
            link: link)
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
        startStream()
        seedFromShow()
    }

    private func seedFromShow() {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/log")
        process.arguments = [
            "show", "--last", "10m", "--style", "compact",
            "--predicate", #"eventMessage CONTAINS "[UA55] sens" OR eventMessage CONTAINS "[UA55] autosens" OR eventMessage CONTAINS "[UA55] dt1" OR eventMessage CONTAINS "[UA55] bypass" OR eventMessage CONTAINS "[UA55] link""#
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
            _ = applyBypassRead(text)
            _ = applyLinkRead(text)
            _ = applySetup(text)
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

    /// Procura o DT1 gravado depois de `beginRead`. Para no primeiro bloco de 59 bytes.
    func collectState() {
        lock.lock()
        collectGen += 1
        let generation = collectGen
        lock.unlock()
        queue.async { [weak self] in
            self?.collectStateAttempt(0, generation: generation)
        }
    }

    private func collectStateAttempt(_ attempt: Int, generation: Int) {
        lock.lock()
        let current = collectGen
        lock.unlock()
        guard generation == current else { return }
        guard attempt < 6 else {
            lock.lock()
            let setup = setupSeen
            let bypassOk = bypassSeen[0] && bypassSeen[1]
            let linkOk = linkSeen
            lock.unlock()
            if !setup {
                PanelLog.write("state read missed")
            } else if !bypassOk {
                PanelLog.write("bypass read missed")
            } else if !linkOk {
                PanelLog.write("link read missed")
            }
            return
        }
        if pullStateSinceRead() {
            return
        }
        queue.asyncAfter(deadline: .now() + 0.25) { [weak self] in
            self?.collectStateAttempt(attempt + 1, generation: generation)
        }
    }

    private func pullStateSinceRead() -> Bool {
        lock.lock()
        let cutoff = readAfter
        lock.unlock()
        guard let cutoff else { return false }
        let stamp = DateFormatter()
        stamp.locale = Locale(identifier: "en_US_POSIX")
        stamp.timeZone = .current
        stamp.dateFormat = "yyyy-MM-dd HH:mm:ss"
        let start = stamp.string(from: cutoff.addingTimeInterval(-1))
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/log")
        process.arguments = [
            "show", "--start", start, "--style", "compact",
            "--predicate", #"eventMessage CONTAINS "[UA55] sens" OR eventMessage CONTAINS "[UA55] autosens" OR eventMessage CONTAINS "[UA55] dt1" OR eventMessage CONTAINS "[UA55] bypass" OR eventMessage CONTAINS "[UA55] link""#
        ]
        let pipe = Pipe()
        process.standardOutput = pipe
        process.standardError = FileHandle.nullDevice
        do {
            try process.run()
        } catch {
            PanelLog.write("state show failed \(error.localizedDescription)")
            return false
        }
        process.waitUntilExit()
        let data = pipe.fileHandleForReading.readDataToEndOfFile()
        guard let text = String(data: data, encoding: .utf8) else { return false }
        var found = false
        for line in text.split(whereSeparator: \.isNewline) {
            let text = String(line)
            _ = applyDevice(text)
            if applyBypassRead(text) {
                let shown = switches()
                PanelLog.write("bypass read by1=\(shown.bypass1.map { $0 ? 1 : 0 } ?? -1) by2=\(shown.bypass2.map { $0 ? 1 : 0 } ?? -1)")
            }
            if applyLinkRead(text) {
                let shown = switches()
                PanelLog.write("link read \(shown.link.map { $0 ? 1 : 0 } ?? -1)")
            }
            if applySetup(text) {
                found = true
            }
            _ = apply(text)
        }
        if found {
            let shown = switches()
            PanelLog.write("state lo1=\(shown.loCut1.map { $0 ? 1 : 0 } ?? -1) lo2=\(shown.loCut2.map { $0 ? 1 : 0 } ?? -1) ph1=\(shown.phase1.map { $0 ? 1 : 0 } ?? -1) ph2=\(shown.phase2.map { $0 ? 1 : 0 } ?? -1) by1=\(shown.bypass1.map { $0 ? 1 : 0 } ?? -1) by2=\(shown.bypass2.map { $0 ? 1 : 0 } ?? -1) link=\(shown.link.map { $0 ? 1 : 0 } ?? -1)")
        }
        lock.lock()
        let bypassOk = bypassSeen[0] && bypassSeen[1]
        let linkOk = linkSeen
        lock.unlock()
        return found && bypassOk && linkOk
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
            "--predicate", #"eventMessage CONTAINS "[UA55] sens" OR eventMessage CONTAINS "[UA55] autosens" OR eventMessage CONTAINS "[UA55] dt1" OR eventMessage CONTAINS "[UA55] bypass" OR eventMessage CONTAINS "[UA55] link""#
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
            if applyBypassRead(text) {
                let shown = switches()
                PanelLog.write("bypass read by1=\(shown.bypass1.map { $0 ? 1 : 0 } ?? -1) by2=\(shown.bypass2.map { $0 ? 1 : 0 } ?? -1)")
            }
            if applyLinkRead(text) {
                let shown = switches()
                PanelLog.write("link read \(shown.link.map { $0 ? 1 : 0 } ?? -1)")
            }
            if applySetup(text) {
                let shown = switches()
                PanelLog.write("state lo1=\(shown.loCut1.map { $0 ? 1 : 0 } ?? -1) lo2=\(shown.loCut2.map { $0 ? 1 : 0 } ?? -1) ph1=\(shown.phase1.map { $0 ? 1 : 0 } ?? -1) ph2=\(shown.phase2.map { $0 ? 1 : 0 } ?? -1) by1=\(shown.bypass1.map { $0 ? 1 : 0 } ?? -1) by2=\(shown.bypass2.map { $0 ? 1 : 0 } ?? -1) link=\(shown.link.map { $0 ? 1 : 0 } ?? -1)")
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
        guard isCurrentRead(message) else { return false }
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

    /// DT1 01 00 00 00 com 59 bytes.
    /// Byte 23: bit 0 = LO-CUT 1, bit 1 = LO-CUT 2.
    /// Byte 24: bit 0 = PHASE 1, bit 1 = PHASE 2.
    /// SENS 1 e 2 são os bytes 25 e 26.
    /// BYPASS não entra aqui: cada canal chega na própria resposta 00 05 <canal> 06.
    /// AUTO-SENS é o byte 21: 02 ligado, 00 desligado. Não devolve comando nenhum.
    private func applySetup(_ message: String) -> Bool {
        guard let range = message.range(of: "[UA55] dt1 ") else { return false }
        let token = message[range.upperBound...].split(whereSeparator: \.isWhitespace).first.map(String.init) ?? ""
        let hex = token.lowercased()
        guard hex.count == 126, hex.hasPrefix("01000000"), isCurrentRead(message) else { return false }
        let body = hex.dropFirst(8)
        var bytes = [UInt8]()
        bytes.reserveCapacity(59)
        var index = body.startIndex
        while index < body.endIndex {
            guard let next = body.index(index, offsetBy: 2, limitedBy: body.endIndex),
                  body.distance(from: index, to: next) == 2,
                  let value = UInt8(body[index..<next], radix: 16) else {
                return false
            }
            bytes.append(value)
            index = next
        }
        guard bytes.count == 59 else { return false }
        lock.lock()
        defer { lock.unlock() }
        loCut[0] = (bytes[23] & 0x01) != 0
        loCut[1] = (bytes[23] & 0x02) != 0
        phase[0] = (bytes[24] & 0x01) != 0
        phase[1] = (bytes[24] & 0x02) != 0
        setupSeen = true
        let sens1 = bytes[25]
        let sens2 = bytes[26]
        if sens1 <= 108 {
            left = min(Int(HardwareGain.sensMaxDb), Int(sens1) / 2)
        }
        if sens2 <= 108 {
            right = min(Int(HardwareGain.sensMaxDb), Int(sens2) / 2)
        }
        device = bytes[21] == 0x02 ? "on" : "off"
        buttonGen += 1
        return true
    }

    /// A primeira resposta 00 05 <canal> 06 desta conexão. O canal 0 da placa é o BYPASS 2 da tela.
    private func applyBypassRead(_ message: String) -> Bool {
        guard isCurrentRead(message) else { return false }
        guard let marker = message.range(of: "[UA55] bypass ") else { return false }
        let parts = message[marker.upperBound...].split(whereSeparator: \.isWhitespace)
        guard parts.count >= 2, let channel = Int(parts[0]), let value = Int(parts[1]), channel <= 1 else {
            return false
        }
        let screen = channel == 0 ? 1 : 0
        lock.lock()
        defer { lock.unlock() }
        if bypassSeen[screen] {
            return false
        }
        bypassSeen[screen] = true
        bypass[screen] = value == 1
        buttonGen += 1
        return true
    }

    /// A primeira resposta 00 05 00 05 desta leitura. 01 liga o LINK.
    private func applyLinkRead(_ message: String) -> Bool {
        guard isCurrentRead(message) else { return false }
        guard let marker = message.range(of: "[UA55] link ") else { return false }
        let parts = message[marker.upperBound...].split(whereSeparator: \.isWhitespace)
        guard let value = parts.first.flatMap({ Int($0) }), value <= 1 else {
            return false
        }
        lock.lock()
        defer { lock.unlock() }
        if linkSeen {
            return false
        }
        linkSeen = true
        link = value == 1
        buttonGen += 1
        return true
    }

    /// O relógio do log compacto. Só a leitura desta conexão passa.
    private func isCurrentRead(_ message: String) -> Bool {
        lock.lock()
        let cutoff = readAfter
        lock.unlock()
        guard let cutoff, message.count >= 23 else { return false }
        let stamp = String(message.prefix(23))
        guard let when = logStamp.date(from: stamp) else { return false }
        return when >= cutoff
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
