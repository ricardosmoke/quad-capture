import CryptoKit
import Darwin
import Foundation
import Network
import Security

/// Página Vue na rede local. Os comandos voltam para o PanelModel; o SysEx não passa por aqui.
final class PanelWebServer: @unchecked Sendable {
    static let port: UInt16 = 8745
    static let password = "QuadCapture"

    private let queue = DispatchQueue(label: "dev.ua55.panel.web")
    private let lock = NSLock()
    private var listener: NWListener?
    private var clients: [ObjectIdentifier: WebClient] = [:]
    private var sessions = Set<String>()
    private var onCommand: (([String: Any]) -> Void)?

    func start(onCommand: @escaping ([String: Any]) -> Void) {
        self.onCommand = onCommand
        queue.async { [weak self] in
            self?.listen()
        }
    }

    func stop() {
        queue.async { [weak self] in
            guard let self else { return }
            self.listener?.cancel()
            self.listener = nil
            let open = self.clients.values
            self.clients.removeAll()
            self.sessions.removeAll()
            for client in open {
                client.connection.cancel()
            }
        }
    }

    var hasClients: Bool {
        lock.lock()
        defer { lock.unlock() }
        return !clients.isEmpty
    }

    func broadcast(_ payload: Data) {
        queue.async { [weak self] in
            self?.clients.values.forEach { $0.sendText(payload) }
        }
    }

    func advertisedURL() -> String {
        "http://\(lanAddress()):\(Self.port)"
    }

    private func listen() {
        do {
            let listener = try NWListener(using: .tcp, on: NWEndpoint.Port(rawValue: Self.port)!)
            listener.newConnectionHandler = { [weak self] connection in
                self?.accept(connection)
            }
            listener.stateUpdateHandler = { state in
                if case let .failed(error) = state {
                    PanelLog.write("web listener \(error)")
                }
            }
            listener.start(queue: queue)
            self.listener = listener
            PanelLog.write("web \(advertisedURL())")
        } catch {
            PanelLog.write("web listener \(error.localizedDescription)")
        }
    }

    private func accept(_ connection: NWConnection) {
        connection.stateUpdateHandler = { [weak self] state in
            if case .ready = state {
                let remote = connection.currentPath?.remoteEndpoint ?? connection.endpoint
                guard let self, self.isLocal(remote) else {
                    connection.cancel()
                    return
                }
                let client = WebClient(connection: connection, server: self)
                self.lock.lock()
                self.clients[ObjectIdentifier(client)] = client
                self.lock.unlock()
                client.start()
            }
            if case .failed = state {
                self?.drop(connection)
            }
            if case .cancelled = state {
                self?.drop(connection)
            }
        }
        connection.start(queue: queue)
    }

    fileprivate func drop(_ connection: NWConnection) {
        lock.lock()
        clients = clients.filter { $0.value.connection !== connection }
        lock.unlock()
    }

    fileprivate func acceptSession(_ token: String) -> Bool {
        lock.lock()
        defer { lock.unlock() }
        return sessions.contains(token)
    }

    fileprivate func makeSession() -> String {
        var bytes = [UInt8](repeating: 0, count: 16)
        _ = SecRandomCopyBytes(kSecRandomDefault, bytes.count, &bytes)
        let token = bytes.map { String(format: "%02x", $0) }.joined()
        lock.lock()
        sessions.insert(token)
        lock.unlock()
        return token
    }

    fileprivate func deliver(_ object: [String: Any]) {
        onCommand?(object)
    }

    private func isLocal(_ endpoint: NWEndpoint) -> Bool {
        guard case let .hostPort(host, _) = endpoint else { return false }
        switch host {
        case let .ipv4(address):
            return Self.localIPv4(address)
        case let .ipv6(address):
            return Self.localIPv6(address)
        default:
            return false
        }
    }

    private static func localIPv4(_ address: IPv4Address) -> Bool {
        let bytes = Array(address.rawValue)
        guard bytes.count == 4 else { return false }
        let a = bytes[0]
        let b = bytes[1]
        if a == 127 || a == 10 { return true }
        if a == 192 && b == 168 { return true }
        if a == 172 && (16...31).contains(b) { return true }
        if a == 169 && b == 254 { return true }
        return false
    }

    private static func localIPv6(_ address: IPv6Address) -> Bool {
        let bytes = Array(address.rawValue)
        guard bytes.count == 16 else { return false }
        if bytes == [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1] { return true }
        if bytes[0] == 0xFE && (bytes[1] & 0xC0) == 0x80 { return true }
        if (bytes[0] & 0xFE) == 0xFC { return true }
        if bytes[0] == 0 && bytes[1] == 0 && bytes[10] == 0xFF && bytes[11] == 0xFF,
           let mapped = IPv4Address(Data(bytes[12...15])) {
            return localIPv4(mapped)
        }
        return false
    }

