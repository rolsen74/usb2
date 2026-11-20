
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef WIRELESS_CRYPTO_H
#define WIRELESS_CRYPTO_H

#include "Types.h"

#define WIRELESS_CCMP_IV 8
#define WIRELESS_CCMP_MIC 8
#define WIRELESS_TKIP_TK 16
#define WIRELESS_TKIP_MIC 8
#define WIRELESS_TKIP_ICV 4
#define WIRELESS_TKIP_KEY_SIZE 32
#define WIRELESS_PN_MAX 0xffffffffffffULL
#define WIRELESS_NONQOS 16
#define WIRELESS_MGMT 17

#define WIRELESS_IGTK_COUNT 2
#define WIRELESS_IGTK_FIRST 4
#define WIRELESS_MMIE_LENGTH 18

struct WirelessIGTK
{
	U64 ipn;
	U8 round_keys[176];
	U8 active;
};

S32 Wireless_InstallIGTK( struct WirelessIGTK *keys, U32 index, const U8 *key, const U8 *ipn );
S32 Wireless_CheckBIP( struct WirelessIGTK *keys, const U8 *frame, U32 length );

void Wireless_AESExpand( U8 *round_keys, const U8 *key );
void Wireless_AESBlock( const U8 *round_keys, U8 *block );
U32 Wireless_AESSubstitute( U32 value );
void Wireless_TKIPSeed( const U8 *key, const U8 *sender, U64 number, U8 *seed );
void Wireless_TKIPMichael( const U8 *key, const U8 *addresses, const U8 *payload, U32 size, U8 *mic );
S32 Wireless_TKIP( const U8 *key, const U8 *frame, U32 tid, U64 number, U8 *payload, U32 size, S32 encrypt );
void Wireless_CMAC( const U8 *round_keys, const U8 *data, U32 length, U8 *mac );
S32 Wireless_CCMP( const U8 *round_keys, const U8 *frame, U32 tid, U64 number,
	U8 *payload, U32 length, U8 *mic, S32 encrypt );

#endif
