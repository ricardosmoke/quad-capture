# Roland QUAD-CAPTURE (UA-55) — macOS audio driver

An **experimental** open-source audio driver for the [Roland QUAD-CAPTURE](https://www.roland.com/global/products/quad-capture/) USB interface on **Apple Silicon Macs**.

Roland’s original Mac driver is old and no longer a good fit for modern macOS. This project aims to make the interface usable again: plug it in, see it in macOS, play and record audio.

> **Status:** Works for day-to-day playback and recording at **44.1 kHz** (stable build **34**).  
> This is **not** an official Roland product. Use at your own risk.

---

## What you get

| | |
| --- | --- |
| **Device name in macOS** | `QUAD-CAPTURE UA-55` |
| **Outputs** | 4 channels |
| **Inputs** | 6 channels (mics + other inputs) |
| **Sample rate** | 44.1 kHz (for now) |
| **Where it shows up** | Audio MIDI Setup, System Settings → Sound, GarageBand, Logic, etc. |

In plain terms: after you install the driver, the QUAD-CAPTURE should behave like a normal Mac audio device—you can listen through it and record into it.

---

## What it does *not* do (yet)

- **No 48 / 96 / 192 kHz** — only 44.1 kHz in this cut  
- **No replacement for Roland’s Control Panel** — software control of preamp gain (SENS knobs), compressors, etc. is not ready  
- **Not signed for mass distribution** — you need an Apple Developer account and to approve a system extension on your Mac  
- **Apple Silicon only** — not aimed at Intel Macs  

Physical SENS knobs on the box still work the usual way (they’re analog). The on-screen “control panel” app in this repo is a separate experiment and does **not** drive those knobs yet.

---

## How it works (short version)

1. A small **installer app** loads a **DriverKit system extension** (a sandboxed driver Apple allows on modern macOS).  
2. The driver talks to the interface over **USB** and registers it with **Core Audio**.  
3. macOS apps send/receive normal audio; the driver converts it to the format the hardware expects.

You don’t need to understand USB or DriverKit to use it—but if something fails, the steps below help you check that the extension is actually active.

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

Or from a terminal:

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
3. Open the app → **Deactivate** (if needed) → **Activate driver**.  
4. Approve the extension when macOS asks (System Settings → Privacy & Security / Extensions).  
5. Unplug and reconnect the interface once.

More detail (capabilities, provisioning, logs): **[docs/BUILD.md](docs/BUILD.md)**.

### 3. Check that it worked

1. Open **Audio MIDI Setup** — you should see **QUAD-CAPTURE UA-55** with **4 out / 6 in** at **44.1 kHz**.  
2. System Settings → **Sound** → choose it as output and play something.  
3. Optional: record the inputs in QuickTime, GarageBand, or your DAW.

Useful commands:

```bash
# Is the extension installed and activated?
systemextensionsctl list

# Recent driver log lines
log show --last 10m --style compact --predicate 'eventMessage CONTAINS "[UA55]"'
```

You want a log line that mentions **`BUILD 34`** (or whatever build you just installed).

---

## Project layout

| Piece | Role |
| --- | --- |
| `UA55DiagnosticApp` | Simple UI to activate / deactivate the system extension |
| `UA55DiagnosticDriver` | The actual audio + USB driver (dext) |
| `Shared/` | USB constants and helpers shared by the driver |
| `docs/` | Build, entitlements, design notes |
| `QuadCapturePanelMac/` | Optional on-screen panel (meters / UI mock) — **not** required for audio |

---

## Troubleshooting

| Symptom | What to try |
| --- | --- |
| Device never appears | Extension not approved, or not under `/Applications`. Check `systemextensionsctl list`. |
| Wrong / old build still loaded | Deactivate in the app, rebuild, copy again to `/Applications`, Activate, reconnect USB. Confirm `[UA55] BUILD …` in the log. |
| Clicks / glitches on record | Prefer the known-good **build 34** settings; avoid experimental MIDI/control builds until they’re marked stable. |
| Mac becomes unstable after a “control” experiment | Reinstall the **build 34** backup app and Activate again; reconnect the interface. |

Entitlements checklist: **[docs/ENTITLEMENTS.md](docs/ENTITLEMENTS.md)**.

---

## Stability note

**Build 34** is the cut we treat as the daily driver: duplex audio at 44.1 kHz with comfortable buffering. Newer experiments (extra MIDI / software gain) are paused until they’re proven safe.

If you keep a local backup of that build, prefer it when you only need “sound in and out.”

---

## Contributing / mindset

This is a reverse‑engineering and learning project around a discontinued USB audio product. PRs and reports are welcome, especially:

- Clear “works / doesn’t work” notes (macOS version, build number, log snippets)  
- Fixes that stay within DriverKit rules (no kernel extensions)  
- Careful experiments that don’t brick the machine

Please don’t expect feature parity with Roland’s old Control Panel overnight.

---

## License / disclaimer

Experimental software provided as-is, without warranty.  
**Not affiliated with Roland Corporation.**  
Using unsigned or developer-signed system extensions can affect system security—only install what you trust, on machines you’re happy to debug.
