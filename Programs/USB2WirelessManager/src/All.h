
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_WIRELESS_MANAGER_H
#define USB2_WIRELESS_MANAGER_H

#define __USE_INLINE__

#include <devices/sana2.h>
#include <devices/timer.h>
#include <devices/usb2rtl8188.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <usb2wpa2.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
#define S2PORT_MANAGED 7

#define WIRELESS_DEVICE "usb2rtl8188.device"
#define WIRELESS_DEVICE_PATH "DEVS:Networks/" WIRELESS_DEVICE
#define WIRELESS_MAX_CHANNEL 11
#define WIRELESS_MAX_SSID 32
#define WIRELESS_ADDR_SIZE 6
#define WIRELESS_FRAME_SIZE 2048
#define WIRELESS_80211_HEADER 24
#define WIRELESS_EAPOL 0x888e
#define WIRELESS_RSN_MAX 257

enum WirelessResult
{
	WIRELESS_IO_ERROR = -2,
	WIRELESS_STOPPED,
	WIRELESS_TIMEOUT,
	WIRELESS_MATCH,
};

struct AccessPoint
{
	U8 BSSID[WIRELESS_ADDR_SIZE];
	U8 Rates[4];
	U32 RateCount;
	U32 Channel;
	S32 Protected;
	S32 RatesSupported;
	U16 RSNLength;
	U8 RSN[WIRELESS_RSN_MAX];
};

struct Wireless
{
	struct IOSana2Req *Command;
	struct IOSana2Req *Receive;
	struct TimeRequest *Timer;
	struct Library *WPA2Base;
	struct USB2WPA2IFace *IWPA2;
	struct USB2WPA2Session *WPA2;
	struct MsgPort *CommandPort;
	struct MsgPort *ReceivePort;
	struct MsgPort *TimerPort;
	struct Sana2Hook CopyHook;
	struct AccessPoint AP;
	struct USB2WPA2Reply WPA2Reply;

	const char *SSID;
	U8 Station[WIRELESS_ADDR_SIZE];
	U32 SSIDLength;
	U32 UnitNumber;
	U16 Sequence;
	U16 Status;
	U16 AssocID;
	S32 DeviceOpen;
	S32 TimerOpen;
	S32 SetOptions;
	S32 WPA2Result;
};

typedef S32 (*FrameMatch)( struct Wireless *wireless, const U8 *frame, U32 length );

S32 Wireless_Open( struct Wireless *wireless );
void Wireless_Close( struct Wireless *wireless );
S32 Wireless_Command( struct Wireless *wireless, U16 command );
S32 Wireless_Set_Options( struct Wireless *wireless, U32 channel, const U8 *bssid, U16 aid,
	U32 encryption, S32 operational );
S32 Wireless_Set_Key( struct Wireless *wireless, U32 index, const U8 *key, const U8 *rsc );
S32 Wireless_Set_Copy_Hook( struct Wireless *wireless );
S32 Wireless_Send_EAPOL( struct Wireless *wireless, const struct USB2WPA2Reply *reply );
enum WirelessResult Wireless_Wait_Frame( struct Wireless *wireless, U32 micros,
	const U8 *transmit, U32 length, FrameMatch match, S32 eapol );
S32 Wireless_Match_Beacon( struct Wireless *wireless, const U8 *frame, U32 length );
enum WirelessResult Wireless_Connect( struct Wireless *wireless, U32 channel );
S32 Wireless_Wait_Link( struct Wireless *wireless );
enum WirelessResult WPA2_Open( struct Wireless *wireless );
void WPA2_Close( struct Wireless *wireless );
enum WirelessResult WPA2_Handshake( struct Wireless *wireless );
S32 WPA2_Wait_Link( struct Wireless *wireless );

#endif
