import CoreAudio
import CoreMIDI
import Foundation

/// DT1 de LO-CUT da UA-55, no cabo USB MIDI 1 (o cabo 0 é o DIN).
enum LoCutMIDI {
    private static var client = MIDIClientRef()
    private static var output = MIDIPortRef()
    private static var opened = false
    /// 'uLct' — propriedade do device de áudio. O dext escreve o pacote no bulk OUT 0x06.
    private static let driverSelector: AudioObjectPropertySelector = 0x754C6374

    /// `channel` 0 é o canal 1 da tela. `on` true envia 01.
    static func send(channel: UInt8, on: Bool) -> String? {
        guard channel <= 1 else { return "Falha ao enviar LO-CUT" }
        let sysex = message(channel: channel, on: on)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logSend(channel: channel, on: on, dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao enviar LO-CUT (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logSend(channel: channel, on: on, dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao enviar LO-CUT (\(status))"
        }
        return nil
    }

    /// Igual ao LO-CUT, com o parâmetro 02.
    static func sendPhase(channel: UInt8, on: Bool) -> String? {
        guard channel <= 1 else { return "Falha ao enviar PHASE" }
        let sysex = phaseMessage(channel: channel, on: on)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logSend(channel: channel, on: on, dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes, label: "phase")
            if status != noErr {
                return "Falha ao enviar PHASE (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logSend(channel: channel, on: on, dest: "driver", wrap: true, status: status, bytes: packets, label: "phase")
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao enviar PHASE (\(status))"
        }
        return nil
    }

    /// BYPASS do compressor. O canal é o da placa: 1 é a faixa 1 da tela. 01 deixa o compressor em bypass.
    static func sendBypass(channel: UInt8, on: Bool) -> String? {
        guard channel <= 1 else { return "Falha ao enviar BYPASS" }
        let sysex = bypassMessage(channel: channel, on: on)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logSend(channel: channel, on: on, dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes, label: "bypass")
            if status != noErr {
                return "Falha ao enviar BYPASS (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logSend(channel: channel, on: on, dest: "driver", wrap: true, status: status, bytes: packets, label: "bypass")
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao enviar BYPASS (\(status))"
        }
        return nil
    }

    /// LINK do compressor. Endereço fixo 00 05 00 05. 01 liga.
    static func sendLink(on: Bool) -> String? {
        let sysex = linkMessage(on: on)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("link on=\(on ? 1 : 0)", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao enviar LINK (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("link on=\(on ? 1 : 0)", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao enviar LINK (\(status))"
        }
        return nil
    }

    /// GATE do compressor. Faixa 1 é o canal 0. O passo 0 é -INF e o 50 é -20 dB.
    static func sendGate(channel: UInt8, step: UInt8) -> String? {
        guard channel <= 1, step <= 50 else { return "Falha ao enviar GATE" }
        let sysex = gateMessage(channel: channel, step: step)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("gate ch=\(channel) step=\(step)", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao enviar GATE (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("gate ch=\(channel) step=\(step)", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao enviar GATE (\(status))"
        }
        return nil
    }

    /// THRESHOLD do compressor. Faixa 1 é o canal 0. O passo 0 é -50 dB e o 50 é 0 dB.
    static func sendThreshold(channel: UInt8, step: UInt8) -> String? {
        guard channel <= 1, step <= 50 else { return "Falha ao enviar THRESHOLD" }
        let sysex = thresholdMessage(channel: channel, step: step)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("threshold ch=\(channel) step=\(step)", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao enviar THRESHOLD (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("threshold ch=\(channel) step=\(step)", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao enviar THRESHOLD (\(status))"
        }
        return nil
    }

    /// RATIO do compressor. Faixa 1 é o canal 0. O passo 0 é 1:1.0 e o 8 é 1:INF.
    static func sendRatio(channel: UInt8, step: UInt8) -> String? {
        guard channel <= 1, step <= 8 else { return "Falha ao enviar RATIO" }
        let sysex = ratioMessage(channel: channel, step: step)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("ratio ch=\(channel) step=\(step)", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao enviar RATIO (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("ratio ch=\(channel) step=\(step)", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao enviar RATIO (\(status))"
        }
        return nil
    }

    /// Pressionamento do AUTO-SENS. O dado é sempre 01; a placa liga e desliga sozinha.
    static func sendAutoSens() -> String? {
        let sysex = autoSensMessage()
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("autosens", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao enviar AUTO-SENS (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("autosens", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao enviar AUTO-SENS (\(status))"
        }
        return nil
    }

    /// Uma vez por conexão. A resposta DT1 só atualiza a tela.
    static func requestState() -> String? {
        let sysex = stateRequest()
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("state rq1", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao ler o estado (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("state rq1", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao ler o estado (\(status))"
        }
        return nil
    }

    /// Um byte em 00 05 <canal> 06. Cada faixa tem a própria resposta.
    static func requestBypass(channel: UInt8) -> String? {
        guard channel <= 1 else { return "Falha ao ler o BYPASS" }
        let sysex = bypassRequest(channel: channel)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("bypass rq1 ch=\(channel)", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao ler o BYPASS (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("bypass rq1 ch=\(channel)", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao ler o BYPASS (\(status))"
        }
        return nil
    }

    /// Um byte em 00 05 <canal> 07. Faixa 1 é o canal 0. A resposta só move o knob.
    static func requestGate(channel: UInt8) -> String? {
        guard channel <= 1 else { return "Falha ao ler o GATE" }
        let sysex = gateRequest(channel: channel)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("gate rq1 ch=\(channel)", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao ler o GATE (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("gate rq1 ch=\(channel)", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao ler o GATE (\(status))"
        }
        return nil
    }

    /// Um byte em 00 05 00 05. A resposta só atualiza o LINK.
    static func requestLink() -> String? {
        let sysex = linkRequest()
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("link rq1", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao ler o LINK (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("link rq1", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao ler o LINK (\(status))"
        }
        return nil
    }

    /// SENS: canal 0 é o knob 1. `step` é o dB da tela vezes 2, de 0 a 108.
    static func sendSens(channel: UInt8, step: UInt8) -> String? {
        guard channel <= 1, step <= 108 else { return "Falha ao enviar SENS" }
        let sysex = sensMessage(channel: channel, step: step)
        if let target = findTarget() {
            let bytes = target.wrapCable ? usbPackets(cable: 1, sysex: sysex) : sysex
            let status = transmit(bytes, to: target.endpoint)
            logFixed("sens ch=\(channel) step=\(step)", dest: target.name, wrap: target.wrapCable, status: status, bytes: bytes)
            if status != noErr {
                return "Falha ao enviar SENS (\(status))"
            }
            return nil
        }

        let packets = usbPackets(cable: 1, sysex: sysex)
        let status = sendToDriver(packets)
        logFixed("sens ch=\(channel) step=\(step)", dest: "driver", wrap: true, status: status, bytes: packets)
        if status != noErr {
            if status == kAudioHardwareBadDeviceError {
                return "QUAD-CAPTURE não encontrada"
            }
            return "Falha ao enviar SENS (\(status))"
        }
        return nil
    }

    private static func logFixed(_ label: String, dest: String, wrap: Bool, status: OSStatus, bytes: [UInt8]) {
        let hex = bytes.map { String(format: "%02X", $0) }.joined(separator: " ")
        PanelLog.write("\(label) dest=\(dest) wrap=\(wrap ? 1 : 0) status=\(status) \(hex)")
    }

    private static func logSend(channel: UInt8, on: Bool, dest: String, wrap: Bool, status: OSStatus, bytes: [UInt8], label: String = "lo-cut") {
        let hex = bytes.map { String(format: "%02X", $0) }.joined(separator: " ")
        PanelLog.write("\(label) ch=\(channel) on=\(on ? 1 : 0) dest=\(dest) wrap=\(wrap ? 1 : 0) status=\(status) \(hex)")
    }

    /// O CoreMIDI não publica a UA-55. O dext recebe estes bytes pela propriedade de áudio 'uLct'.
    private static func sendToDriver(_ bytes: [UInt8]) -> OSStatus {
        guard let device = UA55Device.find() else { return kAudioHardwareBadDeviceError }
        let hex = bytes.map { String(format: "%02X", $0) }.joined() as CFString
        var raw = Unmanaged.passUnretained(hex).toOpaque()
        var address = AudioObjectPropertyAddress(
            mSelector: driverSelector,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: kAudioObjectPropertyElementMain)
        return AudioObjectSetPropertyData(
            device, &address, 0, nil, UInt32(MemoryLayout<UnsafeMutableRawPointer?>.size), &raw)
    }

    private static func message(channel: UInt8, on: Bool) -> [UInt8] {
        let value: UInt8 = on ? 0x01 : 0x00
        let total = 0x00 + 0x05 + Int(channel) + 0x01 + Int(value)
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x12, 0x00, 0x05, channel, 0x01, value, sum, 0xF7]
    }

    private static func phaseMessage(channel: UInt8, on: Bool) -> [UInt8] {
        let value: UInt8 = on ? 0x01 : 0x00
        let total = 0x00 + 0x05 + Int(channel) + 0x02 + Int(value)
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x12, 0x00, 0x05, channel, 0x02, value, sum, 0xF7]
    }

    private static func linkMessage(on: Bool) -> [UInt8] {
        let value: UInt8 = on ? 0x01 : 0x00
        let total = 0x00 + 0x05 + 0x00 + 0x05 + Int(value)
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x12, 0x00, 0x05, 0x00, 0x05, value, sum, 0xF7]
    }

    private static func gateMessage(channel: UInt8, step: UInt8) -> [UInt8] {
        let total = 0x00 + 0x05 + Int(channel) + 0x07 + Int(step)
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x12, 0x00, 0x05, channel, 0x07, step, sum, 0xF7]
    }

    private static func thresholdMessage(channel: UInt8, step: UInt8) -> [UInt8] {
        let total = 0x00 + 0x05 + Int(channel) + 0x0A + Int(step)
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x12, 0x00, 0x05, channel, 0x0A, step, sum, 0xF7]
    }

    private static func ratioMessage(channel: UInt8, step: UInt8) -> [UInt8] {
        let total = 0x00 + 0x05 + Int(channel) + 0x0B + Int(step)
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x12, 0x00, 0x05, channel, 0x0B, step, sum, 0xF7]
    }

    private static func bypassMessage(channel: UInt8, on: Bool) -> [UInt8] {
        let value: UInt8 = on ? 0x01 : 0x00
        let total = 0x00 + 0x05 + Int(channel) + 0x06 + Int(value)
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x12, 0x00, 0x05, channel, 0x06, value, sum, 0xF7]
    }

    private static func autoSensMessage() -> [UInt8] {
        let total = 0x00 + 0x02 + 0x01 + 0x02 + 0x01
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x12, 0x00, 0x02, 0x01, 0x02, 0x01, sum, 0xF7]
    }

    /// RQ1 de 1 byte em 00 05 <canal> 07. O checksum é (0 - soma) & 0x7F sobre o endereço e o tamanho.
    private static func gateRequest(channel: UInt8) -> [UInt8] {
        let total = 0x00 + 0x05 + Int(channel) + 0x07 + 0x00 + 0x00 + 0x00 + 0x01
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x11, 0x00, 0x05, channel, 0x07, 0x00, 0x00, 0x00, 0x01, sum, 0xF7]
    }

    /// RQ1 de 1 byte em 00 05 00 05. O checksum é (0 - soma) & 0x7F sobre o endereço e o tamanho.
    private static func linkRequest() -> [UInt8] {
        let total = 0x00 + 0x05 + 0x00 + 0x05 + 0x00 + 0x00 + 0x00 + 0x01
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x11, 0x00, 0x05, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, sum, 0xF7]
    }

    /// RQ1 de 1 byte. O checksum é (0 - soma) & 0x7F sobre o endereço e o tamanho.
    private static func bypassRequest(channel: UInt8) -> [UInt8] {
        let total = 0x00 + 0x05 + Int(channel) + 0x06 + 0x00 + 0x00 + 0x00 + 0x01
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x11, 0x00, 0x05, channel, 0x06, 0x00, 0x00, 0x00, 0x01, sum, 0xF7]
    }

    /// RQ1 01 00 00 00, tamanho 00 00 00 3B. O checksum 44 é (0 - soma) & 0x7F.
    private static func stateRequest() -> [UInt8] {
        let total = 0x01 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 0x00 + 0x3B
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, sum, 0xF7]
    }

    private static func sensMessage(channel: UInt8, step: UInt8) -> [UInt8] {
        let total = 0x00 + 0x05 + Int(channel) + 0x04 + Int(step)
        let sum = UInt8((0 - total) & 0x7F)
        return [0xF0, 0x41, 0x10, 0x00, 0x00, 0x56, 0x12, 0x00, 0x05, channel, 0x04, step, sum, 0xF7]
    }

    /// Pacotes USB MIDI 1.0. Nibble alto = cabo. O fim de 2 bytes usa CIN 0x6.
    private static func usbPackets(cable: UInt8, sysex: [UInt8]) -> [UInt8] {
        var out: [UInt8] = []
        var index = 0
        while index < sysex.count {
            let take = min(3, sysex.count - index)
            let cin: UInt8
            if take == 3 {
                cin = sysex[index + 2] == 0xF7 ? 0x07 : 0x04
            } else if take == 2 {
                cin = 0x06
            } else {
                cin = 0x05
            }
            out.append((cable << 4) | cin)
            out.append(sysex[index])
            out.append(take > 1 ? sysex[index + 1] : 0)
            out.append(take > 2 ? sysex[index + 2] : 0)
            index += take
        }
        return out
    }

    private struct Target {
        var endpoint: MIDIEndpointRef
        var name: String
        /// CoreMIDI só publicou o DIN: os bytes já vão com o nibble do cabo 1.
        var wrapCable: Bool
    }

    private static func findTarget() -> Target? {
        let devices = MIDIGetNumberOfDevices()
        for index in 0..<devices {
            let device = MIDIGetDevice(index)
            guard device != 0, isQuad(device) else { continue }
            let entities = MIDIDeviceGetNumberOfEntities(device)
            if entities >= 2, let destination = firstDestination(MIDIDeviceGetEntity(device, 1)) {
                return Target(endpoint: destination, name: displayName(destination), wrapCable: false)
            }
            if entities >= 1, let destination = firstDestination(MIDIDeviceGetEntity(device, 0)) {
                return Target(endpoint: destination, name: displayName(destination), wrapCable: true)
            }
        }

        var found: [(MIDIEndpointRef, String)] = []
        let count = MIDIGetNumberOfDestinations()
        for index in 0..<count {
            let destination = MIDIGetDestination(index)
            guard destination != 0 else { continue }
            let name = displayName(destination)
            if isQuadName(name) {
                found.append((destination, name))
            }
        }
        if let control = found.first(where: { isControlPort($0.1) }) {
            return Target(endpoint: control.0, name: control.1, wrapCable: false)
        }
        if found.count == 1 {
            return Target(endpoint: found[0].0, name: found[0].1, wrapCable: true)
        }
        return nil
    }

    private static func firstDestination(_ entity: MIDIEntityRef) -> MIDIEndpointRef? {
        guard entity != 0, MIDIEntityGetNumberOfDestinations(entity) > 0 else { return nil }
        let destination = MIDIEntityGetDestination(entity, 0)
        return destination == 0 ? nil : destination
    }

    private static func isQuad(_ object: MIDIObjectRef) -> Bool {
        isQuadName(displayName(object)) || isQuadName(stringProperty(object, kMIDIPropertyModel))
    }

    private static func isQuadName(_ name: String) -> Bool {
        let folded = name.lowercased()
        return folded.contains("quad-capture") || folded.contains("quad capture") || folded.contains("ua-55") || folded.contains("ua55")
    }

    /// Porta 2 do CoreMIDI é o cabo 1 (a porta 1 é o DIN).
    private static func isControlPort(_ name: String) -> Bool {
        let folded = name.lowercased()
        return folded.contains("port 2") || folded.contains("midi 2") || folded.contains("cable 2")
            || folded.contains("ctrl") || folded.contains("control")
    }

    private static func displayName(_ object: MIDIObjectRef) -> String {
        let display = stringProperty(object, kMIDIPropertyDisplayName)
        if !display.isEmpty { return display }
        return stringProperty(object, kMIDIPropertyName)
    }

    private static func stringProperty(_ object: MIDIObjectRef, _ key: CFString) -> String {
        var value: Unmanaged<CFString>?
        guard MIDIObjectGetStringProperty(object, key, &value) == noErr, let value else { return "" }
        return value.takeRetainedValue() as String
    }

    private static func transmit(_ bytes: [UInt8], to destination: MIDIEndpointRef) -> OSStatus {
        let readyStatus = prepare()
        if readyStatus != noErr { return readyStatus }
        var storage = [UInt8](repeating: 0, count: 256)
        return storage.withUnsafeMutableBytes { raw in
            guard let base = raw.baseAddress else { return kMIDIInvalidClient }
            let list = base.assumingMemoryBound(to: MIDIPacketList.self)
            let packet = MIDIPacketListInit(list)
            let added = bytes.withUnsafeBufferPointer { buffer -> Bool in
                guard let data = buffer.baseAddress else { return false }
                return MIDIPacketListAdd(list, 256, packet, 0, buffer.count, data) != nil
            }
            guard added else { return kMIDIUnknownError }
            return MIDISend(output, destination, list)
        }
    }

    private static func prepare() -> OSStatus {
        if opened { return noErr }
        var status = MIDIClientCreate("QuadCapturePanel" as CFString, nil, nil, &client)
        if status != noErr { return status }
        status = MIDIOutputPortCreate(client, "LoCut" as CFString, &output)
        if status != noErr { return status }
        opened = true
        return noErr
    }
}
