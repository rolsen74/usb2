
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8152_COMMANDS_H
#define RTL8152_COMMANDS_H

// --

void Cmd_Read( struct devUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Write( struct devUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Flush( struct devUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Invalid( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_Sana2Hook( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_DeviceQuery( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_GetStationAddress( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_ConfigInterface( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_AddMultiCastAddress( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_DelMultiCastAddress( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_MultiCast( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_BroadCast( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_GetSpecialStats( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_GetGlobalStats( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_OnEvent( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_ReadOrphan( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_Online( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_Offline( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_GetExtendedGlobalStats( struct devUnit *unit, struct IOSana2Req *ioreq );
void s2_Sample_ThroughPut( struct devUnit *unit, struct IOSana2Req *ioreq );
void QueueReadRequest( struct devUnit *unit, struct IOSana2Req *ioreq );
void QueueWriteRequest( struct devUnit *unit, struct IOSana2Req *ioreq );
int AbortAll( struct devUnit *unit, struct BufferManagement *back_ptr, U32 wire_error );
S32 AbortReqOnListBackPtr( struct devUnit *unit, struct List *list, struct BufferManagement *back_ptr, U32 wire_error );

#endif
