
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

static struct SmartCardCommand *
Run( struct SmartCardReader *reader )
{
	struct SmartCardCommand *command;

	U32 signals;
	U32 mask;

	mask = ( 1UL << reader->Commands->mp_SigBit ) |
		reader->Registration->Stack_MsgPortBit | SIGBREAKF_CTRL_C;
	signals = IExec->SetSignal( 0, SIGBREAKF_CTRL_C );

	for( ;; )
	{
		SmartCard_Notice( reader, signals );

		if ( reader->Stopped )
		{
			return ( NULL );
		}

		command = (PTR) IExec->GetMsg( reader->Commands );

		if ( ! command )
		{
			signals = IExec->Wait( mask );

			continue;
		}

		command->Result = SmartCard_Exchange( reader, command );

		if ( ( reader->Stopped ) || ( reader->Faulted ) )
		{
			return ( command );
		}

		IExec->ReplyMsg( (PTR) command );

		signals = IExec->SetSignal( 0, SIGBREAKF_CTRL_C );
	}
}

// --

static BOOL
OpenClock( struct SmartCardReader *reader )
{
	reader->ClockRequest = IExec->AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( *reader->ClockRequest ), ASOIOR_ReplyPort, reader->Commands, TAG_END );

	if ( ! reader->ClockRequest )
	{
		return ( FALSE );
	}

	if ( IExec->OpenDevice( "timer.device", UNIT_MICROHZ, (PTR)reader->ClockRequest, 0 ) )
	{
		reader->ClockRequest->Request.io_Device = NULL;

		return ( FALSE );
	}

	reader->Clock = (PTR) IExec->GetInterface( (PTR)reader->ClockRequest->Request.io_Device, "main", 1, NULL );

	return ( reader->Clock != NULL );
}

// --

static void
CloseClock( struct SmartCardReader *reader )
{
	if ( reader->Clock )
	{
		IExec->DropInterface( (PTR)reader->Clock );
	}

	if ( reader->ClockRequest )
	{
		if ( reader->ClockRequest->Request.io_Device )
		{
			IExec->CloseDevice( (PTR)reader->ClockRequest );
		}

		IExec->FreeSysObject( ASOT_IOREQUEST, reader->ClockRequest );
	}
}

// --

U32
SmartCard_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg )
{
	struct SmartCardReader *reader;
	struct SmartCardDriver *base;
	struct SmartCardCommand *command;
	struct SmartCardCommand *active;
	struct USB2_EPResource *control;
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

	reader = IExec->AllocVecTags( sizeof( *reader ), AVT_Type, MEMF_SHARED, AVT_Clear, 0, TAG_END );

	if ( ! reader )
	{
		return ( FALSE );
	}

	base = (PTR) Self->Data.LibBase;
	usb = msg->IUSB2;
	interface = msg->InterfaceDescriptor->InterfaceNumber;
	reader->USB = usb;
	reader->Interface = interface;
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

	if ( ! OpenClock( reader ) )
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

	control = reader->Registration->Res_Control;

	if ( ( ! control ) || ( ! control->SetupData ) || ( ! control->IORequests ) || ( ! control->IORequests[0] ) )
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

	if ( ( ! reader->In ) || ( ! reader->Out ) ||
		( reader->Info.MaxMessage < reader->Out->EndPointMax ) ||
		( ( reader->Info.Features & USB2SMARTCARD_EXTENDED_APDU ) &&
		  ( reader->Info.MaxMessage < CCID_HEADER + USB2SMARTCARD_APDU_MAX ) ) )
	{
		goto bailout;
	}

	// One extra byte lets a packet-aligned maximum-size reply consume its ZLP.
	reader->ReceiveSize = reader->Info.MaxMessage + 1;

	reader->Buffer = IExec->AllocVecTags( reader->ReceiveSize, AVT_Type, MEMF_SHARED, TAG_END );

	if ( ! reader->Buffer )
	{
		goto bailout;
	}

	IExec->ObtainSemaphore( & base->Lock );

	for( unit = 0; unit < USB2SMARTCARD_UNITS; unit++ )
	{
		if ( ! base->Readers[unit] )
		{
			if ( ! ++base->NextInstance )
			{
				base->NextInstance++;
			}

			reader->Instance = base->NextInstance;
			base->Readers[unit] = reader;
			break;
		}
	}

	IExec->ReleaseSemaphore( & base->Lock );

	if ( unit == USB2SMARTCARD_UNITS )
	{
		goto bailout;
	}

	active = Run( reader );

	IExec->ObtainSemaphore( & base->Lock );

	base->Readers[unit] = NULL;

	IExec->ReleaseSemaphore( & base->Lock );

	if ( active )
	{
		IExec->ReplyMsg( (PTR)active );
	}

	while(( command = (PTR) IExec->GetMsg( reader->Commands ) ))
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

	CloseClock( reader );

	if ( reader->Commands )
	{
		IExec->FreeSysObject( ASOT_PORT, reader->Commands );
	}

	IExec->FreeVec( reader );

	return ( success );
}

// --
