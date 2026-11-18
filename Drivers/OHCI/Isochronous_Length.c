
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "OHCI.h"

// --

SEC_CODE U32
OHCI_Isochronous_Length ( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct USBBase *usbbase;
	struct OHCI_ITD *itd;

	U32 offset;
	U32 total;
	U32 index;
	U32 status;
	U32 actual;
	U32 code;
	S32 in;
	S32 okay;

	usbbase = hn->hn_USBBase;
	offset = 0;
	total = 0;
	index = 0;
	in = req->req_Public.io_Command == CMD_READ;

	for ( itd = req->req_HCD.OHCI.IsoHead; ( itd ) && ( itd->Next ); itd = itd->Next )
	{
		status = LE_SWAP16( itd->itd_offset[0] );
		code = status >> 12;
		okay = ( code == OHCI_CC_NO_ERROR ) ||
			( in && req->req_Public.io_AllowShortPackets && ( code == OHCI_CC_DATA_UNDERRUN ) );
		actual = 0;

		if ( okay )
		{
			actual = itd->Length;

			if ( in )
			{
				actual = MIN( actual, status & 0xfff );

				if ( actual )
				{
					MEM_COPY( (U8 *) itd + OHCI_ITD_DATA, (U8 *) req->req_Public.io_Data + offset, actual );
				}
			}
		}

		if ( req->req_Public.io_IsoPacketLengths )
		{
			req->req_Public.io_IsoPacketLengths[index++] = okay ? actual : USB2_ISO_PACKET_ERROR;
			offset += itd->Length;
		}
		else
		{
			offset += actual;
		}

		total += actual;
	}

	return ( total );
}

// --
