# EHCI Host Controller

EHCI handles high-speed USB 2.0. During root-port reset, the driver hands
low-speed devices and devices that fail to enter high-speed mode to a
companion controller. The companion can be [OHCI](OHCI.md) or [UHCI](UHCI.md)
depending on the hardware.

## Build and use

The default `usb2.device` build includes EHCI through `DRV_EHCI=1`.
Run `make -j4` from the repository root, then follow the
[stack installation steps](../README.md#building-the-distribution-archive).

The driver includes high-speed isochronous transfers for applicable audio
and video devices. EHCI split isochronous transfers through a high-speed
hub's transaction translator are unsupported.
