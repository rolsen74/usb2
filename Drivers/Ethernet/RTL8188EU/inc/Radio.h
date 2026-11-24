
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8188EU_RADIO_H
#define RTL8188EU_RADIO_H

// --

#include "WirelessSana2.h"

#define RTL8188EU_RX_DESC_SIZE 24
#define RTL8188EU_TX_DESC_SIZE 32
#define RTL8188EU_FRAME_MAX 1532
#define RTL8188EU_MTU 1500
#define RTL8188EU_RF_CHANNEL 0x18
#define RTL8188EU_RF_CHANNEL_MASK 0x3ff
#define RTL8188EU_RF_BW20 0x0c00
#define RTL8188EU_RF_CHANNEL_DEFAULT 0x0f407

S32 RTL8188EU_InitDMA( struct USB2_EPResource *control, U32 endpoints );
S32 RTL8188EU_WriteLLT( struct USB2_EPResource *control, U8 page, U8 next );
S32 RTL8188EU_InitMAC( struct USB2_EPResource *control );
S32 RTL8188EU_InitBaseband( struct USB2_EPResource *control );
S32 RTL8188EU_InitRF( struct USB2_EPResource *control );
S32 RTL8188EU_WriteRF( struct USB2_EPResource *control, U16 address, U32 value );
S32 RTL8188EU_Write32( struct USB2_EPResource *control, U16 address, U32 value );
S32 RTL8188EU_Write16( struct USB2_EPResource *control, U16 address, U16 value );
S32 RTL8188EU_Update32( struct USB2_EPResource *control, U16 address, U32 clear, U32 set );
S32 RTL8188EU_StartRadio( struct EthernetUnit *unit );
S32 RTL8188EU_SetChannel( struct USB2_EPResource *control, U32 channel );
S32 RTL8188EU_TuneRadio( struct EthernetUnit *unit, U32 channel );
S32 RTL8188EU_SetAssociation( struct EthernetUnit *unit, U8 *bssid, U16 aid );
void RTL8188EU_QueueControl( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void RTL8188EU_Transmit( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void RTL8188EU_ReceiveData( struct EthernetUnit *unit, U8 *frame, U32 length );
void RTL8188EU_AbortData( struct EthernetUnit *unit, S32 error, U32 wire, S32 keep_eapol );
void RTL8188EU_SetOnline( struct EthernetUnit *unit, S32 online );
void RTL8188EU_DropLink( struct EthernetUnit *unit );
S32 RTL8188EU_CopyPacket( struct EthernetUnit *unit, struct IOSana2Req *ioreq,
	PTR to, PTR from, U32 length, S32 rx );
U32 RTL8188EU_CountTXEndpoints( struct EthernetUnit *unit );
void Process_RadioCommands( struct EthernetUnit *unit );
void RTL8188EU_StopRX( struct EthernetUnit *unit );
void RTL8188EU_ReceiveRobustMgmt( struct EthernetUnit *unit, U8 *frame, U32 length );
void RTL8188EU_ReceiveManagement( struct EthernetUnit *unit, U8 *frame, U32 length );
void RTL8188EU_DecodeRX( struct EthernetUnit *unit, U8 *data, U32 length );

static inline S32
RTL8188EU_DeferredLink( const struct EthernetUnit *unit )
{
	return ( ( unit->unit_SAE ) || ( unit->unit_RequireOperState && unit->unit_Encryption != S2ENC_NONE ) );
}

#endif
