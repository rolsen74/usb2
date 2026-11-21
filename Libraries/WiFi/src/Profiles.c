
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "WiFiScan.h"

#include "ProfileFormat.h"

// --

struct WiFiClaim
{
	struct WiFiClaim *Next;
	struct Task *Owner;
	struct WiFiScan *Scan;
	U32 Adapter;
	U32 Unit;
};

static struct WiFiClaim *Claims;

// --

const char *
WiFi_DeviceName( U32 adapter )
{
	switch ( adapter )
	{
		case USB2WIFI_RTL8188: return ( "usb2rtl8188.device" );
		case USB2WIFI_RT2870: return ( "usb2rt2870.device" );
	}

	return ( NULL );
}

// --

static void
CopyPassword( struct USB2WiFiProfile *target, const char *password )
{
	memset( target->Password, 0, sizeof( target->Password ) );

	strcpy( target->Password, password );
}

// --

void
WiFi_FindPassword( struct USB2WiFiProfile *profile )
{
	struct ProfileDatabase db;
	struct ProfileNetwork *network;

	if ( ( profile->Security < USB2WIFI_WPA ) || ( profile->Security > USB2WIFI_WPA3 ) )
	{
		return;
	}

	ObtainSemaphore( &WiFiLock );

	if ( WiFi_ReadProfiles( &db ) == USB2WIFI_OK )
	{
		network = WiFi_ProfileNetwork( &db, profile, FALSE );

		if ( network )
		{
			CopyPassword( profile, network->Profile.Password );
		}
	}

	WiFi_FreeProfiles( &db );

	ReleaseSemaphore( &WiFiLock );
}

// --

static S32
WiFi_Load( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit, struct USB2WiFiProfile *profile )
{
	struct ProfileDatabase db;
	struct ProfileAdapter *entry;
	struct ProfileNetwork *network;

	const char *device;
	S32 result;

	device = WiFi_DeviceName( adapter );

	if ( ( ! profile ) || ( ! device ) )
	{
		return ( USB2WIFI_ERROR );
	}

	memset( profile, 0, sizeof( *profile ) );

	ObtainSemaphore( &WiFiLock );

	result = WiFi_ReadProfiles( &db );

	if ( result == USB2WIFI_OK )
	{
		entry = WiFi_ProfileAdapter( &db, device, unit, FALSE );

		result = USB2WIFI_MISSING;

		if ( entry )
		{
			network = WiFi_ProfileNetwork( &db, &entry->Profile, FALSE );

			*profile = entry->Profile;
			profile->Options |= network->Profile.Options;

			CopyPassword( profile, network->Profile.Password );

			result = WiFi_ValidProfile( profile, adapter ) ? USB2WIFI_OK : USB2WIFI_ERROR;
		}
	}

	WiFi_FreeProfiles( &db );

	ReleaseSemaphore( &WiFiLock );

	return ( result );
}

// --

static S32
WiFi_Save( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit,
	const struct USB2WiFiProfile *profile, U32 passwordAction )
{
	struct ProfileDatabase db;
	struct ProfileAdapter *entry;
	struct ProfileNetwork *network;

	const char *device;
	S32 result;

	device = WiFi_DeviceName( adapter );

	if ( ( ! profile ) || ( ! device ) || ( passwordAction > USB2WIFI_FORGET_PASSWORD )
		|| ( ! WiFi_ValidProfile( profile, adapter ) ) )
	{
		return ( USB2WIFI_ERROR );
	}

	ObtainSemaphore( &WiFiLock );

	result = WiFi_ReadProfiles( &db );

	if ( result >= 0 )
	{
		entry = WiFi_ProfileAdapter( &db, device, unit, TRUE );

		network = WiFi_ProfileNetwork( &db, profile, TRUE );

		result = USB2WIFI_ERROR;

		if ( ( entry ) && ( network ) )
		{
			strcpy( entry->Profile.SSID, profile->SSID );

			entry->Profile.Security = profile->Security;
			entry->Profile.Channel = profile->Channel;
			entry->Profile.Options = profile->Options & USB2WIFI_PROFILE_AUTO;
			network->Profile.Options = profile->Options & USB2WIFI_PROFILE_HIDDEN;

			if ( passwordAction != USB2WIFI_KEEP_PASSWORD )
			{
				CopyPassword( &network->Profile, passwordAction == USB2WIFI_SAVE_PASSWORD ? profile->Password : "" );
			}

			result = WiFi_WriteProfiles( &db );
		}
	}

	WiFi_FreeProfiles( &db );

	ReleaseSemaphore( &WiFiLock );

	return ( result );
}

// --

