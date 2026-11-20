
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct IORequest *USB2IOReq;
struct MsgPort *USB2MsgPort;
struct Device *USB2Base;
struct USB2IFace *IUSB2;

// --

S32
Unit_Startup_USB( struct EthernetUnit *unit UNUSED )
{
	S32 error;

	if ( IUSB2 )
	{
		return ( TRUE );
	}

	if ( ! USB2MsgPort )
	{
		USB2MsgPort = AllocSysObjectTags( ASOT_PORT, ASOPORT_AllocSig, FALSE, TAG_END );

		if ( ! USB2MsgPort )
		{
			return ( FALSE );
		}
	}

	if ( ! USB2IOReq )
	{
		USB2IOReq = AllocSysObjectTags( ASOT_IOREQUEST,
			ASOIOR_Size, sizeof( *USB2IOReq ), ASOIOR_ReplyPort, USB2MsgPort, TAG_END );

		if ( ! USB2IOReq )
		{
			return ( FALSE );
		}
	}

	if ( ! USB2Base )
	{
		error = OpenDevice( "usb2.device", 0, USB2IOReq, 0 );

		if ( error )
		{
			return ( FALSE );
		}

		USB2Base = (PTR) USB2IOReq->io_Device;
	}

	IUSB2 = (PTR) GetInterface( (PTR) USB2Base, "main", 1, NULL );

	return ( IUSB2 != NULL );
}

// --