    private func lanAddress() -> String {
        var pointer: UnsafeMutablePointer<ifaddrs>?
        guard getifaddrs(&pointer) == 0, let first = pointer else { return "127.0.0.1" }
        defer { freeifaddrs(pointer) }
        var cursor: UnsafeMutablePointer<ifaddrs>? = first
        var fallback = "127.0.0.1"
        while let item = cursor {
            cursor = item.pointee.ifa_next
            guard let address = item.pointee.ifa_addr, address.pointee.sa_family == UInt8(AF_INET) else { continue }
            var host = [CChar](repeating: 0, count: Int(NI_MAXHOST))
            guard getnameinfo(
                address,
                socklen_t(address.pointee.sa_len),
                &host,
                socklen_t(host.count),
                nil,
                0,
                NI_NUMERICHOST
            ) == 0 else { continue }
            let ip = String(cString: host)
            guard !ip.hasPrefix("127.") else { continue }
            let name = String(cString: item.pointee.ifa_name)
            if name == "en0" { return ip }
            if fallback.hasPrefix("127.") { fallback = ip }
        }
        return fallback
    }
}

private final class WebClient {
    let connection: NWConnection
    private weak var server: PanelWebServer?
    private var buffer = Data()
    private var webSocket = false
    private var fragment = Data()

    init(connection: NWConnection, server: PanelWebServer) {
        self.connection = connection
        self.server = server
    }

    func start() {
        receive()
    }

    func sendText(_ payload: Data) {
        guard webSocket else { return }
        var frame = Data([0x81])
        if payload.count < 126 {
            frame.append(UInt8(payload.count))
        } else if payload.count < 65536 {
            frame.append(126)
            frame.append(UInt8((payload.count >> 8) & 0xFF))
            frame.append(UInt8(payload.count & 0xFF))
        } else {
            return
        }
        frame.append(payload)
        connection.send(content: frame, completion: .contentProcessed { _ in })
    }

    private func receive() {
        connection.receive(minimumIncompleteLength: 1, maximumLength: 65536) { [weak self] data, _, isComplete, error in
            guard let self else { return }
            if let data { self.buffer.append(data) }
            if self.webSocket {
                self.pumpFrames()
            } else {
                self.pumpHTTP()
            }
            if isComplete || error != nil {
                self.server?.drop(self.connection)
                self.connection.cancel()
                return
            }
            self.receive()
        }
    }

    private func pumpHTTP() {
        guard let headerEnd = buffer.range(of: Data("\r\n\r\n".utf8)) else { return }
        let head = String(data: buffer[..<headerEnd.lowerBound], encoding: .utf8) ?? ""
        let lines = head.split(separator: "\r\n", omittingEmptySubsequences: false)
        guard let request = lines.first?.split(separator: " ") , request.count >= 2 else {
            reply(400, "Bad Request", "text/plain", Data())
            return
        }
        let method = String(request[0])
        let target = String(request[1])
        var headers: [String: String] = [:]
        for line in lines.dropFirst() {
            guard let colon = line.firstIndex(of: ":") else { continue }
            let name = line[..<colon].trimmingCharacters(in: .whitespaces).lowercased()
            let value = line[line.index(after: colon)...].trimmingCharacters(in: .whitespaces)
            headers[name] = value
        }
        let length = Int(headers["content-length"] ?? "0") ?? 0
        let bodyStart = headerEnd.upperBound
        guard buffer.count >= bodyStart + length else { return }
        let body = buffer[bodyStart..<(bodyStart + length)]
        buffer.removeSubrange(..<(bodyStart + length))
        route(method: method, target: target, headers: headers, body: Data(body))
    }

    private func route(method: String, target: String, headers: [String: String], body: Data) {
        let path = target.split(separator: "?", maxSplits: 1).first.map(String.init) ?? target
        if method == "POST" && path == "/api/login" {
            login(body)
            return
        }
        if method == "GET" && path == "/api/session" {
            if sessionToken(headers) != nil {
                reply(204, "No Content", "text/plain", Data())
            } else {
                reply(401, "Unauthorized", "text/plain", Data())
            }
            return
        }
        if method == "GET" && path == "/ws" {
            guard sessionToken(headers) != nil else {
                reply(401, "Unauthorized", "text/plain", Data())
                return
            }
            openSocket(headers)
            return
        }
        if method == "GET" {
            serve(path)
            return
        }
        reply(404, "Not Found", "text/plain", Data())
    }

    private func login(_ body: Data) {
        let object = (try? JSONSerialization.jsonObject(with: body)) as? [String: Any]
        let given = object?["password"] as? String ?? ""
        guard passwordsMatch(given) else {
            reply(401, "Unauthorized", "text/plain", Data())
            return
        }
        let token = server?.makeSession() ?? ""
        let header = "HTTP/1.1 204 No Content\r\nSet-Cookie: qc=\(token); HttpOnly; Path=/; SameSite=Lax\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
        connection.send(content: Data(header.utf8), completion: .contentProcessed { [weak self] _ in
            self?.connection.cancel()
        })
    }

