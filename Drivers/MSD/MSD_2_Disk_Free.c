
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
MSD_Disk_Free( struct USBBase *usbbase, struct MSDDisk *msddisk )
{
	struct MSDBase *msdbase;

	S32 release;
	U32 unit;

	msdbase = msddisk->msddisk_MSDBase;

	SEMAPHORE_OBTAIN( & msdbase->msdbase_MSDDisk_Semaphore );

	unit = msddisk->msddisk_UnitNr;

	if ( ( unit < MAX_MSD_UNITS ) && ( msdbase->msdbase_MSDDisk_Units[unit] == msddisk ) )
	{
		msdbase->msdbase_MSDDisk_Units[unit] = NULL;
		msdbase->msdbase_Reserved[unit] += msddisk->msddisk_OpenCnt;
	}

	msddisk->msddisk_Released = TRUE;
	release = msddisk->msddisk_OpenCnt == 0;

	SEMAPHORE_RELEASE( & msdbase->msdbase_MSDDisk_Semaphore );

	if ( release )
	{
		MEM_FREEVEC( msddisk );
	}
}

// --
