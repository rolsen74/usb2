
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8188_ALL_H
#define RTL8188_ALL_H

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/usb2.h>
#include <proto/utility.h>
#include <interfaces/timer.h>
#include <interfaces/usb2eth.h>

#include <devices/newstyle.h>
#include <devices/sana2.h>
#include "Sana2Commands.h"
#include <devices/usb2wireless.h>
#include <exec/emulation.h>
#include <dos/dostags.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// --

#include "Types.h"
#include "Version.h"

#define ETH_CLASS_ADAPTER USB2ETH_ADAPTER_RTL8188
// --
#include "Debug.h"
#include "Crypto.h"

// --

struct BufferManagement
{
	struct Hook *bm_CopyHook;
	PTR bm_CopyToBuff;
	PTR bm_CopyFromBuff;
};

struct EthernetUnit
{
	struct Unit						unit_Unit;
	ULONG							unit_UnitNr;

	struct Task *					unit_TaskAdr;

	struct USB2_Register *			unit_USB_Register;
	struct USB2_Interface *			unit_USB_Interface;

	struct USB2_EPResource *		unit_Bulk_Tx_Resource;
	struct USB2_EPResource *		unit_Bulk_Rx_Resource;

	struct SignalSemaphore			unit_CommandLock;
	struct List						unit_Pending;
	struct List unit_MulticastRanges;
	struct List unit_TrackedTypes;
	struct List						unit_Multicast;
	struct TimeVal unit_LastStart;
	U8 unit_BringupStarted;
	U64								unit_PacketsRx;
	U64								unit_PacketsTx;
	U64								unit_UnknownPackets;

	U8								unit_StationAddress[6];
	U8								unit_BSSID[6];
	U8								unit_CCKPower[4];
	struct RTL8188EUKey				unit_Keys[RTL8188EU_KEY_COUNT];
	struct WirelessIGTK				unit_IGTK[WIRELESS_IGTK_COUNT];
	U64								unit_TKIPLastFailure;
	U64								unit_TKIPBlockedUntil;
	U16								unit_AssocID;
	U8								unit_Encryption;
	U8								unit_Channel;
	U8								unit_RequireOperState;
	U8								unit_Operational;
	U8								unit_SAE;
	U8								unit_Configured;
	U8 unit_WiFiStartPending;
	U8								unit_Exclusive;
	U8								unit_Online;
	U8								unit_TxReady;
	U8								unit_StationAddressValid;
	U8								unit_CrystalCap;
	U8								unit_RadioReady;
	U8								unit_RxActive;
	U8								unit_FirmwareRunning;
	U8								unit_Detached;
};

// --



extern struct TimerIFace *IRTL8188Timer;
extern struct Interface *INewlib;
extern struct Library *NewlibBase;

struct EthernetUnit *Unit_Alloc( void );

S32		Resources_Init( void );
void	Resources_Free( void );

S32	Process_Init( struct EthernetUnit *unit, struct USB2_DriverMessage *msg );
void	Process_Free(			struct EthernetUnit *unit );
void	Process_Main(			struct EthernetUnit *unit );
S32		RTL8188EU_Probe(		struct USB2_EPResource *control );
S32		RTL8188EU_RegisterIO( struct USB2_EPResource *control, U32 command, U16 address, U8 *data, U16 length );
S32		RTL8188EU_LoadFirmware( struct EthernetUnit *unit );
void	RTL8188EU_StopFirmware( struct USB2_EPResource *control );
S32		RTL8188EU_ReadStationAddress( struct EthernetUnit *unit );
void	RTL8188EU_SubmitRX( struct EthernetUnit *unit );
void	RTL8188EU_CompleteRX( struct EthernetUnit *unit );
void	Sana2BeginIO(			struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void	Unit_Free(				struct EthernetUnit *unit );

// --

#endif
