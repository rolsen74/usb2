#ifndef RT2870_ALL_H
#define RT2870_ALL_H

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/usb2.h>
#include <interfaces/usb2eth.h>
#include <proto/utility.h>
#include <interfaces/timer.h>
#include <devices/newstyle.h>
#include <devices/sana2.h>
#include "Sana2Commands.h"
#include <devices/usb2rt2870.h>
#include <exec/emulation.h>
#include <dos/dostags.h>
#include <stddef.h>
#include <string.h>

#include "Types.h"
#include "WirelessCrypto.h"
#include "Version.h"

#define ETH_CLASS_ADAPTER USB2ETH_ADAPTER_RT2870
#define ETH_CLASS_HAS_WORK_LOCK

#define RT2870_VENDOR 0x07d1
#define RT2870_PRODUCT 0x3c09
#define RT2870_AP_WCID 1
#define RT2870_MTU 1500
#define RT2870_READ_REGION 0x07
#define RT2870_READ_EEPROM 0x09
#define RT2870_MAX_FRAME 2304
#define RT2870_EAPOL_TYPE 0x888e
#define RT2870_EAPOL_MAX 2048
#define RT2870_KEY_COUNT 5
#define RT2870_PAIRWISE_KEY 1U
#define RT2870_GROUP_KEYS 0x1eU

#define USBDEBUG( ... ) ((void)0)
#define USBERROR( fmt, ... ) DebugPrintF( fmt "\n", ##__VA_ARGS__ )
#define USBINFO( fmt, ... ) DebugPrintF( fmt "\n", ##__VA_ARGS__ )

struct BufferManagement
{
	PTR bm_CopyToBuff;
	PTR bm_CopyFromBuff;
	struct Hook *bm_CopyHook;
};

struct RT2870Key
{
	union
	{
		U8 round_keys[176];
		U8 tkip_key[WIRELESS_TKIP_KEY_SIZE];
	};
	U64 rx_number[WIRELESS_NONQOS + 1];
};

