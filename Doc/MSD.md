# USB Mass Storage

The MSD driver uses USB Bulk-Only Transport. It accepts SCSI transparent,
ATAPI CD/DVD, UFI floppy, and SFF-8070i interfaces that use this transport.
It exposes the devices through `usb2disk.device`.

- Block reads and writes support 32-bit and 64-bit byte offsets. The driver
  uses SCSI READ/WRITE(10) or READ/WRITE(16) as needed.
- `TD_SEEK`, `TD_FORMAT`, and their 64-bit counterparts are implemented.
  `TD_FORMAT` writes the supplied sector data without issuing SCSI FORMAT UNIT.
- Removable media is checked every two seconds. The driver tracks media
  presence and disk changes and signals registered `TD_ADDCHANGEINT` handlers.
- `TD_EJECT` loads or ejects media on drives that support it. `TD_MOTOR` sends
  START STOP UNIT for UFI and SFF floppy devices.
- CD-ROM media is reported as write-protected. `CMD_UPDATE` synchronizes the
  cache on supported writable devices.

## Limitations

- USB floppy drives using CBI transport are not supported.
- Exec queue and device-control commands `CMD_START`, `CMD_STOP`, `CMD_FLUSH`,
  and `CMD_RESET` are unsupported. Use `TD_MOTOR` for a supported floppy
  motor and `CMD_UPDATE` to commit buffered writes.
- `TD_REMOVE`, `TD_GETNUMTRACKS`, `TD_RAWREAD`, `TD_RAWWRITE`, and the ETD
  commands are unsupported. `TD_GETGEOMETRY` returns the device's sector size
  and total sector count with a synthetic track layout.
- The recent CD/DVD, floppy, media-change, and 64-bit paths build successfully
  but have not been verified with physical drives in this workspace.

## Build and install

This driver is enabled in the default `usb2.device` build by
`DRV_MSD=1`. Build with `make -j4` from the repository root and follow the
[stack installation steps](../README.md#building-the-distribution-archive).
