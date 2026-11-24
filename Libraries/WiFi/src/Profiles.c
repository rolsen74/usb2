
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

#include <interfaces/usb2eth.h>

// --

struct WiFiClaim
{
	struct WiFiClaim *Next;
	struct Task *Owner;
	struct WiFiScan *Scan;
	U32 Adapter;
	U32 Unit;
	ULONG Registration;
};

static struct WiFiClaim *Claims;

// --

ULONG
WiFi_AdapterProtocol( U32 adapter )
{
	switch ( adapter )
	{
		case USB2WIFI_RTL8188: return ( USB2ETH_ADAPTER_RTL8188 );
		case USB2WIFI_RT2870: return ( USB2ETH_ADAPTER_RT2870 );
		case USB2WIFI_RTL8811AU: return ( USB2ETH_ADAPTER_RTL8811AU );
		case USB2WIFI_RTL8821CU: return ( USB2ETH_ADAPTER_RTL8821CU );
	}

	return ( USB2ETH_ADAPTER_UNKNOWN );
}

// --

static void
CopyPassword( struct USB2WiFiProfile *target, const char *password )
{
	memset( target->Password, 0, sizeof( target->Password ) );

	strcpy( target->Password, password );
}

// --

static void
CopyUsablePassword( struct USB2WiFiProfile *profile, const char *password )
{
	if ( profile->Security != USB2WIFI_OPEN )
	{
		CopyPassword( profile, password );

		if ( ! WiFi_ValidProfile( profile, UINT32_MAX ) )
		{
			CopyPassword( profile, "" );
		}
	}
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
		network = WiFi_ProfileNetwork( &db, profile->SSID, FALSE );

		if ( network )
		{
			CopyUsablePassword( profile, network->Profile.Password );
		}
	}

	WiFi_FreeProfiles( &db );

	ReleaseSemaphore( &WiFiLock );
}

// --

static S32
ReadStation( U32 adapter, U32 unit, U8 *station, char *title, S32 *unique )
{
	struct IOSana2Req request;
	struct MsgPort *port;
	struct USB2EthIFace *ethernet;
	struct USB2EthAdapterInfo info;

	UWORD revision;
	ULONG registration;
	U32 wanted;
	U32 i;
	U32 matches;
	S32 found;

	wanted = WiFi_AdapterProtocol( adapter );
	registration = WiFi_ClaimedRegistration( adapter, unit );
	found = FALSE;
	*unique = FALSE;
	title[0] = 0;

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		return ( FALSE );
	}

	memset( &request, 0, sizeof( request ) );

	request.ios2_Req.io_Message.mn_Length = sizeof( request );
	request.ios2_Req.io_Message.mn_ReplyPort = port;

	if ( ! OpenDevice( USB2_ETH_DEVICE, unit, &request.ios2_Req, 0 ) )
	{
		revision = request.ios2_Req.io_Device->dd_Library.lib_Revision;

		if ( revision >= USB2ETH_ADAPTER_REVISION )
		{
			ethernet = (APTR) GetInterface( (APTR) request.ios2_Req.io_Device, "main", 1, NULL );

			if ( ethernet )
			{
				if ( ( ethernet->GetAdapter( &request.ios2_Req, unit ) == wanted ) &&
					( ( ! registration ) || ( ( revision >= USB2ETH_REGISTRATION_REVISION ) &&
						( ethernet->GetRegistrationID( &request.ios2_Req, unit ) == registration ) ) ) )
				{
					request.ios2_Req.io_Command = S2_GETSTATIONADDRESS;

					if ( ( ! DoIO( &request.ios2_Req ) ) && ( WiFi_HasStation( request.ios2_DstAddr ) ) )
					{
						memcpy( station, request.ios2_DstAddr, WIFI_STATION_SIZE );

						matches = 0;

						if ( revision >= USB2ETH_CLASS_REVISION )
						{
							if ( ( ethernet->GetAdapterInfo( &request.ios2_Req, &info ) ) && ( info.Type == wanted ) )
							{
								memcpy( title, info.Title, sizeof( info.Title ) );
							}
						}
						else if ( ( revision >= USB2ETH_TITLE_REVISION ) &&
							( ethernet->GetAdapterTitle( unit, title ) != wanted ) )
						{
							title[0] = 0;
						}

						for ( i = 0; i < USB2_ETH_MAX_UNITS; i++ )
						{
							if ( ethernet->GetAdapter( NULL, i ) == wanted )
							{
								matches++;
							}
						}

						found = ethernet->GetAdapter( &request.ios2_Req, unit ) == wanted;
						*unique = ( matches == 1 ) && ( found );
					}
				}

				DropInterface( (APTR) ethernet );
			}
		}

		CloseDevice( &request.ios2_Req );
	}

	FreeSysObject( ASOT_PORT, port );

	return ( found );
}

