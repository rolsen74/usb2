
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "EHCI.h"

// --

SEC_CODE U32
EHCI_Isochronous_Length ( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct USBBase *usbbase;
	struct EHCI_ITD *itd;

	U32 word;
	U32 offset;
	U32 total;
	U32 index;
	U32 actual;

	usbbase = hn->hn_USBBase;
	offset = 0;
	total = 0;
	index = 0;

	for ( itd = req->req_HCD.EHCI.IsoHead; itd; itd = itd->Next )
	{
		word = LE_SWAP32( itd->Transaction[itd->Microframe] );
		actual = 0;

		if ( ! ( word & ( EHCI_ITD_ACTIVE | EHCI_ITD_ERRORS ) ) )
		{
			actual = itd->Length;

			if ( req->req_Public.io_Command == CMD_READ )
			{
				actual = MIN( actual, ( word >> 16 ) & 0xfff );

				if ( actual )
				{
					MEM_COPY( (U8 *) itd + EHCI_ITD_DATA, (U8 *) req->req_Public.io_Data + offset, actual );
				}
			}
		}

		if ( req->req_Public.io_IsoPacketLengths )
		{
			req->req_Public.io_IsoPacketLengths[index++] =
				( word & ( EHCI_ITD_ACTIVE | EHCI_ITD_ERRORS ) ) ? USB2_ISO_PACKET_ERROR : actual;
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
