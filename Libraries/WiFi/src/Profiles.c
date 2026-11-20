
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "WiFiScan.h"

#include <stddef.h>

// --

#define PROFILE_VARIABLE "usb2wifi"
#define PROFILE_FLAGS ( GVF_GLOBAL_ONLY | GVF_BINARY_VAR | GVF_DONT_NULL_TERM )
#define ADAPTER_COUNT 2
#define UNIT_COUNT 32

struct ProfileDatabase
{
	U8 Header[4];
	struct USB2WiFiProfile Profiles[ADAPTER_COUNT][UNIT_COUNT];
	U8 ExtraByte;
};

struct ScanSnapshot
{
	struct WiFiNetwork Network[WIFI_SCAN_NETWORKS];
	U32 Count;
};

#define PROFILE_SIZE offsetof( struct ProfileDatabase, ExtraByte )

static const U8 Header[4] = { 'U', 'W', 'F', 1 };
static struct Task *ClaimOwner[ADAPTER_COUNT][UNIT_COUNT];
static struct ScanSnapshot *ScanSnapshots[ADAPTER_COUNT][UNIT_COUNT];

// --

static void
Erase( void *memory, U32 length )
{
	volatile U8 *data;

	data = memory;

	while( length-- )
	{
		*data++ = 0;
	}
}

// --

static S32
ValidProfile( const struct USB2WiFiProfile *profile, U32 adapter )
{
	const char *end;
	const char *p;

	if ( ( ! profile->SSID[0] ) || ( ! memchr( profile->SSID, 0, sizeof( profile->SSID ) ) ) ||
		( profile->Security > USB2WIFI_WPA3 ) || ( profile->Options & ~( USB2WIFI_PROFILE_AUTO | USB2WIFI_PROFILE_HIDDEN ) ) ||
		( profile->Channel > ( adapter == USB2WIFI_RT2870 ? 13 : 11 ) ) ||
		( adapter == USB2WIFI_RT2870 && profile->Security == USB2WIFI_OPEN ) )
	{
		return ( FALSE );
	}

	end = memchr( profile->Password, 0, sizeof( profile->Password ) );

	if ( ! end )
	{
		return ( FALSE );
	}

	if ( end != profile->Password )
	{
		if ( ( profile->Security == USB2WIFI_OPEN ) ||
			( end - profile->Password < ( profile->Security == USB2WIFI_WPA3 ? 1 : 8 ) ) )
		{
			return ( FALSE );
		}

		for( p = profile->Password; p < end; p++ )
		{
			if ( ( (U8) *p < 32 ) || ( (U8) *p > 126 ) )
			{
				return ( FALSE );
			}
		}
	}

	return ( TRUE );
}

// --

static S32
ReadDatabase( struct ProfileDatabase *db )
{
	S32 bytes;

	bytes = GetVar( PROFILE_VARIABLE, (char *) db, sizeof( *db ), PROFILE_FLAGS );

	if ( bytes < 0 )
	{
		if ( IoErr() != ERROR_OBJECT_NOT_FOUND )
		{
			return ( USB2WIFI_ERROR );
		}

		memset( db, 0, sizeof( *db ) );

		memcpy( db->Header, Header, sizeof( Header ) );

		return ( USB2WIFI_MISSING );
	}

	if ( ( bytes != PROFILE_SIZE ) ||
		( memcmp( db->Header, Header, sizeof( Header ) ) ) )
	{
		return ( USB2WIFI_ERROR );
	}

	return ( USB2WIFI_OK );
}

// --

static S32
SameNetwork( const struct USB2WiFiProfile *a, const struct USB2WiFiProfile *b )
{
	return ( a->Security == b->Security && ! strncmp( a->SSID, b->SSID, sizeof( a->SSID ) ) );
}

// --

static void
CopyPassword( struct USB2WiFiProfile *target, const struct USB2WiFiProfile *profile )
{
	Erase( target->Password, sizeof( target->Password ) );

	strcpy( target->Password, profile->Password );
}

// --

static const struct USB2WiFiProfile *
FindPassword( const struct ProfileDatabase *db, U32 adapter, U32 unit, const struct USB2WiFiProfile *network )
{
	const struct USB2WiFiProfile *match;
	const struct USB2WiFiProfile *entry;

	U32 device;
	U32 index;

	entry = & db->Profiles[adapter][unit];

	if ( ( entry->Password[0] ) && ( SameNetwork( entry, network ) ) && ( ValidProfile( entry, adapter ) ) )
	{
		return ( entry );
	}

	match = NULL;

	for( device = 0; device < ADAPTER_COUNT; device++ )
	{
		for( index = 0; index < UNIT_COUNT; index++ )
		{
			entry = & db->Profiles[device][index];

			if ( ( ! entry->Password[0] ) || ( ! SameNetwork( entry, network ) ) || ( ! ValidProfile( entry, device ) ) )
			{
				continue;
			}

			if ( ( match ) && ( strcmp( match->Password, entry->Password ) ) )
			{
				return ( NULL );
			}

			match = entry;
		}
	}

	return ( match );
}

