/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DRIVERS_VIDEO_VIDEO_H
#define DRIVERS_VIDEO_VIDEO_H

#include "VideoDevice.h"

#define VIDEO_MAX_FRAME_SIZE ( 64U * 1024U * 1024U )
#define VIDEO_READ_SIZE 65536
#define VIDEO_OV534_READ_SIZE 16384
#define VIDEO_OV534_PAYLOAD_SIZE 2048
#define VIDEO_ISO_PACKETS 16
#define VIDEO_ISO_MAX_PACKETS 256
#define VIDEO_USB_READS 2
#define VIDEO_IO_ABORT 0x80
#define VIDEO_PAYLOAD_FID 0x01
#define VIDEO_PAYLOAD_EOF 0x02
#define VIDEO_PAYLOAD_PTS 0x04
#define VIDEO_PAYLOAD_ERR 0x40

struct VideoUnit;
struct VideoBase;

struct VideoMode
{
	struct VideoFormat Format;
	U8 FormatIndex;
	U8 FrameIndex;
};

static inline U16
Video_LE16( const U8 *data )
{
	return ( (U16) data[0] | ( (U16) data[1] << 8 ) );
}

static inline U32
Video_LE32( const U8 *data )
{
	return ( (U32) data[0] | ( (U32) data[1] << 8 ) |
		( (U32) data[2] << 16 ) | ( (U32) data[3] << 24 ) );
}

struct VideoContext
{
	struct USB2IFace *IUSB2;
	struct SignalSemaphore Lock;
	struct VideoBase *Device;
};

struct VideoDriverBase
{
	struct Library Library;
	PTR SegList;
	struct VideoContext Context;
};

struct VideoBase
{
	struct Library Library;
	U16 Pad;
	PTR SegList;
	struct VideoContext *Context;
	struct SignalSemaphore Semaphore;
	struct VideoUnit *Units[VIDEO_MAX_UNITS];
};

struct VideoUnit
{
	struct VideoContext *Context;
	struct VideoBase *Base;
	struct USB2_DriverMessage *StartMsg;
	struct USB2_Register *Register;
	struct USB2_EPResource *Control;
	struct USB2_EPResource *Stream;
	struct MsgPort *BeginPort;
	struct MsgPort *StreamPort;
	struct MsgPort *TimerPort;
	struct TimeRequest *Timer;
	struct TimerIFace *ProbeClock;
	struct IOStdReq *ReadActive;
	struct USB2_IORequest *ReadReady;
	struct VideoFormat Format;
	struct VideoMode DefaultMode;
	U32 UnitNr;
	U32 OpenCount;
	U32 FrameUsed;
	U32 PayloadSize;
	U32 StreamPacketSize;
	U32 IsoPacketCount;
	U32 ReadPacket;
	U32 FramePTS;
	U64 ProbeDeadline;
	U16 IsoPacketLengths[VIDEO_USB_READS][VIDEO_ISO_MAX_PACKETS];
	U8 Commit[48];
	U8 InterfaceNr;
	U8 EndPoint;
	U8 BulkEndPoint;
	U8 VideoEndPoint;
	U8 StreamAlt;
	U8 StreamAltActive;
	U8 StreamConfigured;
	U8 StreamInterval;
	U8 FormatIndex;
	U8 FrameIndex;
	U8 FrameID;
	U8 FrameStarted;
	U8 FrameDiscard;
	U8 FrameReports;
	U8 ProbeLength;
	U8 OV534;
	U8 OV534Initialized;
	U8 OV534StopPending;
	U8 ReadPending;
	U8 ReadHead;
	U8 ReadTail;
	U8 ReadGaps;
	U8 TimerPending;
	U8 TimerOpen;
	U8 ReadReset;
	U8 Detached;
	U8 USBDetached;
	U8 Released;
};

