
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
Handle_BulkOutMessages( struct devUnit *unit )
{
	struct USB2IFace *IUSB2;
	struct USB2_IORequest *usbio;
	struct IONode *node;
	struct IOSana2Req *request;

	U8 *frame;
	U32 type;

	IUSB2 = unit->unit_IUSB2;

	while(( usbio = (PTR) IExec->GetMsg( unit->unit_USBBulkOut->MsgPort ) ))
	{
		node = usbio->io_UserData;

		IExec->Remove( & node->Node );

		request = node->Sana2IOReq;
		node->Sana2IOReq = NULL;
		node->Active = FALSE;

		if ( usbio->io_Error == USB2Err_Device_Detached )
		{
			unit->unit_Running = FALSE;
		}

		if ( node->AbortError )
		{
			request->ios2_Req.io_Error = node->AbortError;
			request->ios2_WireError = node->AbortWire ? node->AbortWire : S2WERR_GENERIC_ERROR;
			node->AbortError = 0;
			node->AbortWire = 0;
		}
		else if ( ! unit->unit_LinkUp )
		{
			request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
			request->ios2_WireError = S2WERR_UNIT_DISCONNECTED;
		}
		else if ( ( usbio->io_Error == USB2Err_NoError ) && ( usbio->io_Actual == usbio->io_Length ) )
		{
			unit->unit_PacketsSent++;
			frame = node->Buffer + WIRED_TX_HEADER_SIZE;
			type = ( (U32) frame[12] << 8 ) | frame[13];

			Sana2_AccountType( & unit->unit_TrackedTypes, type,
				node->FrameLength - 14, FALSE, FALSE );

			UpdateThroughput( unit, node->FrameLength, 0 );
		}
		else
		{
			SignalSanaEvent( unit, S2EVENT_ERROR | S2EVENT_TX | S2EVENT_HARDWARE );

			request->ios2_Req.io_Error = S2ERR_TX_FAILURE;
			request->ios2_WireError = S2WERR_GENERIC_ERROR;

			if ( ( usbio->io_Error == USB2Err_Host_Stall ) && ( ! unit->unit_TxHalted ) )
			{
				unit->unit_TxHalted = ! IUSB2->USB2_EPRes_Destall( unit->unit_USBBulkOut );

				if ( unit->unit_TxHalted )
				{
					Sana2_AbortPendingWithReply( & unit->unit_TxList, NULL, NULL,
						S2ERR_TX_FAILURE, S2WERR_GENERIC_ERROR, ReplySanaRequest, unit );
				}
			}
		}

		ReplySanaRequest( unit, (PTR) request );

		IExec->AddTail( & unit->unit_USBBulkOutFreeList, & node->Node );
	}

	Schedule_TxIOReq( unit );
}

// --