    private func sessionToken(_ headers: [String: String]) -> String? {
        guard let cookie = headers["cookie"] else { return nil }
        for part in cookie.split(separator: ";") {
            let piece = part.trimmingCharacters(in: .whitespaces)
            guard piece.hasPrefix("qc=") else { continue }
            let token = String(piece.dropFirst(3))
            if server?.acceptSession(token) == true { return token }
        }
        return nil
    }

    private func openSocket(_ headers: [String: String]) {
        guard let key = headers["sec-websocket-key"] else {
            reply(400, "Bad Request", "text/plain", Data())
            return
        }
        let magic = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
        let digest = Insecure.SHA1.hash(data: Data(magic.utf8))
        let accept = Data(digest).base64EncodedString()
        let header = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: \(accept)\r\n\r\n"
        webSocket = true
        connection.send(content: Data(header.utf8), completion: .contentProcessed { _ in })
        pumpFrames()
    }

    private func serve(_ path: String) {
        let relative = path == "/" ? "index.html" : String(path.dropFirst())
        guard !relative.contains(".."),
              let root = Bundle.main.resourceURL?.appendingPathComponent("Web", isDirectory: true)
        else {
            reply(404, "Not Found", "text/plain", Data())
            return
        }
        let file = root.appendingPathComponent(relative)
        let rootPath = root.path.hasSuffix("/") ? root.path : root.path + "/"
        guard file.path.hasPrefix(rootPath), let data = try? Data(contentsOf: file) else {
            reply(404, "Not Found", "text/plain", Data())
            return
        }
        reply(200, "OK", mime(file.pathExtension), data)
    }

    private func reply(_ status: Int, _ text: String, _ type: String, _ body: Data) {
        var header = "HTTP/1.1 \(status) \(text)\r\nContent-Type: \(type)\r\nContent-Length: \(body.count)\r\nConnection: close\r\n\r\n"
        var packet = Data(header.utf8)
        packet.append(body)
        connection.send(content: packet, completion: .contentProcessed { [weak self] _ in
            self?.connection.cancel()
        })
    }

    private func pumpFrames() {
        while buffer.count >= 2 {
            let b0 = buffer[buffer.startIndex]
            let b1 = buffer[buffer.index(buffer.startIndex, offsetBy: 1)]
            let opcode = b0 & 0x0F
            let masked = (b1 & 0x80) != 0
            var length = Int(b1 & 0x7F)
            var cursor = 2
            if length == 126 {
                guard buffer.count >= 4 else { return }
                length = (Int(buffer[buffer.index(buffer.startIndex, offsetBy: 2)]) << 8)
                    | Int(buffer[buffer.index(buffer.startIndex, offsetBy: 3)])
                cursor = 4
            } else if length == 127 {
                connection.cancel()
                return
            }
            let maskLength = masked ? 4 : 0
            guard buffer.count >= cursor + maskLength + length else { return }
            var payload = Data(buffer[buffer.index(buffer.startIndex, offsetBy: cursor + maskLength)...].prefix(length))
            if masked {
                let maskStart = buffer.index(buffer.startIndex, offsetBy: cursor)
                for index in payload.indices {
                    let offset = payload.distance(from: payload.startIndex, to: index)
                    payload[index] ^= buffer[buffer.index(maskStart, offsetBy: offset % 4)]
                }
            }
            buffer.removeSubrange(..<buffer.index(buffer.startIndex, offsetBy: cursor + maskLength + length))
            if opcode == 8 {
                connection.cancel()
                return
            }
            if opcode == 9 {
                var pong = Data([0x8A, UInt8(min(payload.count, 125))])
                pong.append(payload.prefix(125))
                connection.send(content: pong, completion: .contentProcessed { _ in })
                continue
            }
            if opcode == 0 || opcode == 1 {
                fragment.append(payload)
                if (b0 & 0x80) != 0 {
                    deliverText(fragment)
                    fragment.removeAll()
                }
            }
        }
    }

    private func deliverText(_ data: Data) {
        guard let object = (try? JSONSerialization.jsonObject(with: data)) as? [String: Any] else { return }
        server?.deliver(object)
    }

    private func mime(_ ext: String) -> String {
        switch ext {
        case "html": return "text/html; charset=utf-8"
        case "js": return "text/javascript; charset=utf-8"
        case "css": return "text/css; charset=utf-8"
        case "svg": return "image/svg+xml"
        case "json": return "application/json"
        default: return "application/octet-stream"
        }
    }
}

private func passwordsMatch(_ given: String) -> Bool {
    let expected = Array(PanelWebServer.password.utf8)
    let actual = Array(given.utf8)
    var difference = expected.count ^ actual.count
    let count = max(expected.count, actual.count)
    for index in 0..<count {
        let left = index < expected.count ? Int(expected[index]) : 0
        let right = index < actual.count ? Int(actual[index]) : 0
        difference |= left ^ right
    }
    return difference == 0
}
