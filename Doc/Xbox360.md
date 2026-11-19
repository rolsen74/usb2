# Xbox 360 Wired USB Controller (Experimental)

This USB2 port of the original `XBox` driver matches the wired Xbox 360
gamepad interface with class `ff`, subclass `5d`, and protocol `01`.
That includes the original Microsoft controller (`045e:028e`) and lets
compatible third-party controllers bind through the same interface match.

## Build and install

Run the build from the repository root with the AmigaOS 4 cross-compiler.

```sh
make -C Drivers/AmigaInput/Xbox360
```

Copy `Drivers/AmigaInput/Xbox360/Xbox360.Class` and
`Drivers/AmigaInput/Xbox360/bin/Xbox360.Driver` to `LIBS:USB2/` for a USB2
test boot. Connect the wired controller after the stack starts.

## Status

The driver polls interrupt IN and logs changes through `DebugPrintF`. Under
FreeBSD's HID mapping, button mask bits 0–10 correspond to A, B, X, Y, LB, RB,
Back, Start, left stick click, right stick click, and Guide in that order. The D-pad is
reported as two axes from −1 to +1, with up and left negative. Both trigger
values range from 0 to 255. The analog stick Y axes are negative toward up.

On the first valid input report it requests the top-left player LED over
interrupt OUT. This
gives a visible way to check the USB report path without the AmigaInput SDK.
If interrupt OUT is unavailable it tries a control `SET_REPORT` request.
Polling continues if the LED write fails. A USB detach or task break cancels
and drains the pending read before releasing its resources.

AmigaInput registration and event delivery still need the AmigaInput driver
SDK. At this stage, controller input is available through the debug log.
Headset, chatpad, rumble, and wireless controllers are outside this port.
Hardware testing on AmigaOS 4 is pending.

## FreeBSD sources

The interface match, report mapping, and control LED request follow FreeBSD's
Xbox 360 driver and HID code. The [FreeBSD notices](../Drivers/AmigaInput/Xbox360/LICENSE.FreeBSD)
apply to the adapted portions. [xb360gp.c](https://github.com/freebsd/freebsd-src/blob/main/sys/dev/hid/xb360gp.c),
[hidrdesc.h](https://github.com/freebsd/freebsd-src/blob/main/sys/dev/hid/hidrdesc.h),
[hgame.c](https://github.com/freebsd/freebsd-src/blob/main/sys/dev/hid/hgame.c),
[hidmap.c](https://github.com/freebsd/freebsd-src/blob/main/sys/dev/hid/hidmap.c)