// --

static struct ProfileAdapter *
FindAdapter( struct ProfileDatabase *db, U32 adapter, const U8 *station, const char *title, S32 unique )
{
	struct ProfileAdapter *entry;
	struct ProfileAdapter *legacy;
	struct ProfileAdapter *match;

	S32 shared;

	match = NULL;
	shared = FALSE;

	for ( entry = db->Adapters; entry; entry = entry->Next )
	{
		if ( memcmp( entry->Station, station, WIFI_STATION_SIZE ) )
		{
			continue;
		}

		// The title distinguishes saved entries that share a MAC.
		if ( ( title[0] ) && ( ! strcasecmp( entry->Device, title ) ) )
		{
			return ( entry );
		}

		shared = shared || ( match != NULL );
		match = entry;
	}

	if ( ( match ) || ( ! unique ) )
	{
		return ( shared ? NULL : match );
	}

	legacy = NULL;

	for ( entry = db->Adapters; entry; entry = entry->Next )
	{
		if ( ( entry->Type != adapter ) &&
			( ( ! title[0] ) || ( strcasecmp( entry->Device, title ) ) ) )
		{
			continue;
		}

		// A second saved identity makes the old unit number ambiguous.
		if ( ( legacy ) || ( WiFi_HasStation( entry->Station ) ) )
		{
			return ( NULL );
		}

		legacy = entry;
	}

	return ( legacy );
}

// --

static S32
AdapterIdentityChanged( const struct ProfileAdapter *entry, const char *title )
{
	return ( ( ! WiFi_HasStation( entry->Station ) ) || ( entry->NeedsRewrite ) ||
		( ( title[0] ) && ( strcmp( entry->Device, title ) ) ) );
}

// --

static void
SetAdapterIdentity( struct ProfileAdapter *entry, U32 adapter, const U8 *station, const char *title )
{
	memcpy( entry->Station, station, sizeof( entry->Station ) );

	entry->Unit = 0;
	entry->Type = adapter;
	entry->NeedsRewrite = FALSE;

	if ( title[0] )
	{
		strcpy( entry->Device, title );
	}
}

// --

