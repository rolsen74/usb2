
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8188_COMMANDS_H
#define RTL8188_COMMANDS_H

// --

S32 ValidMulticastAddress( const U8 *address );
struct MulticastAddress *FindMulticastAddress( struct EthernetUnit *unit, const U8 *address );
void Cmd_Invalid( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_Read( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_Write( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Flush( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_DeviceQuery( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetStationAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_ConfigInterface( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_AddMultiCastAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_DelMultiCastAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_MultiCast( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_BroadCast( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetSpecialStats( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetGlobalStats( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_OnEvent( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_ReadOrphan( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Online( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Offline( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetExtendedGlobalStats( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_SampleThroughput( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Sana2Hook( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

S32 Cmd_WriteMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_ReadMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetCryptTypes( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetKey( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetOptions( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

#endif
