# Entitlements

The signed files are what the binary is allowed to do.

## Dext — `UA55DiagnosticDriver/UA55DiagnosticDriver.entitlements`

| Key | Value | Why |
| --- | --- | --- |
| `com.apple.developer.driverkit` | `true` | Allows the dext to load |
| `com.apple.developer.driverkit.family.audio` | `true` | AudioDriverKit / Core Audio HAL |
| `com.apple.developer.driverkit.transport.usb` | vendor `*` | Opens the USB device (same pattern validated in milestone 3) |

In the portal and in Xcode: the **DriverKit Family Audio** capability on the driver App ID, in addition to DriverKit and USB Transport. Regenerate the profile after adding the capability — the same pattern as the USB entitlement.

There is no `com.apple.developer.driverkit.allow-any-userclient-access`. AudioDriverKit creates the audio user client for the Core Audio host.

## App — `UA55DiagnosticApp/UA55DiagnosticApp.entitlements`

| Key | Value | Why |
| --- | --- | --- |
| `com.apple.developer.system-extension.install` | `true` | `OSSystemExtensionRequest` activates the embedded dext |
| `com.apple.security.app-sandbox` | `true` | The app only installs the extension and shows instructions |

## Panel — `QuadCapturePanelMac/QuadCapturePanel/QuadCapturePanel.entitlements`

| Key | Value | Why |
| --- | --- | --- |
| `com.apple.developer.driverkit.userclient-access` | `dev.ua55.UA55DiagnosticApp.driver` | Lets the panel try the driver user client. SENS still comes from the driver log when that open fails |

The panel is not sandboxed. Its local web server (port 8745, password `QuadCapture`) listens from the same process and does not add an entitlement. Do not add `com.apple.developer.driverkit.allow-any-userclient-access`.

## IOKit personality

`UA55DiagnosticDriver/Info.plist`:

- `IOProviderClass` = `IOUSBHostDevice`
- `IOUserClass` = `UA55AudioDriver`
- `IOUserServerName` = dext bundle ID (`dev.ua55.UA55DiagnosticApp.driver`)
- `IOUserAudioDriverUserClientProperties` → `IOUserAudioDriverUserClient`
- `idVendor` = `1410`, `idProduct` = `303`

## Check the signature

```bash
codesign -d --entitlements :- /Applications/UA55DiagnosticApp.app/Contents/Library/SystemExtensions/dev.ua55.UA55DiagnosticApp.driver.dext
```

If `family.audio` is missing from the binary, the dext fails at launch (the same pattern as `transport.usb`).
