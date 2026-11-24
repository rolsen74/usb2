# USB PTP and MTP

cls_PTP.Driver uses one `PTP:` volume for compatible PTP cameras, MTP phones,
and media players. Browse and download files from each device's directory.
Uploads require the device to advertise the standard
`SendObjectInfo` and `SendObject` operations.

`cls_PTP.Class` matches the USB Still Image Capture interface (`06/01/01`).
The driver reads the DeviceInfo vendor extension ID to identify MTP on that
interface. `cls_MTP.Class` also recognizes Android-style vendor interfaces
(`FF/FF/00`) that identify themselves with the USB interface name `MTP`.
The filesystem requires DOS 53.95 or later on AmigaOS 4.

## Build and install

```sh
make -C Drivers/PTP
```

Copy `cls_PTP.Driver`, `cls_PTP.Class`, and `cls_MTP.Class` from the archive's
`usb2/Libs/USB2/` directory to `LIBS:USB2/` and reboot. Connect a camera
in PTP mode or unlock an MTP-capable phone and select **File transfer** in
its USB settings. Open `PTP:` in Workbench or use `List PTP:` in Shell.

## Browse and copy

Each device appears under the model name reported by DeviceInfo.
`Apple iPhone` appears as `iPhone`. If a name is already in use, the driver
appends a suffix such as `-2`. A device without a model name appears as
`Camera-<USB ID>` for PTP or `Device-<USB ID>` for MTP. Storage directories
use the volume label or storage description reported by the device, with
suffixes for duplicates within each device. Unnamed storage appears as
`Storage-<PTP storage ID>`. Use the names
shown by `List PTP:` and `List PTP:iPhone/` in your paths.

```text
List PTP:
List PTP:iPhone/Storage-00010001
Copy PTP:iPhone/Storage-00010001/DCIM/IMG_0001.JPG RAM:
```

Directory listings are cached for each mounted store. Removing media
invalidates the old directory tree and its open handles, even if the same
storage ID returns. If USB event monitoring is unavailable, storage IDs
are checked on access at most once every two seconds. Reconnect to refresh
object changes within the same store or a same-ID swap missed by polling.
Characters that cannot be represented in an Amiga filename are replaced
with underscores.

The driver reads requested ranges when the camera supports PTP partial reads.
For other cameras it downloads the whole object into a temporary file under
`T:` on the first read and removes the file when the object is closed. Make
sure `T:` has room for the full object on those cameras. Objects whose size
cannot be represented in the standard PTP object record cannot be copied.

Dismount `PTP:` after closing its files and disconnecting its cameras.
The next camera attach can start the volume again.

## Upload files

Copy to an existing directory on a writable camera storage area. For example:

```text
Copy RAM:IMG_0002.JPG "PTP:Digital Camera/Storage-00010001/DCIM/"
```

The file is staged under `T:` and uploaded when the destination file handle
closes. `T:` must have space for the full file. A successful close makes the
new file visible in `PTP:`. The camera may limit which file types or filenames
it accepts. Use a new filename for each upload. The driver protects existing
files and folders against replacement, renaming, and deletion.

Directory browsing, downloads, and uploads have been tested with a mocked
camera. MTP identification and shared-volume browsing and transfers have
been tested with mocked devices. Testing on physical hardware is pending.
