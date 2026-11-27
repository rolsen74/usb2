/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DRIVERS_AUDIO_AUDIO_H
#define DRIVERS_AUDIO_AUDIO_H

#include <interfaces/usb2unitidentity.h>

#include "AudioDevice.h"

#define AUDIO_PACKET_COUNT 256
#define AUDIO_BATCH_US 32000U
#define AUDIO_WRITE_BATCH_US 16000U
#define AUDIO_USB_REQUESTS 3
#define AUDIO_WRITE_REQUESTS AUDIO_MAX_QUEUED_WRITES
#define AUDIO_XHCI_PACKET_COUNT 254U
#define AUDIO_CLOCK_PROBE_LIMIT 4U
#define AUDIO_READ_TIMEOUT ( 5U * 1000000U )
#define AUDIO_EMPTY_BATCH_LIMIT 4
#define AUDIO_IO_ABORT 0x80
#define AUDIO_PROTOCOL_UAC2 0x20

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

struct AudioSlot
{
	struct AudioUnit *Unit;
	struct USB2UnitIdentity Identity;
	U32 Rate;
	U8 Open;
};

struct AudioBase
{
	struct Library Library;
	U16 Pad;
	PTR SegList;
	struct AudioContext *Context;
	struct SignalSemaphore Semaphore;
	struct AudioSlot Slots[AUDIO_MAX_UNITS];
};

struct AudioWrite
{
	struct IOStdReq *Request;
	U32 Queued;
	U32 Done;
	U32 Actual;
};

struct AudioReadBatch
{
	U16 PacketLengths[AUDIO_PACKET_COUNT];
	U32 Offset;
	U32 Bytes;
	U32 Missed;
	S8 Error;
	U8 Pending;
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
	const U8 *ParseNext;
	struct AudioWrite Writes[AUDIO_WRITE_REQUESTS];
	struct AudioWrite *WriteOwners[AUDIO_USB_REQUESTS];
	struct AudioReadBatch Reads[AUDIO_USB_REQUESTS];
	struct AudioFormat Format;
	struct AudioEndpoint Endpoint;
	char PortID[AUDIO_ENDPOINT_ID_SIZE];
	struct AudioGain Gain;
	U32 UnitNr;
	U32 ReconnectRate;
	U32 DataUsed;
	U32 PacketBytes;
	U32 PacketCount;
	U32 IntervalUS;
	U32 SingleSplitRate;
	U32 ReadHead;
	U32 ReadCount;
	U32 ReadSlots;
	U32 WriteHead;
	U32 WriteCount;
	U32 WriteSlot;
	U32 EmptyBatches;
	U32 EmptyFailedPackets;
	U32 WriteBytes[AUDIO_USB_REQUESTS];
	U32 WritePhase;
	U16 WritePacketLengths[AUDIO_USB_REQUESTS][AUDIO_PACKET_COUNT];
	U16 ClockKeys[AUDIO_CLOCK_PROBE_LIMIT];
	U32 ClockRates[AUDIO_CLOCK_PROBE_LIMIT];
	U32 ClockProbes;
	U8 InterfaceNr;
	U8 Protocol;
	U8 AltNr;
	U8 EndPoint;
	U8 Detached;
	U8 ReadDiscontinuity;
	U8 ResetStream;
	U8 ActiveAlt;
	U8 FrequencyControl;
	U8 RatePending;
	U8 RateValid;
	U8 TerminalLink;
	U8 ClockInterface;
	U8 ClockEntity;
	U8 GainInterface;
	U8 GainEntity;
	U8 GainChannel;
	U8 GainValid;
};

SEC_CODE U32 Audio_Entry( struct USB2DriverIFace *Self, struct USB2_DriverMessage *msg );
SEC_CODE S32 Audio_CreateDevice( struct AudioContext *ctx );
SEC_CODE struct AudioUnit *Audio_Alloc( struct AudioContext *ctx, struct USB2_DriverMessage *msg );
SEC_CODE void Audio_Free( struct AudioUnit *unit );
SEC_CODE void Audio_Main( struct AudioUnit *unit );
SEC_CODE S32 Audio_Parse( struct AudioUnit *unit );
SEC_CODE S32 Audio_FindClock( struct AudioUnit *unit, U32 config_size, U8 terminal, U8 endpoint );
SEC_CODE U32 Audio_ClockRate( struct AudioUnit *unit );
SEC_CODE S32 Audio_Frequency( struct AudioUnit *unit, U32 sample_rate );
SEC_CODE void Audio_EndpointInit( struct AudioUnit *unit );
SEC_CODE void Audio_EndpointList( struct AudioBase *base, struct IOStdReq *ioreq );
SEC_CODE S32 Audio_EndpointFind( struct AudioBase *base, const char *id, struct AudioUnit **selected );
SEC_CODE S32 Audio_EndpointOpen( struct AudioBase *base, struct IORequest *ioreq, struct AudioUnit **selected );
SEC_CODE void Audio_EndpointRates( struct AudioUnit *unit );
SEC_CODE S32 Audio_Rate_Valid( struct AudioUnit *unit, U32 sample_rate );
SEC_CODE void Audio_FindGain( struct AudioUnit *unit, U32 configSize );
SEC_CODE void Audio_GainCommand( struct AudioUnit *unit, struct IOStdReq *ioreq );
SEC_CODE void Audio_Read_Start( struct AudioUnit *unit );
SEC_CODE void Audio_Read_Done( struct AudioUnit *unit );
SEC_CODE S8 Audio_Read_Reply( struct AudioUnit *unit, S8 error );
SEC_CODE void Audio_Read_Buffered( struct AudioUnit *unit );
SEC_CODE void Audio_Read_Stop( struct AudioUnit *unit );
SEC_CODE void Audio_Write_Start( struct AudioUnit *unit );
SEC_CODE void Audio_Write_Done( struct AudioUnit *unit );
SEC_CODE S8 Audio_Write_Reply( struct AudioUnit *unit, S8 error );
SEC_CODE void Audio_Write_Stop( struct AudioUnit *unit, S8 error );
SEC_CODE void Audio_Write_Account( struct AudioUnit *unit, U32 slot );
SEC_CODE void Audio_Begin( struct AudioUnit *unit );
SEC_CODE void Audio_Abort( struct AudioUnit *unit );

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
