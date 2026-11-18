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

## Human Interface Devices

The HID driver handles USB keyboards and mice in boot and report modes.
Report mode drops incomplete packets before decoding them, so missing fields
do not generate false key or button releases.

Experimental USB HID pen tablet support handles absolute X/Y, optional pressure
and proximity, and tip and barrel buttons in the first input report of a pen
collection. Tilt and vendor-specific tablet protocols are unsupported.

Default and diagnostic AmigaOS cross builds passed. Host tests checked keyboard
report decoding, mouse movement and buttons, and tablet reports. Tablet testing
on AmigaOS hardware is pending.

## Mass Storage

The MSD driver uses USB Bulk-Only Transport. It accepts SCSI transparent,
ATAPI CD/DVD, UFI floppy, and SFF-8070i interfaces that use this transport.
It exposes the devices through `usb2disk.device`.

- Block reads and writes support 32-bit and 64-bit byte offsets. The driver
  uses SCSI READ/WRITE(10) or READ/WRITE(16) as needed.
- `TD_SEEK`, `TD_FORMAT`, and their 64-bit counterparts are implemented.
  `TD_FORMAT` writes the supplied sector data; it does not issue SCSI FORMAT UNIT.
- Removable media is checked every two seconds. The driver tracks media
  presence and disk changes and signals registered `TD_ADDCHANGEINT` handlers.
- `TD_EJECT` loads or ejects media on drives that support it. `TD_MOTOR` sends
  START STOP UNIT for UFI and SFF floppy devices.
- CD-ROM media is reported as write-protected. `CMD_UPDATE` synchronizes the
  cache on supported writable devices.

Mass-storage limitations:

- USB floppy drives using CBI transport are not supported.
- `CMD_START`, `CMD_STOP`, `CMD_FLUSH`, and `CMD_RESET` are not implemented.
  These are Exec queue and device-control commands, not motor or cache-flush
  aliases. Use `TD_MOTOR` for a supported floppy motor and `CMD_UPDATE` to
  commit buffered writes.
- `TD_REMOVE`, `TD_GETNUMTRACKS`, `TD_RAWREAD`, `TD_RAWWRITE`, and the ETD
  commands are not implemented. `TD_GETGEOMETRY` is available, but its track
  layout values are synthetic; its sector size and total sector count come
  from the device.
- The recent CD/DVD, floppy, media-change, and 64-bit paths build successfully
  but have not been verified with physical drives in this workspace.

## PL2303 Serial (Untested)

The external PL2303 driver matches USB device `067b:2303` and exposes it as
`usb2serial.device`. Build it with `make -C Drivers/Serial/PL2303 -j4`, then
install `PL2303.Class` and `PL2303.Driver` in `Libs:USB2`. The cross-compiler
build succeeds; operation with a physical adapter has not been tested.

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
