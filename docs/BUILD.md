# Compilar e instalar

O dext só compila no Xcode, em Mac Apple Silicon, com o SDK DriverKit.

## Conta e capabilities

1. Abra `UA55Diagnostic.xcodeproj`.
2. No alvo `UA55DiagnosticApp` e no alvo `UA55DiagnosticDriver`, escolha o mesmo Team.
3. No portal de desenvolvedor, o App ID do driver precisa de:
   - DriverKit
   - DriverKit USB Transport (no entitlement atual: `idVendor=*`)
   - **DriverKit Family Audio** (`com.apple.developer.driverkit.family.audio`)
4. O App ID do aplicativo precisa de System Extension.
5. Regenere o provisioning profile do driver depois de adicionar Family Audio. Os identifiers atuais são:
   - App: `dev.ua55.UA55DiagnosticApp`
   - Driver: `dev.ua55.UA55DiagnosticApp.driver`

`IOUserServerName` na `Info.plist` usa `$(PRODUCT_BUNDLE_IDENTIFIER)` e precisa continuar igual ao bundle ID da dext.

## Build

Selecione o scheme `UA55DiagnosticApp`, destino My Mac, e rode.

```bash
xcodebuild -project UA55Diagnostic.xcodeproj -scheme UA55DiagnosticApp -destination 'platform=macOS,arch=arm64' build
```

Parser sem hardware:

```bash
c++ -std=c++17 -I Shared Tests/DescriptorWalkTest.cpp Shared/UA55DescriptorWalk.cpp -o /tmp/UA55DescriptorTests
/tmp/UA55DescriptorTests
```

## Ativar (marco 4 — áudio)

1. Copie o app para `/Applications` (system extensions exigem isso).
2. Conecte a QUAD-CAPTURE.
3. No app: **Desativar** → **Ativar driver**; aprove a extensão em Ajustes.
4. Reconecte a interface.
5. Confira:

```bash
systemextensionsctl list
codesign -d --entitlements :- /Applications/UA55DiagnosticApp.app/Contents/Library/SystemExtensions/dev.ua55.UA55DiagnosticApp.driver.dext
log show --last 10m --style compact --predicate 'eventMessage CONTAINS "[UA55]"'
```

6. Abra **Audio MIDI Setup** — device `QUAD-CAPTURE UA-55` com 4 out / 6 in @ 44.1 kHz.
7. Em Ajustes → Som, escolha a placa como saída e toque áudio do Mac.
8. Opcional: grave a entrada no QuickTime.

Versão esperada: marketing `0.1.0`, build `8`.

## Desenvolvimento

`systemextensionsctl developer on` reduz o atrito. SIP fica de acordo com a política da máquina.

Para remover a extensão, use **Desativar** no app ou `systemextensionsctl list`.
