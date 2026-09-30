import Foundation

/// Fila única para Core Audio. A chamada na thread principal trava o HAL
/// (o driver recebe StopIO e só volta ao StartIO dezenas de segundos depois).
enum PanelWork {
    static let queue = DispatchQueue(label: "dev.ua55.panel.audio")
}

/// Texto em `logs/panel.log`, na raiz do repositório, para ler depois da reprodução.
enum PanelLog {
    static let path = "/Users/ricardo/Documents/Quad-Capture-Git/quad-capture/logs/panel.log"

    private static let ioQueue = DispatchQueue(label: "dev.ua55.panel.log")
    private static let stamp: DateFormatter = {
        let formatter = DateFormatter()
        formatter.locale = Locale(identifier: "en_US_POSIX")
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss.SSS"
        return formatter
    }()

    static func write(_ message: String) {
        let where_ = Thread.isMainThread ? "main" : "bg"
        let line = "\(stamp.string(from: Date())) [\(where_)] \(message)\n"
        ioQueue.async {
            append(line)
        }
    }

    /// BEGIN/END e, se passar de 1 s, uma linha STILL por segundo.
    static func measure<T>(_ label: String, _ body: () throws -> T) rethrows -> T {
        let quiet = label.hasPrefix("readSens")
        if !quiet {
            write("BEGIN \(label)")
        }
        let started = Date()
        let timer = DispatchSource.makeTimerSource(queue: DispatchQueue.global(qos: .utility))
        timer.schedule(deadline: .now() + 1, repeating: 1)
        timer.setEventHandler {
            write(String(format: "STILL %@ %.1fs", label, Date().timeIntervalSince(started)))
        }
        timer.resume()
        defer {
            timer.cancel()
            let elapsed = Date().timeIntervalSince(started)
            if !quiet || elapsed > 0.05 {
                write(String(format: "END %@ %.3fs", label, elapsed))
            }
        }
        return try body()
    }

    private static func append(_ line: String) {
        let url = URL(fileURLWithPath: path)
        let directory = url.deletingLastPathComponent()
        try? FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        if !FileManager.default.fileExists(atPath: path) {
            FileManager.default.createFile(atPath: path, contents: nil)
        }
        guard let handle = FileHandle(forWritingAtPath: path) else { return }
        defer { try? handle.close() }
        _ = try? handle.seekToEnd()
        if let data = line.data(using: .utf8) {
            try? handle.write(contentsOf: data)
        }
        try? handle.synchronize()
    }
}
