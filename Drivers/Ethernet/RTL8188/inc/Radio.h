
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

#define S2_SETOPTIONS 0xc012
#define S2_SETKEY 0xc013
#define S2_READMGMT 0xc015
#define S2_WRITEMGMT 0xc016
#define S2_GETCRYPTTYPES 0xc018
#define S2ENC_NONE 0
#define S2ENC_TKIP 2
#define S2ENC_CCMP 3
#define S2INFO_SSID ( TAG_USER + 0 )
#define S2INFO_BSSID ( TAG_USER + 1 )
#define S2INFO_AuthTypes ( TAG_USER + 2 )
#define S2INFO_AssocID ( TAG_USER + 3 )
#define S2INFO_Encryption ( TAG_USER + 4 )
#define S2INFO_PortType ( TAG_USER + 5 )
#define S2INFO_BeaconInterval ( TAG_USER + 6 )
#define S2INFO_Channel ( TAG_USER + 7 )
#define S2INFO_WPAInfo ( TAG_USER + 12 )
#define S2INFO_Band ( TAG_USER + 13 )
#define S2INFO_DefaultKeyNo ( TAG_USER + 14 )
#define S2INFO_OperState ( TAG_USER + 20 )

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
void RTL8188EU_AbortData( struct EthernetUnit *unit, S32 error, U32 wire );
void RTL8188EU_UpdateThroughput( struct EthernetUnit *unit, U32 sent, U32 received );
void RTL8188EU_DropLink( struct EthernetUnit *unit );
S32 RTL8188EU_CopyPacket( struct EthernetUnit *unit, struct IOSana2Req *ioreq,
	PTR to, PTR from, U32 length, S32 rx );
U32 RTL8188EU_CountTXEndpoints( struct EthernetUnit *unit );
void Process_RadioCommands( struct EthernetUnit *unit );
void RTL8188EU_StopRX( struct EthernetUnit *unit );
void RTL8188EU_ReceiveManagement( struct EthernetUnit *unit, U8 *frame, U32 length );
void RTL8188EU_DecodeRX( struct EthernetUnit *unit, U8 *data, U32 length );

#endif
