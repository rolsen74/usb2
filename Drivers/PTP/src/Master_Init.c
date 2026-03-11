
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32 Master_Init( struct M_Struct *ms )
{
S32 retval;

	MYERROR( "PTP : Master_Init : Enter" );

	retval = FALSE;

	// --

	Master_MsgPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! Master_MsgPort )
	{
		MYERROR( "PTP-Master : Error creating MsgPort" );
		goto bailout;
	}

	// --

	ms->FSStatus = MountDeviceTags( "PTP:", MDT_Handler,
		MD_Handler, MDT_FileSystem,
		MD_Entry, _fs_Handler,
		MD_Activate, TRUE,
		TAG_END
	);

	if ( ! ms->FSStatus )
	{
		MYERROR( "PTP-Master : Error mounting FileSystem" );
		goto bailout;
	}

	ms->FSMsgPort = FindPort( "PTP_FileSystem" );

	// --

	retval = TRUE;

bailout:

	return( retval );
}

// --
