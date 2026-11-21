#ifndef SIS163U_ALL_H
#define SIS163U_ALL_H

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/usb2.h>
#include <proto/utility.h>
#include <devices/newstyle.h>
#include <devices/sana2.h>
#include <dos/dostags.h>
#include <stddef.h>
#include <string.h>

#include "Types.h"
#include "Version.h"
#include "Base.h"
#include "Task.h"

#define SIS163U_VENDOR 0x0d8e
#define SIS163U_PRODUCT 0x0163
#define SIS163U_MTU 1500
#define SIS163U_CHANNEL_MAX 13
#define SIS163U_RX_CHECK_READS 16
#define SIS163U_RX_BUFFER_SIZE 2400
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

struct MulticastAddress
{
	struct Node ma_Node;
	U32 ma_UseCount;
	U8 ma_Address[6];
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
	enum TaskState unit_TaskState;
	U32 unit_UnitNr;
	S32 unit_Exclusive;
	S32 unit_Detached;
};

S32 Resources_Init( struct EthernetBase *ethBase );
void Resources_Free( struct EthernetBase *ethBase );
struct EthernetUnit *Unit_Alloc( U32 unitnr );
void Unit_Free( struct EthernetUnit *unit );
S32 Unit_Startup( struct EthernetUnit *unit );
S32 Unit_Startup_USB( struct EthernetUnit *unit );
void Ethernet_Process_Entry( void );
S32 Process_Init( struct EthernetUnit *unit );
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
void SiS163U_StopRX( struct EthernetUnit *unit );
S32 SiS163U_SetChannel( struct EthernetUnit *unit, U32 channel );
S32 SiS163U_WaitRegister( struct USB2_EPResource *control, U16 address, U32 mask );
S32 SiS163U_InitBaseband( struct USB2_EPResource *control );
S32 SiS163U_WriteBaseband( struct USB2_EPResource *control, U8 address, U8 value, U32 ofdm );
S32 SiS163U_WriteRF( struct USB2_EPResource *control, U8 address, U16 value );
S32 SiS163U_ProgramStationAddress( struct EthernetUnit *unit );
S32 SiS163U_ReadRegister( struct USB2_EPResource *control, U16 address, U32 *value );
void Sana2BeginIO( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void AbortPendingCommands( struct EthernetUnit *unit, struct IORequest *request, struct BufferManagement *opener );
void SignalSana2Event( struct EthernetUnit *unit, U32 events );
struct IOSana2Req *SiS163U_FindMgmtRead( struct EthernetUnit *unit );
void _cmd_4000_NSCmd_DeviceQuery( struct IOStdReq *ioreq );

extern struct IORequest *USB2IOReq;
extern struct MsgPort *USB2MsgPort;
extern struct Interface *INewlib;
extern struct Library *NewlibBase;

#endif
