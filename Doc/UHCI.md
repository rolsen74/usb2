# UHCI Host Controller

UHCI handles low- and full-speed USB 1.1. The driver includes control, bulk,
interrupt, and full-speed isochronous transfer paths. The project has reported
UHCI operation on a Pegasus2 under QEMU.

## Build and use

The default `usb2.device` build includes UHCI through `DRV_UHCI=1`.
Run `make` from the repository root, then follow the
[stack installation steps](../README.md#building-the-distribution-archive).
