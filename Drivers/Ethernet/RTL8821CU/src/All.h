
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8821CU_ALL_H
#define RTL8821CU_ALL_H

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <interfaces/usb2.h>

#include <devices/newstyle.h>
#include <devices/sana2.h>
#include "Sana2Commands.h"
#include <devices/usb2wireless.h>

// --

#include "Version.h"

#define RTL8821CU_VENDOR_ID 0x0bda
#define RTL8821CU_PRODUCT_ID 0xc820
#define RTL8821CU_INTERFACE 2
#define RTL8821CU_TX_PIPES 3
#define RTL8821CU_ADDR_SIZE 6
#define RTL8821CU_MTU 1500
#define RTL8821CU_ETH_HEADER 14
#define RTL8821CU_WLAN_HEADER 24
#define RTL8821CU_LLC_HEADER 8
#define RTL8821CU_TX_DESC_SIZE 48
#define RTL8821CU_RX_DESC_SIZE 24
#define RTL8821CU_RX_BUFFER_SIZE 32768
#define RTL8821CU_TX_BUFFER_SIZE ( RTL8821CU_TX_DESC_SIZE + RTL8821CU_WLAN_HEADER + RTL8821CU_LLC_HEADER + RTL8821CU_MTU )
#define RTL8821CU_FIRMWARE_CHUNK_SIZE 4096
#define RTL8821CU_FIRMWARE_BUFFER_SIZE ( RTL8821CU_TX_DESC_SIZE + RTL8821CU_FIRMWARE_CHUNK_SIZE + 1 )
#define RTL8821CU_TX_BEST_EFFORT 2

#include "WirelessSana2.h"

// --

struct BufferManagement
{
	struct Node bm_Node;
	struct Hook *bm_CopyHook;
	PTR bm_CopyToBuff;
	PTR bm_CopyFromBuff;
	struct List bm_Multicast;
	struct List bm_TrackedTypes;
};

struct EthernetUnit
{
	struct Unit unit_Unit;
	ULONG unit_UnitNr;
	struct USB2_Register *unit_USB_Register;
	struct USB2_EPResource *unit_Bulk_In;
	struct USB2_EPResource *unit_Bulk_Out[RTL8821CU_TX_PIPES];
	struct USB2_EPResource *unit_Interrupt_In;
	struct SignalSemaphore unit_CommandLock;
	struct List unit_Openers;
	struct List unit_Pending;
	U64 unit_PacketsRx;
	U64 unit_PacketsTx;
	U64 unit_BadPackets;
	U64 unit_UnknownPackets;
	U8 unit_StationAddress[RTL8821CU_ADDR_SIZE];
	U8 unit_BSSID[RTL8821CU_ADDR_SIZE];
	U8 unit_FirmwareRunning;
	U8 unit_Online;
	U8 unit_RxPending;
	U8 unit_AddressValid;
	U8 unit_Detached;
	U8 unit_Exclusive;
};

S32 Process_Setup_Device( struct EthernetUnit *unit, struct USB2_DriverMessage *msg );
void Process_Free( struct EthernetUnit *unit, struct USB2IFace *usb2 );
void Process_PacketIO( struct EthernetUnit *unit, struct USB2IFace *usb2 );
void RTL8821CU_StopRX( struct EthernetUnit *unit );
void RTL8821CU_ChecksumTX( U8 *descriptor );
U32 RTL8821CU_BuildTX( struct EthernetUnit *unit, struct IOSana2Req *ioreq, U8 *buffer );
void RTL8821CU_DecodeRX( struct EthernetUnit *unit, const U8 *buffer, U32 length );
void RTL8821CU_ReceiveData( struct EthernetUnit *unit, const U8 *frame, U32 length );
S32 RTL8821CU_CopyPacket( struct IOSana2Req *ioreq, PTR to, PTR from, U32 length, S32 rx );
S32 RTL8821CU_SameAddress( const U8 *left, const U8 *right );
S32 RTL8821CU_HasMulticast( struct EthernetUnit *unit, const U8 *address );

S32 RTL8821CU_Probe( struct USB2_EPResource *control );
S32 RTL8821CU_ReadEFuseAddress( struct USB2_EPResource *control, U8 *address );
S32 RTL8821CU_ReadStationAddress( struct EthernetUnit *unit );
S32 RTL8821CU_RegisterIO( struct USB2_EPResource *control, U16 command, U16 address, PTR data, U16 length );
S32 RTL8821CU_LoadFirmware( struct EthernetUnit *unit );
S32 RTL8821CU_PowerOn( struct USB2_EPResource *control );
S32 RTL8821CU_PowerOff( struct USB2_EPResource *control );

struct BufferManagement *FindOpener( struct EthernetUnit *unit, PTR cookie );
void FreeOpener( struct BufferManagement *opener );
void ReportUnitOffline( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 ValidStationAddress( const U8 *address );

#endif
