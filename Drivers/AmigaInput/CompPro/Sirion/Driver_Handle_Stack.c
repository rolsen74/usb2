
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

void Driver_Handle_Stack( struct USB_Driver *in )
{
struct USBNotifyMsg *msg;

	while( TRUE )
	{
		msg = (PTR) GetMsg( in->USB_Function_MsgPort );

		if ( ! msg )
		{
			break;
		}

		if ( msg->Type == USBNM_TYPE_FUNCTIONDETACH )
		{
			DebugPrintF( "Driver Detach\n" );
			in->Running = FALSE;
		}

		ReplyMsg( (PTR) msg );
	}
}
