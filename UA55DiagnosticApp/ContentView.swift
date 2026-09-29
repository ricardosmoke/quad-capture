import SwiftUI

struct ContentView: View {
    @StateObject private var installer = DextInstaller()

    private let logCommand = "log stream --style compact --predicate 'eventMessage CONTAINS \"[UA55]\"'"

    private var appBuildLabel: String {
        let appVersion = Bundle.main.infoDictionary?["CFBundleVersion"] as? String ?? "?"
        let dextURL = Bundle.main.bundleURL
            .appendingPathComponent("Contents/Library/SystemExtensions/dev.ua55.UA55DiagnosticApp.driver.dext")
        let dextVersion = Bundle(url: dextURL)?.infoDictionary?["CFBundleVersion"] as? String ?? "?"
        return "App build \(appVersion) · dext build \(dextVersion)"
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("QUAD-CAPTURE UA-55")
                .font(.title2.weight(.semibold))
            Text(appBuildLabel)
                .font(.system(.body, design: .monospaced))
                .foregroundStyle(.secondary)
            Text("Driver de áudio (AudioDriverKit). Depois de ativar, a placa deve aparecer no Audio MIDI Setup com 4 saídas e 6 entradas a 44.1 kHz.")
                .foregroundStyle(.secondary)

            Text(installer.status)
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding(12)
                .background(Color(nsColor: .windowBackgroundColor))
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(Color.secondary.opacity(0.3))
                )

            if !installer.bundleIdentifier.isEmpty {
                Text(installer.bundleIdentifier)
                    .font(.system(.body, design: .monospaced))
                    .textSelection(.enabled)
            }

            HStack {
                Button("Ativar driver") {
                    installer.activate()
                }
                Button("Desativar") {
                    installer.deactivate()
                }
            }

            Text("1. Ativar driver → aprovar se pedir.\n2. Confirma no log: [UA55] ========== BUILD N loaded ==========\n3. Reconecta a UA-55 se a build não aparecer.\n4. Audio MIDI Setup → QUAD-CAPTURE UA-55.")
            Text(logCommand)
                .font(.system(.body, design: .monospaced))
                .textSelection(.enabled)
                .padding(12)
                .frame(maxWidth: .infinity, alignment: .leading)
                .background(Color(nsColor: .textBackgroundColor))

            Text("[UA55] ========== BUILD 28 loaded (device matched) ==========\n[UA55] build=28 StartIO success (HAL IOOp bridge)\n[UA55] playback ... underruns=N")
                .font(.system(.callout, design: .monospaced))
                .textSelection(.enabled)
        }
        .padding(24)
        .frame(minWidth: 640, minHeight: 420)
    }
}
