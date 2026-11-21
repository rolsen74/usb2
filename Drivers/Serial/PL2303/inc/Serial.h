/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef DRIVERS_SERIAL_SERIAL_H
#define DRIVERS_SERIAL_SERIAL_H

// --

#define DEVICES_SERIAL_H_OBSOLETE
#include <devices/serial.h>
#include <interfaces/usb2port.h>

#define SERIAL_DRIVER_NAME "PL2303.Driver"
#define SERIAL_DRIVER_VERSION 1
#define SERIAL_DRIVER_REVISION 2
#define SERIAL_DRIVER_ID SERIAL_DRIVER_NAME " 1.2 (21.11.2026)\r\n"

#define SERIAL_RX_SIZE 4096
#define SERIAL_UART_INDEX 8
#define SERIAL_PL2303_VENDOR 0x067b
#define SERIAL_PL2303_PRODUCT 0x2303
#define SERIAL_ACM_SUBCLASS 0x02
#define SERIAL_ACM_LINE 0x02
#define SERIAL_ACM_BREAK 0x04

#define SERIAL_UART_DCD 0x01
#define SERIAL_UART_DSR 0x02
#define SERIAL_UART_BREAK 0x04
#define SERIAL_UART_RING 0x08
#define SERIAL_UART_FRAME 0x10
#define SERIAL_UART_PARITY 0x20
#define SERIAL_UART_OVERRUN 0x40
#define SERIAL_UART_CTS 0x80

struct SerialUnit;

struct SerialContext
{
	struct ExecIFace *IExec;
	struct DOSIFace *IDOS;
	struct USB2IFace *IUSB2;
};

struct SerialDriverBase
{
	struct Library Library;
	PTR SegList;
	struct Library *DOSBase;
	struct SerialContext Context;
};

struct SerialUnit
{
	struct IORequest				PortRequest;
	struct USB2PortIFace *		PortInterface;
	struct USB2PortUnit *		PortUnit;

	struct USB2_Interface *		DataInterface;
	U32							ACMNotificationUsed;
	U32							ACMNotificationSize;
	U8							ACMNotification[10];
	U8							ACM;
	U8							ACMCapabilities;
	U8							DataAlt;

	struct USB2_DriverMessage *		StartMsg;
	struct USB2_Register *		Register;
	struct USB2_EPResource *		Control;
	struct USB2_EPResource *		BulkIn;
	struct USB2_EPResource *		BulkOut;
	struct USB2_EPResource *		Interrupt;
	struct MsgPort *			BeginPort;
	struct MsgPort *			AbortPort;
	struct List					ReadList;
	struct List					WriteList;
	struct IOExtSer *				WriteActive;
	U8								RxBuffer[SERIAL_RX_SIZE];
	U8								Line[7];
	U8								UartState;
	U8								Stopped;
	U8								Detached;
	U8								ReadPending;
	U8								WritePending;
	U8								InterruptPending;
	U8								Overrun;
	U32								RxHead;
	U32								RxTail;
};

// --

SEC_CODE U32 Serial_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
SEC_CODE struct SerialUnit *Serial_Alloc( struct SerialContext *ctx, struct USB2_DriverMessage *msg );
SEC_CODE void Serial_Free( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Main( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE S32 Serial_Startup( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE S32 Serial_ACM_Parse( struct SerialUnit *unit, U32 *interface, U32 *alt );
SEC_CODE void Serial_ACM_Notification( struct SerialContext *ctx, struct SerialUnit *unit, const U8 *data, U32 length );
SEC_CODE S32 Serial_Control( struct SerialContext *ctx, struct SerialUnit *unit, U8 type, U8 code, U16 value, U16 index, PTR data, U16 length );
SEC_CODE S32 Serial_Vendor_Read( struct SerialContext *ctx, struct SerialUnit *unit, U16 value, U8 *result );
SEC_CODE S32 Serial_Vendor_Write( struct SerialContext *ctx, struct SerialUnit *unit, U16 value, U16 index );
SEC_CODE void Serial_Read_Start( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Write_Start( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Interrupt_Start( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Read_Done( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Write_Done( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Interrupt_Done( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Read_Drain( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Read_Break( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Begin( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Abort( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_Reply( struct SerialContext *ctx, struct IOExtSer *ioreq );
SEC_CODE void Serial_Flush( struct SerialContext *ctx, struct SerialUnit *unit );
SEC_CODE void Serial_IO_Stop( struct SerialContext *ctx, struct USB2_IORequest *ioreq );
SEC_CODE void Serial_Cmd_Read( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_Write( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_Reset( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_Flush( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_Clear( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_Stop( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_Start( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_SetParams( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_Query( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_Break( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq );
SEC_CODE void Serial_Cmd_DeviceQuery( struct SerialContext *ctx, struct SerialUnit *unit, struct IOStdReq *ioreq );


SEC_CODE struct SerialDriverBase *Driver_Init( PTR dummy, PTR seglist, struct ExecBase *sysbase );
SEC_CODE U32 Driver_Obtain( struct LibraryManagerInterface *Self );
SEC_CODE U32 Driver_Release( struct LibraryManagerInterface *Self );
SEC_CODE PTR Driver_Open( struct LibraryManagerInterface *Self, U32 version );
SEC_CODE PTR Driver_Close( struct LibraryManagerInterface *Self );
SEC_CODE PTR Driver_Expunge( struct LibraryManagerInterface *Self );
SEC_CODE U32 Driver_USB_Obtain( struct USB2DriverIFace *Self );
SEC_CODE U32 Driver_USB_Release( struct USB2DriverIFace *Self );

#endif
