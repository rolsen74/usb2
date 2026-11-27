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
Independent isochronous endpoints can share the periodic schedule when
their transfers fit within the USB 2.0 periodic budget of 100 µs per
125 µs microframe. Split isochronous
transfers on the same high-speed hub are limited to one per USB frame.
Full-speed isochronous OUT requests with per-packet lengths can queue
one batch ahead on the same endpoint. When the next slots are available,
EHCI schedules the second batch immediately after the first.
Each OUT start-split carries up to 188 bytes. A transfer that misses its
scheduled slot can report fewer bytes than requested even with a second
batch queued. See the [Audio guide](Audio.md) for the tested playback rates.
After a normal interrupt or isochronous completion, EHCI keeps the old
descriptors for two frames and continues processing transfers.
Canceling a high-speed isochronous read leaves the periodic schedule running
while the controller finishes using any cached descriptors.
