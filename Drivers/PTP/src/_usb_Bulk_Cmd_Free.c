
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _usb_Bulk_Cmd_Free( struct USB_Struct *us, struct USB_Command *cmd )
{
	MYINFO( "PTP-USB : _usb_Bulk_Cmd_Free" );

	if ( ! cmd )
	{
		goto bailout;
	}

	if ( cmd->ucmd_Data_Buffer )
	{
		FreeVec( cmd->ucmd_Data_Buffer );
		cmd->ucmd_Data_Buffer = NULL;
	}

	if ( us->us_CmdListCnt < 50 )
	{
		AddTail( & us->us_CmdList, & cmd->ucmd_Node );
		us->us_CmdListCnt++;
	}
	else
	{
		FreeVec( cmd );
	}

bailout:

	return;
}
