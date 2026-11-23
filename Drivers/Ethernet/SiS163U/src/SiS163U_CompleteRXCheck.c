
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_RX_TRAILER_SIZE 12
#define SIS163U_RX_MAX_TRANSFER 1600
#define SIS163U_RX_LENGTH_MASK 0x0fff
#define SIS163U_RX_FRAME_OK 0x08000000
#define IEEE80211_SCAN_FRAME_MIN_SIZE 36
#define IEEE80211_BEACON 0x80
#define IEEE80211_PROBE_RESPONSE 0x50

static U32
ReadMgmtLength( const U8 *data, U32 actual )
{
	const U8 *trailer;

	U32 payload;
	U32 status;
	U32 length;

	if ( ( actual < SIS163U_WLAN_HEADER + SIS163U_RX_TRAILER_SIZE ) ||
		( actual >= SIS163U_RX_MAX_TRANSFER ) )
	{
		return ( 0 );
	}

	payload = actual - SIS163U_RX_TRAILER_SIZE;
	trailer = data + payload;
	status = (U32) trailer[0] | ( (U32) trailer[1] << 8 ) |
		( (U32) trailer[2] << 16 ) | ( (U32) trailer[3] << 24 );
	length = status & SIS163U_RX_LENGTH_MASK;

	// The frame starts at byte zero. Up to three padding bytes precede the trailer.
	if ( ( length < SIS163U_WLAN_HEADER ) || ( length > payload ) ||
		( payload - length > 3 ) || ( ! ( status & SIS163U_RX_FRAME_OK ) ) ||
		( data[0] & 0x0f ) )
	{
		return ( 0 );
	}

	return ( length );
}

// --

static void
AdvanceRXCheck( struct EthernetUnit *unit )
{
	U32 channel;
	S32 error;

	channel = unit->unit_Channel;

	if ( channel == SIS163U_CHANNEL_MAX )
	{
		USBINFO( "SiS163U: passive receive check finished without a beacon or probe response" );

		error = SiS163U_SetChannel( unit, 1 );

		if ( error )
		{
			USBERROR( "SiS163U: could not restore channel 1, USB error %ld", error );
		}

		return;
	}

	error = SiS163U_SetChannel( unit, channel + 1 );

	if ( error )
	{
		USBERROR( "SiS163U: passive receive check could not tune channel %lu, USB error %ld", channel + 1, error );

		return;
	}

	unit->unit_RxCheckReads = 0;

	SiS163U_SubmitRXCheck( unit );
}

// --

static void
CompleteMgmtRead( struct EthernetUnit *unit, struct USB2_IORequest *usbio )
{
	struct IOSana2Req *ioreq;

	U32 length;
	S32 error;

	error = usbio->io_Error;
	length = 0;

	if ( usbio->io_Actual > unit->unit_Bulk_Rx_Resource->BufferSize )
	{
		error = USB2Err_Host_Overflow;
	}
	else if ( ( ! error ) && ( usbio->io_Actual ) )
	{
		if ( ! unit->unit_RxCheckReads++ )
		{
			USBINFO( "SiS163U: first scan bulk IN on channel %lu, %lu bytes",
				(U32) unit->unit_Channel, usbio->io_Actual );
		}

		length = ReadMgmtLength( usbio->io_Data, usbio->io_Actual );
	}

	if ( ( error == USB2Err_Host_Timeout ) && ( ! usbio->io_Actual ) )
	{
		error = 0;
	}

	ObtainSemaphore( & unit->unit_CommandLock );

	ioreq = SiS163U_FindMgmtRead( unit );

	if ( ioreq )
	{
		if ( ( error ) || ( length ) )
		{
			Remove( (PTR) ioreq );

			if ( error )
			{
				ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
				ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;
			}
			else if ( length > ioreq->ios2_DataLength )
			{
				ioreq->ios2_Req.io_Error = IOERR_BADLENGTH;
			}
			else
			{
				memcpy( ioreq->ios2_Data, usbio->io_Data, length );

				ioreq->ios2_DataLength = length;
			}

			ReplyMsg( (PTR) ioreq );

			ioreq = SiS163U_FindMgmtRead( unit );
		}

		if ( ioreq )
		{
			SiS163U_SubmitRXCheck( unit );
		}
	}

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --

void
SiS163U_CompleteRXCheck( struct EthernetUnit *unit )
{
	struct USB2_EPResource *resource;
	struct USB2_IORequest *usbio;

	const U8 *frame;
	U32 length;

	resource = unit->unit_Bulk_Rx_Resource;

	usbio = (PTR) GetMsg( resource->MsgPort );

	if ( ! usbio )
	{
		return;
	}

	unit->unit_RxActive = FALSE;

	if ( unit->unit_ManualScan )
	{
		CompleteMgmtRead( unit, usbio );

		return;
	}

	if ( ( usbio->io_Error == USB2Err_Host_Timeout ) && ( ! usbio->io_Actual ) )
	{
		USBINFO( "SiS163U: channel %lu receive timed out after %lu completed reads",
			(U32) unit->unit_Channel, unit->unit_RxCheckReads );

		if ( unit->unit_Channel == 1 )
		{
			SiS163U_LogRXState( unit, "after channel 1 timeout" );
		}

		AdvanceRXCheck( unit );

		return;
	}

	if ( ( usbio->io_Error ) || ( usbio->io_Actual > resource->BufferSize ) )
	{
		USBINFO( "SiS163U: receive check ended, USB error %ld, %lu bytes after %lu completed reads",
			(S32) usbio->io_Error, usbio->io_Actual, unit->unit_RxCheckReads );

		return;
	}

	unit->unit_RxCheckReads++;

	if ( unit->unit_RxCheckReads == 1 )
	{
		USBINFO( "SiS163U: first bulk IN transfer, %lu bytes", usbio->io_Actual );
	}

	frame = usbio->io_Data;

	length = ReadMgmtLength( frame, usbio->io_Actual );

	if ( ( length >= IEEE80211_SCAN_FRAME_MIN_SIZE ) &&
		( ( frame[0] == IEEE80211_BEACON ) || ( frame[0] == IEEE80211_PROBE_RESPONSE ) ) )
	{
		USBINFO( "SiS163U: received a scan frame on channel %lu, %lu bytes", (U32) unit->unit_Channel, length );

		return;
	}

	if ( unit->unit_RxCheckReads == SIS163U_RX_CHECK_READS )
	{
		USBINFO( "SiS163U: channel %lu had no scan frames in %lu reads",
			(U32) unit->unit_Channel, unit->unit_RxCheckReads );

		AdvanceRXCheck( unit );

		return;
	}

	SiS163U_SubmitRXCheck( unit );
}

// --
