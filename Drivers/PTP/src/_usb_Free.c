
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _usb_Free( struct USB_Struct *us )
{
struct USB_Command *cmd;

	MYINFO( "PTP-USB : _usb_Free" );

	if ( us->us_Register )
	{
		USB2_Reg_Unregister( us->us_Register );
		us->us_Register = NULL;
	}

	if ( us->us_USBActive )
	{
		_usb_Bulk_Cmd_Free( us, us->us_USBActive );
		us->us_USBActive = NULL;
	}

	while( TRUE )
	{
		cmd = (PTR) RemHead( & us->us_USBList );

		if ( ! cmd )
		{
			break;
		}

		_usb_Bulk_Cmd_Free( us, cmd );
	}

	while( TRUE )
	{
		cmd = (PTR) RemHead( & us->us_CmdList );

		if ( ! cmd )
		{
			break;
		}

		FreeVec( cmd );
	}

	if ( us->us_CmdMsgPort )
	{
		FreeSysObject( ASOT_PORT, us->us_CmdMsgPort );
		us->us_CmdMsgPort = NULL;
	}
}

// --
