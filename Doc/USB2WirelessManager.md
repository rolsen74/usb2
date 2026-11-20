# USB2WirelessManager

`USB2WirelessManager` scans for a named Wi-Fi network. The default driver,
`usb2rtl8188.device`, can also join open, WPA-Personal (TKIP), WPA2-Personal and WPA3-Personal networks. Select
`DEVICE=usb2rt2870.device` for scanning, WPA-Personal (TKIP), WPA2-Personal and WPA3-Personal on the RT2870. `UNIT`
selects a unit of the chosen driver. See the [RTL8188EU guide](RTL8188.md)
for supported Realtek adapters, firmware, and security limitations.

## Build and install

Build from the repository root with the AmigaOS 4 cross-compiler:

```sh
make -C Libraries/WPA
make -C Libraries/WPA2
make -C Libraries/WPA3
make -C Libraries/WiFi
make -C Programs/USB2WirelessManager
```

Start AmigaOS 4 with `usb2.device` loaded. Follow the [RTL8188EU guide](RTL8188.md) to install that driver. For RT2870,
copy `usb2rt2870.device` to `DEVS:Networks/` and `rt2870.bin` and its
licence to `DEVS:Firmware/`. Run
`Programs/USB2WirelessManager/bin/USB2WirelessManager` from a Shell. The distribution archive places the program at
`usb2/Tools/USB2WirelessManager`.

