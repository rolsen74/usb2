# USB Stack for AmigaOS 4 (Experimental)

This repository contains an **experimental USB host stack for AmigaOS 4**.

The stack is **functional** and supports several common USB device classes, but it was never completed into a fully polished or officially supported system component. The project is published to preserve the work and allow others to study, reuse, or extend it.

## Status

The following functionality has been reported working:

- USB Hubs
- USB Mouse (HID)
- USB Keyboard (HID)
- USB Mass Storage Devices (MSD)
- USB Printer Class (Printer Protocol)
- 2.4 GHz Wi-Fi on the tested RTL8188EU and RT2870 adapters
- UHCI (USB 1.1 Host Controller; tested on Pegasus2 under QEMU)
- OHCI (USB 1.1 Host Controller)
- EHCI (USB 2.0 Host Controller)

The xHCI driver is still experimental and is enabled in the default build.
Compiling a driver does not establish that it works with hardware.

## Driver Guides

| Component | Guides |
| --- | --- |
| Host controllers | [EHCI](Doc/EHCI.md), [OHCI](Doc/OHCI.md), [UHCI](Doc/UHCI.md), [xHCI](Doc/XHCI.md) |
| Built-in device drivers | [HUB](Doc/HUB.md), [HID](Doc/HID.md), [MSD](Doc/MSD.md) |
| Classes | [PTP](Doc/PTP.md), [Printer](Doc/Printer.md), [Video](Doc/Video.md), [Audio](Doc/Audio.md) |
| Serial devices | [PL2303](Doc/PL2303.md) |
| Networking | [RTL8152](Doc/RTL8152.md), [RTL8188EU](Doc/RTL8188.md), [RT2870](Doc/RT2870.md) |
| AHI and AmigaInput | [AHI playback](Doc/AHI.md), [Competition Pro](Doc/CompPro.md), [Xbox 360](Doc/Xbox360.md) |

## Human Interface Devices

USB HID covers keyboards, mice, and experimental pen tablet support. See the
[HID guide](Doc/HID.md).

## Mass Storage

USB Bulk-Only Transport devices are exposed through `usb2disk.device`. See the
[MSD guide](Doc/MSD.md).

## USB Printers

Print through `usb2printer.device` using the USB Printer Class or DOT4. See
the [Printer guide](Doc/Printer.md).

## USB Video (Experimental)

UVC cameras expose MJPEG and YUY2 capture through `usb2video.device`. See the
[Video guide](Doc/Video.md).

## USB Audio (Experimental)

USB Audio Class 1.0 PCM capture and playback use `usb2audio.device`. See the
[Audio guide](Doc/Audio.md) and [AHI playback guide](Doc/AHI.md).

## Serial Devices

Compatible [PL2303](Doc/PL2303.md) USB serial adapters are exposed through
`usb2serial.device`. Hardware testing is pending.

## USB Bluetooth (Experimental)

The Bluetooth USB class driver registers compatible HCI controllers with
`bluetooth2.device` when they are connected and removes them on detach.
[USB2BluetoothTest](Doc/USB2BluetoothTest.md) probes controllers, discovers
nearby Classic Bluetooth devices, and exercises legacy PIN pairing and
reconnect with a saved bond.

## USB Networking

| Adapter | USB vendor:product IDs | SANA-II device | Status |
| --- | --- | --- | --- |
| [RTL8152 wired Ethernet](Doc/RTL8152.md) | `0bda:8152` | `usb2rtl8152.device` | Experimental |
| [RTL8188EU 2.4 GHz Wi-Fi](Doc/RTL8188.md) | `0bda:8179`, `07b8:8179`, `2357:010c` | `usb2rtl8188.device` | Working on `0bda:8179` |
| [RT2870 2.4 GHz Wi-Fi](Doc/RT2870.md) | `07d1:3c09` | `usb2rt2870.device` | Working on `07d1:3c09` |

Install the devices in `DEVS:Networks/`. See the adapter guides for
firmware, setup, and hardware validation.

## General Limitations

- The stack is **not feature-complete**
- Error handling and edge cases may be missing
- Not all USB devices are guaranteed to work
- Performance and stability have not been fully tuned
- No active development roadmap

