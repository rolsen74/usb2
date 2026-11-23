
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

static S32
ValidEAPOL( struct EthernetUnit *unit, struct IOSana2Req *ioreq, U16 *info )
{
	const U8 *payload;
	U32 length;
	U16 version;

	payload = ioreq->ios2_Data;
	length = ioreq->ios2_DataLength;

	if ( ( ioreq->ios2_Req.io_Flags & SANA2IOF_RAW ) ||
		( length < 99 ) || ( length > RT2870_EAPOL_MAX ) ||
		( memcmp( ioreq->ios2_DstAddr, unit->unit_Peer, 6 ) ) ||
		( payload[1] != 3 ) || ( payload[4] != ( unit->unit_TKIP ? 254 : 2 ) ) ||
		( length != 4U + ( ( payload[2] << 8 ) | payload[3] ) ) )
	{
		return ( FALSE );
	}

	*info = ( payload[5] << 8 ) | payload[6];
	if ( unit->unit_TKIP )
	{
		if ( ( payload[7] ) || ( payload[8] != WIRELESS_TKIP_KEY_SIZE ) )
		{
			return ( FALSE );
		}

		if ( ( *info == 0x0109 ) && ( length == 123 ) && ( payload[97] == 0 ) && ( payload[98] == 24 ) )
		{
			*info = RT2870_EAPOL_M2;

			return ( TRUE );
		}

		if ( ( length != 99 ) || ( payload[97] ) || ( payload[98] ) ||
			( ! ( unit->unit_ActiveKeys & RT2870_PAIRWISE_KEY ) ) )
		{
			return ( FALSE );
		}

		if ( ( *info & ~0x0200 ) == 0x0109 )
		{
			*info = RT2870_EAPOL_M4;

			return ( TRUE );
		}

		return ( ( ( *info & ~0x0030 ) == 0x0301 ) &&
			( unit->unit_M4Sent ) && ( unit->unit_ActiveKeys & RT2870_GROUP_KEYS ) );
	}

	version = unit->unit_SAE ? 0 : 2;

	if ( ( *info & 7 ) != version )
	{
		return ( FALSE );
	}

	// Normalize the descriptor version for the M2/M4/group classification.
	*info |= 2;

	return ( ( *info == RT2870_EAPOL_M2 ) ||
		( ( *info == RT2870_EAPOL_M4 ) && ( unit->unit_ActiveKeys & 1 ) && ( unit->unit_ActiveKeys & 0xe ) &&
			( ( ! unit->unit_SAE ) || ( unit->unit_IGTK[0].active ) || ( unit->unit_IGTK[1].active ) ) ) ||
		( ( *info == 0x0302 ) && ( unit->unit_Operational ) ) );
}

// --

