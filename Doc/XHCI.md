# xHCI Host Controller (Experimental)

xHCI manages USB ports across low, full, high, and SuperSpeed links. The
driver includes control, bulk, interrupt, and isochronous transfer paths.
SuperSpeed isochronous burst and Mult modes are unsupported. Hardware
validation remains pending.

Root ports use the controller's Supported Protocol and Speed ID tables.
A trained SuperSpeed link can proceed directly to addressing. USB2
enumeration serializes the reset and address steps on each controller.

Recoverable isochronous errors are reported per interval when a read
supplies packet lengths. Reads without that array return the valid bytes
contiguously and report the loss as an I/O error. Isochronous writes retain
the supplied lengths and report loss through the request status. The
controller still schedules one request per endpoint at a time. Continuous
isochronous delivery across requests needs hardware validation.

## Build and use

The default `usb2.device` build includes xHCI through `DRV_XHCI=1`.
Run `make` from the repository root, then follow the
[stack installation steps](../README.md#building-the-distribution-archive). To build the stack without xHCI,
run `make DRV_XHCI=0`.
