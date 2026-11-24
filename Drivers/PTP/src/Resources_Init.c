
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32
_fs_Start( S32 restart )
{
	struct TaskStartMsg *msg;
	struct MsgPort *reply;
	struct Process *process;

	S32 retval;

	retval = FALSE;
	msg = NULL;
	reply = NULL;

	reply = AllocSysObjectTags( ASOT_PORT, TAG_END );

	msg = AllocVecTags( sizeof( *msg ), AVT_Type, MEMF_SHARED, AVT_Clear, 0, TAG_END );

	if ( ( ! reply ) || ( ! msg ) )
	{
		goto bailout;
	}

	msg->Restart = restart;
	msg->Message.mn_ReplyPort = reply;
	msg->Message.mn_Length = sizeof( *msg );

	process = CreateNewProcTags(
		NP_Name, "PTP Master",
		NP_Child, FALSE,
		NP_Priority, 5,
		NP_StackSize, 1024 * 32,
		NP_Entry, Master_Entry,
		NP_UserData, msg,
		NP_ExitCode, Master_Exit,
		NP_ExitData, msg,
		TAG_END );

	if ( process )
	{
		do
		{
			WaitPort( reply );
		}
		while ( GetMsg( reply ) != &msg->Message );

		retval = msg->Result;
	}

bailout:

	FreeVec( msg );

	if ( reply )
	{
		FreeSysObject( ASOT_PORT, reply );
	}

	return ( retval );
}

// --

S32
Resources_Init( struct PTPBase *libBase UNUSED )
{
	U32 version;
	S32 ready;

	InitSemaphore( &FS_PortGate );

	_fs_Init_Acquire();

	if ( ! myOpenResources( INT_MAX ) )
	{
		return ( FALSE );
	}

	version = ( DOSBase->lib_Version << 16 ) | DOSBase->lib_Revision;

	if ( version < ( ( 53 << 16 ) | 95 ) )
	{
		return ( FALSE );
	}

	FS_Users = 1;

	ready = _fs_Start( FALSE );

	_fs_Release();

	return ( ready );
}

// --
