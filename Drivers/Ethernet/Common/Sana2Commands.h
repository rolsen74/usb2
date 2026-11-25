
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_SANA2_COMMANDS_H
#define USB2_SANA2_COMMANDS_H

#include <devices/sana2.h>

// Shared packet cancellation also handles wireless management frames.
#define S2_READMGMT 0xc015
#define S2_WRITEMGMT 0xc016

void Sana2_FreeList( struct List *list );
void Sana2_TrackType( struct List *list, struct IOSana2Req *ioreq );
void Sana2_AccountType( struct List *list, uint32 type, uint32 bytes, int receive, int dropped );
int Sana2_ValidMulticastAddress( const uint8 *address );
void Sana2_MulticastAddress( struct List *list, struct IOSana2Req *ioreq, uint32 full_error );
void Sana2_MulticastRange( struct List *list, struct IOSana2Req *ioreq );
int Sana2_MulticastMatch( struct List *list, const uint8 *address );

void Sana2_GetSpecialStats( struct IOSana2Req *ioreq, const struct Sana2SpecialStatRecord *record );
void Sana2_GetGlobalStats( struct IOSana2Req *ioreq, const struct Sana2DeviceStats *sample );
void Sana2_GetExtendedGlobalStats( struct IOSana2Req *ioreq, const struct Sana2ExtDeviceStats *sample, uint32 available );
void Sana2_GetPacketStats( struct IOSana2Req *ioreq, uint64 received, uint64 sent,
	uint64 bad, uint64 unknown, const struct TimeVal *last_start );
void Sana2_GetExtendedPacketStats( struct IOSana2Req *ioreq, uint64 received, uint64 sent,
	uint64 bad, uint64 unknown, int partial, const struct TimeVal *last_start );
int Sana2_SetCopyHook( struct IOSana2Req *ioreq, struct Hook **slot, int replace );
int Sana2_CopyPacket( struct IOSana2Req *ioreq, struct Hook *hook, APTR callback,
	ULONG method, APTR to, APTR from, uint32 length );
void Sana2_GetCryptTypes( struct IOSana2Req *ioreq, const uint8 *types, uint32 count );

struct TimerIFace;
void Sana2_GetTime( struct TimeVal *time );
int Sana2_QueueThroughput( struct List *list, struct IOSana2Req *ioreq, struct TimerIFace *timer,
	int copy_partial );
void Sana2_AccountThroughput( struct Sana2ThroughputStats *stats, const struct TimeVal *time,
	uint32 sent, uint32 received );
void Sana2_UpdateThroughput( struct List *list, uint32 sent, uint32 received );

#define SANA2_COMMON_EVENTS ( S2EVENT_ONLINE | S2EVENT_OFFLINE | S2EVENT_ERROR | \
	S2EVENT_HARDWARE | S2EVENT_RX | S2EVENT_TX | S2EVENT_BUFF )

typedef void (*Sana2Reply)( APTR context, struct IOSana2Req *request );
typedef int (*Sana2ReceiveCopy)( APTR context, struct IOSana2Req *request, const uint8 *data );
typedef int (*Sana2ReceiveFilter)( struct IOSana2Req *request, const uint8 *data );

#define SANA2_RX_TYPED 1U
#define SANA2_RX_COPIED 2U
#define SANA2_RX_COPY_FAILED 4U

uint32 Sana2_DeliverReads( struct List *list, const uint8 *frame, uint32 bytes, uint32 type,
	uint8 flags, int allow_orphan, int length_types, Sana2ReceiveFilter filter,
	Sana2ReceiveCopy copy, Sana2Reply reply, APTR context );
int32 Sana2_AbortPendingWithReply( struct List *list, struct IORequest *request, APTR opener,
	int32 error, uint32 wire, Sana2Reply reply, APTR context );
void Sana2_SignalEventWithReply( struct List *list, uint32 events, Sana2Reply reply, APTR context );
int32 Sana2_AbortPending( struct List *list, struct IORequest *request, APTR opener, int32 error, uint32 wire );
void Sana2_AbortPackets( struct List *list, int32 error, uint32 wire );
int Sana2_QueueEvent( struct List *list, struct IOSana2Req *ioreq, uint32 current, uint32 supported );
void Sana2_SignalEvent( struct List *list, uint32 events );

#endif
