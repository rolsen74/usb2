
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
	U32 shortCount;
	U32 firstShort;
	U32 firstStatus;
	U32 firstSchedule;
	U32 firstCount;
	U32 firstEndpoint;

	usbbase = hn->hn_USBBase;
	offset = 0;
	total = 0;
	index = 0;
	split = req->req_Function->fkt_Speed == USBSPEED_Full;
	in = req->req_Public.io_Command == CMD_READ;
	shortCount = 0;
	firstShort = 0;
	firstStatus = 0;
	firstSchedule = 0;
	firstCount = 0;
	firstEndpoint = 0;

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

			if ( ( ! in ) && ( actual < itd->Length ) && ( hn->hn_HCD.EHCI.IsoOutReports < 4 ) )
			{
				if ( ! shortCount )
				{
					firstShort = index;
					firstStatus = word;
					firstSchedule = LE_SWAP32( itd->Split.Schedule );
					firstCount = LE_SWAP32( itd->Split.Buffer[1] ) & 7;
					firstEndpoint = LE_SWAP32( itd->Split.Endpoint );
				}

				shortCount++;
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
			req->req_Public.io_IsoPacketLengths[index] = failed ? USB2_ISO_PACKET_ERROR : actual;
			offset += itd->Length;
		}
		else
		{
			offset += actual;
		}

		total += actual;
		index++;
	}

	if ( shortCount )
	{
		hn->hn_HCD.EHCI.IsoOutReports++;

		usbbase->usb_IExec->DebugPrintF(
			"EHCI split OUT: addr %lu ep %lu via hub %lu port %lu, %lu/%lu short, first index %lu, status $%08lx, mask $%02lx, remaining splits %lu\n",
			req->req_Function->fkt_Address, req->req_EndPoint->ep_Number,
			( firstEndpoint >> 16 ) & 0x7f, ( firstEndpoint >> 24 ) & 0x7f, shortCount, index,
			firstShort, firstStatus, firstSchedule, firstCount );
	}

	return ( total );
}

// --
