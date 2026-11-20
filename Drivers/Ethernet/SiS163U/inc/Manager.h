
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef SIS163U_MANAGER_H
#define SIS163U_MANAGER_H

// --

extern const PTR devInterfaces[];

PTR _manager_Init( PTR Dummy, PTR SegList, struct ExecBase *mySysBase );
void _manager_AbortIO( struct DeviceManagerInterface *Self, struct IORequest *ioreq );
void _manager_BeginIO( struct DeviceManagerInterface *Self, struct IORequest *ioreq );
U32 _manager_Obtain( struct DeviceManagerInterface *Self );
U32 _manager_Release( struct DeviceManagerInterface *Self );
PTR _manager_Expunge( struct DeviceManagerInterface *Self );
S32 _manager_Open( struct DeviceManagerInterface *Self, struct IORequest *ioreq, U32 unitnum, U32 flags );
PTR _manager_Close( struct DeviceManagerInterface *Self, struct IORequest *ioreq );

#endif
