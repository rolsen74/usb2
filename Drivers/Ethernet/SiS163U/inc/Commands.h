
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef SIS163U_COMMANDS_H
#define SIS163U_COMMANDS_H

// --

// Wireless SANA-II commands shared with the RTL8188EU driver.
#define S2_SETOPTIONS 0xc012
#define S2_SETKEY 0xc013
#define S2_READMGMT 0xc015
#define S2_WRITEMGMT 0xc016
#define S2_GETCRYPTTYPES 0xc018
#define S2ENC_NONE 0
#define S2INFO_Channel ( TAG_USER + 7 )

// --

S32 ValidMulticastAddress( const U8 *address );
struct MulticastAddress *FindMulticastAddress( struct EthernetUnit *unit, const U8 *address );
void Cmd_Invalid( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Read( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Write( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Flush( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_DeviceQuery( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetStationAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_ConfigInterface( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_AddMultiCastAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_DelMultiCastAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_MultiCast( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_BroadCast( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetSpecialStats( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetGlobalStats( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_OnEvent( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_ReadOrphan( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Online( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Offline( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetExtendedGlobalStats( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_SampleThroughput( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Sana2Hook( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

void Cmd_WriteMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_ReadMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetCryptTypes( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetKey( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetOptions( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

#endif
