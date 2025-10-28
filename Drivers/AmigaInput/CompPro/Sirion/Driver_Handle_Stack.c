
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
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
