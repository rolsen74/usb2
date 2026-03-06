# USB Stack for AmigaOS 4 (Experimental)

This repository contains an **experimental USB host stack for AmigaOS 4**.

The stack is **functional** and supports several common USB device classes, but it was never completed into a fully polished or officially supported system component. The project is published to preserve the work and allow others to study, reuse, or extend it.

## Status

⚠️ **Working but incomplete**

The following functionality is known to work:

- USB Mass Storage Devices (MSD)
- USB Mouse (HID)
- USB Keyboard (HID)
- USB Hubs
- USB Printer Class (Printer Protocol)
- EHCI (USB 2.0 Host Controller)

Limitations:

- The stack is **not feature-complete**
- Error handling and edge cases may be missing
- Not all USB devices are guaranteed to work
- Performance and stability have not been fully tuned
- No active development roadmap

## Supported Environment

- Target OS: **AmigaOS 4**
- Machine: Amiga X5000/20/40
- USB Host Controller: **EHCI** (Onboard)

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
All source files include SPDX license identifiers.

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
