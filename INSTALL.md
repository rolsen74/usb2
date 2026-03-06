# USB2 Stack Installation Notes

To install the new USB2 stack, the recommended approach is to create a new boot configuration in Kickstart/Kicklayout rather than modifying your existing one. This allows you to safely test the new stack while keeping your original configuration intact.

## Step 1: Clone Existing Configuration

Copy your default configuration section (for example “AmigaOS 4.1”) and rename it to something like:

```
LABEL AmigaOS 4.1 (USB2)
```

## Step 2: Remove Old USB Stack Modules

In the new section only, remove the following modules from the Kickstart list:

```
MODULE Kickstart/usbresource.library
MODULE Kickstart/usbsys.device
MODULE Kickstart/bootmouse.usbfd
MODULE Kickstart/bootkeyboard.usbfd
MODULE Kickstart/hub.usbfd
MODULE Kickstart/massstorage.usbfd
MODULE Kickstart/uhci.usbhcd
MODULE Kickstart/ohci.usbhcd
MODULE Kickstart/ehci.usbhcd
```

These belong to the old USB stack and must not be loaded together with USB2.

## Step 3: Add USB2 Module

In the same location where the old USB modules were listed, add:

```
MODULE Kickstart/usb2.kmod
```

Do NOT load any of the old USB stack components together with usb2.kmod.

## Important Notes

* Only modify the new configuration section.
* Keep your original configuration unchanged as a fallback.
* Do not mix old USB stack modules with usb2.kmod.
* If the system fails to boot, select your original configuration from the boot menu.

After saving the updated Kicklayout, reboot and select the new configuration to test the USB2 stack.


```asm
; Configuration name
LABEL AmigaOS 4.1 (USB2)
EXEC Kickstart/loader
MODULE Kickstart/kernel
;
MODULE Kickstart/FastFileSystem
MODULE Kickstart/CDFileSystem
MODULE Kickstart/NGFileSystem
MODULE Kickstart/SmartFileSystem
;
MODULE Kickstart/battclock.resource.kmod
MODULE Kickstart/bootmenu.kmod
MODULE Kickstart/bootimage
MODULE Kickstart/bootsd.resource.kmod
MODULE Kickstart/con-handler.kmod
MODULE Kickstart/console.device.kmod
MODULE Kickstart/dos.library.kmod
MODULE Kickstart/diskboot.kmod
MODULE Kickstart/diskboot.config
MODULE Kickstart/elf.library.kmod
MODULE Kickstart/env-handler.kmod
MODULE Kickstart/FileSystem.resource.kmod
MODULE Kickstart/gadtools.library.kmod
MODULE Kickstart/hunk.library.kmod
MODULE Kickstart/input.device.kmod
MODULE Kickstart/intuition.library.kmod
MODULE Kickstart/keyboard.device.kmod
MODULE Kickstart/keymap.library.kmod
MODULE Kickstart/mounter.library
MODULE Kickstart/newlib.library.kmod
MODULE Kickstart/nonvolatile.library.kmod
MODULE Kickstart/petunia.library.kmod
MODULE Kickstart/ram-handler.kmod
MODULE Kickstart/ramdrive.device.kmod
MODULE Kickstart/ramlib.kmod
MODULE Kickstart/shell.kmod
MODULE Kickstart/strap.kmod
MODULE Kickstart/timer.device.kmod
;
MODULE Kickstart/graphics.library.kmod
MODULE Kickstart/PCIGraphics.card
MODULE Kickstart/RadeonHD.chip
MODULE Kickstart/ATIRadeon.chip
MODULE Kickstart/3DLabsPermedia2.chip
MODULE Kickstart/3dfxVoodoo.chip
;
MODULE Kickstart/p5020sata.device.kmod
MODULE Kickstart/sii3112ide.device.kmod
MODULE Kickstart/sii3114ide.device.kmod
MODULE Kickstart/sii3512ide.device.kmod
MODULE Kickstart/sii0680ide.device.kmod
MODULE Kickstart/lsi53c8xx.device.kmod
MODULE Kickstart/it8212ide.device.kmod
;
MODULE Kickstart/usb2.kmod
MODULE Kickstart/xena.resource.kmod
```
