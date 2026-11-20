
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2WPA_H
#define USB2WPA_H

#include <exec/interfaces.h>
#include <stdint.h>

// --

#define USB2WPA_NAME "usb2wpa.library"
#define USB2WPA_VERSION 1
#define USB2WPA_ASSOC_IE_SIZE 24
#define USB2WPA_REPLY_SIZE 123

#define USB2WPA_ERR_PASSWORD 1
#define USB2WPA_ERR_NETWORK 2
#define USB2WPA_ERR_MEMORY 3
#define USB2WPA_ERR_CRYPTO 4

#define USB2WPA_INSTALL_PAIRWISE 1
#define USB2WPA_INSTALL_GROUP 2
#define USB2WPA_AUTHORIZE 4
#define USB2WPA_CLEAR_REPLY 8

struct USB2WPASession;

#define USB2WPA_TKIP_KEY_SIZE 32

struct USB2WPANetwork
{
	const uint8 *SSID;
	const uint8 *WPAIE;
	uint32 SSIDLength;
	uint32 WPAIELength;
	uint8 BSSID[6];
	uint8 Station[6];
};

struct USB2WPAReply
{
	uint32 Length;
	uint32 Flags;
	uint32 GroupIndex;
	uint8 Frame[USB2WPA_REPLY_SIZE];
	// TKIP temporal key followed by the station's TX and RX Michael keys.
	uint8 PairwiseKey[USB2WPA_TKIP_KEY_SIZE];
	uint8 GroupKey[USB2WPA_TKIP_KEY_SIZE];
	uint8 GroupRSC[6];
};

struct USB2WPAIFace
{
	struct InterfaceData Data;

	uint32 APICALL (*Obtain)( struct USB2WPAIFace *Self );
	uint32 APICALL (*Release)( struct USB2WPAIFace *Self );
	APTR Reserved[2];
	const uint8 * APICALL (*AssociationIE)( struct USB2WPAIFace *Self );
	struct USB2WPASession * APICALL (*Create)( struct USB2WPAIFace *Self,
		const struct USB2WPANetwork *network, const char *password, uint32_t length, int32_t *error );
	void APICALL (*Destroy)( struct USB2WPAIFace *Self, struct USB2WPASession *session );
	// Consume an EAPOL payload from the associated BSSID. Reply is zeroed on entry.
	// Returns 1 for a reply, 0 to ignore the packet, or -1 on a fatal crypto error.
	int32_t APICALL (*Process)( struct USB2WPAIFace *Self, struct USB2WPASession *session,
		const uint8 *frame, uint32_t length, struct USB2WPAReply *reply );
};

#endif
