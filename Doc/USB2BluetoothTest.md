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

## Discover devices

Make a nearby Classic Bluetooth device discoverable, then run:

```text
USB2BluetoothTest DISCOVER
USB2BluetoothTest DISCOVER UNIT=1
```

Bluetooth2 scans for about five seconds and reports each address and device
class once. Discovery requires Bluetooth.Driver 1.1 or later.

The test resets the selected controller. Run it before starting any other
Bluetooth activity on that adapter. Classic discovery was tested on AmigaOS
with the KY-BT100 (`1131:1001`) and D-Link DBT-122 (`07d1:f101`).

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
repository. Secure Simple Pairing, BLE, and Bluetooth profiles are still in development.

See the [Bluetooth2 repository](https://github.com/rolsen74/bt2) and the
[Bluetooth USB HCI transport specification](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core_v7.0/out/en/host-controller-interface/usbtransport-layer-hci-usb--specification.html).
