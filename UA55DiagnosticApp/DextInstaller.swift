import Foundation
import SystemExtensions

@MainActor
final class DextInstaller: NSObject, ObservableObject {
    @Published var status = "Extensão ainda não ativada."
    @Published var bundleIdentifier = ""

    func activate() {
        guard let identifier = embeddedDextIdentifier() else {
            status = "O app não contém UA55DiagnosticDriver.dext."
            bundleIdentifier = ""
            return
        }
        bundleIdentifier = identifier
        status = "Pedindo ativação de \(identifier)."
        let request = OSSystemExtensionRequest.activationRequest(
            forExtensionWithIdentifier: identifier,
            queue: .global(qos: .userInitiated)
        )
        request.delegate = self
        OSSystemExtensionManager.shared.submitRequest(request)
    }

    func deactivate() {
        guard let identifier = embeddedDextIdentifier() else {
            status = "O app não contém UA55DiagnosticDriver.dext."
            return
        }
        bundleIdentifier = identifier
        status = "Pedindo desativação de \(identifier)."
        let request = OSSystemExtensionRequest.deactivationRequest(
            forExtensionWithIdentifier: identifier,
            queue: .global(qos: .userInitiated)
        )
        request.delegate = self
        OSSystemExtensionManager.shared.submitRequest(request)
    }

    private func embeddedDextIdentifier() -> String? {
        let extensionsURL = Bundle.main.bundleURL.appendingPathComponent("Contents/Library/SystemExtensions")
        guard let enumerator = FileManager.default.enumerator(
            at: extensionsURL,
            includingPropertiesForKeys: nil,
            options: [.skipsHiddenFiles]
        ) else {
            return nil
        }

        for case let url as URL in enumerator {
            var isDirectory: ObjCBool = false
            guard FileManager.default.fileExists(atPath: url.path, isDirectory: &isDirectory),
                  isDirectory.boolValue,
                  url.pathExtension == "dext",
                  let bundle = Bundle(url: url),
                  let identifier = bundle.bundleIdentifier else {
                continue
            }
            return identifier
        }
        return nil
    }
}

extension DextInstaller: OSSystemExtensionRequestDelegate {
    nonisolated func request(
        _ request: OSSystemExtensionRequest,
        actionForReplacingExtension existing: OSSystemExtensionProperties,
        withExtension extensionProperties: OSSystemExtensionProperties
    ) -> OSSystemExtensionRequest.ReplacementAction {
        .replace
    }

    nonisolated func requestNeedsUserApproval(_ request: OSSystemExtensionRequest) {
        Task { @MainActor in
            self.status = "O macOS pediu aprovação em Ajustes do Sistema, na lista de extensões de driver."
        }
    }

    nonisolated func request(
        _ request: OSSystemExtensionRequest,
        didFinishWithResult result: OSSystemExtensionRequest.Result
    ) {
        Task { @MainActor in
            switch result {
            case .completed:
                self.status = "Extensão ativada. Desconecte e conecte a QUAD-CAPTURE, depois leia o log."
            case .willCompleteAfterReboot:
                self.status = "A extensão só conclui a ativação depois de reiniciar o Mac."
            @unknown default:
                self.status = "Ativação terminou com resultado \(result.rawValue)."
            }
        }
    }

    nonisolated func request(_ request: OSSystemExtensionRequest, didFailWithError error: Error) {
        let message = error.localizedDescription
        Task { @MainActor in
            self.status = "Falha na ativação: \(message)"
        }
    }
}
