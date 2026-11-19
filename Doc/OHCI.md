# OHCI Host Controller

OHCI handles low- and full-speed USB 1.1. It can also serve as a companion
controller when [EHCI](EHCI.md) hands off a root port. The driver supports
control, bulk, interrupt, and full-speed isochronous transfers.

## Build and use

The default `usb2.device` build includes OHCI through `DRV_OHCI=1`.
Run `make` from the repository root, then follow the
[stack installation steps](../README.md#building-the-distribution-archive).
