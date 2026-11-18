# xHCI Host Controller (Experimental)

xHCI manages USB ports across low, full, high, and SuperSpeed links. The
driver includes control, bulk, interrupt, and isochronous transfer paths.
SuperSpeed isochronous burst and Mult modes are unsupported. Hardware
validation remains pending.

## Build and use

The default `usb2.device` build includes xHCI through `DRV_XHCI=1`.
Run `make -j4` from the repository root, then follow the
[stack installation steps](README.md#building-the-distribution-archive). To build the stack without xHCI,
run `make DRV_XHCI=0 -j4`.
