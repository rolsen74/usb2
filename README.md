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
| External device drivers | [PTP](Doc/PTP.md), [Printer](Doc/Printer.md), [Video](Doc/Video.md), [Audio](Doc/Audio.md), [PL2303](Doc/PL2303.md) |
| Networking | [RTL8152](Doc/RTL8152.md), [RTL8188EU](Doc/RTL8188.md) |
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

## PL2303 Serial (Untested)

Compatible USB serial adapters are exposed through `usb2serial.device`. See
the [PL2303 guide](Doc/PL2303.md).

## USB Networking (Experimental)

| Adapter | USB vendor:product IDs | SANA-II device |
| --- | --- | --- |
| [RTL8152 wired Ethernet](Doc/RTL8152.md) | `0bda:8152` | `usb2rtl8152.device` |
| [RTL8188EU 2.4 GHz Wi-Fi](Doc/RTL8188.md) | `0bda:8179`, `07b8:8179`, `2357:010c` | `usb2rtl8188.device` |

Install the devices in `DEVS:Networks/`. Physical adapter testing on
AmigaOS 4 remains pending. The RTL8188EU guide covers firmware and supplicant
setup.

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

With the AmigaOS 4 cross-compiler installed, run `make -j4`. The default
build enables UHCI, OHCI, EHCI, xHCI, HUB, HID, and MSD. To exclude xHCI,
use `make DRV_XHCI=0 -j4`; its hardware validation is still pending.

### Building the distribution archive

With the AmigaOS 4 cross-compiler and `lha` installed, run the following
from the repository root.

```sh
make -f makefile_archive -j4
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
│   │   ├── rtl8188eufw.licence
│   │   └── rtl8188eufw.bin
│   ├── Networks/
│   │   ├── usb2rtl8152.device
│   │   └── usb2rtl8188.device
│   └── usb2printer.device
├── Libs/
│   ├── USB2/
│   │   ├── Audio.Class
│   │   ├── Audio.Driver
│   │   ├── PL2303.Class
│   │   ├── PL2303.Driver
│   │   ├── PTP.Class
│   │   ├── PTP.Driver
│   │   ├── Video.Class
│   │   └── Video.Driver
│   └── usb2wpa2.library
└── Tools/
    ├── USB2Analyzer
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
- **External class drivers** — Copy each `.Class` file together with its
  matching `.Driver` file to `LIBS:USB2/`. The stack reads the `.Class`
  files to match USB interfaces and load the corresponding drivers.
  - `Audio` provides PCM capture and playback through `usb2audio.device`.
  - `PL2303` exposes supported USB serial adapters through `usb2serial.device`.
  - [PTP](Doc/PTP.md) lets you browse and download camera files through `PTP:` and upload new files on cameras that support it.
  - `Video` exposes USB webcam capture through `usb2video.device`. Run
    `Tools/USB2VideoTest` for a live preview or run it with `SAVE` to capture
    a still image. See the
    [Video guide](Doc/Video.md).
- **AHI playback** — Copy `Devs/AHI/usb2.audio` and `Devs/AudioModes/USB2`
  to the corresponding directories under `DEVS:`.
- **[USB2Analyzer](Doc/USB2Analyzer.md)** — Run it from the extracted
  `Tools/` directory after booting with the USB2 stack. It displays host
  controllers, attached devices, and their interfaces.
- **[USB2WirelessManager](Doc/USB2WirelessManager.md)** — Run it from
  `Tools/` to find a named Wi-Fi network or join an open or WPA2-Personal
  network with the RTL8188EU
  adapter. It is specific to `usb2rtl8188.device`; `UNIT` selects a unit
  of that driver. For WPA2, copy `Libs/usb2wpa2.library` to `LIBS:` and install
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
