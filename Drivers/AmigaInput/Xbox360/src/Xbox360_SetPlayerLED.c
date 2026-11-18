
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Xbox360.h"

// --

/*
 * FreeBSD xb360gp LED request, copyright 2020 Vladimir Kondratyev and Val Packett.
 * See LICENSE.FreeBSD for the BSD-2-Clause terms.
 */

// --

void
Xbox360_SetPlayerLED( struct USB2IFace *usb, struct USB2_Register *reg, U8 interface )
{
	struct USB2_EPResource *output;
	struct USB2_IORequest *ioreq;
	struct USB2_SetupData *setup;

	U8 *packet;
	S32 error;

	output = usb->USB2_EPRes_ObtainTags( reg,
		USB2Tag_EPRes_InterfaceNr, interface,
		USB2Tag_EPRes_EPType, EPATT_Type_Interrupt,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		USB2Tag_EPRes_NrOfIORequest, 1,
		USB2Tag_EPRes_BufferSize, 3,
		TAG_END );

	if ( ! output )
	{
		// FreeBSD also supports sending the LED report as a control SET_REPORT.
		output = usb->USB2_EPRes_ObtainTags( reg,
			USB2Tag_EPRes_EPType, EPATT_Type_Control,
			USB2Tag_EPRes_NrOfIORequest, 1,
			USB2Tag_EPRes_BufferSize, 3,
			TAG_END );
	}

	if ( ! output )
	{
		DebugPrintF( "Xbox360: no USB endpoint available for player LED\n" );

		return;
	}

	// A wired Xbox 360 LED report has three bytes. Mode 6 lights the top-left quadrant.
	packet = output->Buffers[0];
	packet[0] = 0x01;
	packet[1] = 0x03;
	packet[2] = 0x06;
	ioreq = output->IORequests[0];
	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Data = packet;
	ioreq->io_Length = 3;
	ioreq->io_TimeOut = 250000;
	setup = output->SetupData;

	if ( setup )
	{
		setup->RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
		setup->RequestCode = 0x09;
		setup->Value = LE_SWAP16( 0x0200 );
		setup->Index = LE_SWAP16( interface );
		setup->Length = LE_SWAP16( 3 );
		ioreq->io_SetupData = setup;
		ioreq->io_SetupLength = sizeof( *setup );
	}

	error = DoIO( (PTR) ioreq );

	if ( ( error ) || ( ioreq->io_Actual != ioreq->io_Length ) )
	{
		DebugPrintF( "Xbox360: player LED write failed (USB error %ld, sent %lu bytes)\n",
			error, ioreq->io_Actual );
	}

	usb->USB2_EPRes_Release( output );
}

// --
