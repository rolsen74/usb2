# EHCI Host Controller

EHCI handles high-speed USB 2.0. During root-port reset, the driver hands
low-speed devices and devices that fail to enter high-speed mode to a
companion controller. The companion can be [OHCI](OHCI.md) or [UHCI](UHCI.md)
depending on the hardware.

## Build and use

The default `usb2.device` build includes EHCI through `DRV_EHCI=1`.
Run `make` from the repository root, then follow the
[stack installation steps](../README.md#building-the-distribution-archive).

The driver handles high-speed isochronous transfers and full-speed
isochronous transfers through a high-speed hub's transaction translator.
Split transfers use one descriptor per USB frame. Split IN endpoints are
limited to 752 bytes per packet so their complete-splits fit in one frame.
Each EHCI controller schedules one isochronous batch at a time.
