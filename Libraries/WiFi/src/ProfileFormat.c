
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "ProfileFormat.h"

#include <stdio.h>

// --

#define PROFILE_VARIABLE "usb2wifi"
#define PROFILE_FLAGS ( GVF_GLOBAL_ONLY | GVF_BINARY_VAR | GVF_DONT_NULL_TERM )
#define PROFILE_FILE_LIMIT ( 1024 * 1024 )
#define LEGACY_ADAPTERS 2
#define LEGACY_UNITS 32

static const U8 LegacyHeader[] = { 'U', 'W', 'F', 1 };
static const char *const LegacyNames[] = { "rtl8188", "rt2870" };
const char *const WiFi_ProfileSecurityNames[] = { "OPEN", "WPA", "WPA2", "WPA3", NULL };

// --

static void
ClearStorage( void *memory, U32 length )
{
	volatile U8 *data;

	data = memory;

	while ( length-- )
	{
		*data++ = 0;
	}
}

// --

S32
WiFi_ValidProfile ( const struct USB2WiFiProfile * profile, U32 adapter )
{
	const char * end;
	const char * p;

	if ( ( ! profile->SSID[0] ) || ( ! memchr ( profile->SSID, 0, sizeof ( profile->SSID ) ) )
		 || ( profile->Security > USB2WIFI_WPA3 ) || ( profile->Options & ~( USB2WIFI_PROFILE_AUTO | USB2WIFI_PROFILE_HIDDEN ) )
		 || ( ( ( adapter == USB2WIFI_RTL8188 && profile->Channel > 11 ) ||
			 ( adapter == USB2WIFI_RT2870 && profile->Channel > 13 ) ) )
		 || ( adapter == USB2WIFI_RT2870 && profile->Security == USB2WIFI_OPEN ) )
	{
		return ( FALSE );
	}

	end = memchr ( profile->Password, 0, sizeof ( profile->Password ) );

	if ( ! end )
	{
		return ( FALSE );
	}

	if ( end != profile->Password )
	{
		if ( ( profile->Security == USB2WIFI_OPEN )
			 || ( end - profile->Password < ( profile->Security == USB2WIFI_WPA3 ? 1 : 8 ) ) )
		{
			return ( FALSE );
		}

		for ( p = profile->Password; p < end; p++ )
		{
			if ( ( (U8)*p < 32 ) || ( (U8)*p > 126 ) )
			{
				return ( FALSE );
			}
		}
	}

	return ( TRUE );
}

// --

static const char *
SkipSpaces( const char *text )
{
	while ( ( *text == ' ' ) || ( *text == '\t' ) || ( *text == '\r' ) )
	{
		text++;
	}

	return ( text );
}

// --

static S32
ReadComma( const char **text )
{
	const char *cursor;

	cursor = SkipSpaces( *text );

	if ( *cursor != ',' )
	{
		return ( FALSE );
	}

	*text = cursor + 1;

	return ( TRUE );
}

// --

static S32
HexValue( char digit )
{
	if ( ( digit >= '0' ) && ( digit <= '9' ) )
	{
		return ( digit - '0' );
	}

	if ( ( digit >= 'A' ) && ( digit <= 'F' ) )
	{
		return ( digit - 'A' + 10 );
	}

	if ( ( digit >= 'a' ) && ( digit <= 'f' ) )
	{
		return ( digit - 'a' + 10 );
	}

	return ( -1 );
}

// --

static S32
ReadQuoted( const char **text, char *value, U32 capacity )
{
	const char *cursor;

	U8 byte;
	S32 high;
	S32 low;

	cursor = SkipSpaces( *text );

	if ( *cursor != '"' )
	{
		return ( FALSE );
	}

	cursor++;

	while ( ( *cursor ) && ( *cursor != '"' ) )
	{
		byte = (U8)*cursor++;

		if ( byte == '\\' )
		{
			byte = (U8)*cursor++;

			if ( byte == 'x' )
			{
				if ( ( ! cursor[0] ) || ( ! cursor[1] ) )
				{
					return ( FALSE );
				}

				high = HexValue( cursor[0] );

				low = HexValue( cursor[1] );

				if ( ( high < 0 ) || ( low < 0 ) )
				{
					return ( FALSE );
				}

				byte = (U8)( high * 16 + low );
				cursor += 2;
			}
			else if ( ( byte != '\\' ) && ( byte != '"' ) )
			{
				return ( FALSE );
			}
		}
		else if ( ( byte < 32 ) || ( byte > 126 ) )
		{
			return ( FALSE );
		}

		if ( ( ! byte ) || ( capacity <= 1 ) )
		{
			return ( FALSE );
		}

		*value++ = byte;
		capacity--;
	}

	if ( *cursor != '"' )
	{
		return ( FALSE );
	}

	*value = 0;
	*text = cursor + 1;

	return ( TRUE );
}

