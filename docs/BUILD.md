# Build and install

The dext builds only in Xcode, on an Apple Silicon Mac, with the DriverKit SDK.

## Account and capabilities

1. Open `UA55Diagnostic.xcodeproj`.
2. On the `UA55DiagnosticApp` target and the `UA55DiagnosticDriver` target, select the same Team.
3. In the developer portal, the driver App ID needs:
   - DriverKit
   - DriverKit USB Transport (current entitlement: `idVendor=*`)
   - **DriverKit Family Audio** (`com.apple.developer.driverkit.family.audio`)
4. The application App ID needs System Extension.
5. Regenerate the driver provisioning profile after adding Family Audio. The current identifiers are:
   - App: `dev.ua55.UA55DiagnosticApp`
   - Driver: `dev.ua55.UA55DiagnosticApp.driver`

`IOUserServerName` in `Info.plist` uses `$(PRODUCT_BUNDLE_IDENTIFIER)` and must stay equal to the dext bundle ID.

## Build

Select the `UA55DiagnosticApp` scheme, destination My Mac, and run.

```bash
xcodebuild -project UA55Diagnostic.xcodeproj -scheme UA55DiagnosticApp -destination 'platform=macOS,arch=arm64' build
```

Parser without hardware:

```bash
c++ -std=c++17 -I Shared Tests/DescriptorWalkTest.cpp Shared/UA55DescriptorWalk.cpp -o /tmp/UA55DescriptorTests
/tmp/UA55DescriptorTests
```

## Activate

1. Copy the app to `/Applications` (system extensions require that).
2. Connect the QUAD-CAPTURE.
3. In the app: **Deactivate** → **Activate driver**, then approve the extension in System Settings.
4. Reconnect the interface.
5. Check:

```bash
systemextensionsctl list
codesign -d --entitlements :- /Applications/UA55DiagnosticApp.app/Contents/Library/SystemExtensions/dev.ua55.UA55DiagnosticApp.driver.dext
log show --last 10m --style compact --predicate 'eventMessage CONTAINS "[UA55]"'
```

6. Open **Audio MIDI Setup**. The device is `QUAD-CAPTURE UA-55`: 4 out / 6 in at 44.1, 48, and 96 kHz, and 2 out / 2 in at 192 kHz.
7. In System Settings → Sound, choose the interface as the output and play audio from the Mac.
8. Optional: record an input in QuickTime.

Expected version: marketing `0.1.0`, build `74`. On plug-in the driver prints `[UA55] ========== BUILD 74 loaded`.

## Development

`systemextensionsctl developer on` reduces friction. SIP stays with the machine's policy.

To remove the extension, use **Deactivate** in the app or `systemextensionsctl list`.
