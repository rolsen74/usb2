
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "AF9015.h"

// --

BOOL
AF9015_Open( struct Receiver *receiver )
{
	struct USB2_Interface ifc;

	U32 speed;

	if ( USB2_Ifc_FindTags( USB2Tag_Find_VendorID, AF9015_VENDOR,
		USB2Tag_Find_DeviceID, AF9015_PRODUCT, USB2Tag_Find_Class, USBCLASS_Vendor_Specific,
		USB2Tag_Find_InterfaceNumber, AF9015_INTERFACE, USB2Tag_Find_Copy, & ifc, TAG_END ) != & ifc )
	{
		Printf( "No AF9015 receiver (15A4:9016) on interface 0.\n" );

		return ( FALSE );
	}

	receiver->Reg = USB2_Reg_RegisterTags( USB2Tag_Reg_InterfaceID, & ifc.IFC_ID,
		USB2Tag_Reg_Title, "AF9015 DVB-T", TAG_END );

	if ( ! receiver->Reg )
	{
		Printf( "Could not claim the AF9015 receiver interface.\n" );

		return ( FALSE );
	}

	if ( ( USB2_Attr_Get( receiver->Reg->FKT_ID, USB2Tag_Attr_Speed, & speed, sizeof( speed ) ) != sizeof( speed ) ) ||
		( speed != USBSPEED_High ) )
	{
		Printf( "AF9015 requires a high-speed USB connection.\n" );

		return ( FALSE );
	}

	receiver->Out = USB2_EPRes_ObtainTags( receiver->Reg, USB2Tag_EPRes_InterfaceNr, AF9015_INTERFACE,
		USB2Tag_EPRes_EndPointNr, 0x02, USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_BufferSize, 64, TAG_END );

	receiver->In = USB2_EPRes_ObtainTags( receiver->Reg, USB2Tag_EPRes_InterfaceNr, AF9015_INTERFACE,
		USB2Tag_EPRes_EndPointNr, 0x81, USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_BufferSize, 64, TAG_END );

	if ( ( ! receiver->Out ) || ( ! receiver->In ) )
	{
		Printf( "Could not obtain AF9015 command endpoints $02 and $81.\n" );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

void
AF9015_Close( struct Receiver *receiver )
{
	if ( receiver->In )
	{
		USB2_EPRes_Release( receiver->In );
	}

	if ( receiver->Out )
	{
		USB2_EPRes_Release( receiver->Out );
	}

	if ( receiver->Reg )
	{
		AF9015_Detached( receiver );

		USB2_Reg_Unregister( receiver->Reg );
	}
}

// --

BOOL
AF9015_Detached( struct Receiver *receiver )
{
	struct USB2_NotifyMessage *message;

	while( ( message = (PTR) GetMsg( receiver->Reg->Stack_MsgPort ) ) )
	{
		if ( message->nm_Command == USBNC_Detached )
		{
			receiver->Detached = TRUE;
		}

		ReplyMsg( (PTR) message );
	}

	return ( receiver->Detached );
}

// --

BOOL
AF9015_Transfer( struct USB2_EPResource *ep, BOOL read, U32 size, BOOL shortOK )
{
	struct USB2_IORequest *io;

	S32 error;

	io = ep->IORequests[0];
	io->io_Command = read ? CMD_READ : CMD_WRITE;
	io->io_Data = ep->Buffers[0];
	io->io_Length = size;
	io->io_TimeOut = 2000000;
	io->io_AllowShortPackets = shortOK;
	io->io_Actual = 0;

	error = DoIO( (PTR) io );

	if ( error == USB2Err_Host_Stall )
	{
		USB2_EPRes_Destall( ep );
	}

	if ( ( error ) || ( io->io_Actual > size ) || ( ( ! shortOK ) && ( io->io_Actual != size ) ) )
	{
		Printf( "AF9015 bulk %s failed (%ld, %lu/%lu bytes).\n",
			read ? "IN" : "OUT", error, io->io_Actual, size );

		return ( FALSE );
	}

	return ( TRUE );
}

// --
