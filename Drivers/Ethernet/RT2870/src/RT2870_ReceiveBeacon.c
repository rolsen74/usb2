
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"

// --

#define RT2870_RXINFO_SIZE	4
#define RT2870_RXWI_SIZE	16
#define RT2870_RXD_SIZE		4
#define RT2870_RXD_CRC_ERROR	0x01	// Bit 8 in the little-endian RX descriptor
#define RT2870_RXD_DECRYPTED	0x01	// Bit 16 in the little-endian RX descriptor
#define RT2870_RXD_L2_PADDING	0x40	// Bit 14 in the little-endian RX descriptor
#define IEEE80211_BEACON	0x80
#define IEEE80211_PROBE_REPLY		0x50
#define IEEE80211_AUTH_REPLY	0xb0
#define IEEE80211_ASSOC_REPLY	0x10
#define IEEE80211_REPLY_MIN	30
#define IEEE80211_DEST_OFFSET	4
#define IEEE80211_QOS_DATA	0x88
#define IEEE80211_BEACON_MIN	36
#define IEEE80211_HEADER_SIZE	24
#define IEEE80211_BSSID_OFFSET	16

// --

void
RT2870_ReceiveBeacon( struct EthernetUnit *unit, U8 *data, U32 length )
{
	struct IOSana2Req *request;

	U8 *rxwi;
	U8 *frame;
	const U8 *rxd;
	const U8 *bssid;
	U32 dmaLength;
	U32 frameLength;
	U32 recordLength;
	U32 padding;

	while( length >= RT2870_RXINFO_SIZE + RT2870_RXWI_SIZE + RT2870_RXD_SIZE )
	{
		dmaLength = data[0] | ( (U32) data[1] << 8 );
		recordLength = RT2870_RXINFO_SIZE + dmaLength + RT2870_RXD_SIZE;

		if ( ( dmaLength < RT2870_RXWI_SIZE ) || ( dmaLength & 3 ) || ( recordLength > length ) )
		{
			break;
		}

		rxwi = data + RT2870_RXINFO_SIZE;
		frame = rxwi + RT2870_RXWI_SIZE;
		rxd = rxwi + dmaLength;
		frameLength = rxwi[2] | ( (U32) ( rxwi[3] & 0x0f ) << 8 );
		// Management headers are already 4-byte aligned. QoS data adds two bytes.
		padding = ( ( frameLength >= IEEE80211_HEADER_SIZE ) && ( frame[0] == IEEE80211_QOS_DATA ) &&
			( rxd[1] & RT2870_RXD_L2_PADDING ) ) ? 2 : 0;

		if ( ( frameLength >= IEEE80211_HEADER_SIZE ) &&
			( frameLength + padding <= dmaLength - RT2870_RXWI_SIZE ) &&
			( ! ( rxd[1] & RT2870_RXD_CRC_ERROR ) ) )
		{
			if ( ( frame[0] == 0xc0 ) || ( frame[0] == 0xa0 ) || ( frame[0] == 0xd0 ) )
			{
				if ( RT2870_ReceiveRobustMgmt( unit, frame, frameLength, rxd[2] & RT2870_RXD_DECRYPTED ) )
				{
					return;
				}
			}
			else if ( frame[1] & 0x40 )
			{
				if ( ( unit->unit_TKIP ) && ( unit->unit_M4Sent ) && ( ! unit->unit_Operational ) &&
					( unit->unit_TKIPRxReports < 4 ) &&
					( ! memcmp( frame + 4, unit->unit_StationAddress, 6 ) ) &&
					( ! memcmp( frame + 10, unit->unit_Peer, 6 ) ) )
				{
					unit->unit_TKIPRxReports++;

					USBINFO( "RT2870: WPA protected RX, frame $%02lx%02lx, length %lu, padding %lu, RX flags $%02lx%02lx",
						(U32) frame[0], (U32) frame[1], frameLength, padding, (U32) rxd[2], (U32) rxd[1] );
				}

				if ( ! ( rxd[2] & RT2870_RXD_DECRYPTED ) )
				{
					RT2870_ReceiveData( unit, frame, frameLength, padding );

					if ( ! unit->unit_BeaconResource )
					{
						return;
					}
				}
			}
			else
			{
				RT2870_CacheEAPOL( unit, frame, frameLength, padding );
			}
		}

		if ( ( frameLength >= IEEE80211_REPLY_MIN ) &&
			( frameLength + padding <= dmaLength - RT2870_RXWI_SIZE ) &&
			( ! ( rxd[1] & RT2870_RXD_CRC_ERROR ) ) &&
			( ( ( frame[0] == IEEE80211_BEACON ) && ( frameLength >= IEEE80211_BEACON_MIN ) ) ||
			( ( ( frame[0] == IEEE80211_AUTH_REPLY ) || ( frame[0] == IEEE80211_ASSOC_REPLY ) ||
				( frame[0] == IEEE80211_PROBE_REPLY && frameLength >= IEEE80211_BEACON_MIN ) ) &&
			( ! memcmp( frame + IEEE80211_DEST_OFFSET, unit->unit_StationAddress, 6 ) ) ) ) )
		{
			ObtainSemaphore( & unit->unit_CommandLock );

			request = (PTR) GetHead( & unit->unit_Pending );

			while( request )
			{
				if ( request->ios2_Req.io_Command == S2_READMGMT )
				{
					Remove( (PTR) request );

					if ( request->ios2_DataLength < frameLength )
					{
						request->ios2_Req.io_Error = IOERR_BADLENGTH;
						request->ios2_DataLength = 0;
					}
					else
					{
						memcpy( request->ios2_Data, frame, frameLength );

						request->ios2_DataLength = frameLength;
						if ( frame[0] == IEEE80211_BEACON )
						{
							unit->unit_BeaconsDelivered++;

							memcpy( unit->unit_LastBeaconBSSID, frame + IEEE80211_BSSID_OFFSET, 6 );

							unit->unit_LastBeaconInterval = frame[32] | ( (U16) frame[33] << 8 );
						}
						else if ( frame[0] == IEEE80211_AUTH_REPLY )
						{
							USBINFO( "RT2870: authentication reply algorithm %lu, transaction %lu, status %lu, RX flags $%02lx",
								(U32) frame[24] | ( (U32) frame[25] << 8 ),
								(U32) frame[26] | ( (U32) frame[27] << 8 ),
								(U32) frame[28] | ( (U32) frame[29] << 8 ), (U32) rxd[1] );
						}
						else
						{
							USBINFO( "RT2870: received management reply subtype $%02lx, %lu bytes", (U32) frame[0], frameLength );
						}

						if ( ( frame[0] == IEEE80211_BEACON ) && ( unit->unit_BeaconsDelivered <= 3 ) )
						{
							bssid = frame + IEEE80211_BSSID_OFFSET;

							USBINFO( "RT2870: channel %lu delivered beacon from %02lx:%02lx:%02lx:%02lx:%02lx:%02lx, %lu bytes",
								(U32) unit->unit_Channel, (U32) bssid[0], (U32) bssid[1], (U32) bssid[2],
								(U32) bssid[3], (U32) bssid[4], (U32) bssid[5], frameLength );
						}
					}

					ReplyMsg( (PTR) request );

					break;
				}

				request = (PTR) GetSucc( (PTR) request );
			}

			ReleaseSemaphore( & unit->unit_CommandLock );
		}

		data += recordLength;
		length -= recordLength;
	}
}

// --
