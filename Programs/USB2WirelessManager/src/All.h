
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_WIRELESS_MANAGER_H
#define USB2_WIRELESS_MANAGER_H

#ifndef __USE_INLINE__
#define __USE_INLINE__
#endif

#include <devices/sana2.h>
#include <devices/timer.h>
#include <devices/usb2rtl8188.h>
#include <devices/usb2wireless.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <usb2wifi.h>
#include <usb2wpa.h>
#include <usb2wpa2.h>
#include <usb2wpa3.h>
#include "RSN.h"
#include "WiFiScan.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef USB2WIFI_EMBEDDED
#include "EmbeddedOutput.h"
#endif

// --

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t S32;
typedef char *STR;

/* SANA-II wireless extensions are absent from some AmigaOS SDKs. */
#define S2_SETOPTIONS 0xc012
#define S2_SETKEY 0xc013
#define S2_READMGMT 0xc015
#define S2_WRITEMGMT 0xc016
#define S2INFO_BSSID ( TAG_USER + 1 )
#define S2INFO_AssocID ( TAG_USER + 3 )
#define S2INFO_Encryption ( TAG_USER + 4 )
#define S2INFO_PortType ( TAG_USER + 5 )
#define S2INFO_Channel ( TAG_USER + 7 )
#define S2INFO_OperState ( TAG_USER + 20 )
#define S2ENC_NONE 0
#define S2ENC_CCMP 3
#define S2ENC_TKIP 2
#define S2PORT_MANAGED 7

#define WIRELESS_DEVICE "usb2rtl8188.device"
#define WIRELESS_RT2870_DEVICE "usb2rt2870.device"
#define WIRELESS_SIS163U_DEVICE "usb2sis163u.device"
#define WIRELESS_MAX_CHANNEL 11
#define WIRELESS_MAX_SSID 32
#define WIRELESS_ADDR_SIZE 6
#define WIRELESS_FRAME_SIZE 2048
#define WIRELESS_80211_HEADER 24
#define WIRELESS_EAPOL 0x888e
#define WIRELESS_RSN_MAX 257
#define WIRELESS_SCAN_REPORT_LIMIT 12

enum WirelessResult
{
	WIRELESS_DISCONNECTED = -3,
	WIRELESS_IO_ERROR,
	WIRELESS_STOPPED,
	WIRELESS_TIMEOUT,
	WIRELESS_MATCH,
};

struct AccessPoint
{
	U8 BSSID[WIRELESS_ADDR_SIZE];
	U8 Rates[12];
	U32 RateCount;
	U32 Channel;
	S32 Protected;
	S32 RatesSupported;
	U16 WPAIELength;
	U8 WPAIE[WIRELESS_RSN_MAX];
	U16 RSNLength;
	U8 RSN[WIRELESS_RSN_MAX];
	U16 RSNXLength;
	U8 RSNX[18];
};

struct AutoWiFi
{
	struct USB2WiFiIFace *Interface;
	struct USB2WiFiProfile Profile;
	U32 Adapter;
	U32 Unit;
	S32 Claimed;
	S32 SetupConnect;
	S32 Linked;
	S32 DetectSecurity;
	S32 Retry;
	S32 SavePassword;
};

struct Wireless
{
	struct AutoWiFi *Auto;
	struct IOSana2Req *Command;
	struct IOSana2Req *Receive;
	struct TimeRequest *Timer;
	struct Library *WPA2Base;
	struct USB2WPA2IFace *IWPA2;
	struct USB2WPA2Session *WPA2;
	struct USB2WPAIFace *IWPA;
	struct USB2WPASession *WPA;
	struct Library *WPA3Base;
	struct USB2WPA3IFace *IWPA3;
	struct USB2WPA3Session *WPA3;
	struct MsgPort *CommandPort;
	struct MsgPort *ReceivePort;
	struct MsgPort *TimerPort;
	struct Sana2Hook CopyHook;
	struct AccessPoint AP;
	struct WiFiScan *ScanNetworks;
	struct AccessPoint RejectedAP;
	union
	{
		struct USB2WPA2Reply WPA2;
		struct USB2WPAReply WPA;
	} PSKReply;
	struct USB2WPA3Reply WPA3Reply;
	U8 SAEFrame[USB2SAE_COMMIT_SIZE];
	U32 SAELength;

