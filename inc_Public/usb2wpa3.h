
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2WPA3_H
#define USB2WPA3_H

#include <exec/interfaces.h>
#include <stdint.h>

// --

// Main interface version 1. The EAPOL stage takes keys from a confirmed SAE exchange.
#define USB2WPA3_NAME "usb2wpa3.library"
#define USB2WPA3_VERSION 1
#define USB2WPA3_SAE_REVISION 1
#define USB2WPA3_SAE_MODE_REVISION 2
#define USB2WPA3_SAE_RETRY_REVISION 3
#define USB2WPA3_SAE_CONFIRM_REVISION 4
#define USB2WPA3_SAE_HNP 0
#define USB2WPA3_SAE_H2E 1
#define USB2SAE_COMMIT_SIZE 98
#define USB2SAE_CONFIRM_SIZE 34
#define USB2WPA3_ASSOC_IE_MAX 62
#define USB2WPA3_REPLY_SIZE ( 99 + USB2WPA3_ASSOC_IE_MAX )
#define USB2WPA3_INSTALL_IGTK 16

#define USB2WPA3_ERR_NETWORK 2
#define USB2WPA3_ERR_MEMORY 3
#define USB2WPA3_ERR_CRYPTO 4

#define USB2WPA3_INSTALL_PAIRWISE 1
#define USB2WPA3_INSTALL_GROUP 2
#define USB2WPA3_AUTHORIZE 4
#define USB2WPA3_CLEAR_REPLY 8

struct USB2WPA3Session;
struct USB2SAEAuth;

struct USB2WPA3Network
{
	const uint8 *RSN;
	uint32 RSNLength;
	uint8 BSSID[6];
	uint8 Station[6];
};

struct USB2WPA3Reply
{
	uint32 Length;
	uint32 Flags;
	uint32 GroupIndex;
	uint32 IGTKIndex;
	uint8 Frame[USB2WPA3_REPLY_SIZE];
	uint8 PairwiseKey[16];
	uint8 GroupKey[16];
	uint8 GroupRSC[6];
	uint8 IGTK[16];
	uint8 IGTKIPN[6];
};

struct USB2WPA3IFace
{
	struct InterfaceData Data;

	uint32 APICALL (*Obtain)( struct USB2WPA3IFace *Self );
	uint32 APICALL (*Release)( struct USB2WPA3IFace *Self );
	APTR Reserved[2];
	// pmk and pmkid are the 32- and 16-byte outputs of verified group 19 SAE.
	// rsnxe is the complete AP extension IE, or NULL with length zero.
	struct USB2WPA3Session * APICALL (*Create)( struct USB2WPA3IFace *Self,
		const struct USB2WPA3Network *network, const uint8 *rsnxe, uint32_t rsnxeLength,
		const uint8 *pmk, const uint8 *pmkid, int32_t *error );
	void APICALL (*Destroy)( struct USB2WPA3IFace *Self, struct USB2WPA3Session *session );
	// Use these exact IEs in the association request.
	const uint8 * APICALL (*AssociationIEs)( struct USB2WPA3IFace *Self,
		const struct USB2WPA3Session *session, uint32_t *length );
	int32_t APICALL (*Process)( struct USB2WPA3IFace *Self, struct USB2WPA3Session *session,
		const uint8 *frame, uint32_t length, struct USB2WPA3Reply *reply );
	// Appended in usb2wpa3.library 1.1. Group 19 Hash-to-Element.
	struct USB2SAEAuth * APICALL (*SAEStart)( struct USB2WPA3IFace *Self, const uint8 *ssid,
		uint32_t ssidLength, const char *password, uint32_t length, const uint8 *station,
		const uint8 *ap, uint8 commit[USB2SAE_COMMIT_SIZE] );
	// With 1.3 or later, resubmitting the same peer commit makes a new confirm.
	int32_t APICALL (*SAECommit)( struct USB2WPA3IFace *Self, struct USB2SAEAuth *auth,
		const uint8 *commit, uint32_t length, uint8 confirm[USB2SAE_CONFIRM_SIZE] );
	int32_t APICALL (*SAEConfirm)( struct USB2WPA3IFace *Self, struct USB2SAEAuth *auth,
		const uint8 *confirm, uint32_t length, uint8 pmk[32], uint8 pmkid[16] );
	void APICALL (*SAEDestroy)( struct USB2WPA3IFace *Self, struct USB2SAEAuth *auth );
	// Appended in usb2wpa3.library 1.2. Select the password-element derivation.
	struct USB2SAEAuth * APICALL (*SAEStartMode)( struct USB2WPA3IFace *Self, const uint8 *ssid,
		uint32_t ssidLength, const char *password, uint32_t length, const uint8 *station,
		const uint8 *ap, uint8 commit[USB2SAE_COMMIT_SIZE], uint32_t mode );
};

#endif