// --

static S32
ReadNumber( const char **text, U32 limit, U32 *value )
{
	const char *cursor;

	U32 number;

	cursor = SkipSpaces( *text );

	if ( ( *cursor < '0' ) || ( *cursor > '9' ) )
	{
		return ( FALSE );
	}

	number = 0;

	while ( ( *cursor >= '0' ) && ( *cursor <= '9' ) )
	{
		U32 digit;

		digit = *cursor++ - '0';

		if ( ( digit > limit ) || ( number > ( limit - digit ) / 10 ) )
		{
			return ( FALSE );
		}

		number = number * 10 + digit;
	}

	*value = number;
	*text = cursor;

	return ( TRUE );
}

// --

static S32
ReadName( const char **text, const char *const *names, U32 count, U32 *value )
{
	const char *cursor;

	U32 index;
	U32 length;

	cursor = SkipSpaces( *text );

	for ( index = 0; index < count; index++ )
	{
		length = strlen( names[index] );

		if ( ( ! strncmp( cursor, names[index], length ) ) && ( *SkipSpaces( cursor + length ) == ',' ) )
		{
			*value = index;
			*text = cursor + length;

			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

static char *
WriteQuoted( char *text, const char *value )
{
	static const char hex[] = "0123456789ABCDEF";

	U8 byte;

	*text++ = '"';

	while ( *value )
	{
		byte = (U8)*value++;

		if ( ( byte < 32 ) || ( byte > 126 ) )
		{
			*text++ = '\\';
			*text++ = 'x';
			*text++ = hex[byte >> 4];
			*text++ = hex[byte & 15];
		}
		else
		{
			if ( ( byte == '\\' ) || ( byte == '"' ) )
			{
				*text++ = '\\';
			}

			*text++ = byte;
		}
	}

	*text++ = '"';

	return ( text );
}

// --

struct ProfileNetwork *
WiFi_ProfileNetwork( struct ProfileDatabase *db, const struct USB2WiFiProfile *profile, S32 create )
{
	struct ProfileNetwork *entry;
	struct ProfileNetwork **next;

	next = &db->Networks;

	while ( *next )
	{
		entry = *next;

		if ( ( entry->Profile.Security == profile->Security ) && ( ! strcmp( entry->Profile.SSID, profile->SSID ) ) )
		{
			return ( entry );
		}

		next = &entry->Next;
	}

	entry = NULL;

	if ( create )
	{
		entry = AllocVecTags( sizeof( *entry ), AVT_ClearWithValue, 0, TAG_END );

		if ( entry )
		{
			strcpy( entry->Profile.SSID, profile->SSID );

			entry->Profile.Security = profile->Security;
			*next = entry;
		}
	}

	return ( entry );
}

// --

struct ProfileAdapter *
WiFi_ProfileAdapter( struct ProfileDatabase *db, const char *device, U32 unit, S32 create )
{
	struct ProfileAdapter *entry;
	struct ProfileAdapter **next;

	next = &db->Adapters;

	while ( *next )
	{
		entry = *next;

		if ( ( entry->Unit == unit ) && ( ! strcasecmp( entry->Device, device ) ) )
		{
			return ( entry );
		}

		next = &entry->Next;
	}

	entry = NULL;

	if ( create )
	{
		entry = AllocVecTags( sizeof( *entry ), AVT_ClearWithValue, 0, TAG_END );

		if ( entry )
		{
			strcpy( entry->Device, device );

			entry->Unit = unit;
			*next = entry;
		}
	}

	return ( entry );
}

// --

void
WiFi_FreeProfiles( struct ProfileDatabase *db )
{
	struct ProfileNetwork *network;
	struct ProfileAdapter *adapter;

	while ( db->Networks )
	{
		network = db->Networks;
		db->Networks = network->Next;

		ClearStorage( network->Profile.Password, sizeof( network->Profile.Password ) );

		FreeVec( network );
	}

	while ( db->Adapters )
	{
		adapter = db->Adapters;
		db->Adapters = adapter->Next;

		FreeVec( adapter );
	}
}

// --

static S32
ImportProfile( struct ProfileDatabase *db, U32 adapter, U32 unit, const struct USB2WiFiProfile *profile )
{
	struct ProfileNetwork *network;
	struct ProfileAdapter *entry;

	const char *device;

	device = WiFi_DeviceName( adapter );

	if ( ! WiFi_ValidProfile( profile, adapter ) )
	{
		return ( FALSE );
	}

	entry = WiFi_ProfileAdapter( db, device, unit, TRUE );

	if ( ( ! entry ) || ( entry->Profile.SSID[0] ) )
	{
		return ( FALSE );
	}

	network = WiFi_ProfileNetwork( db, profile, TRUE );

	if ( ! network )
	{
		return ( FALSE );
	}

	network->Profile.Options |= profile->Options & USB2WIFI_PROFILE_HIDDEN;

	// Conflicting passwords cannot be combined into one shared credential.
	if ( ( ! network->ConflictingPassword ) && ( profile->Password[0] ) )
	{
		if ( ( network->Profile.Password[0] ) && ( strcmp( network->Profile.Password, profile->Password ) ) )
		{
			ClearStorage( network->Profile.Password, sizeof( network->Profile.Password ) );

			network->ConflictingPassword = TRUE;
		}
		else
		{
			strcpy( network->Profile.Password, profile->Password );
		}
	}

	strcpy( entry->Profile.SSID, profile->SSID );

	entry->Profile.Security = profile->Security;
	entry->Profile.Channel = profile->Channel;
	entry->Profile.Options = profile->Options & USB2WIFI_PROFILE_AUTO;

	return ( TRUE );
}

// --

static S32
ReadRecord( const char *text, struct ProfileDatabase *db, struct USB2WiFiProfile *profile )
{
	struct ProfileNetwork *network;
	struct ProfileAdapter *entry;

	char device[WIFI_DEVICE_NAME_SIZE];
	U32 adapter;
	U32 unit;
	U32 security;
	U32 channel;
	U32 automatic;
	U32 hidden;
	U32 securityCount;

	text = SkipSpaces( text );

	if ( ( ! *text ) || ( *text == '#' ) )
	{
		return ( TRUE );
	}

	memset( profile, 0, sizeof( *profile ) );

	securityCount = sizeof( WiFi_ProfileSecurityNames ) / sizeof( *WiFi_ProfileSecurityNames ) - 1;

	if ( ! strncmp( text, "ETH:", 4 ) )
	{
		text += 4;

		if ( ( ! ReadQuoted( &text, device, sizeof( device ) ) ) || ( ! device[0] ) || ( ! ReadComma( &text ) )
			|| ( ! ReadNumber( &text, UINT32_MAX, &unit ) ) || ( ! ReadComma( &text ) )
			|| ( ! ReadQuoted( &text, profile->SSID, sizeof( profile->SSID ) ) ) || ( ! profile->SSID[0] ) || ( ! ReadComma( &text ) )
			|| ( ! ReadName( &text, WiFi_ProfileSecurityNames, securityCount, &security ) ) || ( ! ReadComma( &text ) )
			|| ( ! ReadNumber( &text, UINT8_MAX, &channel ) ) || ( ! ReadComma( &text ) )
			|| ( ! ReadNumber( &text, 1, &automatic ) ) || ( *SkipSpaces( text ) ) )
		{
			return ( FALSE );
		}

		entry = WiFi_ProfileAdapter( db, device, unit, TRUE );

		if ( ( ! entry ) || ( entry->Profile.SSID[0] ) )
		{
			return ( FALSE );
		}

		profile->Security = security;
		profile->Channel = channel;
		profile->Options = automatic ? USB2WIFI_PROFILE_AUTO : 0;
		entry->Profile = *profile;

		return ( TRUE );
	}

	if ( strncmp( text, "SSID:", 5 ) )
	{
		return ( FALSE );
	}

	text += 5;

	if ( ( ! ReadQuoted( &text, profile->SSID, sizeof( profile->SSID ) ) ) || ( ! ReadComma( &text ) ) )
	{
		return ( FALSE );
	}

	if ( ReadName( &text, WiFi_ProfileSecurityNames, securityCount, &security ) )
	{
		if ( ( ! ReadComma( &text ) ) || ( ! ReadQuoted( &text, profile->Password, sizeof( profile->Password ) ) )
			|| ( ! ReadComma( &text ) ) || ( ! ReadNumber( &text, 1, &hidden ) ) || ( *SkipSpaces( text ) ) )
		{
			return ( FALSE );
		}

		profile->Security = security;
		profile->Options = hidden ? USB2WIFI_PROFILE_HIDDEN : 0;

		if ( ( ! WiFi_ValidProfile( profile, UINT32_MAX ) ) || ( WiFi_ProfileNetwork( db, profile, FALSE ) ) )
		{
			return ( FALSE );
		}

		network = WiFi_ProfileNetwork( db, profile, TRUE );

		if ( ! network )
		{
			return ( FALSE );
		}

		network->Profile = *profile;

		return ( TRUE );
	}

	// Earlier ASCII files stored the adapter and network on the same line.
	if ( ( ! ReadNumber( &text, 1, &automatic ) ) || ( ! ReadComma( &text ) )
		|| ( ! ReadQuoted( &text, profile->Password, sizeof( profile->Password ) ) ) || ( ! ReadComma( &text ) )
		|| ( ! ReadName( &text, LegacyNames, LEGACY_ADAPTERS, &adapter ) ) || ( ! ReadComma( &text ) )
		|| ( ! ReadNumber( &text, LEGACY_UNITS - 1, &unit ) ) || ( ! ReadComma( &text ) )
		|| ( ! ReadName( &text, WiFi_ProfileSecurityNames, securityCount, &security ) ) || ( ! ReadComma( &text ) )
		|| ( ! ReadNumber( &text, 13, &channel ) ) || ( ! ReadComma( &text ) )
		|| ( ! ReadNumber( &text, 1, &hidden ) ) || ( *SkipSpaces( text ) ) )
	{
		return ( FALSE );
	}

	profile->Security = security;
	profile->Channel = channel;
	profile->Options = ( automatic ? USB2WIFI_PROFILE_AUTO : 0 ) | ( hidden ? USB2WIFI_PROFILE_HIDDEN : 0 );

	return ( ImportProfile( db, adapter, unit, profile ) );
}

// --

static S32
ReadText( char *text, U32 bytes, struct ProfileDatabase *db )
{
	struct USB2WiFiProfile profile;
	struct ProfileAdapter *adapter;

	char *next;
	S32 valid;
	S32 result;

	if ( memchr( text, 0, bytes ) )
	{
		return ( USB2WIFI_ERROR );
	}

	result = USB2WIFI_OK;
	text[bytes] = 0;

	while ( *text )
	{
		next = strchr( text, '\n' );

		if ( next )
		{
			*next = 0;
		}

		valid = ReadRecord( text, db, &profile );

		if ( next )
		{
			*next++ = '\n';
		}

		if ( ! valid )
		{
			result = USB2WIFI_ERROR;
			break;
		}

		if ( ! next )
		{
			break;
		}

		text = next;
	}

	ClearStorage( &profile, sizeof( profile ) );

	for ( adapter = db->Adapters; ( adapter ) && ( result == USB2WIFI_OK ); adapter = adapter->Next )
	{
		if ( ! WiFi_ProfileNetwork( db, &adapter->Profile, FALSE ) )
		{
			result = USB2WIFI_ERROR;
		}
	}

	return ( result );
}

// --

static S32
ReadLegacy( const char *buffer, U32 bytes, struct ProfileDatabase *db )
{
	struct USB2WiFiProfile profile;

	U32 adapter;
	U32 unit;
	S32 result;

	if ( bytes != sizeof( LegacyHeader ) + LEGACY_ADAPTERS * LEGACY_UNITS * sizeof( profile ) )
	{
		return ( USB2WIFI_ERROR );
	}

	result = USB2WIFI_OK;
	buffer += sizeof( LegacyHeader );

	for ( adapter = 0; adapter < LEGACY_ADAPTERS; adapter++ )
	{
		for ( unit = 0; unit < LEGACY_UNITS; unit++ )
		{
			memcpy( &profile, buffer, sizeof( profile ) );

			buffer += sizeof( profile );

			if ( ( profile.SSID[0] ) && ( ! ImportProfile( db, adapter, unit, &profile ) ) )
			{
				result = USB2WIFI_ERROR;
				goto done;
			}
		}
	}

done:

	ClearStorage( &profile, sizeof( profile ) );

	return ( result );
}

// --

static S32
ReadProfileFile( char **text, U32 *length )
{
	char *buffer;
	U32 capacity;
	S32 bytes;
	S32 result;

	*text = NULL;
	*length = 0;

	for ( capacity = 4096; capacity <= PROFILE_FILE_LIMIT; capacity *= 2 )
	{
		buffer = AllocVecTags( capacity + 1, TAG_END );

		if ( ! buffer )
		{
			return ( USB2WIFI_ERROR );
		}

		bytes = GetVar( PROFILE_VARIABLE, buffer, capacity + 1, PROFILE_FLAGS );

		if ( ( bytes >= 0 ) && ( bytes <= (S32)capacity ) )
		{
			buffer[bytes] = 0;
			*text = buffer;
			*length = bytes;

			return ( USB2WIFI_OK );
		}

		result = bytes < 0 && IoErr() == ERROR_OBJECT_NOT_FOUND ? USB2WIFI_MISSING : USB2WIFI_ERROR;

		WiFi_FreeProfileText( buffer, capacity + 1 );

		if ( bytes < 0 )
		{
			return ( result );
		}
	}

	return ( USB2WIFI_ERROR );
}

// --

static S32
IsLegacyFile( const char *text, U32 length )
{
	return ( length >= sizeof( LegacyHeader ) && ! memcmp( text, LegacyHeader, sizeof( LegacyHeader ) ) );
}

// --

S32
WiFi_ReadProfiles( struct ProfileDatabase *db )
{
	char *buffer;
	U32 length;
	S32 result;

	memset( db, 0, sizeof( *db ) );

	result = ReadProfileFile( &buffer, &length );

	if ( result == USB2WIFI_OK )
	{
		result = IsLegacyFile( buffer, length ) ? ReadLegacy( buffer, length, db ) : ReadText( buffer, length, db );

		WiFi_FreeProfileText( buffer, length );
	}

	return ( result );
}

// --

static char *
FormatProfiles( const struct ProfileDatabase *db )
{
	static const char networkHeading[] = "# SSID: name, security, password, hidden\n";
	static const char adapterHeading[] = "# ETH: device filename, unit, last SSID, security, last channel, autoconnect\n";
	const struct ProfileNetwork *network;
	const struct ProfileAdapter *adapter;
	const struct USB2WiFiProfile *profile;

	char *buffer;
	char *text;
	U32 capacity;

	capacity = sizeof( networkHeading ) + sizeof( adapterHeading );

	for ( network = db->Networks; network; network = network->Next )
	{
		capacity += 300;
	}

	for ( adapter = db->Adapters; adapter; adapter = adapter->Next )
	{
		capacity += 180 + 4 * strlen( adapter->Device );
	}

	if ( capacity > PROFILE_FILE_LIMIT )
	{
		return ( NULL );
	}

	buffer = AllocVecTags( capacity, TAG_END );

	if ( ! buffer )
	{
		return ( NULL );
	}

	text = buffer;

	if ( db->Networks )
	{
		memcpy( text, networkHeading, sizeof( networkHeading ) - 1 );

		text += sizeof( networkHeading ) - 1;
	}

	for ( network = db->Networks; network; network = network->Next )
	{
		profile = &network->Profile;

		memcpy( text, "SSID: ", 6 );

		text = WriteQuoted( text + 6, profile->SSID );

		text += sprintf( text, ", %s, ", WiFi_ProfileSecurityNames[profile->Security] );

		text = WriteQuoted( text, profile->Password );

		text += sprintf( text, ", %u\n", ( profile->Options & USB2WIFI_PROFILE_HIDDEN ) ? 1U : 0U );
	}

	if ( db->Adapters )
	{
		if ( db->Networks )
		{
			*text++ = '\n';
		}

		memcpy( text, adapterHeading, sizeof( adapterHeading ) - 1 );

		text += sizeof( adapterHeading ) - 1;
	}

	for ( adapter = db->Adapters; adapter; adapter = adapter->Next )
	{
		profile = &adapter->Profile;

		memcpy( text, "ETH: ", 5 );

		text = WriteQuoted( text + 5, adapter->Device );

		text += sprintf( text, ", %lu, ", (unsigned long)adapter->Unit );

		text = WriteQuoted( text, profile->SSID );

		text += sprintf( text, ", %s, %u, %u\n", WiFi_ProfileSecurityNames[profile->Security],
			(unsigned)profile->Channel, ( profile->Options & USB2WIFI_PROFILE_AUTO ) ? 1U : 0U );
	}

	*text = 0;

	return ( buffer );
}

// --

S32
WiFi_WriteProfiles( const struct ProfileDatabase *db )
{
	char *text;
	U32 length;
	S32 result;

	text = FormatProfiles( db );

	if ( ! text )
	{
		return ( USB2WIFI_ERROR );
	}

	length = strlen( text );

	result = SetVar( PROFILE_VARIABLE, text, length, PROFILE_FLAGS ) ? USB2WIFI_OK : USB2WIFI_ERROR;

	WiFi_FreeProfileText( text, length );

	return ( result );
}

// --

void
WiFi_FreeProfileText( char *text, U32 length )
{
	if ( text )
	{
		ClearStorage( text, length );

		FreeVec( text );
	}
}

// --

S32
WiFi_LoadProfileEdit( struct ProfileDatabase *db, char **original, U32 *length )
{
	S32 result;

	memset( db, 0, sizeof( *db ) );

	ObtainSemaphore( &WiFiLock );

	result = ReadProfileFile( original, length );

	ReleaseSemaphore( &WiFiLock );

	if ( result == USB2WIFI_OK )
	{
		result = IsLegacyFile( *original, *length ) ?
			ReadLegacy( *original, *length, db ) : ReadText( *original, *length, db );
	}

	return ( result );
}

// --

static enum ProfileEditResult
SaveProfileEdit( const char *original, U32 originalLength, char *edited, S32 permanent )
{
	struct ProfileDatabase db;

	char *current;
	U32 currentLength;
	U32 length;
	U32 index;
	U8 byte;
	S32 status;
	enum ProfileEditResult result;

	length = strlen( edited );

	if ( length > PROFILE_FILE_LIMIT )
	{
		return ( PROFILE_EDIT_INVALID );
	}

	for ( index = 0; index < length; index++ )
	{
		byte = edited[index];

		if ( ( byte != '\r' ) && ( byte != '\n' ) && ( byte != '\t' ) && ( ( byte < 32 ) || ( byte > 126 ) ) )
		{
			return ( PROFILE_EDIT_INVALID );
		}
	}

	memset( &db, 0, sizeof( db ) );

	status = ReadText( edited, length, &db );

	WiFi_FreeProfiles( &db );

	if ( status != USB2WIFI_OK )
	{
		return ( PROFILE_EDIT_INVALID );
	}

	ObtainSemaphore( &WiFiLock );

	status = ReadProfileFile( &current, &currentLength );
	result = PROFILE_EDIT_ERROR;

	if ( status != USB2WIFI_ERROR )
	{
		if ( ( ! original && current ) || ( original && ! current ) || ( originalLength != currentLength )
			|| ( originalLength && memcmp( original, current, originalLength ) ) )
		{
			result = PROFILE_EDIT_CHANGED;
		}
		else if ( SetVar( PROFILE_VARIABLE, edited, length, PROFILE_FLAGS | ( permanent ? GVF_SAVE_VAR : 0 ) ) )
		{
			result = PROFILE_EDIT_SAVED;
		}
	}

	WiFi_FreeProfileText( current, currentLength );

	ReleaseSemaphore( &WiFiLock );

	return ( result );
}

// --
enum ProfileEditResult
WiFi_SaveProfileEdit( const struct ProfileDatabase *db, const char *original, U32 originalLength, S32 permanent )
{
	char *text;
	enum ProfileEditResult result;

	text = FormatProfiles( db );

	if ( ! text )
	{
		return ( PROFILE_EDIT_ERROR );
	}

	result = SaveProfileEdit( original, originalLength, text, permanent );

	WiFi_FreeProfileText( text, strlen( text ) );

	return ( result );
}

// --
