
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8811AU_COMMANDS_H
#define RTL8811AU_COMMANDS_H

extern const UWORD EthClass_Commands[];

S32 Sana2BeginIO( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_DeviceQuery( struct IOSana2Req *ioreq );
void Cmd_GetStationAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_ConfigInterface( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Online( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_ReadMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_Read( struct EthernetUnit *unit, struct BufferManagement *opener, struct IOSana2Req *ioreq );
S32 Cmd_Write( struct EthernetUnit *unit, struct BufferManagement *opener, struct IOSana2Req *ioreq );
S32 Cmd_WriteMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetKey( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

#endif
