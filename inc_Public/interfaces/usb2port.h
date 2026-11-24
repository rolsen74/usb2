
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

struct USB2PortUnit;

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

/* Serial backends can use "serial" v1 to publish effective line settings. */
struct IOExtSer;

struct USB2SerialIFace
{
	struct InterfaceData Data;
	ULONG APICALL (*Obtain)( struct USB2SerialIFace *Self );
	ULONG APICALL (*Release)( struct USB2SerialIFace *Self );
	void APICALL (*Expunge)( struct USB2SerialIFace *Self );
	struct Interface *APICALL (*Clone)( struct USB2SerialIFace *Self );
	void APICALL (*SetParams)( struct USB2SerialIFace *Self, struct USB2PortUnit *unit,
		const struct IOExtSer *params );
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
};

#endif
