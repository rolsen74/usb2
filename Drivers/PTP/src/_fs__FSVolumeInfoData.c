
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

int32 _fs__FSVolumeInfoData( struct FSVP *vp, int32 *res2, struct InfoData *info )
{
struct FS_Struct *fs;
int32 retval;

	MYINFO( "PTP-FS : _fs__FSVolumeInfoData" );

	fs = vp->FSV.FSPrivate;
	
	ObtainSemaphore( & fs->fs_Semaphore );

	if (( fs->fs_VolumeNode ) 
	&&	( fs->fs_VolumeNode->vn_DiskType != ID_NO_DISK_PRESENT ))
	{
		retval = _fs__FSDeviceInfoData( vp, res2, info );

		info->id_DiskType = ID_DOS_DISK;
	}
	else
	{
		if ( res2 )
		{
			*res2 = ERROR_DEVICE_NOT_MOUNTED;
		}

		retval	= FALSE;
	}
	
	ReleaseSemaphore( & fs->fs_Semaphore );

	return( retval );
}

// --

