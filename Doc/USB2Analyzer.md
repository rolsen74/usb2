# USB2Analyzer

`USB2Analyzer` is a graphical view of devices connected to `usb2.device` on
AmigaOS 4. It shows host controllers, hubs, devices, and interfaces in a tree.
Select a device to inspect its descriptors, endpoints, and other properties.
The tree updates when the USB stack reports connection changes.

## Build and run

Build from the repository root with the AmigaOS 4 cross-compiler:

```sh
make -C Programs/USB2Analyzer -j4
```

Start AmigaOS 4 with `usb2.device` loaded, then run
`Programs/USB2Analyzer/bin/USB2Analyzer`. The distribution archive places it
at `usb2/Tools/USB2Analyzer`; it can be run from the extracted `Tools/`
directory. The program takes no command-line arguments. Close its window to
exit.

The device tree and properties view are functional. The Capture and I/O
Request panes are present in the window but do not yet capture or display
traffic.
