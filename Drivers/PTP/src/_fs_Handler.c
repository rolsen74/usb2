
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

U32 _fs_Handler( void )
{
struct PTP_FSStruct *fs;
struct DosPacket *startup_dp;
struct Message *startup_msg;
struct Process *self;
U32 retval;

	// Setup Debug Values

	fs = NULL;

	retval = RETURN_FAIL;

	/*
	** ABSOLUTELY must fetch the handler startup dospacket FIRST.
	** It is totally illegal to perform any I/O before this.
	** Or, call anything that may indirectly cause I/O via this process.
	*/

	self = (PTR) FindTask( NULL );

	while( TRUE )
	{
		startup_msg	= GetMsg( & self->pr_MsgPort );

		if ( startup_msg )
		{
			break;
		}

		WaitPort( & self->pr_MsgPort );
	}

	startup_dp = (PTR) startup_msg->mn_Node.ln_Name;

	/*
	** Check if it is actually a dospacket,
	** workbench startup messages have a NULL for ln_Name,
	** some idiot may have stuck an icon onto the binary file
	** and tried to run it from WB.
	*/

	if ( ! startup_dp )
	{
		MYERROR( "PTP-FS : Error not a DOS Packet" );
		goto bailout;
	}

	fs = AllocVecTags( sizeof( struct PTP_FSStruct ),
		AVT_Clear,	0x00,
		AVT_Type,	MEMF_SHARED,
		AVT_Lock,	TRUE,
		TAG_END
	);

	if ( ! fs )
	{
		MYERROR( "PTP-FS : Error Allocating Memory" );
		goto bailout;
	}

	InitSemaphore( & fs->fs_Semaphore );

	fs->fs_DeviceNode = BADDR( startup_dp->dp_Arg3 );

	// --

	if ( _fs_Handler_Init( fs ))
	{
		startup_dp->dp_Res1 = TRUE;
		startup_dp->dp_Res2 = 0;
		ReplyMsg( startup_msg );
		startup_dp = NULL;

		_fs_Handler_Loop( fs );

		retval = RETURN_OK;
	}

	_fs_Handler_Free( fs );

bailout:

	if ( startup_dp )
	{
		startup_dp->dp_Res1 = DOSFALSE;
		startup_dp->dp_Res2 = ERROR_NO_FREE_STORE;
		ReplyMsg( startup_msg );
		startup_dp = NULL;
	}

	if ( fs )
	{
		UnlockMem( fs, sizeof( struct PTP_FSStruct ));
		FreeVec( fs );
	}

	Forbid();

	return( retval );
};

// --
