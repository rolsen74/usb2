# USB2 for AmigaOS 4

Experimental USB host stack for AmigaOS 4. Includes UHCI, OHCI, EHCI, and experimental xHCI support.

## Guides

- Host controllers and built-in drivers. [EHCI](Doc/EHCI.md), [OHCI](Doc/OHCI.md), [UHCI](Doc/UHCI.md), [xHCI](Doc/XHCI.md), [hubs](Doc/HUB.md), [HID](Doc/HID.md), [mass storage](Doc/MSD.md)
- USB classes. [Audio](Doc/Audio.md), [AHI](Doc/AHI.md), [video](Doc/Video.md), [printers](Doc/Printer.md), [PTP and MTP](Doc/PTP.md), [serial](Doc/PL2303.md), [smart cards](Doc/SmartCard.md)
- Networking. [RTL8152](Doc/RTL8152.md), [RTL8188EU](Doc/RTL8188.md), [RT2870](Doc/RT2870.md), [Wi-Fi setup](Doc/USB2WirelessManager.md)
- Bluetooth requires the companion stack. [Bluetooth2](https://github.com/rolsen74/bt2), [USB Bluetooth guide](Doc/USB2BluetoothTest.md)
- Tools and controllers. [USB2Analyzer](Doc/USB2Analyzer.md), [Competition Pro](Doc/CompPro.md), [Xbox 360](Doc/Xbox360.md)

## Install

1. Copy `usb2.kmod` to `SYS:Kickstart/`. Add `MODULE Kickstart/usb2.kmod` to your USB2 Kicklayout entry.
2. Copy the archive's `Devs/` and `Libs/` contents to `DEVS:` and `LIBS:`, keeping their subdirectories.
3. Reboot. Run the utilities from `Tools/`. For networking, select `usb2ethernet.device` and the unit assigned to your adapter.

## Archive layout

```text
usb2/
├── Devs/
│   ├── AHI/
│   │   ├── bluetooth2.audio
│   │   └── usb2.audio
│   ├── AudioModes/
│   │   ├── Bluetooth2
│   │   └── USB2
│   ├── Firmware/
│   │   ├── rt2870.bin
│   │   ├── rt2870.licence
│   │   ├── rtl8188eufw.bin
│   │   └── rtl8188eufw.licence
│   ├── Networks/
│   │   └── usb2ethernet.device
│   ├── usb2parallel.device
│   ├── usb2printer.device
│   └── usb2serial.device
├── Kickstart/
│   └── usb2.kmod
├── Libs/
│   ├── USB2/
│   │   ├── cls_Audio.Class
│   │   ├── cls_Audio.Driver
│   │   ├── cls_Bluetooth.Class
│   │   ├── cls_Bluetooth.Driver
│   │   ├── cls_CDCACM.Class
│   │   ├── cls_MTP.Class
│   │   ├── cls_PTP.Class
│   │   ├── cls_PTP.Driver
│   │   ├── cls_Printer.Class
│   │   ├── cls_Printer.Driver
│   │   ├── cls_SmartCard.Class
│   │   ├── cls_SmartCard.Driver
│   │   ├── cls_Video.Class
│   │   ├── cls_Video.Driver
│   │   ├── vid_PlayStationEye.Class
│   │   ├── eth_RT2870.Class
│   │   ├── eth_RT2870.Driver
│   │   ├── eth_RTL8152.Class
│   │   ├── eth_RTL8152.Driver
│   │   ├── eth_RTL8188EU.Class
│   │   ├── eth_RTL8188EU.Driver
│   │   ├── eth_RTL8821CU.Class
│   │   ├── eth_RTL8821CU.Driver
│   │   ├── eth_SiS163U.Class
│   │   ├── eth_SiS163U.Driver
│   │   ├── ser_PL2303.Class
│   │   └── ser_PL2303.Driver
│   ├── usb2wifi.library
│   ├── usb2wpa.library
│   ├── usb2wpa2.library
│   └── usb2wpa3.library
└── Tools/
    ├── USB2Analyzer
    ├── USB2AudioTest
    ├── USB2BluetoothTest
    ├── USB2SDRTest
    ├── USB2SmartCardTest
    ├── USB2VideoTest
    └── USB2WirelessManager
```

## Build

Use the AmigaOS 4 cross-compiler. Run `make` for the stack or `make -f makefile_archive` to build `usb2.lha` with `lha` installed.

## License

Copyright 2012–2026 Rene W. Olsen. [GPL-3.0-or-later](LICENSE).
