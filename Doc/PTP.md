# USB Picture Transfer Protocol

The PTP driver gives AmigaOS 4 filesystem access to compatible digital
cameras. `PTP.Class` matches USB imaging interfaces with class `0x06`, and
`PTP.Driver` mounts the `PTP:` filesystem handler.

## Build and install

```sh
make -C Drivers/PTP -j4
```

Copy `Drivers/PTP/bin/PTP.Driver` and `Drivers/PTP/PTP.Class` to
`LIBS:USB2/`. The distribution archive includes both files under
`usb2/Libs/USB2/`. Start AmigaOS 4 with `usb2.device` loaded, connect the
camera in PTP mode, and open `PTP:` in Workbench or list it from the Shell.

The driver cross-compiles for AmigaOS 4.