static S32
WiFi_Load( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit, struct USB2WiFiProfile *profile )
{
	struct ProfileDatabase db;
	struct ProfileAdapter *entry;
	struct ProfileNetwork *network;

	U8 station[WIFI_STATION_SIZE];
	char title[USB2ETH_TITLE_SIZE];
	S32 unique;
	S32 result;

	if ( ( ! profile ) || ( ! WiFi_AdapterProtocol( adapter ) ) )
	{
		return ( USB2WIFI_ERROR );
	}

	memset( profile, 0, sizeof( *profile ) );

	if ( ! ReadStation( adapter, unit, station, title, &unique ) )
	{
		return ( USB2WIFI_MISSING );
	}

	ObtainSemaphore( &WiFiLock );

	result = WiFi_ReadProfiles( &db );

	if ( result == USB2WIFI_OK )
	{
		entry = FindAdapter( &db, adapter, station, title, unique );

		result = USB2WIFI_MISSING;

		if ( entry )
		{
			network = WiFi_ProfileNetwork( &db, entry->Profile.SSID, FALSE );

			*profile = entry->Profile;
			profile->Options |= network->Profile.Options;

			CopyUsablePassword( profile, network->Profile.Password );

			result = WiFi_ValidProfile( profile, adapter ) ? USB2WIFI_OK : USB2WIFI_ERROR;

			if ( ( result == USB2WIFI_OK ) && ( AdapterIdentityChanged( entry, title ) ) )
			{
				SetAdapterIdentity( entry, adapter, station, title );

				WiFi_WriteProfiles( &db );
			}
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
	U8 station[WIFI_STATION_SIZE];
	char title[USB2ETH_TITLE_SIZE];
	S32 unique;
	S32 result;

	if ( ( ! profile ) || ( ! WiFi_AdapterProtocol( adapter ) ) || ( passwordAction > USB2WIFI_FORGET_PASSWORD )
		|| ( ! WiFi_ValidProfile( profile, adapter ) ) )
	{
		return ( USB2WIFI_ERROR );
	}

	if ( ! ReadStation( adapter, unit, station, title, &unique ) )
	{
		return ( USB2WIFI_ERROR );
	}

	ObtainSemaphore( &WiFiLock );

	result = WiFi_ReadProfiles( &db );

	if ( result >= 0 )
	{
		entry = FindAdapter( &db, adapter, station, title, unique );

		if ( ! entry )
		{
			device = title[0] ? title : WiFi_LegacyProfileName( adapter );

			entry = WiFi_ProfileStation( &db, device, unit, station, TRUE );
		}

		network = WiFi_ProfileNetwork( &db, profile->SSID, TRUE );

		result = USB2WIFI_ERROR;

		if ( ( entry ) && ( network ) )
		{
			SetAdapterIdentity( entry, adapter, station, title );

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

	U8 station[WIFI_STATION_SIZE];
	char title[USB2ETH_TITLE_SIZE];
	S32 unique;
	S32 result;

	if ( ( ! profile ) || ( ! WiFi_AdapterProtocol( adapter ) ) || ( ! WiFi_ValidProfile( profile, adapter ) ) )
	{
		return ( USB2WIFI_ERROR );
	}

	if ( ! ReadStation( adapter, unit, station, title, &unique ) )
	{
		return ( USB2WIFI_MISSING );
	}

	ObtainSemaphore( &WiFiLock );

	result = WiFi_ReadProfiles( &db );

	if ( result == USB2WIFI_OK )
	{
		entry = FindAdapter( &db, adapter, station, title, unique );

		network = WiFi_ProfileNetwork( &db, profile->SSID, FALSE );

		if ( ( ! entry ) || ( ! network ) || ( entry->Profile.Security != profile->Security )
			|| ( strcmp( entry->Profile.SSID, profile->SSID ) ) )
		{
			result = USB2WIFI_MISSING;
		}
		else if ( ( entry->Profile.Channel != profile->Channel ) || ( savePassword ) ||
			( AdapterIdentityChanged( entry, title ) ) )
		{
			SetAdapterIdentity( entry, adapter, station, title );

			entry->Profile.Channel = profile->Channel;

			if ( ( savePassword ) && ( profile->Security != USB2WIFI_OPEN ) )
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
FindOwnedClaim( U32 adapter, U32 unit )
{
	struct WiFiClaim **link;
	struct Task *owner;

	owner = FindTask( NULL );

	for ( link = &Claims; *link; link = &(*link)->Next )
	{
		if ( ( (*link)->Adapter == adapter ) && ( (*link)->Unit == unit ) && ( (*link)->Owner == owner ) )
		{
			break;
		}
	}

	return ( link );
}

// --

ULONG
WiFi_ClaimedRegistration( U32 adapter, U32 unit )
{
	struct WiFiClaim *claim;

	ULONG registration;

	ObtainSemaphore( &WiFiLock );

	claim = *FindOwnedClaim( adapter, unit );
	registration = claim ? claim->Registration : 0;

	ReleaseSemaphore( &WiFiLock );

	return ( registration );
}

// --

S32
WiFi_UnitClaimed( U32 adapter, U32 unit, ULONG registration )
{
	struct WiFiClaim *claim;

	// Caller holds WiFiLock.
	for ( claim = Claims; claim; claim = claim->Next )
	{
		if ( ( claim->Adapter == adapter ) && ( claim->Unit == unit ) && ( claim->Registration == registration ) )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
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

	claim = *FindOwnedClaim( adapter, unit );

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

	claim = *FindOwnedClaim( adapter, unit );

	if ( ( claim ) && ( claim->Scan ) && ( claim->Owner == FindTask( NULL ) ) )
	{
		scan->Count = claim->Scan->Count;

		memcpy( scan->Network, claim->Scan->Network, scan->Count * sizeof( *scan->Network ) );
	}

	ReleaseSemaphore( &WiFiLock );
}

// --

static S32
WiFi_ClaimRegistration( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit, U32 registration )
{
	struct WiFiClaim *claim;

	ULONG live;

	if ( ( ! WiFi_AdapterProtocol( adapter ) ) || ( ! WiFi_LiveRegistration( unit, &live ) ) || ( live != registration ) )
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
	claim->Registration = registration;

	ObtainSemaphore( &WiFiLock );

	if ( ( *FindOwnedClaim( adapter, unit ) ) || ( WiFi_UnitClaimed( adapter, unit, registration ) ) )
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

static S32
WiFi_Claim( struct USB2WiFiIFace *self, U32 adapter, U32 unit )
{
	ULONG registration;

	if ( ! WiFi_LiveRegistration( unit, &registration ) )
	{
		return ( FALSE );
	}

	return ( WiFi_ClaimRegistration( self, adapter, unit, registration ) );
}

// --

static void
WiFi_Unclaim( struct USB2WiFiIFace *self __attribute__((unused)), U32 adapter, U32 unit )
{
	struct WiFiClaim **link;
	struct WiFiClaim *claim;

	ObtainSemaphore( &WiFiLock );

	link = FindOwnedClaim( adapter, unit );

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
		WiFi_StartPromptForUnit, //
		WiFi_ClaimRegistration, //
		(APTR)-1			   //
	};