static S32
WiFi_Connected( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit,
	const struct USB2WiFiProfile *profile, U32 savePassword )
{
	struct ProfileDatabase db;
	struct ProfileAdapter *entry;
	struct ProfileNetwork *network;

	const char *device;
	S32 result;

	device = WiFi_DeviceName( adapter );

	if ( ( ! profile ) || ( ! device ) || ( ! WiFi_ValidProfile( profile, adapter ) ) )
	{
		return ( USB2WIFI_ERROR );
	}

	ObtainSemaphore( &WiFiLock );

	result = WiFi_ReadProfiles( &db );

	if ( result == USB2WIFI_OK )
	{
		entry = WiFi_ProfileAdapter( &db, device, unit, FALSE );

		network = WiFi_ProfileNetwork( &db, profile, FALSE );

		if ( ( ! entry ) || ( ! network ) || ( entry->Profile.Security != profile->Security )
			|| ( strcmp( entry->Profile.SSID, profile->SSID ) ) )
		{
			result = USB2WIFI_MISSING;
		}
		else if ( ( entry->Profile.Channel != profile->Channel ) || ( savePassword ) )
		{
			entry->Profile.Channel = profile->Channel;

			if ( savePassword )
			{
				CopyPassword( &network->Profile, profile->Password );
			}

			result = WiFi_WriteProfiles( &db );
		}
	}

	WiFi_FreeProfiles( &db );

	ReleaseSemaphore( &WiFiLock );

	return ( result );
}

// --

static struct WiFiClaim **
FindClaim( U32 adapter, U32 unit )
{
	struct WiFiClaim **link;

	link = &Claims;

	while ( ( *link ) && ( ( (*link)->Adapter != adapter ) || ( (*link)->Unit != unit ) ) )
	{
		link = &(*link)->Next;
	}

	return ( link );
}

// --

S32
WiFi_UnitClaimed( U32 adapter, U32 unit )
{
	// Caller holds WiFiLock.
	return ( *FindClaim( adapter, unit ) != NULL );
}

// --

void
WiFi_RememberScan( U32 adapter, U32 unit, const struct WiFiScan *scan )
{
	struct WiFiClaim *claim;

	if ( ( ! scan->Count ) || ( scan->Count > WIFI_SCAN_NETWORKS ) )
	{
		return;
	}

	ObtainSemaphore( &WiFiLock );

	claim = *FindClaim( adapter, unit );

	if ( ( claim ) && ( claim->Owner == FindTask( NULL ) ) )
	{
		if ( ! claim->Scan )
		{
			claim->Scan = AllocVecTags( sizeof( *claim->Scan ), TAG_END );
		}

		if ( claim->Scan )
		{
			claim->Scan->Count = scan->Count;

			memcpy( claim->Scan->Network, scan->Network, scan->Count * sizeof( *scan->Network ) );
		}
	}

	ReleaseSemaphore( &WiFiLock );
}

// --

void
WiFi_RestoreScan( U32 adapter, U32 unit, struct WiFiScan *scan )
{
	struct WiFiClaim *claim;

	ObtainSemaphore( &WiFiLock );

	claim = *FindClaim( adapter, unit );

	if ( ( claim ) && ( claim->Scan ) && ( claim->Owner == FindTask( NULL ) ) )
	{
		scan->Count = claim->Scan->Count;

		memcpy( scan->Network, claim->Scan->Network, scan->Count * sizeof( *scan->Network ) );
	}

	ReleaseSemaphore( &WiFiLock );
}

// --

static S32
WiFi_Claim( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit )
{
	struct WiFiClaim *claim;

	if ( ! WiFi_DeviceName( adapter ) )
	{
		return ( FALSE );
	}

	claim = AllocVecTags( sizeof( *claim ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! claim )
	{
		return ( FALSE );
	}

	claim->Owner = FindTask( NULL );

	claim->Adapter = adapter;
	claim->Unit = unit;

	ObtainSemaphore( &WiFiLock );

	if ( WiFi_UnitClaimed( adapter, unit ) )
	{
		ReleaseSemaphore( &WiFiLock );

		FreeVec( claim );

		return ( FALSE );
	}

	claim->Next = Claims;
	Claims = claim;

	ReleaseSemaphore( &WiFiLock );

	return ( TRUE );
}

// --

static void
WiFi_Unclaim( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit )
{
	struct WiFiClaim **link;
	struct WiFiClaim *claim;

	ObtainSemaphore( &WiFiLock );

	link = FindClaim( adapter, unit );

	claim = *link;

	if ( claim )
	{
		*link = claim->Next;
	}

	ReleaseSemaphore( &WiFiLock );

	if ( claim )
	{
		if ( claim->Scan )
		{
			FreeVec( claim->Scan );
		}

		FreeVec( claim );
	}
}

// --

const APTR WiFiVectors[] = //
	{
		WiFi_InterfaceObtain,  //
		WiFi_InterfaceRelease, //
		NULL,				   //
		NULL,				   //
		WiFi_Load,			   //
		WiFi_Save,			   //
		WiFi_Connected,		   //
		WiFi_Claim,			   //
		WiFi_Unclaim,		   //
		WiFi_Setup,			   //
		WiFi_StartPrompt,	   //
		WiFi_CancelPrompt,	   //
		WiFi_StartAuto,		   //
		WiFi_SetupRetry,	   //
		WiFi_EditProfiles,	   //
		(APTR)-1			   //
	};
