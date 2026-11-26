
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8811AU_RADIO_H
#define RTL8811AU_RADIO_H

#define RTL8811AU_EXT_PA_5GHZ  0x01
#define RTL8811AU_EXT_LNA_5GHZ 0x02
#define RTL8811AU_EXT_5GHZ ( RTL8811AU_EXT_PA_5GHZ | RTL8811AU_EXT_LNA_5GHZ )
#define RTL8811AU_BOARD_DEFAULT 0x04
#define RTL8811AU_BOARD_BT 0x08

struct RTL8811AU_Board
{
	U8 BoardType;
	U8 CrystalCap;
	U8 ExternalLNA2GHz;
	U8 CCKPower[4];
};

S32 RTL8811AU_Probe( struct USB2_EPResource *control, U8 *station, struct RTL8811AU_Board *board );
S32 RTL8811AU_InitMAC( struct USB2_EPResource *control, U8 *station );
S32 RTL8811AU_InitDMA( struct USB2_EPResource *control, U32 max_packet );
S32 RTL8811AU_InitBaseband( struct USB2_EPResource *control, const struct RTL8811AU_Board *board );
S32 RTL8811AU_InitRF( struct USB2_EPResource *control, const struct RTL8811AU_Board *board );
S32 RTL8811AU_UpdateReg( struct USB2_EPResource *control, U16 address, U16 size, U32 clear, U32 set );
S32 RTL8811AU_ReadRF( struct USB2_EPResource *control, U8 address, U32 *value );
S32 RTL8811AU_WriteRF( struct USB2_EPResource *control, U8 address, U32 value );
S32 RTL8811AU_SetChannel( struct USB2_EPResource *control, const struct RTL8811AU_Board *board, U8 channel );
S32 RTL8811AU_StartRadio( struct EthernetUnit *unit, const struct RTL8811AU_Board *board, U32 max_packet );
S32 RTL8811AU_TuneRadio( struct EthernetUnit *unit, const struct RTL8811AU_Board *board, U8 channel );
S32 RTL8811AU_SetAssociation( struct EthernetUnit *unit, U8 *bssid, U16 aid );
void RTL8811AU_SetOnline( struct EthernetUnit *unit, S32 online );
S32 RTL8811AU_DataAllowed( const struct EthernetUnit *unit, const struct IOSana2Req *ioreq );
void RTL8811AU_DecodeRX( struct EthernetUnit *unit, U8 *data, U32 length );
void RTL8811AU_ReceiveData( struct EthernetUnit *unit, U8 *frame, U32 length );
void RTL8811AU_Transmit( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void RTL8811AU_AccountType( struct EthernetUnit *unit, U32 type, U32 length, S32 receive, S32 dropped );
S32 RTL8811AU_CopyPacket( struct EthernetUnit *unit, struct IOSana2Req *ioreq, PTR to, PTR from, U32 length, S32 rx );
S32 RTL8811AU_ProtectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 type );
S32 RTL8811AU_UnprotectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 header );
void Cmd_SetOptions( struct EthernetUnit *unit, const struct RTL8811AU_Board *board, struct IOSana2Req *ioreq,
	struct USB2_EPResource *rx );
S32 Process_Main( struct EthernetUnit *unit, struct USB2_Register *reg, struct USB2_EPResource *rx, const struct RTL8811AU_Board *board );

#endif