struct EthernetUnit
{
	struct Unit unit_Unit;
	struct Task *unit_TaskAdr;
	struct USB2_Register *unit_USB_Register;
	struct USB2_EPResource *unit_BeaconResource;
	struct USB2_EPResource *unit_BulkOut;
	U32 unit_BeaconDMA;
	U32 unit_BeaconReads;
	U32 unit_BeaconBytes;
	U32 unit_BeaconTimeouts;
	U32 unit_BeaconsDelivered;
	S32 unit_BeaconEnabled;
	struct SignalSemaphore unit_CommandLock;
	struct SignalSemaphore unit_WorkLock;
	struct List unit_Pending;
	struct List unit_MulticastRanges;
	struct List unit_TrackedTypes;
	struct TimeVal unit_LastStart;
	U8 unit_RxHalted;
	U8 unit_TxHalted;
	U64 unit_PacketsRx;
	U64 unit_PacketsTx;
	U64 unit_UnknownPackets;
	U8 unit_StationAddress[6];
	U8 unit_Peer[6];
	U8 unit_LastBeaconBSSID[6];
	U16 unit_LastBeaconInterval;
	U16 unit_PeerBeaconInterval;
	S32 unit_TimingActive;
	S32 unit_PeerConfigured;
	S32 unit_DataOFDM;
	U8 unit_EAPOL[RT2870_EAPOL_MAX];
	U32 unit_EAPOLLength;
	U16 unit_AssocID;
	U8 unit_ActiveKeys;
	struct RT2870Key unit_Keys[RT2870_KEY_COUNT];
	struct WirelessIGTK unit_IGTK[WIRELESS_IGTK_COUNT];
	U64 unit_ManagementRxPN;
	S32 unit_SAE;
	S32 unit_TKIP;
	U64 unit_TKIPLastFailure;
	U64 unit_TKIPBlockedUntil;
	U64 unit_TxNumber;
	S32 unit_Online;
	S32 unit_Operational;
	S32 unit_M4Sent;
	U8 unit_TKIPRxReports;
	U8 unit_TKIPRxFailures;
	U8 unit_TKIPEAPOLReports;
	S32 unit_ReceivePending;
	S32 unit_TransmitEnabled;
	S32 unit_DataSent;
	S32 unit_DataReceived;
	U32 unit_DHCPSent;
	U32 unit_DHCPDelivered;
	U32 unit_DHCPDropped;
	U8 unit_Configured;
	U8 unit_WiFiStartPending;
	U8 unit_RxCheckReady;
	U8 unit_Channel;
	U8 unit_ManagementPID;
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
void Process_Free( struct EthernetUnit *unit );
S32 RT2870_Control( struct USB2_EPResource *control, U8 direction, U8 command, U16 value, U16 address, PTR data, U16 length );
S32 RT2870_ReadRegister( struct USB2_EPResource *control, U16 address, U32 *value );
S32 RT2870_WaitRegisterIdle( struct USB2_EPResource *control, U16 address, U32 busy, U32 *value );
S32 RT2870_TuneChannel( struct USB2_EPResource *control, U32 channel );
S32 RT2870_WriteWord( struct USB2_EPResource *control, U16 address, U16 value );
S32 RT2870_WriteRegister( struct USB2_EPResource *control, U16 address, U32 value );
S32 RT2870_LoadFirmware( struct USB2_EPResource *control, U32 chip );
S32 RT2870_ResetMAC( struct USB2_EPResource *control );
S32 RT2870_InitBaseband( struct USB2_EPResource *control, U32 asicVersion );
S32 RT2870_Probe( struct EthernetUnit *unit );
S32 RT2870_StartReceive( struct EthernetUnit *unit );
U64 RT2870_TKIPTime( void );
void RT2870_TKIPFailure( struct EthernetUnit *unit );
void RT2870_ClearKeys( struct EthernetUnit *unit );
S32 RT2870_TransmitFrame( struct EthernetUnit *unit, const U8 *frame, U32 frameLength, const U8 *bssid, S32 data, const char *report );
void RT2870_WriteManagement( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 RT2870_ReadBeacon( struct EthernetUnit *unit );
S32 RT2870_SetStationPeer( struct EthernetUnit *unit, S32 enable );
S32 RT2870_SetStationTiming( struct EthernetUnit *unit, U16 interval );
void RT2870_DropLink( struct EthernetUnit *unit, S32 detached );
void RT2870_StopBeacon( struct EthernetUnit *unit, S32 detached );
void RT2870_CacheEAPOL( struct EthernetUnit *unit, const U8 *frame, U32 length, U32 padding );
void RT2870_CacheEAPOLBody( struct EthernetUnit *unit, const U8 *body, U32 length );
S32 RT2870_DeliverEAPOL( struct EthernetUnit *unit );
void RT2870_ReceiveBeacon( struct EthernetUnit *unit, U8 *data, U32 length );
void RT2870_ReceiveData( struct EthernetUnit *unit, U8 *frame, U32 length, U32 padding );
U32 RT2870_DHCPType( const U8 *packet, U32 length, const U8 *station, S32 receive );
void RT2870_ReportDHCP( U32 *seen, U32 type, const char *result );
S32 RT2870_ReceiveRobustMgmt( struct EthernetUnit *unit, U8 *frame, U32 length, S32 decrypted );
S32 RT2870_ProtectData( struct EthernetUnit *unit, U8 *frame, U32 *length );
S32 RT2870_CopyPacket( struct IOSana2Req *ioreq, PTR to, PTR from, U32 length, S32 receive );
void RT2870_SetOnline( struct EthernetUnit *unit, S32 online );
void Process_RadioCommands( struct EthernetUnit *unit );
void Sana2BeginIO( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

extern struct Interface *INewlib;
extern struct Library *NewlibBase;
extern struct TimerIFace *IRT2870Timer;

#endif
