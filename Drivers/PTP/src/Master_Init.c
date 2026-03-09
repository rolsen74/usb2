
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32 Master_Init( struct intern *in )
{
S32 retval;

	retval = FALSE;

	// --
	
	in->FSStatus = MountDeviceTags( "PTP:", MDT_Handler,
		MD_Activate, TRUE,
		MD_Entry, _fs_Handler,
		TAG_END
	);

	if ( ! in->FSStatus )
	{
		MYERROR( "PTP-FS : Error mounting FileSystem" );
		goto bailout;
	}

	in->FSMsgPort = FindPort( "PTP_FileSystem" );

	// --

	retval = TRUE;

bailout:

	return( retval );
}

// --
