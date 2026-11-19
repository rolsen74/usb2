/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DRIVERS_AUDIO_AUDIO_H
#define DRIVERS_AUDIO_AUDIO_H

#include "AudioDevice.h"

#define AUDIO_MAX_UNITS 32
#define AUDIO_PACKET_COUNT 32
#define AUDIO_BATCH_US 32000U
#define AUDIO_READ_TIMEOUT ( 5U * 1000000U )
#define AUDIO_EMPTY_BATCH_LIMIT 4
#define AUDIO_IO_ABORT 0x80

struct AudioUnit;
struct AudioBase;

struct AudioContext
{
	struct USB2IFace *IUSB2;
	struct SignalSemaphore Lock;
	struct AudioBase *Device;
};

struct AudioDriverBase
{
	struct Library Library;
	PTR SegList;
	struct AudioContext Context;
};

struct AudioBase
{
	struct Library Library;
	U16 Pad;
	PTR SegList;
	struct AudioContext *Context;
	struct SignalSemaphore Semaphore;
	struct AudioUnit *Units[AUDIO_MAX_UNITS];
};

struct AudioUnit
{
	struct AudioContext *Context;
	struct AudioBase *Base;
	struct USB2_DriverMessage *StartMsg;
	struct USB2_Register *Register;
	struct USB2_EPResource *Stream;
	struct MsgPort *BeginPort;
	struct IOStdReq *ReadActive;
	const U8 *RateDescriptor;
	struct AudioFormat Format;
	U32 UnitNr;
	U32 OpenCount;
	U32 DataUsed;
	U32 PacketBytes;
	U32 PacketCount;
	U32 IntervalUS;
	U32 BufferedBytes;
	U32 EmptyBatches;
	U32 EmptyFailedPackets;
	U32 BufferOffset;
	U32 WriteBytes;
	U32 WritePhase;
	U16 IsoPacketLengths[AUDIO_PACKET_COUNT];
	U8 InterfaceNr;
	U8 AltNr;
	U8 EndPoint;
	U8 Detached;
	U8 Released;
	U8 ReadPending;
	U8 ActiveAlt;
	U8 FrequencyControl;
	U8 RatePending;
	U8 RateValid;
};

SEC_CODE U32 Audio_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
SEC_CODE S32 Audio_CreateDevice( struct AudioContext *ctx );
SEC_CODE struct AudioUnit *Audio_Alloc( struct AudioContext *ctx, struct USB2_DriverMessage *msg );
SEC_CODE void Audio_Free( struct AudioUnit *unit );
SEC_CODE void Audio_Main( struct AudioUnit *unit );
SEC_CODE S32 Audio_Parse( struct AudioUnit *unit );
SEC_CODE S32 Audio_Frequency( struct AudioUnit *unit, U32 sample_rate );
SEC_CODE S32 Audio_Rate_Valid( struct AudioUnit *unit, U32 sample_rate );
SEC_CODE void Audio_Read_Start( struct AudioUnit *unit );
SEC_CODE void Audio_Read_Done( struct AudioUnit *unit );
SEC_CODE void Audio_Read_Reply( struct AudioUnit *unit, S8 error );
SEC_CODE void Audio_Read_Buffered( struct AudioUnit *unit );
SEC_CODE void Audio_Write_Start( struct AudioUnit *unit );
SEC_CODE void Audio_Write_Done( struct AudioUnit *unit );
SEC_CODE void Audio_Begin( struct AudioUnit *unit );
SEC_CODE void Audio_Abort( struct AudioUnit *unit );
SEC_CODE void Audio_IO_Stop( struct USB2_IORequest *ioreq );

SEC_CODE struct AudioBase *Audio_Manager_Init( struct AudioBase *base, PTR seglist, struct ExecIFace *IExec );
SEC_CODE U32 Audio_Manager_Obtain( struct DeviceManagerInterface *Self );
SEC_CODE U32 Audio_Manager_Release( struct DeviceManagerInterface *Self );
SEC_CODE S32 Audio_Manager_Open( struct DeviceManagerInterface *Self, struct IORequest *ioreq, U32 unitnr, U32 flags );
SEC_CODE PTR Audio_Manager_Close( struct DeviceManagerInterface *Self, struct IORequest *ioreq );
SEC_CODE PTR Audio_Manager_Expunge( struct DeviceManagerInterface *Self );
SEC_CODE void Audio_Manager_BeginIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq );
SEC_CODE void Audio_Manager_AbortIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq );

SEC_CODE struct AudioDriverBase *Driver_Init( PTR dummy, PTR seglist, struct ExecBase *sysbase );
SEC_CODE U32 Driver_Obtain( struct LibraryManagerInterface *Self );
SEC_CODE U32 Driver_Release( struct LibraryManagerInterface *Self );
SEC_CODE PTR Driver_Open( struct LibraryManagerInterface *Self, U32 version );
SEC_CODE PTR Driver_Close( struct LibraryManagerInterface *Self );
SEC_CODE PTR Driver_Expunge( struct LibraryManagerInterface *Self );
SEC_CODE U32 Driver_USB_Obtain( struct USB2DriverIFace *Self );
SEC_CODE U32 Driver_USB_Release( struct USB2DriverIFace *Self );

#endif
