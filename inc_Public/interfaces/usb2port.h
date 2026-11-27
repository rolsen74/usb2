
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INTERFACES_USB2PORT_H
#define INTERFACES_USB2PORT_H

#include <exec/interfaces.h>
#include <exec/ports.h>

// --

#define USB2_SERIAL_DEVICE "usb2serial.device"
#define USB2_PARALLEL_DEVICE "usb2parallel.device"
#define USB2_PRINTER_DEVICE "usb2printer.device"
#define USB2_PORT_BACKEND_UNIT 0xffffffffUL
#define USB2_SERIAL_MAIN_REVISION 8
#define USB2_SERIAL_RECONNECT_REVISION 9
#define USB2_PARALLEL_RECONNECT_REVISION 7
#define USB2_PRINTER_RECONNECT_REVISION 11

struct USB2PortUnit;
struct USB2_DriverMessage;

/*
 * Open the device on USB2_PORT_BACKEND_UNIT with an IORequest, then obtain
 * "main" version 1. Register returns the lowest free unit number and an
 * opaque handle. Both ports must belong to the backend worker and use
 * PA_SIGNAL. I/O arrives on the begin port. Abort sets the standard
 * IOSERF_ABORT/IOPARF_ABORT bit and signals the abort port.
 *
 * Unregister before draining or freeing the ports. The worker still owns
 * all queued and active requests and must reply to them before exiting.
 * Drop the interface and close the device after unregistering.
 */
struct USB2PortIFace
{
	struct InterfaceData Data;
	ULONG APICALL (*Obtain)( struct USB2PortIFace *Self );
	ULONG APICALL (*Release)( struct USB2PortIFace *Self );
	void APICALL (*Expunge)( struct USB2PortIFace *Self );
	struct Interface *APICALL (*Clone)( struct USB2PortIFace *Self );

	struct USB2PortUnit *APICALL (*Register)( struct USB2PortIFace *Self,
		struct MsgPort *begin, struct MsgPort *abort, ULONG *number );
	void APICALL (*Unregister)( struct USB2PortIFace *Self, struct USB2PortUnit *unit );
};

/*
 * RegisterDevice uses the class driver message to identify a returning unit.
 * Use it on the matching device with its RECONNECT_REVISION or later.
 * Unregister stops routing and the worker must still finish previously queued I/O.
 */
struct USB2ParallelIFace
{
	struct InterfaceData Data;
	ULONG APICALL (*Obtain)( struct USB2ParallelIFace *Self );
	ULONG APICALL (*Release)( struct USB2ParallelIFace *Self );
	void APICALL (*Expunge)( struct USB2ParallelIFace *Self );
	struct Interface *APICALL (*Clone)( struct USB2ParallelIFace *Self );
	struct USB2PortUnit *APICALL (*Register)( struct USB2ParallelIFace *Self,
		struct MsgPort *begin, struct MsgPort *abort, ULONG *number );
	void APICALL (*Unregister)( struct USB2ParallelIFace *Self, struct USB2PortUnit *unit );
	struct USB2PortUnit *APICALL (*RegisterDevice)( struct USB2ParallelIFace *Self,
		struct MsgPort *begin, struct MsgPort *abort, ULONG *number, const struct USB2_DriverMessage *msg );
};

enum USB2SerialSession
{
	USB2_SERIAL_IDLE,
	USB2_SERIAL_OPEN,
	USB2_SERIAL_CLOSING
};

struct IOExtSer;

/* Serial device 1.8 extends "main" v1 after Register and Unregister. */
struct USB2SerialIFace
{
	struct InterfaceData Data;
	ULONG APICALL (*Obtain)( struct USB2SerialIFace *Self );
	ULONG APICALL (*Release)( struct USB2SerialIFace *Self );
	void APICALL (*Expunge)( struct USB2SerialIFace *Self );
	struct Interface *APICALL (*Clone)( struct USB2SerialIFace *Self );
	struct USB2PortUnit *APICALL (*Register)( struct USB2SerialIFace *Self,
		struct MsgPort *begin, struct MsgPort *abort, ULONG *number );
	void APICALL (*Unregister)( struct USB2SerialIFace *Self, struct USB2PortUnit *unit );
	void APICALL (*SetParams)( struct USB2SerialIFace *Self, struct USB2PortUnit *unit,
		const struct IOExtSer *params );
	struct USB2PortUnit *APICALL (*RegisterSession)( struct USB2SerialIFace *Self,
		struct MsgPort *begin, struct MsgPort *abort, ULONG *number );
	enum USB2SerialSession APICALL (*Session)( struct USB2SerialIFace *Self, struct USB2PortUnit *unit );
	void APICALL (*FinishClose)( struct USB2SerialIFace *Self, struct USB2PortUnit *unit );
	struct USB2PortUnit *APICALL (*RegisterDevice)( struct USB2SerialIFace *Self,
		struct MsgPort *begin, struct MsgPort *abort, ULONG *number, const struct USB2_DriverMessage *msg );
	BOOL APICALL (*GetParams)( struct USB2SerialIFace *Self, struct USB2PortUnit *unit, struct IOExtSer *params );
};

/*
 * Printer drivers obtain "main" v1 on USB2_PRINTER_DEVICE. The usual
 * abort-port signal also wakes the worker on last close. Flush the print
 * job, then call FinishJob. Printer device revision 8 adds Quiesce.
 * Call Quiesce before retiring USB and replying to queued requests.
 * Unregister then releases the unit and any waiting closer.
 */
struct USB2PrinterIFace
{
	struct InterfaceData Data;
	ULONG APICALL (*Obtain)( struct USB2PrinterIFace *Self );
	ULONG APICALL (*Release)( struct USB2PrinterIFace *Self );
	void APICALL (*Expunge)( struct USB2PrinterIFace *Self );
	struct Interface *APICALL (*Clone)( struct USB2PrinterIFace *Self );

	struct USB2PortUnit *APICALL (*Register)( struct USB2PrinterIFace *Self,
		struct MsgPort *begin, struct MsgPort *abort, ULONG *number );
	void APICALL (*Unregister)( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit );
	BOOL APICALL (*JobClosing)( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit );
	void APICALL (*FinishJob)( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit );
	void APICALL (*Quiesce)( struct USB2PrinterIFace *Self, struct USB2PortUnit *unit );
	struct USB2PortUnit *APICALL (*RegisterDevice)( struct USB2PrinterIFace *Self,
		struct MsgPort *begin, struct MsgPort *abort, ULONG *number, const struct USB2_DriverMessage *msg );
};

#endif
