
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_PORTDEVICE_H
#define USB2_PORTDEVICE_H

#include "usb2_all.h"

#define DEVICES_SERIAL_H_OBSOLETE
#include <devices/serial.h>
#include <devices/parallel.h>
#include <devices/newstyle.h>
#include <devices/usb2defaults.h>
#include <interfaces/usb2port.h>
#include <interfaces/usb2unitidentity.h>

// --

enum USB2PortType
{
	PORT_SERIAL,
	PORT_PARALLEL,
	PORT_PRINTER
};

struct USB2PortUnit
{
	struct USB2PortUnit *Next;
	struct MsgPort *BeginPort;
	struct MsgPort *AbortPort;
	U32 Number;
	U32 Opens;
	U8 Shared;
	U8 Closing;
	U8 Quiescing;
	struct Message *CloseReply;
	U8 SerialSessions;
	U8 SerialParamsValid;
	struct IOExtSer SerialParams;
	struct USB2UnitIdentity Identity;
};

struct USB2PortDevice
{
	struct Library Library;
	struct ExecIFace *IExec;
	struct USBBase *USB;
	struct SignalSemaphore Lock;
	struct USB2PortUnit *Units;
	enum USB2PortType Type;
};

SEC_CODE S32 PortDevice_Create( struct USBBase *usbbase );
SEC_CODE S32 PortDevice_Open( struct DeviceManagerInterface *Self, struct IORequest *request, U32 number, U32 flags );
SEC_CODE PTR PortDevice_Close( struct DeviceManagerInterface *Self, struct IORequest *request );
SEC_CODE void PortDevice_BeginIO( struct DeviceManagerInterface *Self, struct IOStdReq *request );
SEC_CODE void PortDevice_AbortIO( struct DeviceManagerInterface *Self, struct IOStdReq *request );
SEC_CODE void PortDevice_EnumUnits( struct USB2PortDevice *base, struct IOStdReq *request );
SEC_CODE void PortDevice_Query( struct USB2PortDevice *base, struct IOStdReq *request );
SEC_CODE struct USB2PortUnit *PortDevice_Register( struct USB2PortIFace *Self, struct MsgPort *begin, struct MsgPort *abort, ULONG *number );
SEC_CODE struct USB2PortUnit *PortDevice_RegisterDevice( struct USB2PortIFace *Self, struct MsgPort *begin,
	struct MsgPort *abort, ULONG *number, const struct USB2_DriverMessage *msg );
SEC_CODE void PortDevice_Unregister( struct USB2PortIFace *Self, struct USB2PortUnit *unit );
SEC_CODE void PortDevice_UnlinkUnit( struct USB2PortDevice *base, struct USB2PortUnit *unit );
SEC_CODE BOOL PortDevice_JobClosing( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit );
SEC_CODE void PortDevice_Quiesce( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit );
SEC_CODE void PortDevice_FinishJob( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit );
SEC_CODE struct USB2PortUnit *PortDevice_RegisterSession( struct USB2SerialIFace *Self,
	struct MsgPort *begin, struct MsgPort *abort, ULONG *number );
SEC_CODE enum USB2SerialSession PortDevice_SerialSession( struct USB2SerialIFace *Self, struct USB2PortUnit *unit );
SEC_CODE void PortDevice_SerialFinishClose( struct USB2SerialIFace *Self, struct USB2PortUnit *unit );
SEC_CODE void PortDevice_SetParams( struct USB2SerialIFace *Self, struct USB2PortUnit *unit, const struct IOExtSer *params );
SEC_CODE BOOL PortDevice_GetParams( struct USB2SerialIFace *Self, struct USB2PortUnit *unit, struct IOExtSer *params );
SEC_CODE void PortDevice_ReplyClose( struct ExecIFace *IExec, struct USB2PortUnit *unit );

#endif
