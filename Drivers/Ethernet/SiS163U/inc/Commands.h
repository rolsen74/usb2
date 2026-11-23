
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

#include "WirelessSana2.h"

// --

void Cmd_Read( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Write( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Flush( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_DeviceQuery( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetStationAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_ConfigInterface( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_MultiCast( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_BroadCast( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_OnEvent( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_ReadOrphan( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Online( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Offline( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

S32 Cmd_WriteMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_ReadMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetCryptTypes( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetKey( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetOptions( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

#endif
