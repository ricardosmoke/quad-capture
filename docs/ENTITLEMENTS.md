# Entitlements

Os arquivos assinados são a fonte do que o binário pode fazer.

## Dext — `UA55DiagnosticDriver/UA55DiagnosticDriver.entitlements`

| Chave | Valor | Motivo |
| --- | --- | --- |
| `com.apple.developer.driverkit` | `true` | Permite carregar a dext |
| `com.apple.developer.driverkit.family.audio` | `true` | AudioDriverKit / Core Audio HAL |
| `com.apple.developer.driverkit.transport.usb` | vendor `*` | Abre o dispositivo USB (mesmo padrão validado no marco 3) |

No portal/Xcode: capability **DriverKit Family Audio** no App ID do driver, além de DriverKit e USB Transport. Regenere o profile depois de adicionar a capability — o mesmo padrão do entitlement USB.

Não há `com.apple.developer.driverkit.allow-any-userclient-access`. O user client de áudio é criado pelo AudioDriverKit para o host Core Audio.

## App — `UA55DiagnosticApp/UA55DiagnosticApp.entitlements`

| Chave | Valor | Motivo |
| --- | --- | --- |
| `com.apple.developer.system-extension.install` | `true` | `OSSystemExtensionRequest` ativa a dext embutida |
| `com.apple.security.app-sandbox` | `true` | O app só instala a extensão e mostra instruções |

## Personalidade IOKit

`UA55DiagnosticDriver/Info.plist`:

- `IOProviderClass` = `IOUSBHostDevice`
- `IOUserClass` = `UA55AudioDriver`
- `IOUserServerName` = bundle ID da dext (`dev.ua55.UA55DiagnosticApp.driver`)
- `IOUserAudioDriverUserClientProperties` → `IOUserAudioDriverUserClient`
- `idVendor` = `1410`, `idProduct` = `303`

## Conferir assinatura

```bash
codesign -d --entitlements :- /Applications/UA55DiagnosticApp.app/Contents/Library/SystemExtensions/dev.ua55.UA55DiagnosticApp.driver.dext
```

Se `family.audio` não aparecer no binário, o dext falha no launch (mesmo padrão do `transport.usb`).