SEC_CODE U32 Video_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
SEC_CODE S32 Video_CreateDevice( struct VideoContext *ctx );
SEC_CODE struct VideoUnit *Video_Alloc( struct VideoContext *ctx, struct USB2_DriverMessage *msg );
SEC_CODE void Video_Free( struct VideoUnit *unit );
SEC_CODE void Video_Main( struct VideoUnit *unit );
SEC_CODE S32 Video_Find_Endpoint( struct VideoUnit *unit );
SEC_CODE S32 Video_Parse( struct VideoUnit *unit, U32 number, struct VideoMode *mode );
SEC_CODE S32 Video_Stream_Open( struct VideoUnit *unit, const struct VideoMode *mode );
SEC_CODE S8 Video_Stream_Start( struct VideoUnit *unit );
SEC_CODE S8 Video_Stream_Close( struct VideoUnit *unit );
SEC_CODE S8 Video_Set_Mode( struct VideoUnit *unit, U32 number );
SEC_CODE S32 Video_Select_Stream( struct VideoUnit *unit, U32 payloadsize );
SEC_CODE S32 Video_Probe( struct VideoUnit *unit );
SEC_CODE S32 Video_Stream_SetAlt( struct VideoUnit *unit, U32 alt );
SEC_CODE S32 Video_Control( struct VideoUnit *unit, U8 type, U8 code, U16 value, PTR data, U16 length );
SEC_CODE S32 Video_Control_Index( struct VideoUnit *unit, U8 type, U8 code, U16 value, U16 index,
	PTR data, U16 length, U32 timeout );
SEC_CODE S8 Video_OV534_Init( struct VideoUnit *unit );
SEC_CODE S32 Video_OV534_Mode( struct VideoUnit *unit, U32 number, struct VideoMode *mode );
SEC_CODE S8 Video_OV534_Start( struct VideoUnit *unit );
SEC_CODE S8 Video_OV534_Stop( struct VideoUnit *unit );
SEC_CODE void Video_Read_Start( struct VideoUnit *unit );
SEC_CODE void Video_Read_Done( struct VideoUnit *unit );
SEC_CODE S32 Video_Read_Payload( struct VideoUnit *unit, U8 *data, U32 length );
SEC_CODE void Video_Read_Reply( struct VideoUnit *unit, S8 error );
SEC_CODE void Video_Read_Stop( struct VideoUnit *unit );
SEC_CODE void Video_Begin( struct VideoUnit *unit );
SEC_CODE void Video_Abort( struct VideoUnit *unit );

SEC_CODE struct VideoBase *Video_Manager_Init( struct VideoBase *base, PTR seglist, struct ExecIFace *IExec );
SEC_CODE U32 Video_Manager_Obtain( struct DeviceManagerInterface *Self );
SEC_CODE U32 Video_Manager_Release( struct DeviceManagerInterface *Self );
SEC_CODE S32 Video_Manager_Open( struct DeviceManagerInterface *Self, struct IORequest *ioreq, U32 unitnr, U32 flags );
SEC_CODE PTR Video_Manager_Close( struct DeviceManagerInterface *Self, struct IORequest *ioreq );
SEC_CODE PTR Video_Manager_Expunge( struct DeviceManagerInterface *Self );
SEC_CODE void Video_EnumUnits( struct VideoBase *base, struct IOStdReq *request );
SEC_CODE void Video_Manager_BeginIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq );
SEC_CODE void Video_Manager_AbortIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq );

SEC_CODE struct VideoDriverBase *Driver_Init( PTR dummy, PTR seglist, struct ExecBase *sysbase );
SEC_CODE U32 Driver_Obtain( struct LibraryManagerInterface *Self );
SEC_CODE U32 Driver_Release( struct LibraryManagerInterface *Self );
SEC_CODE PTR Driver_Open( struct LibraryManagerInterface *Self, U32 version );
SEC_CODE PTR Driver_Close( struct LibraryManagerInterface *Self );
SEC_CODE PTR Driver_Expunge( struct LibraryManagerInterface *Self );
SEC_CODE U32 Driver_USB_Obtain( struct USB2DriverIFace *Self );
SEC_CODE U32 Driver_USB_Release( struct USB2DriverIFace *Self );

#endif
