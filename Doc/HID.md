# USB HID

The HID driver handles USB keyboards and mice in boot and report modes.
Report mode drops incomplete packets before decoding them, so missing fields
do not generate false key or button releases.

Experimental USB HID pen tablet support handles absolute X/Y, optional pressure
and proximity, and tip and barrel buttons in the first input report of a pen
collection. Tilt and vendor-specific tablet protocols are unsupported.

Default and diagnostic AmigaOS cross builds passed. Host tests checked keyboard
report decoding, mouse movement and buttons, and tablet reports. Tablet testing
on AmigaOS hardware is pending.

## Build and install

This driver is enabled in the default `usb2.device` build by
`DRV_HID=1`. Build with `make` from the repository root and follow the
[stack installation steps](../README.md#building-the-distribution-archive).
