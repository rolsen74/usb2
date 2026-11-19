# Competition Pro AmigaInput Driver

The Competition Pro USB2 port targets the USB game controller with vendor
and product ID `0079:181c`. Its sources are in
[`Drivers/AmigaInput/CompPro/src/`](../Drivers/AmigaInput/CompPro/src/) and its
headers are in [`Drivers/AmigaInput/CompPro/inc/`](../Drivers/AmigaInput/CompPro/inc/).

## Build and status

```sh
make -C Drivers/AmigaInput/CompPro
```

The build produces the startup stub at `Drivers/AmigaInput/CompPro/bin/CompPro.Driver`.
USB attachment, input reports, and AmigaInput registration are pending.
The AmigaInput callback sources require the SDK header
`proto/amigainput_driver.h` when integrated into the build.