// --

void
WiFi_FindPassword( U32 adapter, U32 unit, struct USB2WiFiProfile *network )
{
	struct ProfileDatabase db;
	const struct USB2WiFiProfile *match;

	if ( ( adapter >= ADAPTER_COUNT ) || ( unit >= UNIT_COUNT ) ||
		( network->Security < USB2WIFI_WPA ) || ( network->Security > USB2WIFI_WPA3 ) )
	{
		return;
	}

	ObtainSemaphore( & WiFiLock );

	if ( ReadDatabase( & db ) == USB2WIFI_OK )
	{
		match = FindPassword( & db, adapter, unit, network );

		if ( match )
		{
			CopyPassword( network, match );
		}
	}

	Erase( & db, sizeof( db ) );

	ReleaseSemaphore( & WiFiLock );
}

// --

static S32
WiFi_Load( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit, struct USB2WiFiProfile *profile )
{
	struct ProfileDatabase db;
	const struct USB2WiFiProfile *match;

	S32 result;

	if ( ( ! profile ) || ( adapter >= ADAPTER_COUNT ) || ( unit >= UNIT_COUNT ) )
	{
		return ( USB2WIFI_ERROR );
	}

	memset( profile, 0, sizeof( *profile ) );

	ObtainSemaphore( & WiFiLock );

	result = ReadDatabase( & db );

	if ( result == USB2WIFI_OK )
	{
		if ( ! db.Profiles[adapter][unit].SSID[0] )
		{
			result = USB2WIFI_MISSING;
		}
		else if ( ValidProfile( & db.Profiles[adapter][unit], adapter ) )
		{
			*profile = db.Profiles[adapter][unit];

			if ( ( ! profile->Password[0] ) && ( profile->Security != USB2WIFI_OPEN ) )
			{
				match = FindPassword( & db, adapter, unit, profile );

				if ( match )
				{
					CopyPassword( profile, match );
				}
			}
		}
		else
		{
			result = USB2WIFI_ERROR;
		}
	}

	Erase( & db, sizeof( db ) );

	ReleaseSemaphore( & WiFiLock );

	return ( result );
}

// --

static S32
WiFi_Save( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit,
	const struct USB2WiFiProfile *profile, U32 passwordAction )
{
	struct ProfileDatabase db;
	struct USB2WiFiProfile *target;
	struct USB2WiFiProfile next;

	S32 result;

	if ( ( ! profile ) || ( adapter >= ADAPTER_COUNT ) || ( unit >= UNIT_COUNT ) ||
		( passwordAction > USB2WIFI_FORGET_PASSWORD ) || ( ! ValidProfile( profile, adapter ) ) )
	{
		return ( USB2WIFI_ERROR );
	}

	memset( & next, 0, sizeof( next ) );

	strcpy( next.SSID, profile->SSID );

	next.Security = profile->Security;
	next.Channel = profile->Channel;
	next.Options = profile->Options;

	if ( passwordAction == USB2WIFI_SAVE_PASSWORD )
	{
		CopyPassword( & next, profile );
	}

	ObtainSemaphore( & WiFiLock );

	result = ReadDatabase( & db );

	if ( result >= 0 )
	{
		target = & db.Profiles[adapter][unit];

		if ( ( passwordAction == USB2WIFI_KEEP_PASSWORD ) && ( SameNetwork( target, & next ) ) &&
			( ValidProfile( target, adapter ) ) )
		{
			CopyPassword( & next, target );
		}

		*target = next;

		result = SetVar( PROFILE_VARIABLE, (const char *) & db, PROFILE_SIZE, PROFILE_FLAGS ) ?
			USB2WIFI_OK : USB2WIFI_ERROR;
	}

	Erase( & next, sizeof( next ) );

	Erase( & db, sizeof( db ) );

	ReleaseSemaphore( & WiFiLock );

	return ( result );
}

// --

