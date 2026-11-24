
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

// Serialize remounts without holding the gate needed by handler startup.
static struct SignalSemaphore MountGate;

// --

void
_fs_Init_Acquire( void )
{
	InitSemaphore( &MountGate );
}

// --

S32
_fs_Acquire( void )
{
	S32 start;
	S32 result;

	ObtainSemaphore( &MountGate );

	ObtainSemaphore( &FS_PortGate );

	start = ( ! FS_CmdMsgPort ) && ( ! FS_Closing );

	if ( start )
	{
		FS_Users++;
	}

	ReleaseSemaphore( &FS_PortGate );

	if ( start )
	{
		_fs_Start( TRUE );
	}

	ObtainSemaphore( &FS_PortGate );

	result = ( FS_CmdMsgPort != NULL ) && ( ! FS_Closing );

	if ( ( result ) && ( ! start ) )
	{
		FS_Users++;
	}
	else if ( ( ! result ) && ( start ) )
	{
		FS_Users--;
	}

	ReleaseSemaphore( &FS_PortGate );

	ReleaseSemaphore( &MountGate );

	return ( result );
}

// --
