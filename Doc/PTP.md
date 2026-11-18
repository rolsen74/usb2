# USB Picture Transfer Protocol

The PTP driver mounts a `PTP:` filesystem for compatible digital cameras.
It can read files and upload new files on cameras that support PTP uploads.
`PTP.Class` matches the USB Still Image Capture interface
(`06/01/01`). The filesystem requires DOS 53.95 or later on AmigaOS 4.

## Build and install

```sh
make -C Drivers/PTP -j4
```

Copy `Drivers/PTP/bin/PTP.Driver` and `Drivers/PTP/PTP.Class` to
`LIBS:USB2/`. The distribution archive includes both files under
`usb2/Libs/USB2/`. Start AmigaOS 4 with `usb2.device` loaded, connect the
camera in PTP mode, and open `PTP:` in Workbench or list it from the Shell.

## Browse and copy

Each camera appears as `Camera-<USB ID>`. Its storage areas appear as
`Storage-<PTP storage ID>` directories. For example, with the IDs shown by
your camera and computer, you can list and copy a photo with:

```text
List PTP:
List PTP:Camera-0000000000000001/Storage-00010001
Copy PTP:Camera-0000000000000001/Storage-00010001/DCIM/IMG_0001.JPG RAM:
```

Directory listings are cached while the camera is attached. Reconnect it to
refresh the listings. Characters that cannot be represented in an Amiga
filename are replaced with underscores.

The driver reads requested ranges when the camera supports PTP partial reads.
For other cameras it downloads the whole object into a temporary file under
`T:` on the first read and removes the file when the object is closed. Make
sure `T:` has room for the full object on those cameras. Objects whose size
cannot be represented in the standard PTP object record cannot be copied.

## Upload files

Copy to an existing directory on a writable camera storage area. For example:

```text
Copy RAM:IMG_0002.JPG PTP:Camera-0000000000000001/Storage-00010001/DCIM/
```

The file is staged under `T:` and uploaded when the destination file handle
closes. `T:` must have space for the full file. A successful close makes the
new file visible in `PTP:`. The camera may limit which file types or filenames
it accepts. Use a new filename for each upload. The driver protects existing
files and folders against replacement, renaming, and deletion.

Directory browsing, downloads, and uploads have been tested with a mocked
camera. Testing on AmigaOS 4 with a physical camera is pending.
