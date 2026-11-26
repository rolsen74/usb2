
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8821CU_RADIO_H
#define RTL8821CU_RADIO_H

#define RTL8821CU_FW_TX_OFFSET_PAGES 48U
#define RTL8821CU_BEACON_TIMEOUT ( 5U * TICKS_PER_SECOND )

// --

S32 RTL8821CU_UpdateRF( struct EthernetUnit *unit, U8 address, U32 mask, U32 value );
S32 RTL8821CU_InitDMA( struct USB2_EPResource *control, U32 max_packet );
S32 RTL8821CU_InitPHY( struct EthernetUnit *unit );
S32 RTL8821CU_CalibrateRF( struct EthernetUnit *unit );
S32 RTL8821CU_InitCoexistence( struct EthernetUnit *unit );
S32 RTL8821CU_StartRadio( struct EthernetUnit *unit );
S32 RTL8821CU_TuneRadio( struct EthernetUnit *unit, U8 channel );
S32 RTL8821CU_DrainTX( struct EthernetUnit *unit );
S32 RTL8821CU_SetTxPower( struct EthernetUnit *unit );
S32 RTL8821CU_SetAssociation( struct EthernetUnit *unit, U8 *bssid, U16 aid, S32 receiveData );
S32 RTL8821CU_DataAllowed( const struct EthernetUnit *unit, const struct IOSana2Req *ioreq );
S32 RTL8821CU_ProtectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 type );
S32 RTL8821CU_UnprotectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 header );
void RTL8821CU_SetOnline( struct EthernetUnit *unit, S32 online );
S32 RTL8821CU_CanGoOnline( const struct EthernetUnit *unit );
void RTL8821CU_CheckLink( struct EthernetUnit *unit );
void RTL8821CU_DropLink( struct EthernetUnit *unit );
void Cmd_Online( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetOptions( struct EthernetUnit *unit, struct USB2IFace *usb2, struct IOSana2Req *ioreq );
void Cmd_SetKey( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_ReadMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_WriteMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

#endif

// --
