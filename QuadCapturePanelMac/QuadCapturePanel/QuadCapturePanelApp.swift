import AppKit
import SwiftUI

@main
struct QuadCapturePanelApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) private var appDelegate

    var body: some Scene {
        // Scene vazio — a janela é criada em AppDelegate (WindowGroup
        // estava a abrir o processo sem nenhuma NSWindow neste macOS).
        Settings {
            EmptyView()
        }
    }
}

final class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.regular)

        let root = PanelView()
            .frame(minWidth: 900, minHeight: 520)

        let hosting = NSHostingController(rootView: root)
        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 930, height: 592),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false)
        window.contentViewController = hosting
        window.setContentSize(NSSize(width: 930, height: 592))
        window.title = "QUAD-CAPTURE Painel de controle"
        window.isReleasedWhenClosed = false
        // Sem autosave — evita "Unable to find className=(null)" na restauro.
        window.makeKeyAndOrderFront(nil)
        placeCentered(window)
        self.window = window
        // O hosting ajusta o frame no próximo ciclo. Centraliza de novo com o tamanho final.
        DispatchQueue.main.async { [weak self] in
            guard let window = self?.window else { return }
            self?.placeCentered(window)
        }

        NSApp.activate(ignoringOtherApps: true)
    }

    /// Meio da área útil (abaixo da barra de menus e acima do Dock).
    private func placeCentered(_ window: NSWindow) {
        let screen = window.screen ?? NSScreen.main
        guard let screen else { return }
        let visible = screen.visibleFrame
        var origin = NSPoint(
            x: (visible.midX - window.frame.width / 2).rounded(),
            y: (visible.midY - window.frame.height / 2).rounded())
        origin.x = min(max(origin.x, visible.minX), visible.maxX - window.frame.width)
        origin.y = min(max(origin.y, visible.minY), visible.maxY - window.frame.height)
        window.setFrameOrigin(origin)
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        true
    }

    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        if !flag {
            window?.makeKeyAndOrderFront(nil)
        }
        return true
    }
}
