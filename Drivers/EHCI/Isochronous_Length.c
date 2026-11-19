
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
	U32 residual;
	U32 split;
	U32 in;
	U32 failed;

	usbbase = hn->hn_USBBase;
	offset = 0;
	total = 0;
	index = 0;
	split = req->req_Function->fkt_Speed == USBSPEED_Full;
	in = req->req_Public.io_Command == CMD_READ;

	for ( itd = req->req_HCD.EHCI.IsoHead; itd; itd = itd->Next )
	{
		actual = 0;

		if ( split )
		{
			word = LE_SWAP32( itd->Split.Status );
			residual = ( word >> 16 ) & 0x3ff;
			failed = ( word & ( EHCI_SITD_ACTIVE | EHCI_SITD_ERRORS ) ) || ( residual > itd->Length );

			if ( ! failed )
			{
				actual = itd->Length - residual;
			}
		}
		else
		{
			word = LE_SWAP32( itd->Transaction[itd->Microframe] );
			failed = word & ( EHCI_ITD_ACTIVE | EHCI_ITD_ERRORS );

			if ( ! failed )
			{
				actual = in ? MIN( itd->Length, ( word >> 16 ) & 0xfff ) : itd->Length;
			}
		}

		if ( ( in ) && ( actual ) )
		{
			MEM_COPY( (U8 *) itd + EHCI_ITD_DATA, (U8 *) req->req_Public.io_Data + offset, actual );
		}

		if ( req->req_Public.io_IsoPacketLengths )
		{
			req->req_Public.io_IsoPacketLengths[index++] = failed ? USB2_ISO_PACKET_ERROR : actual;
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
