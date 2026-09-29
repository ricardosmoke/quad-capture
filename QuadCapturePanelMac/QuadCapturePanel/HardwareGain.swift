import Foundation

/// Conversão local de SENS (0–54 dB). Sem I/O HAL — a dext estável (34)
/// não expõe telemetria de ganho; MIDI OUT/RQ1 derrubava o device (-308).
enum HardwareGain {
    static let sensMinDb: Double = 0
    static let sensMaxDb: Double = 54

    static func db(fromNormalized value: Double) -> Int {
        let clamped = min(1, max(0, value))
        return Int((clamped * sensMaxDb).rounded())
    }

    static func normalized(fromDb db: Int) -> Double {
        Double(min(Int(sensMaxDb), max(Int(sensMinDb), db))) / sensMaxDb
    }
}
