
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef RTL8152_UNIT_H
#define RTL8152_UNIT_H

/**************************************************************************/

struct IONode
{
	struct Node 		Node;
	uint8 *			    Buffer;
	U32			    BufferSize;
	int				Active;
	int				AbortError;
	U32 AbortWire;

	struct IOSana2Req * Sana2IOReq;

	struct USB2_IORequest *   USBRequest;
};

#define RTL8152_REQUEST_COUNT 3
#define RTL8152_RX_BUFFER_SIZE 16384
#define RTL8152_RX_DESCRIPTOR_SIZE 24
#define RTL8152_TX_DESCRIPTOR_SIZE 8
#define RTL8152_TX_BUFFER_SIZE ( ETHERNET_MTU + RTL8152_TX_DESCRIPTOR_SIZE )
#define RTL8152_RX_LENGTH_MASK 0x7fff
#define RTL8152_TX_SEGMENTS 0xc0000000U

struct devUnit
{
	struct Unit				unit_Base;
	ULONG					unit_UnitNumber;

	int					unit_Running;
	U8 unit_AcceptIO;
	U8 unit_Exclusive;
	struct List unit_Requests;
	struct SignalSemaphore unit_DispatchLock;

	uint16					unit_ChipVersion;

	int					unit_Online;
	int					unit_Configured;
	int					unit_Support_GMII;

	struct List				unit_RxList;

	struct List				unit_TxList;
	
	struct List				unit_EventsList;

	struct List				unit_SampleList;
	struct List				unit_MultiCastList;
	struct List unit_MulticastRanges;
	struct List unit_TrackedTypes;
	U32					unit_MultiCastRejects;

	struct MsgPort *		unit_BeginIOMsgPort;
	U32					unit_BeginIOMsgPortBit;


	struct USB2IFace *unit_IUSB2;
	struct USB2_Interface *unit_USBInterface;
	struct USB2_Register *unit_USBRegister;
	struct USB2_IORequest *unit_USBControlIOReq;
	S32 unit_USBControlError;
	struct MsgPort *unit_LinkPort;
	struct TimeRequest *unit_LinkTimer;
	U8 unit_LinkTimerPending;
	U8 unit_LinkUp;
	U8 unit_RxHalted;
	U8 unit_TxHalted;
	U8 unit_RxRecovery;
	U8 unit_TxRecovery;
	U8 unit_ControlNeedsSetup;
	struct USB2_EPResource *unit_USBBulkIn;
	struct USB2_EPResource *unit_USBBulkOut;
	struct List unit_USBBulkInActiveList;
	struct List unit_USBBulkInFreeList;
	struct List unit_USBBulkOutActiveList;
	struct List unit_USBBulkOutFreeList;

	uint8					unit_DefaultAddress[6];			// default MAC (hardware) address
	uint8					unit_CurrentAddress[6];			// current MAC address

	uint64					unit_Reconfigurations;			// Statistics
	uint64					unit_Overruns;

	uint64					unit_BadData;
	uint64					unit_UnknownTypesReceived;
	uint64					unit_PacketsReceived;
	uint64					unit_PacketsSent;
	struct TimeVal			unit_LastStart;
};

/**************************************************************************/

void UpdateThroughput( struct devUnit *unit, U32 sent, U32 received );
S32 RTL8152_OpenLinkTimer( struct devUnit *unit );
void RTL8152_CloseLinkTimer( struct devUnit *unit );
void RTL8152_ArmLinkTimer( struct devUnit *unit );
void RTL8152_UpdateLink( struct devUnit *unit );

S32 RTL8152_SetMacAddress( struct devUnit *unit, const U8 *address );

// --

int Driver_Setup( struct devUnit *unit );

int unit_Process_Init( struct devUnit *unit, struct USB2_DriverMessage *msg );
void unit_Process_Free( struct devUnit *unit );
int unit_Process_InitMisc( struct devUnit *unit );
void unit_Process_FreeMisc( struct devUnit *unit );
int unit_Process_InitUSB( struct devUnit *unit, struct USB2_DriverMessage *msg );
void unit_Process_FreeUSB( struct devUnit *unit );
void unit_Process_MainLoop( struct devUnit *unit );
void Handle_BeginIOMessages( struct devUnit *unit );
void Handle_AbortIOMessages( struct devUnit *unit );
struct SanaRequest *FindSanaRequest( struct devUnit *unit, const struct IOSana2Req *request );
void ReplySanaRequest( APTR adapter, struct IOSana2Req *request );
void SignalSanaEvent( struct devUnit *unit, U32 events );
void Handle_USBFunction( struct devUnit *unit );
void Handle_BulkInMessages( struct devUnit *unit );
void Handle_BulkOutMessages( struct devUnit *unit );
struct IONode *AllocIONode( struct USB2_EPResource *resource, U32 index );
void FreeIONode( struct devUnit *unit, struct IONode *node );
void AbortPacketIO( struct devUnit *unit, U32 wire );
void Schedule_RxIOReq( struct devUnit *unit );
void Schedule_TxIOReq( struct devUnit *unit );
S32 AbortReqOnList( struct devUnit *unit, struct List *list, struct IOSana2Req *abort_ioreq, U32 wire_error );
void ReadPacket( struct devUnit *unit, U8 *data, U32 length );
int Copy_To_Buffer( struct devUnit *unit, struct IOSana2Req *ioreq, APTR buffer );
int SendPacket( struct devUnit *unit, struct IONode *node );
int Copy_From_Buffer( struct devUnit *unit, struct IOSana2Req *ioreq, APTR buffer );

#endif

