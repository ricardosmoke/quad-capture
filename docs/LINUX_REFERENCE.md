# Linux reference

Source: `sound/usb/quirks-table.h` on the kernel mainline, entry `USB_DEVICE(0x0582, 0x012f)`. This is an ALSA quirk, not a dump of this unit's configuration descriptor. The macOS driver does not copy those numbers.

Summary of the entry that limits the device to 44.1 kHz:

| Interface | Role in the quirk | Alternate | Endpoint | Attribute | Declared format |
| --- | --- | --- | --- | --- | --- |
| 0 | fixed audio | 1 | `0x05` | `0x05` | S32_LE, 4 channels, 44100 |
| 1 | fixed audio | 1 | `0x85` | `0x25` | S32_LE, 6 channels, 44100 |
| 2 | fixed MIDI | — | in/out cables `0x0001` | — | one cable each way |
| 3 | ignored | — | — | — | — |
| 4 | ignored | — | — | — | — |

URL: https://github.com/torvalds/linux/blob/master/sound/usb/quirks-table.h

Later patches describe alternate settings 2, 3, and 4 for 48, 96, and 192 kHz, and a vendor clock request also associated with the OCTA-CAPTURE `0582:0120`. This driver sends that clock request (vendor request 3) for the QUAD-CAPTURE and does not match `0x0120`.

MultiRolandDriver recognizes PID `0x012F` and states that, on that audio+MIDI composite, it claims only the MIDI interface. It is not a QUAD-CAPTURE audio implementation.

Allowed use of this data: compare it with the `[UA55] interface` and `[UA55] endpoint` lines read from the unit. That comparison is recorded in [HYPOTHESES.md](HYPOTHESES.md).
