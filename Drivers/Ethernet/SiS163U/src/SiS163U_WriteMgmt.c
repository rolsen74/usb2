
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_TX_SINGLE_FRAME 0x80002000U
#define SIS163U_TX_PROBE_REQUEST 0x10000000U
#define SIS163U_TX_PROBE_RESPONSE 0x08000000U
#define SIS163U_TX_RATE_1MBPS ( 2U << 16 )
#define SIS163U_TX_ACK_DURATION ( 10 + 192 + 14 * 8 )

// --

void
SiS163U_WriteMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	struct USB2_EPResource *bulk;
	struct USB2_IORequest *usbio;

	const U8 *frame;
	U8 *buffer;
	U32 length;
	U32 descriptor;
	U32 word;
	S32 error;

	bulk = unit->unit_Bulk_Tx_Resource;
	usbio = bulk->IORequests[0];
	frame = ioreq->ios2_Data;
	buffer = bulk->Buffers[0];
	length = ioreq->ios2_DataLength;
	descriptor = SIS163U_TX_SINGLE_FRAME | SIS163U_TX_RATE_1MBPS | length;

	if ( frame[0] == 0x40 )
	{
		descriptor |= SIS163U_TX_PROBE_REQUEST;
	}
	else if ( frame[0] == 0x50 )
	{
		descriptor |= SIS163U_TX_PROBE_RESPONSE;
	}

	// The USB record starts with three little-endian words. The MAC appends the FCS.
	word = LE_SWAP32( descriptor );

	memcpy( buffer, & word, sizeof( word ) );

	word = LE_SWAP32( length );

	memcpy( buffer + 4, & word, sizeof( word ) );

	memset( buffer + 8, 0, 4 );

	memcpy( buffer + SIS163U_TX_DESC_SIZE, frame, length );

	// Unicast reserves SIFS and an ACK at the long-preamble 1 Mbps rate.
	if ( ! ( frame[4] & 1 ) )
	{
		buffer[SIS163U_TX_DESC_SIZE + 2] = SIS163U_TX_ACK_DURATION & 0xff;
		buffer[SIS163U_TX_DESC_SIZE + 3] = SIS163U_TX_ACK_DURATION >> 8;
	}

	usbio->io_Command = CMD_WRITE;
	usbio->io_Length = SIS163U_TX_DESC_SIZE + length;
	usbio->io_Data = buffer;
	usbio->io_TimeOut = 500000;

	error = DoIO( (PTR) usbio );

	if ( error == USB2Err_Device_Detached )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_DISCONNECTED;
	}
	else if ( ( error ) || ( usbio->io_Actual != usbio->io_Length ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;

		if ( error == USB2Err_Host_Stall )
		{
			USB2_EPRes_Destall( bulk );
		}

		Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_TX | S2EVENT_HARDWARE );
	}
}

// --
