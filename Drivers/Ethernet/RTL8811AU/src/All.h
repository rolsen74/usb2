
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8811AU_ALL_H
#define RTL8811AU_ALL_H

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>
#include <interfaces/usb2.h>
#include <devices/newstyle.h>
#include <devices/usb2wireless.h>
#include "Sana2Commands.h"
#include "Wireless_ReceiveQueue.h"
#include "WirelessSana2.h"
#include "WirelessCrypto.h"

#define RTL8811AU_VENDOR_ID  0x2357
#define RTL8811AU_PRODUCT_ID 0x011e
#define RTL8811AU_MAC_SIZE   6
#define RTL8811AU_MTU        1500
#define RTL8811AU_ETH_HEADER 14
#define RTL8811AU_TX_DESC_SIZE 40
#define RTL8811AU_FRAME_MAX ( RTL8811AU_MTU + 32 )
#define RTL8811AU_TX_BUFFER_SIZE ( RTL8811AU_TX_DESC_SIZE + 8 + RTL8811AU_FRAME_MAX + WIRELESS_CCMP_IV + WIRELESS_CCMP_MIC )
#define RTL8811AU_EAPOL 0x888e

struct RTL8811AU_Key
{
	U64 TxNumber;
	U64 RxNumber[WIRELESS_NONQOS + 1];
	U8 RoundKeys[176];
	U8 Installed;
	U8 Active;
};

#define RTL8811AU_SYS_ISO    0x0000
#define RTL8811AU_SYS_FUNC   0x0002
#define RTL8811AU_POWER      0x0004
#define RTL8811AU_SYS_CLOCK  0x0008
#define RTL8811AU_RESERVED   0x001c
#define RTL8811AU_RF_CTRL    0x001f
#define RTL8811AU_LDO        0x0020
#define RTL8811AU_EFUSE_CTRL 0x0030
#define RTL8811AU_FW_CTRL    0x0080
#define RTL8811AU_SYS_CFG    0x00f0
#define RTL8811AU_MAC_CTRL   0x0100
#define RTL8811AU_TX_PAUSE   0x0522

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
	struct SignalSemaphore unit_CommandLock;
	struct List unit_Openers;
	struct List unit_Pending;
	struct TimeVal unit_LastStart;
	struct USB2_EPResource *unit_Control;
	struct USB2_EPResource *unit_Tx;
	struct USB2IFace *unit_USB2;
	struct RTL8811AU_Key unit_Keys[4];
	U64 unit_PacketsRx;
	U64 unit_PacketsTx;
	U64 unit_UnknownPackets;
	ULONG unit_OpenCount;
	ULONG unit_UnitNumber;
	U16 unit_AssocID;
	U16 unit_TxSequence;
	U8 unit_StationAddress[RTL8811AU_MAC_SIZE];
	U8 unit_BSSID[RTL8811AU_MAC_SIZE];
	U8 unit_Channel;
	U8 unit_Encryption;
	U8 unit_Online;
	U8 unit_Operational;
	U8 unit_RequireOperState;
	U8 unit_TxReady;
	U8 unit_ManualOffline;
	U8 unit_StartAuto;
	U8 unit_Exclusive;
	U8 unit_Configured;
	U8 unit_Detached;
	U32 unit_RxPending;
	U8 unit_RadioReady;
	U8 unit_BeaconReceived;
};

S32 RTL8811AU_RegisterIO( struct USB2_EPResource *control, U16 command, U16 address, PTR data, U16 length );
S32 RTL8811AU_ReadReg( struct USB2_EPResource *control, U16 address, U16 size, U32 *value );
S32 RTL8811AU_WriteReg( struct USB2_EPResource *control, U16 address, U16 size, U32 value );
S32 RTL8811AU_Update8( struct USB2_EPResource *control, U16 address, U8 clear, U8 set );
S32 RTL8811AU_PollReg( struct USB2_EPResource *control, U16 address, U16 size, U32 mask, U32 wanted, U32 polls, U32 *value );
S32 RTL8811AU_ReadEFuse( struct USB2_EPResource *control, U8 *rom, U32 size );
S32 RTL8811AU_PowerOn( struct USB2_EPResource *control );
void RTL8811AU_PowerOff( struct USB2_EPResource *control );
S32 RTL8811AU_LoadFirmware( struct USB2_EPResource *control );
S32 RTL8811AU_UploadFirmware( struct USB2_EPResource *control, U8 *image, U32 size );
S32 RTL8811AU_ResetMCU( struct USB2_EPResource *control );

struct BufferManagement *FindOpener( struct EthernetUnit *unit, PTR cookie );
void FreeOpener( struct BufferManagement *opener );
void ReportUnitOffline( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

#endif
