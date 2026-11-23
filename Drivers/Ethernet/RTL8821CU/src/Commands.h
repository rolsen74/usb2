
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8821CU_COMMANDS_H
#define RTL8821CU_COMMANDS_H

void Cmd_DeviceQuery( struct IOSana2Req *ioreq );
S32 Cmd_Read( struct EthernetUnit *unit, struct BufferManagement *opener, struct IOSana2Req *ioreq );
S32 Cmd_Write( struct EthernetUnit *unit, struct BufferManagement *opener, struct IOSana2Req *ioreq );
S32 Sana2BeginIO( struct EthernetUnit *unit, struct IOSana2Req *sana );
void Cmd_Flush( struct EthernetUnit *unit, S32 error, U32 wire );
void Cmd_GetStationAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_ConfigInterface( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_MulticastAddress( struct BufferManagement *opener, struct IOSana2Req *ioreq );
void Cmd_TrackType( struct BufferManagement *opener, struct IOSana2Req *ioreq );

#endif