For legacy WPA-Personal, copy `usb2wpa.library` to `LIBS:`. For WPA2, copy `Libraries/WPA2/bin/usb2wpa2.library` to `LIBS:`. The archive
includes it at `usb2/Libs/usb2wpa2.library`. For WPA3, copy
`usb2wpa3.library` version 1.4 or later to `LIBS:`. Install
[AmiSSL](https://github.com/jens-maus/amissl/releases) v5 (5.16+) or v6
(6.1+) as well.

## Use

```text
USB2WirelessManager "My Network"
USB2WirelessManager "My Network" OPEN
USB2WirelessManager "My Network" WPA
USB2WirelessManager "My Network" WPA2
USB2WirelessManager "My Network" WPA3
USB2WirelessManager "My Network" WPA2 CHANNEL=5
USB2WirelessManager "My Network" WPA2 UNIT=1
USB2WirelessManager "My Network" DEVICE=usb2rt2870.device CHANNEL=5
USB2WirelessManager "My Network" DEVICE=usb2rt2870.device CHANNEL=5 AUTH
USB2WirelessManager "My Network" DEVICE=usb2rt2870.device CHANNEL=5 ASSOCIATE
USB2WirelessManager "My Network" DEVICE=usb2rt2870.device WPA
USB2WirelessManager "My Network" DEVICE=usb2rt2870.device CHANNEL=5 WPA2
USB2WirelessManager "My Network" DEVICE=usb2rt2870.device WPA2 PASSWORD="example passphrase"
USB2WirelessManager "My Network" DEVICE=usb2rt2870.device WPA3 PASSWORD="example passphrase"
USB2WirelessManager "My Network" DEVICE=usb2rt2870.device WPA2 WPA3
```

With only an SSID, the program scans channels 1–11, reports a matching access
point's address, channel, and security, then exits. `OPEN` joins an open
network. `WPA` explicitly selects legacy WPA-Personal with TKIP. It requires
`usb2wpa.library`, with RTL8188EU 1.3 and manager 0.21 or later, or
RT2870 1.22 and manager 0.23 or later. Use `WPA` on its own, with an 8–63 character passphrase.
On mixed networks, select `WPA2` when the access point offers CCMP for both
pairwise and group traffic. `WPA2` joins a WPA2-Personal network using AES-CCMP and accepts a
passphrase of 8–63 printable ASCII characters. Supply `PASSWORD="..."`
with `WPA`, `WPA2` or `WPA3` to skip the prompt. The passphrase can appear in Shell history
or in the script that supplies it. When `PASSWORD` is omitted, the manager
prompts in an interactive Shell and hides the characters as you type.

`WPA3` requires an access point offering SAE group 19, CCMP-128 and BIP-CMAC-128.
Use RTL8188EU 1.2 or later with USB2WirelessManager 0.20 or later, or use RT2870
1.21 or later. The manager selects
Hash-to-Element when advertised. Older access points can use
Hunting-and-Pecking. This
version supports SAE passwords of 1–63 printable ASCII characters.
APs that require an SAE anti-clogging token or a password identifier
need additional support.

Use both `WPA2 WPA3` to accept either mode. The manager selects the first
matching access point with compatible security and prefers WPA3 if that
access point and the selected driver support it. A failed WPA3 exchange
ends the attempt. `WPA3` by itself requires WPA3.

`CHANNEL` limits the scan to one channel from 1 to 11 on RTL8188EU, or 1
to 13 on RT2870. `UNIT` selects a SANA-II unit from 0 to 31 and defaults to 0. Use the same unit number in the
TCP/IP configuration. The access point must advertise its SSID for passive
scanning.

On RT2870, `AUTH` sends an 802.11 authentication request to the matching
access point and reports its reply. `ASSOCIATE` tests WPA2-Personal
association and waits for message 1 of the WPA2 handshake. These tests
finish before password entry. `WPA2` brings the interface online after
the handshake. Use
`usb2rt2870.device` and the same unit in your TCP/IP configuration.

Wait for `WPA connected`, `WPA2 connected`, `WPA3 connected`, or `Associated` before starting the TCP/IP
interface. Leave the manager running while connected so it can handle group-key updates. Press Ctrl-C to disconnect.

If the Shell only reports `Found ... (protected)`, scanning succeeded but the
network has not been joined. Run with `WPA2` or `WPA3` to match the access point and enter the passphrase. If the
driver reports `S2_CONFIGINTERFACE error 4, wire error 15`, another client has
already configured that unit; the manager accepts this state. A driver log
with `AID 0, online 0` means association has not completed yet.

## Saved profiles and startup

Install `usb2wifi.library` 1.20 or later in `LIBS:`.
Use RTL8188EU 1.7 or RT2870 1.27 or later. When TCP/IP configures one of
these adapters, the library opens the picker before it scans. Networks
appear as they are found, and the picker waits for **Connect**.
The optional USB2WirelessManager Shell tool (0.33 or later) can also open
the ReAction settings for a chosen device and unit. End an existing
connection on that unit before opening setup.

```text
USB2WirelessManager SETUP
USB2WirelessManager SETUP DEVICE=usb2rt2870.device UNIT=0
```

Use Up and Down to select a network, fill its fields and move the cursor
to the end of **Password**. Tab and Shift+Tab move between the fields.
Return or keypad Enter moves from **SSID** to **Password** and connects
from **Password**. Selecting a network fills the **SSID** field. The **Security**
column shows the mode the adapter can use. Selecting a network also fills
the masked password field if the cache has a password for that SSID and
security mode. The library can reuse a matching password from another
adapter if this device and unit have none. The picker opens with empty fields
and selects the last successful SSID for the current adapter and unit when
that network appears, then focuses **Password**. You can use the list or
arrow keys to choose a network while the scan runs.
For a hidden network, type its exact name in
**SSID**. The manager sends a directed probe to discover its security.
Enter the password and choose **Connect**. Leaving the password blank
reuses a saved password for the same network and security mode or opens a
password prompt when one is needed.

A successful connection caches the network, channel and password in
`ENV:usb2wifi` for the current boot. Older profiles already in
`ENVARC:usb2wifi` can still be loaded through the normal AmigaOS startup.

Use the same `DEVICE` and `UNIT` options you used to configure TCP/IP.
`AUTO` also opens the picker, so existing startup scripts still bring up
the window. The following commands open it from a Shell task.

```text
Run <NIL: >NIL: USB2WirelessManager AUTO
Run <NIL: >NIL: USB2WirelessManager AUTO DEVICE=usb2rt2870.device UNIT=0
```

Use the line for the adapter you want to start. Keep the manager running
while connected.
It tries the last successful channel first and scans the rest if the network
has moved. Configure TCP/IP with the same SANA-II device and unit. If your
TCP/IP setup times out while Wi-Fi is waiting for a password, start the
interface after the manager reports that it is connected.

If the key exchange fails and can be retried, the full picker reopens with
the networks it already found and focuses the blank **Password** field for
the failed SSID. Newly discovered networks appear as the scan runs. You can
enter a new password or select another network, then choose **Connect** again.
Closing the picker cancels the attempt. A network initially submitted without a password
can still bring up the masked password requester. The library caches a
password after authentication succeeds.
