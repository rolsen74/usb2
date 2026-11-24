# RTL8811AU USB Wi-Fi

This experimental AmigaOS 4 USB class driver matches the TP-Link Archer T2U
Nano with USB ID `2357:011e`. The RTL8811AU uses RTL8821A firmware and register
programming.

```sh
make -C Drivers/Ethernet/RTL8811AU
```

The distribution archive includes the driver and class in `Libs/USB2/` and
the firmware and license in `Devs/Firmware/`. For a manual install, copy
`bin/eth_RTL8811AU.Driver` and `eth_RTL8811AU.Class` to `LIBS:USB2/` and
`Firmware/rtw8821a_fw.bin` to `DEVS:Firmware/`. After the stack loads the
class, attach the dongle and check the serial log for `RTL8811AU:` messages.

The driver probes the USB interface, reads the factory station address from
eFuse, and registers a unit on `usb2ethernet.device`. The serial log reports
the assigned unit number and the outcome of MAC and firmware initialization.
The driver supports SANA-II queries, configuration with the factory address,
multicast lists, type tracking, statistics, events, throughput sampling, and
copy-hook registration. Flush, abort, close, and detach complete pending requests.

Use the matching `USB2WirelessManager` and `usb2wifi.library` from the archive.
The manager can scan, associate, and keep the connection open on 2.4 GHz
channels 1–11. Select a network with `SETUP UNIT 0`, or connect from a Shell:

```text
USB2WirelessManager "My network" WPA2 UNIT 0
```

Use `OPEN` for an unprotected network. WPA2 requires CCMP. Keep the manager
running and configure the TCP/IP interface for `usb2ethernet.device` and the
unit number reported during registration. The driver supports 20 MHz channels
with 1 Mbps CCK transmit using the adapter's eFuse power index. A missing or
invalid power index leaves that channel receive-only.

The serial log shows the RF channel, TX readiness, the first received beacon,
and online or offline transitions. Association and packet transfer need
validation on physical hardware.

The power sequence and MCU reset use the
[FreeBSD rtwn RTL8821A implementation](https://cgit.freebsd.org/src/tree/sys/dev/rtwn/rtl8821a)
as their reference. The firmware binary is unchanged from
[linux-firmware](https://gitlab.com/kernel-firmware/linux-firmware/-/blob/main/rtw88/rtw8821a_fw.bin).
The redistribution notice is in `Firmware/rtw8821a_fw.licence`.
