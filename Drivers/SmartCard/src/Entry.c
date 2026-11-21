
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/SmartCard.h"
#include <dos/dos.h>

// --

static void
Run( struct SmartCardReader *reader )
{
	struct USB2_NotifyMessage *notice;
	struct SmartCardCommand *command;

	U32 commandSignal;
	U32 signals;
	S32 running;

	commandSignal = 1UL << reader->Commands->mp_SigBit;
	running = TRUE;

	while( running )
	{
		signals = IExec->Wait( commandSignal | reader->Registration->Stack_MsgPortBit | SIGBREAKF_CTRL_C );

		if ( signals & reader->Registration->Stack_MsgPortBit )
		{
			while( ( notice = (PTR) IExec->GetMsg( reader->Registration->Stack_MsgPort ) ) )
			{
				if ( notice->nm_Command == USBNC_Detached )
				{
					running = FALSE;
				}

				IExec->ReplyMsg( (PTR) notice );
			}
		}

		if ( signals & SIGBREAKF_CTRL_C )
		{
			running = FALSE;
		}

		if ( running )
		{
			// Take one command per loop so detach is serviced between requests.
			command = (PTR) IExec->GetMsg( reader->Commands );

			if ( command )
			{
				command->Result = SmartCard_Exchange( reader, command );

				IExec->ReplyMsg( (PTR) command );

				IExec->SetSignal( commandSignal, commandSignal );
			}
		}
	}
}

// --

U32
SmartCard_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg )
{
	struct SmartCardReader *reader;
	struct SmartCardDriver *base;
	struct SmartCardCommand *command;
	struct USB2IFace *usb;

	U32 interface;
	U32 unit;
	U32 success;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) ||
		( ! msg->InterfaceDescriptor ) || ( msg->InterfaceDescriptor->InterfaceClass != USBCLASS_Smart_Card ) ||
		( msg->InterfaceDescriptor->InterfaceSubClass != 0 ) || ( msg->InterfaceDescriptor->InterfaceProtocol != 0 ) )
	{
		return ( FALSE );
	}

	reader = IExec->AllocVecTags( sizeof( *reader ), AVT_Clear, 0, TAG_END );

	if ( ! reader )
	{
		return ( FALSE );
	}

	base = (PTR) Self->Data.LibBase;
	usb = msg->IUSB2;
	interface = msg->InterfaceDescriptor->InterfaceNumber;
	success = FALSE;

	if ( ! SmartCard_Parse( msg, & reader->Info ) )
	{
		goto bailout;
	}

	reader->Commands = IExec->AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! reader->Commands )
	{
		goto bailout;
	}

	reader->Buffer = IExec->AllocVecTags( reader->Info.MaxMessage, AVT_Type, MEMF_SHARED, TAG_END );

	if ( ! reader->Buffer )
	{
		goto bailout;
	}

	reader->Registration = usb->USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, "USB Smart Card",
		TAG_END );

	if ( ! reader->Registration )
	{
		goto bailout;
	}

	reader->In = usb->USB2_EPRes_ObtainTags( reader->Registration,
		USB2Tag_EPRes_InterfaceNr, interface,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END );

	reader->Out = usb->USB2_EPRes_ObtainTags( reader->Registration,
		USB2Tag_EPRes_InterfaceNr, interface,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		TAG_END );

	if ( ( ! reader->In ) || ( ! reader->Out ) )
	{
		goto bailout;
	}

	IExec->ObtainSemaphore( & base->Lock );

	for( unit = 0; unit < USB2SMARTCARD_UNITS; unit++ )
	{
		if ( ! base->Readers[unit] )
		{
			base->Readers[unit] = reader;
			break;
		}
	}

	IExec->ReleaseSemaphore( & base->Lock );

	if ( unit == USB2SMARTCARD_UNITS )
	{
		goto bailout;
	}

	Run( reader );

	IExec->ObtainSemaphore( & base->Lock );

	base->Readers[unit] = NULL;

	IExec->ReleaseSemaphore( & base->Lock );

	while( ( command = (PTR) IExec->GetMsg( reader->Commands ) ) )
	{
		command->Result = USB2SC_NoReader;

		IExec->ReplyMsg( (PTR) command );
	}

	success = TRUE;

bailout:
	if ( reader->Registration )
	{
		usb->USB2_Reg_Unregister( reader->Registration );
	}

	if ( reader->Buffer )
	{
		IExec->FreeVec( reader->Buffer );
	}

	if ( reader->Commands )
	{
		IExec->FreeSysObject( ASOT_PORT, reader->Commands );
	}

	IExec->FreeVec( reader );

	return ( success );
}

// --
