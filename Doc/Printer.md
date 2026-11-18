# USB Printer Class

The printer driver exposes `usb2printer.device`. It
supports the USB Printer Class unidirectional and bidirectional protocols,
plus printing over the IEEE 1284.4 (DOT4) `PRINT` channel. DOT4 support
has been cross-compiled and awaits physical printer testing.

Set the device name to `usb2printer.device` in printer preferences or
applications that reference the previous `usb2_printer.device` name. The
driver still looks for per-unit settings under `usb2_printer.device/unitN`.

## Build and install

```sh
make -C Drivers/Printer USBTYPE=USB2 -j4
```

Copy `Drivers/Printer/bin/usb2printer.device` to `DEVS:`. The archive
contains the device at `usb2/Devs/usb2printer.device`.
