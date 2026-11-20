
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8188EU_CRYPTO_H
#define RTL8188EU_CRYPTO_H

#include "WirelessCrypto.h"

// --

#define RTL8188EU_EXT_IV_SIZE WIRELESS_CCMP_IV
#define RTL8188EU_TEMPORAL_KEY_SIZE 16
#define RTL8188EU_CCMP_MIC WIRELESS_CCMP_MIC
#define RTL8188EU_TKIP_MIC WIRELESS_TKIP_MIC
#define RTL8188EU_TKIP_ICV WIRELESS_TKIP_ICV
#define RTL8188EU_PACKET_NUMBER_MAX WIRELESS_PN_MAX
#define RTL8188EU_NONQOS WIRELESS_NONQOS
#define RTL8188EU_KEY_COUNT 5
#define RTL8188EU_EAPOL 0x888e

struct EthernetUnit;

struct RTL8188EUKey
{
	U64 tx_number;
	U64 rx_number[WIRELESS_MGMT + 1];
	U8 round_keys[176];
	U8 tkip_key[RTL8188EU_TEMPORAL_KEY_SIZE + 2 * RTL8188EU_TKIP_MIC];
	U8 type;
	U8 active;
};

void RTL8188EU_ClearKeys( struct EthernetUnit *unit );
U64 RTL8188EU_TKIPTime( void );
void RTL8188EU_TKIPFailure( struct EthernetUnit *unit );
S32 RTL8188EU_ProtectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 type );
S32 RTL8188EU_UnprotectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 *header );

#endif
