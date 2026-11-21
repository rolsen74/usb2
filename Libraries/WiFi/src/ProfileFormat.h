
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef WIFI_PROFILE_FORMAT_H
#define WIFI_PROFILE_FORMAT_H

#define WIFI_DEVICE_NAME_SIZE 256

struct ProfileNetwork
{
	struct ProfileNetwork *Next;
	struct USB2WiFiProfile Profile;
	S32 ConflictingPassword;
};

struct ProfileAdapter
{
	struct ProfileAdapter *Next;
	struct USB2WiFiProfile Profile;
	U32 Unit;
	char Device[WIFI_DEVICE_NAME_SIZE];
};

struct ProfileDatabase
{
	struct ProfileNetwork *Networks;
	struct ProfileAdapter *Adapters;
};

enum ProfileEditResult
{
	PROFILE_EDIT_SAVED,
	PROFILE_EDIT_INVALID,
	PROFILE_EDIT_CHANGED,
	PROFILE_EDIT_ERROR
};

extern const char *const WiFi_ProfileSecurityNames[];

S32 WiFi_ValidProfile( const struct USB2WiFiProfile *profile, U32 adapter );
S32 WiFi_ReadProfiles( struct ProfileDatabase *db );
S32 WiFi_WriteProfiles( const struct ProfileDatabase *db );
void WiFi_FreeProfiles( struct ProfileDatabase *db );
struct ProfileNetwork *WiFi_ProfileNetwork( struct ProfileDatabase *db, const struct USB2WiFiProfile *profile, S32 create );
struct ProfileAdapter *WiFi_ProfileAdapter( struct ProfileDatabase *db, const char *device, U32 unit, S32 create );
S32 WiFi_LoadProfileEdit( struct ProfileDatabase *db, char **original, U32 *length );
enum ProfileEditResult WiFi_SaveProfileEdit( const struct ProfileDatabase *db, const char *original, U32 originalLength, S32 permanent );
void WiFi_FreeProfileText( char *text, U32 length );

#endif
