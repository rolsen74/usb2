
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct USB_Command *_usb_Bulk_Cmd_Get( struct USB_Struct *us )
{
struct USB_Command *cmd;

	MYINFO( "PTP-USB : _usb_Bulk_Cmd_Get" );

	cmd = (PTR) RemHead( & us->us_USBList );

	if ( ! cmd )
	{
		cmd = AllocVecTags( sizeof( struct USB_Command ),
			AVT_Clear, 0,
			TAG_END
		);
	}
	else
	{
		us->us_CmdListCnt--;
	}

	return( cmd );
}
