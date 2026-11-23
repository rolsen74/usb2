# USB2Analyzer

`USB2Analyzer` is a graphical view of devices connected to `usb2.device` on
AmigaOS 4. It shows host controllers, hubs, devices, and interfaces in a tree.
Select a device or interface to inspect its descriptors in the Field, Size,
Value, and Description columns. Expand or collapse descriptor groups in
the properties pane.
The tree updates when the USB stack reports connection changes.

## Build and run

Build from the repository root with the AmigaOS 4 cross-compiler:

```sh
make -C Programs/USB2Analyzer
```

Start AmigaOS 4 with `usb2.device` loaded, then run
`Programs/USB2Analyzer/bin/USB2Analyzer`. The distribution archive places it
at `usb2/Tools/USB2Analyzer`; it can be run from the extracted `Tools/`
directory. Run it without arguments to open the GUI. The Project menu uses
the AmigaOS 4 menu class, which requires Intuition 54.6 or later. Use
Right Amiga + `?` for **About** and Right Amiga + `Q` for **Quit**.
Choose **Edit > Preferences...** to select a Workbench window, a Workbench
clone, or a custom screen. Use **Screen mode** to select a custom mode.
**Use** applies the settings for the current boot, **Save** keeps them
across reboots, and **Cancel** discards the changes.

## Save the USB tree

Choose **Project > Save tree** and save the suggested `USB2Tree.txt`
file. From an AmigaShell, use:

```text
USB2Analyzer SAVETREE RAM:USB2Tree.txt
```

The Shell command saves the report without opening a window. Both exports
include host controllers, hubs, ports, and devices. The ASCII report shows
the same descriptor groups and fields as the properties pane. It includes
the device and hub descriptors, the raw manufacturer, product, and serial
string descriptors, and the full cached configuration stream (index 0).
That stream includes all interface alternate settings, endpoint descriptors,
and class-specific or vendor-specific records supplied with the configuration.
Fully decoded device, interface, and endpoint descriptors appear as fields.
Extra bytes and undecoded records retain their raw data.
The export reads the descriptors exposed by `usb2.device`.

The device tree and properties view are functional. The Capture and I/O
Request panes are present in the window but do not yet capture or display
traffic.