## Supported Environment

- Target OS: **AmigaOS 4**
- Tested UHCI setup: **Pegasus2 under QEMU**
- Target hardware: **Amiga X5000/20/40**, with onboard EHCI

## Building

With the AmigaOS 4 cross-compiler installed, run `make`. The default
build enables UHCI, OHCI, EHCI, xHCI, HUB, HID, and MSD. To exclude xHCI,
use `make DRV_XHCI=0`; its hardware validation is still pending.

### Building the distribution archive

With the AmigaOS 4 cross-compiler and `lha` installed, run the following
from the repository root.

```sh
make -f makefile_archive
```

The archive is written to `usb2.lha` in the repository root. Extracting it
creates this directory tree.

```text
usb2/
├── Kickstart/
│   └── usb2.device
├── Devs/
│   ├── AHI/
│   │   └── usb2.audio
│   ├── AudioModes/
│   │   └── USB2
│   ├── Firmware/
│   │   ├── rt2870.bin
│   │   ├── rt2870.licence
│   │   ├── rtl8188eufw.licence
│   │   └── rtl8188eufw.bin
│   ├── Networks/
│   │   ├── usb2rtl8152.device
│   │   ├── usb2rtl8188.device
│   │   ├── usb2rt2870.device
│   │   └── usb2sis163u.device
│   └── usb2printer.device
├── Libs/
│   ├── USB2/
│   │   ├── Audio.Class
│   │   ├── Audio.Driver
│   │   ├── Bluetooth.Class
│   │   ├── Bluetooth.Driver
│   │   ├── PL2303.Class
│   │   ├── PL2303.Driver
│   │   ├── PTP.Class
│   │   ├── PTP.Driver
│   │   ├── Video.Class
│   │   └── Video.Driver
│   ├── usb2wifi.library
│   ├── usb2wpa.library
│   ├── usb2wpa2.library
│   └── usb2wpa3.library
└── Tools/
    ├── USB2Analyzer
    ├── USB2AudioTest
    ├── USB2BluetoothTest
    ├── USB2SDRTest
    ├── USB2WirelessManager
    └── USB2VideoTest
```

- **Core stack** — Copy `usb2.device` to `SYS:Kickstart/` and reference the
  installed filename in your USB2 Kicklayout entry. The default build includes
  the host controller, hub, HID, and mass-storage drivers.
- **Printer** — Copy `usb2printer.device` to `DEVS:` and select that device in
  your printer preferences.
- **Networking** — Copy the RTL8152 or RTL8188EU device from `Devs/Networks/`
  to `DEVS:Networks/`. For RTL8188EU, copy the files from `Devs/Firmware/`
  to `DEVS:Firmware/` and configure a wireless supplicant as described in the
  [RTL8188EU guide](Doc/RTL8188.md).
- **Classes** — Copy each `.Class` file together with its
  matching `.Driver` file to `LIBS:USB2/`. The stack reads the `.Class`
  files to match USB interfaces and load the corresponding drivers.
  - `Audio` provides PCM capture and playback through `usb2audio.device`.
    Run `Tools/USB2AudioTest` to check microphone capture.
  - [PTP](Doc/PTP.md) lets you browse and download camera files through `PTP:` and upload new files on cameras that support it.
  - `Video` exposes USB webcam capture through `usb2video.device`. Run
    `Tools/USB2VideoTest` for a live preview or run it with `SAVE` to capture
    a still image. See the
    [Video guide](Doc/Video.md).
- **Serial devices** — Copy `PL2303.Class` and `PL2303.Driver` to
  `LIBS:USB2/` to use supported PL2303 adapters through `usb2serial.device`.
- **AHI playback** — Copy `Devs/AHI/usb2.audio` and `Devs/AudioModes/USB2`
  to the corresponding directories under `DEVS:`.
