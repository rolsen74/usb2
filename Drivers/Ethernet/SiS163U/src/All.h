#ifndef SIS163U_ALL_H
#define SIS163U_ALL_H

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/usb2.h>
#include <interfaces/usb2eth.h>
#include <proto/utility.h>
#include <devices/newstyle.h>
#include <devices/sana2.h>
#include "Sana2Commands.h"
#include <dos/dostags.h>
#include <stddef.h>
#include <string.h>

#include "Types.h"
#include "Version.h"

#define ETH_CLASS_ADAPTER USB2ETH_ADAPTER_SIS163U

#define SIS163U_VENDOR 0x0d8e
#define SIS163U_PRODUCT 0x0163
#define SIS163U_MTU 1500
#define SIS163U_CHANNEL_MAX 13
#define SIS163U_RX_CHECK_READS 16
#define SIS163U_RX_BUFFER_SIZE 2400
#define SIS163U_WLAN_HEADER 24
#define SIS163U_TX_DESC_SIZE 12
#define SIS163U_TX_BUFFER_SIZE 1600
#define SIS163U_MAC_RX_MODE 0x54922000

#define USBDEBUG( ... ) ((void)0)
#define USBERROR( fmt, ... ) DebugPrintF( fmt "\n", ##__VA_ARGS__ )
#define USBINFO( fmt, ... ) DebugPrintF( fmt "\n", ##__VA_ARGS__ )

struct BufferManagement
{
	struct Hook *bm_CopyHook;
	PTR bm_CopyToBuff;
	PTR bm_CopyFromBuff;
};

struct EthernetUnit
{
	struct Unit unit_Unit;
	struct Task *unit_TaskAdr;
	struct USB2_Register *unit_USB_Register;
	struct USB2_EPResource *unit_Bulk_Rx_Resource;
	struct USB2_EPResource *unit_Bulk_Tx_Resource;
	U32 unit_RxCheckReads;
	S32 unit_RxActive;
	S32 unit_RxReady;
	S32 unit_ManualScan;
	struct SignalSemaphore unit_CommandLock;
	struct List unit_Pending;
	struct List unit_MulticastRanges;
	struct List unit_TrackedTypes;
	struct List unit_Multicast;
	U64 unit_PacketsRx;
	U64 unit_PacketsTx;
	U64 unit_UnknownPackets;
	U8 unit_StationAddress[6];
	U8 unit_StationAddressValid;
	U8 unit_StationAddressProgrammed;
	U16 unit_BoardOptions;
	U8 unit_RFType;
	U8 unit_ChipRevision;
	U8 unit_Channel;
	S32 unit_RadioInitialized;
	U8 unit_Configured;
	ULONG unit_UnitNr;
	S32 unit_Exclusive;
	S32 unit_Detached;
};

S32 Resources_Init( void );
void Resources_Free( void );
struct EthernetUnit *Unit_Alloc( void );
void Unit_Free( struct EthernetUnit *unit );
S32 Process_Init( struct EthernetUnit *unit, struct USB2_DriverMessage *msg );
void Process_Main( struct EthernetUnit *unit );
void Process_RadioCommands( struct EthernetUnit *unit );
void Process_Free( struct EthernetUnit *unit );
struct TimerIFace;

S32 SiS163U_RegisterIO( struct USB2_EPResource *control, U16 command, U8 requestCode, U16 address, U8 *data );
S32 SiS163U_WriteRegister( struct USB2_EPResource *control, U16 address, U32 value );
S32 SiS163U_ReadEEPROM( struct USB2_EPResource *control, struct TimerIFace *timer, U8 word, U16 *data );
S32 SiS163U_ReadStationAddress( struct EthernetUnit *unit );
S32 SiS163U_StartRadio( struct EthernetUnit *unit );
S32 SiS163U_StopRadio( struct EthernetUnit *unit );
void SiS163U_StartRXCheck( struct EthernetUnit *unit );
void SiS163U_SubmitRXCheck( struct EthernetUnit *unit );
void SiS163U_CompleteRXCheck( struct EthernetUnit *unit );
void SiS163U_LogRXState( struct EthernetUnit *unit, const char *stage );
void SiS163U_WriteMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void SiS163U_StopRX( struct EthernetUnit *unit );
S32 SiS163U_SetChannel( struct EthernetUnit *unit, U32 channel );
S32 SiS163U_WaitRegister( struct USB2_EPResource *control, U16 address, U32 mask );
S32 SiS163U_InitBaseband( struct USB2_EPResource *control );
S32 SiS163U_WriteBaseband( struct USB2_EPResource *control, U8 address, U8 value, U32 ofdm );
S32 SiS163U_WriteRF( struct USB2_EPResource *control, U8 address, U16 value );
S32 SiS163U_ProgramStationAddress( struct EthernetUnit *unit );
S32 SiS163U_ReadRegister( struct USB2_EPResource *control, U16 address, U32 *value );
void Sana2BeginIO( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
struct IOSana2Req *SiS163U_FindMgmtRead( struct EthernetUnit *unit );

extern struct Interface *INewlib;
extern struct Library *NewlibBase;

#endif
