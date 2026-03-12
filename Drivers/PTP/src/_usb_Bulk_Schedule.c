
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _usb_Bulk_Schedule( struct USB_Struct *us )
{
struct USB_Command *cmd;

	MYINFO( "PTP-USB : _usb_Bulk_Schedule" );

	if ( us->us_USBActive )
	{
		// We already have an active cmd
		goto bailout;
	}

	cmd = (PTR) RemHead( & us->us_USBList );

	if ( ! cmd )
	{
		// Nothing to be done
		goto bailout;
	}

	cmd->ucmd_State = CMDSTAGE_Send_Header1;
	us->us_USBActive = cmd;

	_usb_Bulk_Stage_Send_Header1( us, cmd );

bailout:

	return;
}

// --
