
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
Handle_BulkInMessages( struct devUnit *unit )
{
	struct USB2IFace *IUSB2;
	struct USB2_IORequest *usbio;
	struct IONode *node;

	U8 *buffer;
	U32 offset;
	U32 length;

	IUSB2 = unit->unit_IUSB2;

	while(( usbio = (PTR) IExec->GetMsg( unit->unit_USBBulkIn->MsgPort ) ))
	{
		node = usbio->io_UserData;

		IExec->Remove( & node->Node );

		node->Active = FALSE;

		if ( usbio->io_Error == USB2Err_Device_Detached )
		{
			unit->unit_Running = FALSE;

			Sana2_SignalEvent( & unit->unit_EventsList, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_HARDWARE, TRUE );
		}
		else if ( ( ! node->AbortError ) && ( unit->unit_Online ) &&
			( usbio->io_Error == USB2Err_NoError ) && ( usbio->io_Actual <= node->BufferSize ) )
		{
			buffer = node->Buffer;

			for( offset = 0; usbio->io_Actual - offset >= RTL8152_RX_DESCRIPTOR_SIZE; )
			{
				length = buffer[offset] | ( (U32) buffer[offset + 1] << 8 );
				length &= RTL8152_RX_LENGTH_MASK;
				offset += RTL8152_RX_DESCRIPTOR_SIZE;

				if ( ( length < 18 ) || ( length > ETHERNET_MTU + 4 ) || ( length > usbio->io_Actual - offset ) )
				{
					unit->unit_BadData++;

					Sana2_SignalEvent( & unit->unit_EventsList, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_HARDWARE, TRUE );

					break;
				}

				ReadPacket( unit, buffer + offset, length - 4 );

				offset = ( offset + length + 7 ) & ~7U;

				if ( offset >= usbio->io_Actual )
				{
					break;
				}
			}
		}
		else if ( ( ! node->AbortError ) && ( unit->unit_Online ) )
		{
			Sana2_SignalEvent( & unit->unit_EventsList, S2EVENT_ERROR | S2EVENT_RX | S2EVENT_HARDWARE, TRUE );

			if ( usbio->io_Error == USB2Err_Host_Stall )
			{
				IUSB2->USB2_EPRes_Destall( unit->unit_USBBulkIn );
			}
		}

		node->AbortError = 0;

		IExec->AddTail( & unit->unit_USBBulkInFreeList, & node->Node );
	}

	Schedule_RxIOReq( unit );
}

// --
