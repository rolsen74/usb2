# USB2BluetoothTest

USB2BluetoothTest probes a controller registered with `bluetooth2.device`.
It sends HCI Reset, Read Local Version Information, and Read BD_ADDR.
The Shell output shows the USB ID, interrupt event endpoint, controller
version, manufacturer ID, and Bluetooth address.

## Install and test

1. Build `bluetooth2.kmod` from the companion Bluetooth2 repository.
   Use Bluetooth2 1.3 or later for discovery. Install it in `SYS:Kickstart/` and add
   `MODULE Kickstart/bluetooth2.kmod` to your USB2 Kicklayout entry.
2. Copy `Bluetooth.Class` and `Bluetooth.Driver` from the USB2 archive's
   `Libs/USB2/` directory to `LIBS:USB2/`, and reboot.
3. Plug in a Bluetooth dongle. The serial console reports its assigned
   unit. Run `USB2BluetoothTest` from a Shell for unit 0 or pass `UNIT=1`
   to test unit 1.
4. Unplug a dongle and check for the detach message. A probe for that unit
   then reports that the controller is unavailable. Plug it back in and
   use the unit shown in the new attach message.

### Realtek RTL8761BU adapters

Bluetooth.Driver 1.19 can load firmware for the ASUS USB-BT500
(`0B05:190E`), TP-Link UB500 (`2357:0604`), and Realtek reference
adapter (`0BDA:8771`). The USB IDs identify the revisions covered here.
Copy `rtl8761bu_fw.bin` and, if available, `rtl8761bu_config.bin` from
the [linux-firmware Realtek directory](https://kernel.googlesource.com/pub/scm/linux/kernel/git/firmware/linux-firmware/+/refs/tags/20260221/rtl_bt/)
to `DEVS:Firmware/`. The firmware files are distributed by linux-firmware.

After plugging in the adapter, look for `RTL8761BU firmware ready` and the
controller's assigned unit. Run `USB2BluetoothTest UNIT=<unit>` and then
`USB2BluetoothTest UNIT=<unit> LESCAN SECONDS=10`. The firmware loader has
passed simulated tests and awaits testing on these physical adapters.

## Discover devices

Make a nearby Classic Bluetooth device discoverable, then run:

```text
USB2BluetoothTest DISCOVER
USB2BluetoothTest DISCOVER UNIT=1
```

Bluetooth2 scans for about five seconds and reports each address and device
class once. Discovery requires Bluetooth.Driver 1.1 or later.

### Look up a device name

Use Bluetooth2 1.6, Bluetooth.Driver 1.4, and USB2BluetoothTest 0.8. Copy an
address from discovery and keep the peer within range.

```text
USB2BluetoothTest UNIT=1 NAME=AA:BB:CC:DD:EE:FF
```

Name lookup can run before pairing. It may take several seconds for the
controller to reach the peer. The Shell replaces non-ASCII bytes with `?`.

### Make the Amiga discoverable

With Bluetooth2 1.7, Bluetooth.Driver 1.5, and USB2BluetoothTest 0.9, run:

```text
USB2BluetoothTest UNIT=0 VISIBLE AS="Amiga X5000" SECONDS=60
```

Start a Classic Bluetooth scan on a nearby PC while the command runs.
`VISIBLE` defaults to the name `Amiga` and a 30-second window. It accepts
`SECONDS` from 1 to 120 and ends discoverability automatically. The test
answers name lookups and declines incoming connection requests.

The test resets the selected controller. Run it before starting any other
Bluetooth activity on that adapter. Classic discovery was tested on AmigaOS
with the KY-BT100 (`1131:1001`) and D-Link DBT-122 (`07d1:f101`).

## Test Bluetooth data with L2CAP echo

With Bluetooth2 1.8, Bluetooth.Driver 1.6, and USB2BluetoothTest 0.10, substitute
the address of a nearby Classic Bluetooth peer:

```text
USB2BluetoothTest UNIT=0 ECHO=AA:BB:CC:DD:EE:FF
```

The test opens a temporary Classic ACL link, sends a single L2CAP echo
request, and disconnects. The test reports separately when the link fails
to open, when the peer rejects the request, or when an open link gets no
echo response.

## Browse Classic Bluetooth services

With Bluetooth2 1.9, Bluetooth.Driver 1.7, and USB2BluetoothTest 0.11, run:

```text
USB2BluetoothTest UNIT=0 SERVICES=AA:BB:CC:DD:EE:FF
```

The test queries the peer's public SDP records over a Basic Mode L2CAP
channel. Each row shows the record handle, its first service-class UUID,
and the name when the peer supplies one. Bluetooth2 1.10 and later also
show the RFCOMM channel when it is published. The test lists up to 32 records.

## Test an RFCOMM serial channel

Use Bluetooth2 1.10, Bluetooth.Driver 1.8, and USB2BluetoothTest 0.12.
Start a Classic Bluetooth serial service on the peer and use `SERVICES`
to find its RFCOMM channel. Then run the test with that channel number:

```text
USB2BluetoothTest UNIT=0 RFCOMM=AA:BB:CC:DD:EE:FF CHANNEL=1
```

If a saved bond exists, the test uses it to authenticate and enable
encryption. It opens the RFCOMM multiplexer and serial channel, exchanges
modem status, and closes the connection.

With Bluetooth2 1.11, Bluetooth.Driver 1.9, and USB2BluetoothTest 0.13,
add `TEXT` to send data and `REPLY` to wait for the peer's first non-empty
frame. Use a peer serial application that can receive and answer the text.

```text
USB2BluetoothTest RFCOMM=AA:BB:CC:DD:EE:FF CHANNEL=1 TEXT="Hello from Amiga" REPLY
```

The tool sends the text as entered without appending a newline. For
credit-based flow control it accepts up to 512 bytes and sends them in
negotiated frame sizes. For basic flow control it sends one frame.
`REPLY` displays up to 127 bytes and escapes nonprintable bytes as hex.
An Amiga serial device for continuous transfers is future work.

### Send a binary file

With Bluetooth2 1.12, Bluetooth.Driver 1.10, and USB2BluetoothTest 0.14,
start a serial receiver on the peer and capture its raw input to a file.
Then run:

```text
USB2BluetoothTest RFCOMM=AA:BB:CC:DD:EE:FF CHANNEL=1 FILE="RAM:sample.bin"
```

The command opens one RFCOMM connection and streams the file as raw bytes.
The peer must negotiate credit-based flow control. Press Ctrl-C to cancel.
Compare the captured file with the source to verify the transfer on hardware.

### Receive a binary file

With Bluetooth2 1.13, Bluetooth.Driver 1.11, and USB2BluetoothTest 0.15,
enter the exact size of the file the peer will send:

```text
USB2BluetoothTest UNIT=0 RFCOMM=AA:BB:CC:DD:EE:FF CHANNEL=1 SAVE="RAM:received.bin" BYTES=4096
```

Send the raw bytes from the peer's serial application once the channel opens.
The command saves the file after it receives the requested count. Choose a
new destination filename. The peer must negotiate credit-based flow control,
and Ctrl-C cancels the capture.

### Send with Bluetooth Object Push

With Bluetooth2 1.16, Bluetooth.Driver 1.12 or later, and USB2BluetoothTest
0.18, start the peer's Bluetooth file receiver. The sender looks up the
Object Push service (class `$1105`) directly.

```text
USB2BluetoothTest UNIT=0 RFCOMM=AA:BB:CC:DD:EE:FF PUSH="RAM:sample.bin"
```

You can specify `CHANNEL=3` if the receiver's Object Push channel is 3.
Bluetooth2 1.14 and 1.15 require the explicit channel.

The peer receives the source filename and can ask whether to accept the
file. The command reports success after the receiver accepts the entire
file. It supports files up to 4 GiB minus one byte. Press Ctrl-C to cancel.

An Amiga successfully sent a file to Windows using the `07D1:F101` dongle.

### Use the ReAction window

```text
USB2BluetoothTest GUI UNIT=0
```

Click **Scan Classic**, then select an address or enter it manually.
Open the peer's Bluetooth file receiver. Choose a local file and click
**Send file**. Leave the channel at `0` to look it up automatically.
The window stays responsive during scanning and sending. **Stop** requests
cancellation, and the window title shows the result.

## Play a WAV on a Bluetooth speaker

Install Bluetooth2 1.17, Bluetooth.Driver 1.14, and USB2BluetoothTest 0.19.
Put the speaker in pairing mode and use **Scan Classic** or `DISCOVER` to
find its address. The speaker must support Classic A2DP and the SBC codec.
If it requires authentication, playback can reuse a bond saved with `BOND`.

```text
USB2BluetoothTest UNIT=0 SPEAKER=AA:BB:CC:DD:EE:FF PLAY="RAM:music.wav"
```

Use a 16-bit PCM WAV at 44.1 or 48 kHz, with one or two channels. The tool
streams the file and encodes SBC during playback. Press Ctrl-C to stop.
In the ReAction window, select the speaker and WAV file, then click
**Play WAV**. Playback on a physical speaker is awaiting validation.

Bluetooth2 1.20 lets audio connections negotiate faster BR/EDR packet
types. Bluetooth.Driver 1.16 queues controller events ahead of audio writes.
Look for the `Bluetooth2 A2DP:` line in serial output after playback.
It compares the audio duration with the controller send time and reports
late packets and peak lag in milliseconds.

For application audio, select a speaker and click **Use for AHI**, then
follow the [Bluetooth AHI setup](AHI.md#bluetooth-speaker-playback).

## Monitor a Classic Bluetooth keyboard or mouse

Monitoring requires Bluetooth2 1.21, Bluetooth.Driver 1.17, and
USB2BluetoothTest 0.21 or later.
Use `DISCOVER` to find the address of a Classic Bluetooth keyboard or mouse
that supports Boot Protocol. If needed, use `BOND` first to save a link key
for the same adapter and peripheral.

```text
USB2BluetoothTest UNIT=0 HID=AA:BB:CC:DD:EE:FF SECONDS=15
```

Press a few keys or move and click the mouse while the command runs. The
Shell prints keyboard usage codes or mouse movement and buttons when the
capture finishes. It stops after 32 input reports, after the selected
duration, or when you press Ctrl-C.

### Use a Classic keyboard or mouse in Workbench

With Bluetooth2 1.22, Bluetooth.Driver 1.18, and USB2BluetoothTest 0.22,
save a bond using the same adapter and peripheral, then start input.

```text
USB2BluetoothTest UNIT=0 BOND=AA:BB:CC:DD:EE:FF
USB2BluetoothTest UNIT=0 HID=AA:BB:CC:DD:EE:FF INPUT
```

The session sends Boot Protocol keys, modifiers, relative movement and
three mouse buttons to `input.device`. Ctrl-C or disconnect ends the
session and releases any keys or buttons that are still held. The adapter
is reserved for input while the command runs. Physical validation, key
repeat, keyboard LEDs, and BLE HID over GATT are pending.

## Scan and advertise over Bluetooth LE

Bluetooth2 1.15, Bluetooth.Driver 1.13, and USB2BluetoothTest 0.17 add
Bluetooth Low Energy testing with an LE-capable controller.

```text
USB2BluetoothTest UNIT=0 LESCAN SECONDS=10
USB2BluetoothTest UNIT=0 LESCAN PASSIVE SECONDS=10
USB2BluetoothTest UNIT=0 LEADVERTISE AS="Amiga" SECONDS=30
```

An active scan can request extra data such as the device name. A passive
scan only listens. The tool lists up to 64 distinct addresses with address
type, signal strength, and name when available. Use a BLE scanner on a
phone to see the Amiga's nonconnectable advertisement. Both commands
use legacy LE advertising on the primary channels and support Ctrl-C.

## Test legacy PIN pairing

Install Bluetooth2 1.4 and Bluetooth.Driver 1.2. Put a Classic Bluetooth
device that supports legacy PIN pairing into pairing mode and find its address
with `DISCOVER`. Use that address in the command:

```text
USB2BluetoothTest UNIT=1 PAIR=AA:BB:CC:DD:EE:FF
```

Enter the same PIN on both devices when prompted. The test verifies that the
link authenticates and enables encryption, then disconnects. `PAIR` leaves
the saved bonds alone.

## Save and reuse a bond

With Bluetooth2 1.5, Bluetooth.Driver 1.3, and USB2BluetoothTest 0.7, run
`BOND` while the peer is in pairing mode. Then run `RECONNECT` using the same
dongle and peer address.

```text
USB2BluetoothTest UNIT=1 BOND=AA:BB:CC:DD:EE:FF
USB2BluetoothTest UNIT=1 RECONNECT=AA:BB:CC:DD:EE:FF
```

`BOND` saves a new link key following successful authentication and
encryption. The binary key is stored under `ENV:Bluetooth2/` and
`ENVARC:Bluetooth2/` using both Bluetooth addresses. `RECONNECT` can reuse
it after reboot without prompting for a PIN. If the peer still has a bond
from an earlier `PAIR` test, forget the Amiga on that device before bonding
again.

An Amiga connected to a PC using a legacy PIN, saved the bond, and reused the
saved link key after reboot without prompting for the PIN.

## Build

```sh
make -f makefile_archive build_bluetooth_test
```

`Drivers/Bluetooth/USBHCI` owns USB registration and HCI endpoint I/O.
Each class task registers a request port with Bluetooth2 and performs the
requested operation on the same task that owns the USB requests. Host HCI
parsing, inquiry, and the controller registry live in the companion Bluetooth2
repository. Secure Simple Pairing, LE connections and GATT, and additional
Bluetooth profiles are still in development.

See the [Bluetooth2 repository](https://github.com/rolsen74/bt2) and the
[Bluetooth USB HCI transport specification](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core_v7.0/out/en/host-controller-interface/usbtransport-layer-hci-usb--specification.html).
