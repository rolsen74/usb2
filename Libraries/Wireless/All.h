
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_WIRELESS_ALL_H
#define USB2_WIRELESS_ALL_H

#include <dos/dos.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <stdint.h>
#include <string.h>
#include <usb2wpa.h>
#include <usb2wpa2.h>
#include <usb2wpa3.h>
#include "RSN.h"

// --

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t S32;

#define WPA2_RSN_MAX 257
#define WPA2_NONCE_SIZE 32
#define WPA2_REPLAY_SIZE 8
#define WPA2_KEY_HEADER 99
#define WPA2_MAX_EAPOL 1500

#define WPA2_PAIRWISE 0x0008
#define WPA2_INSTALL 0x0040
#define WPA2_ACK 0x0080
#define WPA2_MIC 0x0100
#define WPA2_SECURE 0x0200
#define WPA2_ENCRYPTED 0x1000
#define WPA2_DESCRIPTOR_V2 2

struct Crypto
{
	struct Library *MasterBase;
	struct AmiSSLMasterIFace *Master;
	struct Interface *SSL;
	S32 Opened;
	S32 Version6;
};

struct WirelessKeySession
{
	struct Crypto Crypto;

	U8 PMK[32];
	U8 PTK[64];
	U8 ANonce[WPA2_NONCE_SIZE];
	U8 SNonce[WPA2_NONCE_SIZE];
	U8 Replay[WPA2_REPLAY_SIZE];
	U8 AP[6];
	U8 Station[6];
	U8 RSN[WPA2_RSN_MAX];
	U32 RSNLength;
	U8 RSNX[18];
	U8 AssocIEs[USB2WPA3_ASSOC_IE_MAX];
	U32 RSNXLength;
	U32 AssocLength;
	S32 SAE;
	S32 HaveMessage1;
	S32 HavePairwise;
	S32 Authorized;
};

extern const U8 AssociationRSN[USB2WPA2_ASSOC_IE_SIZE];

S32 Crypto_Open( struct Crypto *crypto );
void Crypto_Close( struct Crypto *crypto );
S32 Crypto_Random( struct Crypto *crypto, U8 *buffer, U32 size );
S32 Crypto_PMK( struct Crypto *crypto, const char *password, U32 length,
	const U8 *ssid, U32 ssid_length, U8 *pmk );
S32 Crypto_HMAC( struct Crypto *crypto, const U8 *key, U32 key_length,
	const U8 *data, U32 data_length, U8 *hash );
S32 Crypto_HMAC_MD5( struct Crypto *crypto, const U8 *key, const U8 *data, U32 length, U8 *mac );
S32 Derive_PTK( struct WirelessKeySession *session, const U8 *anonce, U32 ptk_length );
S32 Crypto_HMAC_SHA256( struct Crypto *crypto, const U8 *key, U32 key_length,
	const U8 *data, U32 data_length, U8 *hash );
S32 Crypto_CMAC( struct Crypto *crypto, const U8 *key, const U8 *data, U32 length, U8 *mac );
S32 Crypto_Unwrap( struct Crypto *crypto, const U8 *key, const U8 *input, U32 length, U8 *output );
void Clear_Secret( void *memory, U32 length );

struct USB2WPA2Session * APICALL WPA2_Create( struct USB2WPA2IFace *self,
	const struct USB2WPA2Network *network, const char *password, U32 length, S32 *error );
void APICALL WPA2_Destroy( struct USB2WPA2IFace *self, struct USB2WPA2Session *session );
S32 APICALL WPA2_Process( struct USB2WPA2IFace *self, struct USB2WPA2Session *session,
	const U8 *frame, U32 length, struct USB2WPA2Reply *reply );

void Destroy_KeySession( struct WirelessKeySession *session );
S32 Process_Key( struct WirelessKeySession *session, const U8 *frame, U32 length, struct USB2WPA3Reply *reply );
struct USB2WPA3Session * APICALL WPA3_Create( struct USB2WPA3IFace *self,
	const struct USB2WPA3Network *network, const U8 *rsnxe, U32 rsnxeLength,
	const U8 *pmk, const U8 *pmkid, S32 *error );
void APICALL WPA3_Destroy( struct USB2WPA3IFace *self, struct USB2WPA3Session *session );
const U8 * APICALL WPA3_AssociationIEs( struct USB2WPA3IFace *self,
	const struct USB2WPA3Session *session, U32 *length );
S32 APICALL WPA3_Process( struct USB2WPA3IFace *self, struct USB2WPA3Session *session,
	const U8 *frame, U32 length, struct USB2WPA3Reply *reply );

U32 APICALL WPA_InterfaceObtain( struct Interface *self );
U32 APICALL WPA_InterfaceRelease( struct Interface *self );
extern const APTR WPAInterfaceVectors[];

#endif