	const char *SSID;
	const char *DeviceName;
	char *Password;
	U8 Station[WIRELESS_ADDR_SIZE];
	U32 SSIDLength;
	U32 UnitNumber;
	U32 ScanBeaconCount;
	U32 ScanReportedCount;
	U32 ScanMalformedMatches;
	U32 ScanOtherChannelMatches;
	U32 ScanSecurityMismatch;
	U8 ScanReportedBSSIDs[WIRELESS_SCAN_REPORT_LIMIT][WIRELESS_ADDR_SIZE];
	S32 ScanRT2870;
	S32 UseWPA;
	S32 Hidden;
	S32 DetectSecurity;
	S32 AllowWPA2;
	S32 AllowWPA3;
	S32 UseSAE;
	U16 SAETransaction;
	U16 Sequence;
	U16 Status;
	U16 AssocID;
	S32 DeviceOpen;
	S32 TimerOpen;
	S32 SetOptions;
	S32 RetryPassword;
	S32 WPA2Result;
};

typedef S32 (*FrameMatch)( struct Wireless *wireless, const U8 *frame, U32 length );

S32 Wireless_PromptPassword( struct Wireless *wireless, U32 *action );
enum WirelessResult WiFi_Passphrase( struct Wireless *wireless, char *input, char **password, U32 *length );
S32 WiFi_Run( const char *device, U32 unit, S32 setup );
void WiFi_Connected( struct Wireless *wireless, U32 channel );
S32 Run_Wireless( struct Wireless *wireless, U32 channel, U32 maxChannel, S32 open, S32 auth, S32 associate );
S32 Valid_Password( const char *password, U32 minimum );
S32 Wireless_Open( struct Wireless *wireless );
void Wireless_Close( struct Wireless *wireless );
S32 Wireless_Command( struct Wireless *wireless, U16 command );
S32 Wireless_Set_Options( struct Wireless *wireless, U32 channel, const U8 *bssid, U16 aid,
	U32 encryption, S32 operational );
S32 Wireless_Set_Key( struct Wireless *wireless, U32 index, const U8 *key, const U8 *rsc, U32 cipher, U32 length );
S32 Wireless_Set_Copy_Hook( struct Wireless *wireless );
S32 Wireless_Send_EAPOL( struct Wireless *wireless, const U8 *frame, U32 length, U32 flags );
enum WirelessResult Wireless_Wait_Frame( struct Wireless *wireless, U32 micros,
	const U8 *transmit, U32 length, FrameMatch match, S32 eapol );
U32 Wireless_ProbeRequest( struct Wireless *wireless, U8 *frame );
S32 Wireless_Match_Beacon( struct Wireless *wireless, const U8 *frame, U32 length );
void Wireless_Report_Scan( const struct Wireless *wireless );
void Wireless_Report_Networks( const struct WiFiScan *scan );
S32 Wireless_ScanSiS163U( U32 unit, U32 channel );
void Wireless_Report_Security( const struct Wireless *wireless );
enum WirelessResult Wireless_Authenticate( struct Wireless *wireless );
enum WirelessResult Wireless_Associate( struct Wireless *wireless, const U8 *associationIE, U32 associationLength );
void Wireless_Frame_Header( struct Wireless *wireless, U8 *frame, U8 subtype );
S32 Wireless_Reply_From_AP( struct Wireless *wireless, const U8 *frame, U32 length, U8 subtype );
enum WirelessResult Wireless_Connect( struct Wireless *wireless, U32 channel );
S32 Wireless_Wait_Link( struct Wireless *wireless );
enum WirelessResult WPA2_Test_Association( struct Wireless *wireless );
enum WirelessResult PSK_Open( struct Wireless *wireless );
void PSK_Close( struct Wireless *wireless );
enum WirelessResult PSK_Handshake( struct Wireless *wireless );
S32 PSK_Wait_Link( struct Wireless *wireless );

void Clear_Secret( void *memory, U32 length );
enum WirelessResult WPA_Read_Passphrase( struct Wireless *wireless, char *input, char **password, U32 *length );
enum WirelessResult WPA3_Authenticate( struct Wireless *wireless );
enum WirelessResult WPA3_Handshake( struct Wireless *wireless );
S32 WPA3_Wait_Link( struct Wireless *wireless );
void WPA3_Close( struct Wireless *wireless );

#endif
