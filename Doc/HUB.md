# USB Hub Driver

The hub driver powers and scans downstream ports, monitors port changes, and
enumerates attached USB devices. It also removes devices from the stack when
they disconnect. Each downstream device uses the driver for its own USB
interface or function.

## Build and use

The default `usb2.device` build includes hub support through `DRV_HUB=1`.
Run `make -j4` from the repository root, then follow the
[stack installation steps](../README.md#building-the-distribution-archive).
[USB2Analyzer](USB2Analyzer.md) can display the hub
and its attached devices.
