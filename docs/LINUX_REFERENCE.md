# Referência Linux

Fonte consultada: `sound/usb/quirks-table.h` no ramo principal do kernel, entrada `USB_DEVICE(0x0582, 0x012f)`. Isso é quirk do ALSA, não um dump do configuration descriptor deste aparelho. O driver macOS não copia esses números.

Resumo da entrada que limita o dispositivo a 44.1 kHz:

| Interface | Papel no quirk | Alternate | Endpoint | Atributo | Formato declarado |
| --- | --- | --- | --- | --- | --- |
| 0 | áudio fixo | 1 | `0x05` | `0x05` | S32_LE, 4 canais, 44100 |
| 1 | áudio fixo | 1 | `0x85` | `0x25` | S32_LE, 6 canais, 44100 |
| 2 | MIDI fixo | — | cabos in/out `0x0001` | — | um cabo em cada direção |
| 3 | ignorada | — | — | — | — |
| 4 | ignorada | — | — | — | — |

URL: https://github.com/torvalds/linux/blob/master/sound/usb/quirks-table.h

Patches posteriores, ainda não tratados como comportamento deste hardware, descrevem alternate settings 2, 3 e 4 para 48, 96 e 192 kHz e uma request vendor de clock também associada à OCTA-CAPTURE `0582:0120`. Este projeto não envia essa request e não faz match de `0x0120`.

MultiRolandDriver reconhece o PID `0x012F` e declara que, nesse composto áudio+MIDI, reivindica só a interface MIDI. Não é implementação de áudio da QUAD-CAPTURE.

Uso permitido destes dados: comparar, depois, com as linhas `[UA55] interface` e `[UA55] endpoint` lidas do aparelho. Enquanto essa comparação não existir, endpoint, alternate setting e canal continuam hipótese.
