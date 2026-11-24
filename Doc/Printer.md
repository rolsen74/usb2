# USB Printer Class

`usb2.kmod` provides `usb2printer.device`. When a printer is plugged in,
`cls_Printer.Driver` registers it on the first free unit, normally unit 0.
The class driver supports unidirectional and bidirectional USB printing,
plus printing over the IEEE 1284.4 (DOT4) `PRINT` channel. DOT4 support
has been cross-compiled and awaits physical printer testing.

Set the device name to `usb2printer.device` in printer preferences.
Unit 0 answers `NSCMD_DEVICEQUERY` even with the printer unplugged. The
device reports a parallel-compatible interface with `CMD_WRITE` and
`PDCMD_QUERY`. Buffered writes flush after 250 ms of idle time and
when the print job closes. Each unit keeps up to four buffers. Larger
writes wait for USB to free space. A successful buffered write confirms
that the driver accepted the data. If a pending write is aborted,
`io_Actual` reports bytes already accepted, which can still print.

The driver reads `BUFFERSIZE` in KiB from `ENV:usb2_printer.device/unitN`.
Set `BUFFERSIZE=0` for direct writes. `cls_Printer.Class` controls which USB
interfaces bind to the driver.

## Build and install

```sh
make -C Drivers/Printer USBTYPE=USB2
```

Install the matching `usb2.kmod`. Copy `cls_Printer.Driver` and
`cls_Printer.Class` from `usb2/Libs/USB2/` in the archive to `LIBS:USB2/`.
Copy the archive's one-byte `usb2/Devs/usb2printer.device` file to
`DEVS:` for use in file pickers. This replaces the older standalone
printer device binary. The matching `usb2.kmod` registers the device.
