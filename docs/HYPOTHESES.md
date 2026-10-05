# Hypotheses

Split: fact measured on this Mac, USB 2.0 layout, Apple contract, Linux reference.

## Milestone 1 — confirmed (2026-09-28)

- Dext `dev.ua55.UA55DiagnosticApp.driver` v0.1.0/2, USB entitlement `idVendor=*`.
- `[UA55] Start` → `SetConfiguration(1,true)=SUCCESS` → `diagnostic resident`.
- Device: VID `0x0582` PID `0x012F`, `bcdUSB=0x0200`, `deviceClass=0xff`, `bcdDevice=0x0100`, 480 Mbps.
- Config 1: `wTotalLength=511`, `bMaxPower=225` (450 mA), 5 interfaces, 19 iface / 17 ep / 26 other descriptors.
- `ioreg`: five `IOUSBHostInterface` (0–4), all `bInterfaceClass=255`.

### Measured map (source: `[UA55]` log)

CS descriptors `0x24` subtype `0x02` (FORMAT_TYPE-like): `bNrChannels`, subframe 4, bit resolution 24 (`04 18`).

| IF | Alt | EP | Dir | Type | maxPacket | Channels (CS) | Role |
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

Packet check (HS, `bInterval=1` = 125 µs): `7 × 4ch × 4 B = 112`, `7 × 6ch × 4 B = 168`. That matches alt 1/2.

### vs Linux quirk (`quirks-table.h` 0x0582:0x012f)

| Item | Linux quirk | Measured on this unit |
| --- | --- | --- |
| Playback | IF0 alt1 EP`0x05` S32_LE 4ch 44.1k | IF0 alt1 EP`0x05` 4ch 24-in-32, maxPacket 112 |
| Capture | IF1 alt1 EP`0x85` S32_LE 6ch 44.1k | IF1 alt1 EP`0x85` 6ch 24-in-32, maxPacket 168 |
| MIDI | IF2 | IF2 bulk `0x06`/`0x86` |
| IF3/4 | ignored | interrupt status; not audio |

The Linux quirk only declares alt 1 at 44.1 kHz. The hardware exposes alts 2–4 (likely 48/96/192, and 2-channel mode on alt 4). The exact rates are **not** in the truncated CS-byte dump; the 3 `tSamFreq` bytes still had to be read, or proved with a stream.

## Milestone 2 — confirmed on hardware (build 3/4)

Measured on 2026-09-28:

- Claim IF0/IF1, Open, `SelectAlternateSetting(1)`, `CopyPipe` → SUCCESS
- Capture `IsochIO` sync → `0x00000000`, 8/8 frames OK
- Packets `complete=144` and `120` (= 6 and 5 samples × 6 ch × 4 B) → **44.1 kHz HS**
- `nonzeroS32=60` → real data in the capture buffer
- Playback sync `IsochIO` **stalls** `Start` (EP OUT attr `0x05` async). Removed in build 4; it returns in milestone 3 as async/duplex

## Milestone 3 — async duplex confirmed (build 7, 2026-09-28)

- `diagnostic resident` + `duplex async streaming started`
- Capture and playback: `errors=0`, continuous completions (~1000/s with depth=4 × 8 µframes)
- Capture settles at **~1,058,400 B/s** = 44100 × 6 ch × 4 B → 44.1 kHz confirmed on a continuous stream
- Playback sends silence at a full maxPacket (112 × 8 × N) without a stall
- Buffer via `IOBufferMemoryDescriptor::Create` + submit outside `Start` (avoids the workloop deadlock)

## Milestone 4 — AudioDriverKit (build 8)

Implemented in the code at that point:

- Entitlement `family.audio` + link `AudioDriverKit`
- `UA55AudioDriver` (`IOUserAudioDriver`) + `UA55AudioDevice` (4 out / 6 in Float32 at 44.1 kHz)
- `UA55UsbStream` (reusable duplex isochronous ring)
- HAL ↔ USB bridge: `IOOperationHandler` Float32↔S32 + StartIO/StopIO on the isochronous stream
- Device name: `QUAD-CAPTURE UA-55`

Hardware validation: pending at that date (Audio MIDI Setup + playback).

## Still open at that date

1. Alts 2–4 / vendor clock.
2. MIDI and controls.
3. Fine-tuning of latency and USB timestamps.

`Shared/UA55Quirks.h`: `kUA55VendorRequestsEnabled` remained `false`.
