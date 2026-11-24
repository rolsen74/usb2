
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "MSD.h"

// --

SEC_CODE void
MSD_Disk_Announce( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
	S32 optical;
	S32 announced;

	if ( ( msddisk->msddisk_Announced ) || ( msddisk->msddisk_MSDDev->msddev_Detached ) )
	{
		return;
	}

	optical = ( msddisk->msddisk_DeviceType == DG_CDROM ) ||
		( msddisk->msddisk_DeviceType == DG_OPTICAL_DISK );

	// The initial mount scan needs readable optical media.
	if ( ( optical ) && ( ( ! msddisk->msddisk_MediaPresent ) || ( ! msddisk->msddisk_Block_Size ) ) )
	{
		MSDDEBUG( "MSD: unit %lu LUN %lu deferring optical mounter announcement until media is ready\n",
			msddisk->msddisk_UnitNr, msddisk->msddisk_Lun );

		return;
	}

	announced = usbbase->usb_IMounter->AnnounceDeviceTags( MSD_Device_Name, msddisk->msddisk_UnitNr,
		MNTA_DosNamePrefixHint, "USB", TAG_END );

	msddisk->msddisk_Announced = announced ? TRUE : FALSE;

	MSDDEBUG( "MSD: unit %lu LUN %lu media %lu, mounter announce %ld\n",
		msddisk->msddisk_UnitNr, msddisk->msddisk_Lun, (U32) msddisk->msddisk_MediaPresent, announced );
}

// --
