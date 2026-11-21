
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
BTUSB_Close( struct BTUSB *usb )
{
	struct Message *message;

	BTUSB_CancelEvent( usb );

	if ( usb->Events )
	{
		usb->USB2->USB2_EPRes_Release( usb->Events );
	}

	if ( usb->Registration )
	{
		while(( message = GetMsg( usb->Registration->Stack_MsgPort ) ))
		{
			ReplyMsg( message );
		}

		usb->USB2->USB2_Reg_Unregister( usb->Registration );
	}
}

// --
