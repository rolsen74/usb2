
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8188EU_CRYPTO_H
#define RTL8188EU_CRYPTO_H

// --

#define RTL8188EU_EXT_IV_SIZE 8
#define RTL8188EU_TEMPORAL_KEY_SIZE 16
#define RTL8188EU_CCMP_MIC 8
#define RTL8188EU_TKIP_MIC 8
#define RTL8188EU_TKIP_ICV 4
#define RTL8188EU_PACKET_NUMBER_MAX 0xffffffffffffULL
#define RTL8188EU_NONQOS 16
#define RTL8188EU_KEY_COUNT 4
#define RTL8188EU_EAPOL 0x888e

struct EthernetUnit;

struct RTL8188EUKey
{
	U64 tx_number;
	U64 rx_number[RTL8188EU_NONQOS + 1];
	U8 round_keys[176];
	U8 tkip_key[RTL8188EU_TEMPORAL_KEY_SIZE + 2 * RTL8188EU_TKIP_MIC];
	U8 type;
	U8 active;
};

void RTL8188EU_AESExpand( U8 *round_keys, const U8 *key );
void RTL8188EU_AESBlock( const U8 *round_keys, U8 *block );
U32 RTL8188EU_AESSubstitute( U32 value );
void RTL8188EU_ClearKeys( struct EthernetUnit *unit );
S32 RTL8188EU_CCMP( const struct RTL8188EUKey *key, const U8 *frame, U32 tid, U64 number,
	U8 *payload, U32 length, U8 *mic, S32 encrypt );
void RTL8188EU_TKIPSeed( const U8 *key, const U8 *sender, U64 number, U8 *seed );
void RTL8188EU_TKIPMichael( const U8 *key, const U8 *addresses, const U8 *payload, U32 size, U8 *mic );
S32 RTL8188EU_TKIP( const struct RTL8188EUKey *key, const U8 *frame, U32 tid, U64 number,
	U8 *payload, U32 size, S32 encrypt );
U64 RTL8188EU_TKIPTime( void );
void RTL8188EU_TKIPFailure( struct EthernetUnit *unit );
S32 RTL8188EU_ProtectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 type );
S32 RTL8188EU_UnprotectData( struct EthernetUnit *unit, U8 *frame, U32 *length, U32 *header );

#endif
