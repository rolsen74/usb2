
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32
_fs_Handler_Shutdown( struct FS_Struct *fs )
{
	S32 error;

	ObtainSemaphore( &fs->fs_Semaphore );

	ObtainSemaphore( &FS_PortGate );

	if ( ( FS_Users ) || ( fs->fs_LockList ) || ( FS_Closing ) )
	{
		ReleaseSemaphore( &FS_PortGate );

		ReleaseSemaphore( &fs->fs_Semaphore );

		return ( ERROR_OBJECT_IN_USE );
	}

	FS_Closing = TRUE;

	ReleaseSemaphore( &FS_PortGate );

	error = 0;

	if ( ! NonBlockingModifyDosEntry( (PTR)fs->fs_VolumeNode, NBM_REMFREEDOSENTRY, 0, 0 ) )
	{
		error = ERROR_OBJECT_IN_USE;
	}
	else
	{
		fs->fs_VolumeAdded = FALSE;
		fs->fs_VolumeNode = NULL;
		fs->fs_Running = FALSE;

		NonBlockingModifyDosEntry( NULL, NBM_DISKREMOVED, 0, 0 );
	}

	ObtainSemaphore( &FS_PortGate );

	if ( error )
	{
		FS_Closing = FALSE;
	}
	else
	{
		FS_CmdMsgPort = NULL;
	}

	ReleaseSemaphore( &FS_PortGate );

	ReleaseSemaphore( &fs->fs_Semaphore );

	return ( error );
}

// --
