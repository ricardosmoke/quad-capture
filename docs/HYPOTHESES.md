# Hipóteses

Separação: fato medido neste Mac, layout USB 2.0, contrato Apple, referência Linux.

## Marco 1 — confirmado (2026-09-28)

- Dext `dev.ua55.UA55DiagnosticApp.driver` v0.1.0/2, entitlement USB `idVendor=*`.
- `[UA55] Start` → `SetConfiguration(1,true)=SUCCESS` → `diagnostic resident`.
- Device: VID `0x0582` PID `0x012F`, `bcdUSB=0x0200`, `deviceClass=0xff`, `bcdDevice=0x0100`, 480 Mbps.
- Config 1: `wTotalLength=511`, `bMaxPower=225` (450 mA), 5 interfaces, 19 iface / 17 ep / 26 other descriptors.
- `ioreg`: cinco `IOUSBHostInterface` (0–4), todas `bInterfaceClass=255`.

### Mapa medido (fonte: log `[UA55]`)

Descritores CS `0x24` subtype `0x02` (FORMAT_TYPE-like): `bNrChannels`, subframe 4, bitres 24 (`04 18`).

| IF | Alt | EP | Dir | Tipo | maxPacket | Canais (CS) | Papel |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 0 | — | — | — | — | — | idle |
| 0 | 1 | `0x05` | OUT | isoc attr`0x05` | 112 | 4 | playback |
| 0 | 2 | `0x05` | OUT | isoc | 112 | 4 | playback |
| 0 | 3 | `0x05` | OUT | isoc | 208 | 4 | playback |
| 0 | 4 | `0x05` | OUT | isoc | 200 | 2 | playback (2 ch) |
| 1 | 0 | — | — | — | — | — | idle |
| 1 | 1 | `0x85` | IN | isoc attr`0x25` | 168 | 6 | capture |
| 1 | 2 | `0x85` | IN | isoc | 168 | 6 | capture |
| 1 | 3 | `0x85` | IN | isoc | 312 | 6 | capture |
| 1 | 4 | `0x85` | IN | isoc | 200 | 2 | capture (2 ch) |
| 2 | 0 | `0x06`/`0x86` | OUT/IN | bulk 512 | — | — | MIDI |
| 2 | 1 | `0x06`/`0x86` | OUT/IN | interrupt 512 | — | — | MIDI alt |
| 3 | 0 | — | — | — | — | — | idle |
| 3 | 1 | `0x82` | IN | interrupt 8 | — | — | status/ctrl |
| 4 | 0 | — | — | — | — | — | idle |
| 4 | 1–4 | `0x81` | IN | interrupt 8 | — | — | status/ctrl (interval 1–4) |

Sanidade de pacote (HS, `bInterval=1` = 125 µs): `7 × 4ch × 4 B = 112`, `7 × 6ch × 4 B = 168`. Fecha com alt 1/2.

### vs quirk Linux (`quirks-table.h` 0x0582:0x012f)

| Item | Linux quirk | Medido neste aparelho |
| --- | --- | --- |
| Playback | IF0 alt1 EP`0x05` S32_LE 4ch 44.1k | IF0 alt1 EP`0x05` 4ch 24-in-32, maxPacket 112 |
| Capture | IF1 alt1 EP`0x85` S32_LE 6ch 44.1k | IF1 alt1 EP`0x85` 6ch 24-in-32, maxPacket 168 |
| MIDI | IF2 | IF2 bulk `0x06`/`0x86` |
| IF3/4 | ignoradas | interrupt status; não áudio |

O quirk Linux só declara alt 1 @ 44.1 kHz. O hardware expõe alts 2–4 (provável 48/96/192 e modo 2ch no alt 4). Frequências exatas **não** estão no dump truncado dos bytes CS; falta ler os 3 bytes de `tSamFreq` ou provar com stream.

## Marco 2 — confirmado no hardware (build 3/4)

Medido em 2026-09-28:

- Claim IF0/IF1, Open, `SelectAlternateSetting(1)`, `CopyPipe` → SUCCESS
- Capture `IsochIO` sync → `0x00000000`, 8/8 frames OK
- Pacotes `complete=144` e `120` (= 6 e 5 samples × 6 ch × 4 B) → **44.1 kHz HS**
- `nonzeroS32=60` → dados reais no buffer de captura
- Playback sync `IsochIO` **trava** o `Start` (EP OUT attr `0x05` async). Removido no build 4; volta no marco 3 como async/duplex

## Marco 3 — duplex async confirmado (build 7, 2026-09-28)

- `diagnostic resident` + `duplex async streaming started`
- Capture e playback: `errors=0`, completions contínuos (~1000/s com depth=4 × 8 µframes)
- Capture estabiliza em **~1 058 400 B/s** = 44100 × 6 ch × 4 B → 44.1 kHz confirmado em stream contínuo
- Playback envia silêncio a maxPacket cheio (112 × 8 × N) sem stall
- Buffer via `IOBufferMemoryDescriptor::Create` + submit fora do `Start` (evita deadlock do workloop)

## Marco 4 — AudioDriverKit (build 8)

Implementado no código:

- Entitlement `family.audio` + link `AudioDriverKit`
- `UA55AudioDriver` (`IOUserAudioDriver`) + `UA55AudioDevice` (4 out / 6 in Float32 @ 44.1 kHz)
- `UA55UsbStream` (anel isoc duplex reutilizável)
- Ponte HAL ↔ USB: `IOOperationHandler` Float32↔S32 + StartIO/StopIO no isoc
- Device name: `QUAD-CAPTURE UA-55`

Validação no hardware: pendente (Audio MIDI Setup + playback).

## Ainda aberto

1. Alts 2–4 / vendor clock.
2. MIDI e controles.
3. Ajuste fino de latência/timestamps USB.

`Shared/UA55Quirks.h`: `kUA55VendorRequestsEnabled` continua `false`.
