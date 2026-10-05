# Roland QUAD-CAPTURE (UA-55) — macOS audio driver

An **experimental** open-source audio driver for the [Roland QUAD-CAPTURE](https://www.roland.com/global/products/quad-capture/) USB interface on **Apple Silicon Macs**.

Roland’s original Mac driver is old and no longer a good fit for modern macOS. This project aims to make the interface usable again: plug it in, see it in macOS, play and record audio, and watch the hardware preamp knobs on screen.

> **Status:** Playback and recording work at **44.1, 48, and 96 kHz** (4 out / 6 in) and at **192 kHz** (2 out / 2 in). Current driver build **74**.  
> This is **not** an official Roland product. Use at your own risk.

---

## What you get

| | |
| --- | --- |
| **Device name in macOS** | `QUAD-CAPTURE UA-55` |
| **44.1 / 48 / 96 kHz** | 4 outputs, 6 inputs |
| **192 kHz** | 2 outputs, 2 inputs (the interface only offers that rate in stereo) |
| **Output volume** | Main, left, and right controls in macOS |
| **Where it shows up** | Audio MIDI Setup, System Settings → Sound, GarageBand, Logic, etc. |
| **On-screen panel** | Mirrors the hardware SENS knobs (0–54 dB) and AUTO SENS, sends the compressor, LINK, and mixer monitor knobs, and switches sample rate |

In plain terms: after you install the driver, the QUAD-CAPTURE should behave like a normal Mac audio device. You can listen through it, record into it, and change the sample rate from Audio MIDI Setup.

---

## What it does *not* do (yet)

- **No full Roland Control Panel.** The companion app sends LO-CUT, PHASE, AUTO SENS, the SENS knobs, compressor BYPASS, LINK, GATE, THRESHOLD, RATIO, ATTACK, RELEASE, GAIN, and the mixer INPUT 1, INPUT 2, and COAX knobs. A mixer turn changes the monitor level, not the recording level.
- **Sample rate goes through Core Audio.** The footer control asks macOS to change the device clock, the same way Audio MIDI Setup does.
- **Not signed for mass distribution.** You need an Apple Developer account and must approve a system extension on your Mac.
- **Apple Silicon only.** Not aimed at Intel Macs.

The physical SENS knobs still set the gain. The panel reads that gain from the driver and shows one decibel for every two device counts (a raw byte of 108 is 54 dB, the hardware maximum).

---

## How it works (short version)

1. A small **installer app** loads a **DriverKit system extension** (a sandboxed driver Apple allows on modern macOS).
2. The driver talks to the interface over **USB** and registers it with **Core Audio**.
3. macOS apps send and receive normal audio; the driver converts it to the format the hardware expects.
4. When a SENS knob or the AUTO SENS button moves, the driver logs it. **QuadCapturePanel** reads that log and updates the matching control.

You don’t need to understand USB or DriverKit to use it. If something fails, the steps below help you check that the extension is actually active.

---

## Requirements

- Mac with **Apple Silicon** (M1 / M2 / M3 / …)
- **macOS** recent enough for DriverKit audio (Ventura or newer recommended)
- **Xcode** and an **Apple Developer** team (paid account; DriverKit entitlements)
- A Roland **QUAD-CAPTURE (UA-55)** — USB id `0582:012F`

---

## Quick start

### 1. Build

Open `UA55Diagnostic.xcodeproj` in Xcode, select the **UA55DiagnosticApp** scheme, destination **My Mac**, then Build.

Or from a terminal, at the repo root:

```bash
xcodebuild -project UA55Diagnostic.xcodeproj \
  -scheme UA55DiagnosticApp \
  -destination 'platform=macOS,arch=arm64' \
  build
```

### 2. Install

System extensions must live under `/Applications`:

1. Copy `UA55DiagnosticApp.app` into **Applications**.
2. Plug in the QUAD-CAPTURE.
3. Open the app → **Deactivate** (if an older extension is loaded) → **Activate driver**.
4. Approve the extension when macOS asks (System Settings → Privacy & Security / Extensions).
5. Unplug and reconnect the interface once.

More detail (capabilities, provisioning, logs): **[docs/BUILD.md](docs/BUILD.md)**.

### 3. Check that it worked

1. Open **Audio MIDI Setup**. You should see **QUAD-CAPTURE UA-55**.
2. At 44.1, 48, or 96 kHz the device is **4 out / 6 in**. At 192 kHz it is **2 out / 2 in**.
3. System Settings → **Sound** → choose it as output and play something.
4. Optional: record the inputs in QuickTime, GarageBand, or your DAW.

Useful commands:

```bash
# Is the extension installed and activated?
systemextensionsctl list

# Recent driver log lines
log show --last 10m --style compact --predicate 'eventMessage CONTAINS "[UA55]"'
```

You want a log line that mentions **`BUILD 74`** (or whatever build you just installed). On plug-in the driver prints `[UA55] ========== BUILD 74 loaded`.

### 4. Control panel (optional)

Audio does not need this app. It mirrors the two preamp knobs and the AUTO SENS button. The **SAMPLE RATE** box opens a list — 44.1, 48, 96, or 192 kHz — and the choice goes through Core Audio. The driver performs the USB clock change. At 192 kHz the device becomes 2 out / 2 in.

```bash
xcodebuild -project QuadCapturePanelMac/QuadCapturePanel.xcodeproj \
  -scheme QuadCapturePanel \
  -destination 'platform=macOS,arch=arm64' \
  build
```

Copy `QuadCapturePanel.app` to **Applications** and open it. The SENS readouts move when you turn the knobs on the box. AUTO SENS lights when the hardware button is on and goes gray when it is off.

The panel follows `[UA55] sens` and `[UA55] autosens` from the loaded driver. If the knobs stay still, the running extension is older than build 74: deactivate, install the new app, activate, and reconnect the USB cable. LO-CUT, PHASE, AUTO SENS, the SENS knobs, the compressor BYPASS buttons, LINK, the GATE, THRESHOLD, RATIO, ATTACK, RELEASE, and GAIN knobs, and the mixer INPUT 1, INPUT 2, and COAX knobs need that same build: the click or drag sends the SysEx through the driver. Opening the panel, or the board coming back after it was off, reads LO-CUT, PHASE, AUTO SENS, both BYPASS buttons, LINK, both GATE knobs, and the three mixer knobs. LO-CUT, PHASE, AUTO SENS, and LINK ask the board for the button state again after the click. A SENS turn does the same when the drag ends, because the board turns AUTO SENS on by itself. Mixer and compressor knobs show their value beside the knob only while you are turning it.

---

## Project layout

| Piece | Role |
| --- | --- |
| `UA55DiagnosticApp` | Simple UI to activate / deactivate the system extension |
| `UA55DiagnosticDriver` | The audio + USB driver (dext), including SENS and AUTO SENS logging |
| `Shared/` | USB constants, sample-rate table, and helpers shared by the driver |
| `docs/` | Build, entitlements, design notes |
| `QuadCapturePanelMac/` | On-screen panel. Optional for audio. Displays hardware SENS and AUTO SENS. LO-CUT, PHASE, AUTO SENS, the SENS knobs, compressor BYPASS, LINK, GATE, THRESHOLD, RATIO, ATTACK, RELEASE, GAIN, and the mixer INPUT 1, INPUT 2, and COAX knobs send SysEx through the driver |

---

## Troubleshooting

| Symptom | What to try |
| --- | --- |
| Device never appears | Extension not approved, or the app is not under `/Applications`. Check `systemextensionsctl list`. |
| Wrong / old build still loaded | Deactivate in the app, rebuild, copy again to `/Applications`, Activate, reconnect USB. Confirm `[UA55] ========== BUILD 74 loaded`. |
| Sample rate will not stick | Pick the rate in Audio MIDI Setup, then reconnect if the device disappears. 192 kHz is stereo only (2 out / 2 in). |
| Panel knobs do not follow the hardware | The loaded dext is not logging SENS. Reactivate build 74 and reconnect. |

Entitlements checklist: **[docs/ENTITLEMENTS.md](docs/ENTITLEMENTS.md)**.

---

## Stability note

**Build 74** is the current driver: duplex audio at 44.1, 48, and 96 kHz, stereo at 192 kHz, macOS output volume, a read-only view of the preamp knobs until you drag them, and LO-CUT, PHASE, AUTO SENS, SENS, compressor BYPASS, LINK, GATE, THRESHOLD, RATIO, ATTACK, RELEASE, GAIN, and mixer monitor SysEx written to USB MIDI cable 1. When the board connects, the panel asks for the 59-byte state block, both BYPASS channels, LINK, both GATE knobs, and the three mixer monitor levels.

An older extension stays in force until you deactivate it and activate the copy in `/Applications`. The log line `BUILD 74` is the check that the new dext actually loaded.

---

## Contributing / mindset

This is a reverse-engineering and learning project around a discontinued USB audio product. PRs and reports are welcome, especially:

- Clear “works / doesn’t work” notes (macOS version, build number, sample rate, log snippets)
- Fixes that stay within DriverKit rules (no kernel extensions)
- Careful experiments that don’t brick the machine

The on-screen panel mirrors the hardware SENS knobs and AUTO SENS, and sends the compressor, LINK, and mixer controls listed above. It is not a replacement for Roland’s old Control Panel.

---

## License / disclaimer

Experimental software provided as-is, without warranty.  
**Not affiliated with Roland Corporation.**  
Using unsigned or developer-signed system extensions can affect system security—only install what you trust, on machines you’re happy to debug.
