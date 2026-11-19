# USB2WirelessManager

`USB2WirelessManager` scans for a named Wi-Fi network and can join an open or
WPA2-Personal network. It is specific to `usb2rtl8188.device` and its wireless
commands. `UNIT` selects a unit of that driver; it does not select another
driver. See the [RTL8188EU guide](RTL8188.md) for supported adapters, firmware,
and security limitations.

## Build and install

Build from the repository root with the AmigaOS 4 cross-compiler:

```sh
make -C Libraries/WPA2
make -C Programs/USB2WirelessManager
```

Start AmigaOS 4 with `usb2.device` loaded and install `usb2rtl8188.device`
and its firmware as described in the RTL8188EU guide. Run
`Programs/USB2WirelessManager/bin/USB2WirelessManager` from an interactive
Shell. The distribution archive places the program at
`usb2/Tools/USB2WirelessManager`.

For WPA2, copy `Libraries/WPA2/bin/usb2wpa2.library` to `LIBS:`. The archive
includes it at `usb2/Libs/usb2wpa2.library`. Install
[AmiSSL](https://github.com/jens-maus/amissl/releases) v5 (5.16+) or v6
(6.1+) as well.

## Use

```text
USB2WirelessManager "My Network"
USB2WirelessManager "My Network" OPEN
USB2WirelessManager "My Network" WPA2
USB2WirelessManager "My Network" WPA2 CHANNEL=5
USB2WirelessManager "My Network" WPA2 UNIT=1
```

With only an SSID, the program scans channels 1–11, reports a matching access
point's address, channel, and security, then exits. `OPEN` joins an open
network. `WPA2` joins a WPA2-Personal network using AES-CCMP and prompts for a
passphrase of 8–63 printable characters. The passphrase does not echo while
typing; press Return to submit it. Use an interactive Shell for WPA2.

`CHANNEL` limits the scan to one channel from 1 to 11. `UNIT` selects a
SANA-II unit from 0 to 31 and defaults to 0. Use the same unit number in the
TCP/IP configuration. The access point must advertise its SSID for passive
scanning.

Wait for `WPA2 connected` or `Associated` before starting the TCP/IP
interface. Leave the manager running while connected; it handles later WPA2
group-key updates. Press Ctrl-C to disconnect.

If the Shell only reports `Found ... (protected)`, scanning succeeded but the
network has not been joined. Run with `WPA2` and enter the passphrase. If the
driver reports `S2_CONFIGINTERFACE error 4, wire error 15`, another client has
already configured that unit; the manager accepts this state. A driver log
with `AID 0, online 0` means association has not completed yet.