void
Cmd_Write( struct EthernetUnit *unit, struct IOSana2Req *ioreq )
{
	static const U8 broadcast[] = { 255, 255, 255, 255, 255, 255 };
	static const U8 snap[] = { 0xaa, 0xaa, 3, 0, 0, 0 };
	U8 frame[32 + RT2870_EAPOL_MAX + WIRELESS_CCMP_IV + WIRELESS_TKIP_MIC + WIRELESS_TKIP_ICV];
	U8 ethernet[14 + RT2870_MTU];
	const U8 *destination;
	U32 length;
	U32 type;
	U32 dhcp;
	U32 payload_length;
	U16 info;
	S32 raw;
	S32 eapol;
	S32 error;

	length = ioreq->ios2_DataLength;
	type = ioreq->ios2_PacketType;
	raw = ioreq->ios2_Req.io_Flags & SANA2IOF_RAW;
	eapol = ! raw && ( type == RT2870_EAPOL_TYPE ) && ( ioreq->ios2_Req.io_Command == CMD_WRITE );
	destination = ioreq->ios2_DstAddr;
	info = 0;
	dhcp = 0;

	if ( ( ! unit->unit_AssocID ) || ( ( ! eapol ) && ( ! unit->unit_Online ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		return;
	}

	if ( ! ioreq->ios2_Data )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	if ( ( eapol ) && ( ! ValidEAPOL( unit, ioreq, & info ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;

		return;
	}

	if ( ( ! eapol ) && ( ( length > RT2870_MTU + ( raw ? 14 : 0 ) ) || ( raw && length < 14 ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_MTU_EXCEEDED;

		return;
	}

	if ( ioreq->ios2_Req.io_Command == S2_BROADCAST )
	{
		destination = broadcast;
	}

	if ( eapol )
	{
		memcpy( frame + 32, ioreq->ios2_Data, length );
	}
	else if ( raw )
	{
		if ( ! RT2870_CopyPacket( ioreq, ethernet, ioreq->ios2_Data, length, FALSE ) )
		{
			goto copy_failed;
		}

		if ( ioreq->ios2_Req.io_Command != S2_BROADCAST )
		{
			destination = ethernet;
		}

		type = ( (U32) ethernet[12] << 8 ) | ethernet[13];
		length -= 14;

		memcpy( frame + 32, ethernet + 14, length );
	}
	else if ( ! RT2870_CopyPacket( ioreq, frame + 32, ioreq->ios2_Data, length, FALSE ) )
	{
		goto copy_failed;
	}

	if ( ( type < 0x0600 ) || ( type > 0xffff ) || ( ( ! eapol ) && ( type == RT2870_EAPOL_TYPE ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;

		return;
	}

	if ( ( ioreq->ios2_Req.io_Command == S2_MULTICAST ) && ( ! ( destination[0] & 1 ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ADDRESS;
		ioreq->ios2_WireError = S2WERR_BAD_MULTICAST;

		return;
	}

	if ( type == 0x0800 )
	{
		dhcp = RT2870_DHCPType( frame + 32, length, unit->unit_StationAddress, FALSE );
	}

	memset( frame, 0, 24 );

	frame[0] = 0x08;
	frame[1] = 0x01; // To DS

	memcpy( frame + 4, unit->unit_Peer, 6 );

	memcpy( frame + 10, unit->unit_StationAddress, 6 );

	memcpy( frame + 16, destination, 6 );

	memcpy( frame + 24, snap, sizeof( snap ) );

	frame[30] = type >> 8;
	frame[31] = type;
	payload_length = length;
	length += 32;

	if ( ( info != RT2870_EAPOL_M2 ) && ( info != RT2870_EAPOL_M4 ) &&
		( ! RT2870_ProtectData( unit, frame, & length ) ) )
	{
		ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;
		ioreq->ios2_WireError = S2WERR_UNIT_OFFLINE;

		RT2870_DropLink( unit, FALSE );

		return;
	}

	if ( ( info == RT2870_EAPOL_M2 ) || ( info == RT2870_EAPOL_M4 ) )
	{
		USBINFO( "RT2870: sending %s message %lu", unit->unit_TKIP ? "WPA" : ( unit->unit_SAE ? "WPA3" : "WPA2" ), info == RT2870_EAPOL_M4 ? 4UL : 2UL );
	}

	error = RT2870_TransmitFrame( unit, frame, length, unit->unit_Peer, TRUE,
		eapol ? "EAPOL" : ( unit->unit_DataSent ? NULL : "first encrypted data" ) );

	if ( error )
	{
		ioreq->ios2_Req.io_Error = S2ERR_TX_FAILURE;
		ioreq->ios2_WireError = S2WERR_RCVREL_HDW_ERR;
	}
	else if ( info == RT2870_EAPOL_M4 )
	{
		unit->unit_M4Sent = TRUE;
	}
	else if ( ( ! eapol ) && ( ! unit->unit_DataSent ) )
	{
		unit->unit_DataSent = TRUE;

		USBINFO( "RT2870: first encrypted packet sent, EtherType $%04lx", type );
	}

	if ( ! error )
	{
		if ( ! eapol )
		{
			ObtainSemaphore( & unit->unit_CommandLock );

			unit->unit_PacketsTx++;

			Sana2_AccountType( & unit->unit_TrackedTypes, type, payload_length, FALSE, FALSE );

			Sana2_UpdateThroughput( & unit->unit_Pending, payload_length + 14, 0 );

			ReleaseSemaphore( & unit->unit_CommandLock );
		}

		RT2870_ReportDHCP( & unit->unit_DHCPSent, dhcp, "submitted to the radio" );
	}

	return;

copy_failed:

	ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
	ioreq->ios2_WireError = S2WERR_BUFF_ERROR;
}

// --
