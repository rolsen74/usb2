
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void
_usb_Free( struct USB_Struct *us )
{
	struct CommandMessage request;

	if ( ( us->us_Session ) && ( us->us_Running ) && ( ! us->us_Detached ) )
	{
		memset( & request, 0, sizeof( request ) );

		request.cm_Operation = PTP_Close_Session;

		_usb_Transaction( us, & request );
	}

	// Unregister releases the endpoint resources owned by this registration.
	if ( us->us_Register )
	{
		us->us_StartMsg->IUSB2->USB2_Reg_Unregister( us->us_Register );
	}

	if ( us->us_CmdMsgPort )
	{
		FreeSysObject( ASOT_PORT, us->us_CmdMsgPort );
	}
}

// --
