
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2WPA2_H
#define USB2WPA2_H

#include <exec/interfaces.h>
#include <stdint.h>

// --

#define USB2WPA2_NAME "usb2wpa2.library"
#define USB2WPA2_VERSION 1
#define USB2WPA2_ASSOC_IE_SIZE 22
#define USB2WPA2_REPLY_SIZE 121

#define USB2WPA2_ERR_PASSWORD 1
#define USB2WPA2_ERR_NETWORK 2
#define USB2WPA2_ERR_MEMORY 3
#define USB2WPA2_ERR_CRYPTO 4

#define USB2WPA2_INSTALL_PAIRWISE 1
#define USB2WPA2_INSTALL_GROUP 2
#define USB2WPA2_AUTHORIZE 4
#define USB2WPA2_CLEAR_REPLY 8

struct USB2WPA2Session;

struct USB2WPA2Network
{
	const uint8 *SSID;
	const uint8 *RSN;
	uint32 SSIDLength;
	uint32 RSNLength;
	uint8 BSSID[6];
	uint8 Station[6];
};

struct USB2WPA2Reply
{
	uint32 Length;
	uint32 Flags;
	uint32 GroupIndex;
	uint8 Frame[USB2WPA2_REPLY_SIZE];
	uint8 PairwiseKey[16];
	uint8 GroupKey[16];
	uint8 GroupRSC[6];
};

struct USB2WPA2IFace
{
	struct InterfaceData Data;

	uint32 APICALL (*Obtain)( struct USB2WPA2IFace *Self );
	uint32 APICALL (*Release)( struct USB2WPA2IFace *Self );
	APTR Reserved[2];
	const uint8 * APICALL (*AssociationIE)( struct USB2WPA2IFace *Self );
	struct USB2WPA2Session * APICALL (*Create)( struct USB2WPA2IFace *Self,
		const struct USB2WPA2Network *network, const char *password, uint32_t length, int32_t *error );
	void APICALL (*Destroy)( struct USB2WPA2IFace *Self, struct USB2WPA2Session *session );
	// Consume an EAPOL payload from the associated BSSID. Reply is zeroed on entry.
	// Returns 1 for a reply, 0 to ignore the packet, or -1 on a fatal crypto error.
	int32_t APICALL (*Process)( struct USB2WPA2IFace *Self, struct USB2WPA2Session *session,
		const uint8 *frame, uint32_t length, struct USB2WPA2Reply *reply );
};

#endif