static S32
WiFi_Connected( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit,
	const struct USB2WiFiProfile *profile, U32 savePassword )
{
	struct ProfileDatabase db;
	struct USB2WiFiProfile *target;

	S32 result;

	if ( ( ! profile ) || ( adapter >= ADAPTER_COUNT ) || ( unit >= UNIT_COUNT ) || ( ! ValidProfile( profile, adapter ) ) )
	{
		return ( USB2WIFI_ERROR );
	}

	ObtainSemaphore( & WiFiLock );

	result = ReadDatabase( & db );

	if ( result == USB2WIFI_OK )
	{
		target = & db.Profiles[adapter][unit];

		if ( ( ! SameNetwork( target, profile ) ) || ( ! ValidProfile( target, adapter ) ) )
		{
			result = USB2WIFI_MISSING;
		}
		else if ( ( target->Channel != profile->Channel ) || ( savePassword ) )
		{
			target->Channel = profile->Channel;

			if ( savePassword )
			{
				CopyPassword( target, profile );
			}

			result = SetVar( PROFILE_VARIABLE, (const char *) & db, PROFILE_SIZE, PROFILE_FLAGS ) ?
				USB2WIFI_OK : USB2WIFI_ERROR;
		}
	}

	Erase( & db, sizeof( db ) );

	ReleaseSemaphore( & WiFiLock );

	return ( result );
}

// --

void
WiFi_RememberScan( U32 adapter, U32 unit, const struct WiFiScan *scan )
{
	struct ScanSnapshot *snapshot;
	struct Task *task;

	if ( ( ! scan->Count ) || ( scan->Count > WIFI_SCAN_NETWORKS ) ||
		( adapter >= ADAPTER_COUNT ) || ( unit >= UNIT_COUNT ) )
	{
		return;
	}

	task = FindTask( NULL );

	ObtainSemaphore( & WiFiLock );

	if ( ClaimOwner[adapter][unit] == task )
	{
		snapshot = ScanSnapshots[adapter][unit];

		if ( ! snapshot )
		{
			snapshot = AllocVecTags( sizeof( *snapshot ), TAG_END );
			ScanSnapshots[adapter][unit] = snapshot;
		}

		if ( snapshot )
		{
			snapshot->Count = scan->Count;

			memcpy( snapshot->Network, scan->Network, scan->Count * sizeof( *scan->Network ) );
		}
	}

	ReleaseSemaphore( & WiFiLock );
}

// --

void
WiFi_RestoreScan( U32 adapter, U32 unit, struct WiFiScan *scan )
{
	const struct ScanSnapshot *snapshot;
	struct Task *task;

	if ( ( adapter >= ADAPTER_COUNT ) || ( unit >= UNIT_COUNT ) )
	{
		return;
	}

	task = FindTask( NULL );

	ObtainSemaphore( & WiFiLock );

	snapshot = ScanSnapshots[adapter][unit];

	if ( ( snapshot ) && ( ClaimOwner[adapter][unit] == task ) )
	{
		scan->Count = snapshot->Count;

		memcpy( scan->Network, snapshot->Network, scan->Count * sizeof( *scan->Network ) );
	}

	ReleaseSemaphore( & WiFiLock );
}

// --

static S32
WiFi_Claim( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit )
{
	struct Task *task;

	U32 bit;
	S32 claimed;

	if ( ( adapter >= ADAPTER_COUNT ) || ( unit >= UNIT_COUNT ) )
	{
		return ( FALSE );
	}

	bit = 1UL << unit;

	task = FindTask( NULL );

	ObtainSemaphore( & WiFiLock );

	claimed = ! ( ActiveUnits[adapter] & bit );

	if ( claimed )
	{
		ClaimOwner[adapter][unit] = task;
		ActiveUnits[adapter] |= bit;
	}

	ReleaseSemaphore( & WiFiLock );

	return ( claimed );
}

// --

static void
WiFi_Unclaim( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit )
{
	if ( ( adapter >= ADAPTER_COUNT ) || ( unit >= UNIT_COUNT ) )
	{
		return;
	}

	ObtainSemaphore( & WiFiLock );

	if ( ScanSnapshots[adapter][unit] )
	{
		FreeVec( ScanSnapshots[adapter][unit] );

		ScanSnapshots[adapter][unit] = NULL;
	}

	ClaimOwner[adapter][unit] = NULL;
	ActiveUnits[adapter] &= ~( 1UL << unit );

	ReleaseSemaphore( & WiFiLock );
}

// --

const APTR WiFiVectors[] =
{
	WiFi_InterfaceObtain, WiFi_InterfaceRelease, NULL, NULL, WiFi_Load, WiFi_Save, WiFi_Connected,
	WiFi_Claim, WiFi_Unclaim, WiFi_Setup, WiFi_StartPrompt, WiFi_CancelPrompt, WiFi_StartAuto, WiFi_SetupRetry, (APTR) -1
};