- **RTL-SDR test** — Run `Tools/USB2SDRTest` from a Shell with an
  `0BDA:2838` receiver attached. It reports the Bulk IN endpoint and
  probes the tuner ID. To build just the tool, run
  `make -C Programs/USB2SDRTest`.
  With an R820T-family tuner and a high-speed USB connection, run
  `USB2SDRTest CAPTURE 99500000` to tune to 99.5 MHz and save one second
  of unsigned 8-bit interleaved I/Q at 1,024,000 samples per second.
  Use a local FM frequency in Hz. The file is 2,048,000 bytes.
  `FILE path` overrides `RAM:usb2sdr.iq`. A successful capture replaces
  an existing file at the selected path.
  To capture FM audio directly, run
  `USB2SDRTest CAPTURE 99500000 SECONDS 5 WAV RAM:fm.wav`. This saves a
  48 kHz mono PCM16 WAV. Omit `WAV` to save raw I/Q. `SECONDS` accepts
  1 to 5 and defaults to 1 for either output.
  Run `USB2SDRTest DECODE RAM:usb2sdr.iq` to demodulate the saved capture
  into a 48 kHz mono PCM16 WAV using 50 µs FM deemphasis. The default
  output is `RAM:usb2sdr.wav`. Use `WAV path` to choose another file.
  Decoding runs without the USB receiver attached.
  For live mono FM, run `USB2SDRTest LISTEN 99500000` with a USB audio
  output attached and `usb2audio.device` available. The tool uses the
  first compatible playback unit. Specify `OUTPUT 16` to select a unit,
  and press Ctrl+C to stop.
- **SiS163U Wi-Fi (development)** — `Devs/Networks/usb2sis163u.device`
  matches the Topcom 54 Mbps Wi-Fi stick (`0D8E:0163`). Opening the unit
  checks register access and endpoint setup, then reads the station address
  from the adapter EEPROM. SANA-II queries, events,
  multicast bookkeeping, and buffer hooks are available. RF type `$08`
  supports baseband initialization, RF calibration, and channel selection
  from 1 to 13 through `S2_SETOPTIONS`. The unit stays offline pending
  association and packet transfer. Build it with `make -C Drivers/Ethernet/SiS163U`.
- **[RT2870 Wi-Fi](Doc/RT2870.md)** — Install
  `Devs/Networks/usb2rt2870.device` in `DEVS:Networks/` and copy
  `rt2870.bin` and `rt2870.licence` from `Devs/Firmware/` to
  `DEVS:Firmware/`.
- **[USB2Analyzer](Doc/USB2Analyzer.md)** — Run it from the extracted
  `Tools/` directory after booting with the USB2 stack. It displays host
  controllers, attached devices, and their interfaces.
- **WPA library** — `Libs/usb2wpa.library` handles legacy WPA-Personal
  with TKIP on RTL8188EU and RT2870. Build it with `make -C Libraries/WPA`.
- **WPA3 library** — `Libs/usb2wpa3.library` provides SAE group 19 with
  Hash-to-Element and Hunting-and-Pecking, plus the WPA3 four-way handshake
  for RTL8188EU and RT2870. Build it with `make -C Libraries/WPA3`.
- **[USB2WirelessManager](Doc/USB2WirelessManager.md)** — Run it from
  `Tools/` to find a named Wi-Fi network with RTL8188EU or RT2870.
  Run it without arguments to edit saved profiles. Use `SETUP` to open
  the picker or `AUTO` to start Wi-Fi using the saved autoconnect flag.
  Both drivers support WPA2-Personal. RTL8188EU also supports open networks.
  For WPA2, copy `Libs/usb2wpa2.library` to `LIBS:` and install
  AmiSSL v5 (5.16+) or v6 (6.1+).

## Project Goals (Original)

- Implement a USB host stack for AmigaOS 4
- Support common HID, storage, and printer devices
- Explore USB host controller and class driver design
- Prototype low-level OS integration

## Intended Use

This project is useful for:

- Studying a working USB stack on AmigaOS 4
- Reusing or adapting code for similar systems
- Experimentation and further development
- Technical and historical reference

## License

This project is licensed under the **GNU General Public License v3.0 or later**.

See the `LICENSE` file for the full license text.

## Contributions

Contributions are welcome, with the understanding that:

- The code reflects experimental design decisions
- Cleanup and refactoring may be required
- Contributions must be compatible with GPL-3.0-or-later

## Author

Rene W. Olsen  
(2012–2026)

---

*Published to keep a working implementation available, not to present a finished product.*
