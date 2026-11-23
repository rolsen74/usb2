
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "ProfileFormat.h"
#include "ProfileEditor.h"

#include <classes/window.h>
#include <gadgets/checkbox.h>
#include <gadgets/chooser.h>
#include <gadgets/layout.h>
#include <gadgets/string.h>
#include <images/label.h>
#include <stdio.h>
#include <stdlib.h>

// --

struct EntryFields
{
	Object *SSID;
	Object *Security;
	Object *Password;
	Object *Device;
	Object *Unit;
	Object *Channel;
	Object *Option;
	char PasswordBuffer[64];
	char UndoBuffer[64];
	char WorkBuffer[64];
};

// --

static Object *
AddField( const struct ProfileEditor *editor, Object *layout, Object *field, const char *caption )
{
	struct TagItem tags[2];

	Object *label;

	if ( ! field )
	{
		return ( NULL );
	}

	label = NewObject( editor->Classes[EDITOR_LABEL], NULL, LABEL_Text, caption, TAG_END );

	if ( ! label )
	{
		DisposeObject( field );

		return ( NULL );
	}

	tags[0].ti_Tag = CHILD_Label;
	tags[0].ti_Data = (uintptr_t)label;
	tags[1].ti_Tag = TAG_END;
	tags[1].ti_Data = 0;

	if ( ! WiFi_AddEditorChild( layout, field, tags ) )
	{
		DisposeObject( label );

		return ( NULL );
	}

	return ( field );
}

// --

static Object *
AddString( const struct ProfileEditor *editor, Object *layout, const char *caption, const char *text, U32 size )
{
	Object *field;

	field = NewObject( editor->Classes[EDITOR_STRING], NULL, GA_TabCycle, TRUE, STRINGA_MaxChars, size,
		STRINGA_MinVisible, 24, STRINGA_TextVal, text, TAG_END );

	return ( AddField( editor, layout, field, caption ) );
}

// --

static S32
BuildFields( const struct ProfileEditor *editor, Object *layout, struct EntryFields *fields, S32 adapter, APTR entry )
{
	static const struct USB2WiFiProfile empty = { .Security = USB2WIFI_WPA2 };
	const struct ProfileAdapter *eth;
	const struct ProfileNetwork *network;
	const struct USB2WiFiProfile *profile;

	char unit[12];
	char channel[4];
	U32 flag;

	eth = adapter ? entry : NULL;
	network = adapter ? NULL : entry;
	profile = &empty;

	if ( eth )
	{
		profile = &eth->Profile;
	}

	if ( network )
	{
		profile = &network->Profile;
	}

	if ( adapter )
	{
		snprintf( unit, sizeof( unit ), "%lu", (unsigned long)( eth ? eth->Unit : 0 ) );

		snprintf( channel, sizeof( channel ), "%u", (unsigned)profile->Channel );

		fields->Device = AddString( editor, layout, "Device filename", eth ? eth->Device : "", WIFI_DEVICE_NAME_SIZE );

		fields->Unit = AddString( editor, layout, "Unit", unit, sizeof( unit ) );

		if ( ( ! fields->Device ) || ( ! fields->Unit ) )
		{
			return ( FALSE );
		}
	}

	fields->SSID = AddString( editor, layout, "SSID", profile->SSID, sizeof( profile->SSID ) );

	fields->Security = AddField( editor, layout, NewObject( editor->Classes[EDITOR_CHOOSER], NULL,
		GA_TabCycle, TRUE, CHOOSER_LabelArray, WiFi_ProfileSecurityNames, CHOOSER_Selected, profile->Security, TAG_END ), "Security" );

	if ( ( ! fields->SSID ) || ( ! fields->Security ) )
	{
		return ( FALSE );
	}

	if ( adapter )
	{
		fields->Channel = AddString( editor, layout, "Last channel", channel, sizeof( channel ) );

		if ( ! fields->Channel )
		{
			return ( FALSE );
		}
	}
	else
	{
		fields->Password = AddField( editor, layout, NewObject( editor->Classes[EDITOR_STRING], NULL,
			GA_TabCycle, TRUE, STRINGA_MaxChars, sizeof( fields->PasswordBuffer ), STRINGA_MinVisible, 24,
			STRINGA_HookType, SHK_PASSWORD, STRINGA_Buffer, fields->PasswordBuffer, STRINGA_UndoBuffer, fields->UndoBuffer,
			STRINGA_WorkBuffer, fields->WorkBuffer, STRINGA_TextVal, profile->Password,
			STRINGA_DisablePopup, TRUE, STRINGA_AllowMarking, FALSE, TAG_END ), "Password" );

		if ( ! fields->Password )
		{
			return ( FALSE );
		}
	}

	flag = adapter ? USB2WIFI_PROFILE_AUTO : USB2WIFI_PROFILE_HIDDEN;

	fields->Option = AddField( editor, layout, NewObject( editor->Classes[EDITOR_CHECK], NULL,
		GA_TabCycle, TRUE, CHECKBOX_Checked, ( profile->Options & flag ) != 0, TAG_END ),
		adapter ? "Autoconnect" : "Hidden" );

	return ( fields->Option != NULL );
}

// --

static const char *
FieldText( Object *field )
{
	ULONG value;

	value = 0;

	GetAttr( STRINGA_TextVal, field, &value );

	return ( value ? (const char *)value : "" );
}

// --

static S32
FieldNumber( Object *field, U32 limit, U32 *value )
{
	const char *text;
	char *end;
	unsigned long long number;

	text = FieldText( field );

	if ( ( *text < '0' ) || ( *text > '9' ) )
	{
		return ( FALSE );
	}

	number = strtoull( text, &end, 10 );

	if ( ( *end ) || ( number > limit ) )
	{
		return ( FALSE );
	}

	*value = number;

	return ( TRUE );
}

// --

static U32
DeviceType( const char *device )
{
	if ( ! strcasecmp( device, WiFi_DeviceName( USB2WIFI_RTL8188 ) ) )
	{
		return ( USB2WIFI_RTL8188 );
	}

	if ( ! strcasecmp( device, WiFi_DeviceName( USB2WIFI_RT2870 ) ) )
	{
		return ( USB2WIFI_RT2870 );
	}

	return ( UINT32_MAX );
}

// --

static const char *
StoreAdapter( struct ProfileDatabase *db, const struct EntryFields *fields, struct ProfileAdapter *entry,
	struct USB2WiFiProfile *profile )
{
	struct ProfileAdapter *match;

	const char *device;
	U32 unit;
	U32 channel;

	device = FieldText( fields->Device );

	if ( ( ! device[0] ) || ( strlen( device ) >= WIFI_DEVICE_NAME_SIZE )
		|| ( ! FieldNumber( fields->Unit, UINT32_MAX, &unit ) )
		|| ( ! FieldNumber( fields->Channel, UINT8_MAX, &channel ) ) )
	{
		return ( "Enter a device filename, unit and channel" );
	}

	profile->Channel = channel;

	if ( ! WiFi_ProfileNetwork( db, profile, FALSE ) )
	{
		return ( "Add the SSID and security mode to Networks first" );
	}

	if ( ! WiFi_ValidProfile( profile, DeviceType( device ) ) )
	{
		return ( "The selected security mode or channel is invalid for this device" );
	}

	match = WiFi_ProfileStation( db, device, unit,
		( ( entry ) && ( WiFi_HasStation( entry->Station ) ) ) ? entry->Station : NULL, FALSE );

	if ( ( match ) && ( match != entry ) )
	{
		return ( "That adapter already exists" );
	}

	if ( ! entry )
	{
		entry = WiFi_ProfileAdapter( db, device, unit, TRUE );

		if ( ! entry )
		{
			return ( "Could not allocate the device entry" );
		}
	}

	strcpy( entry->Device, device );

	entry->Profile = *profile;
	entry->Unit = unit;

	return ( NULL );
}

// --

static S32
ReferencesNetwork( const struct ProfileAdapter *adapter, const struct ProfileNetwork *network )
{
	return ( adapter->Profile.Security == network->Profile.Security &&
		! strcmp( adapter->Profile.SSID, network->Profile.SSID ) );
}

// --

static const char *
StoreNetwork( struct ProfileDatabase *db, struct ProfileNetwork *entry, struct USB2WiFiProfile *profile )
{
	struct ProfileNetwork *match;
	struct ProfileAdapter *adapter;

	if ( ! WiFi_ValidProfile( profile, UINT32_MAX ) )
	{
		return ( "Check the SSID and password for this security mode" );
	}

	match = WiFi_ProfileNetwork( db, profile, FALSE );

	if ( ( match ) && ( match != entry ) )
	{
		return ( "That SSID and security mode already exist" );
	}

	if ( entry )
	{
		for ( adapter = db->Adapters; adapter; adapter = adapter->Next )
		{
			if ( ( ReferencesNetwork( adapter, entry ) ) && ( profile->Security == USB2WIFI_OPEN )
				&& ( DeviceType( adapter->Device ) == USB2WIFI_RT2870 ) )
			{
				return ( "An RT2870 device references this network and requires security" );
			}
		}

		for ( adapter = db->Adapters; adapter; adapter = adapter->Next )
		{
			if ( ReferencesNetwork( adapter, entry ) )
			{
				strcpy( adapter->Profile.SSID, profile->SSID );

				adapter->Profile.Security = profile->Security;
			}
		}
	}
	else
	{
		entry = WiFi_ProfileNetwork( db, profile, TRUE );

		if ( ! entry )
		{
			return ( "Could not allocate the network entry" );
		}
	}

	entry->Profile = *profile;

	return ( NULL );
}

// --

static const char *
StoreEntry( struct ProfileDatabase *db, const struct EntryFields *fields, S32 adapter, APTR entry )
{
	struct USB2WiFiProfile profile;

	const char *text;
	const char *error;
	volatile U8 *wipe;
	ULONG security;
	ULONG checked;
	U32 index;

	security = USB2WIFI_WPA2;
	checked = FALSE;

	GetAttr( CHOOSER_Selected, fields->Security, &security );

	GetAttr( CHECKBOX_Checked, fields->Option, &checked );

	text = FieldText( fields->SSID );

	if ( ( ! text[0] ) || ( strlen( text ) >= sizeof( profile.SSID ) ) || ( security > USB2WIFI_WPA3 ) )
	{
		return ( "Enter an SSID and select a security mode" );
	}

	memset( &profile, 0, sizeof( profile ) );

	strcpy( profile.SSID, text );

	profile.Security = security;

	if ( adapter )
	{
		profile.Options = checked ? USB2WIFI_PROFILE_AUTO : 0;

		return ( StoreAdapter( db, fields, entry, &profile ) );
	}

	text = FieldText( fields->Password );

	if ( strlen( text ) >= sizeof( profile.Password ) )
	{
		return ( "The password is too long" );
	}

	strcpy( profile.Password, text );

	profile.Options = checked ? USB2WIFI_PROFILE_HIDDEN : 0;

	error = StoreNetwork( db, entry, &profile );

	wipe = (volatile U8 *)profile.Password;

	for ( index = 0; index < sizeof( profile.Password ); index++ )
	{
		wipe[index] = 0;
	}

	return ( error );
}

// --

S32
WiFi_EditProfileEntry( struct ProfileEditor *editor, S32 adapter, APTR entry )
{
	struct EntryFields fields;
	struct Window *window;

	Object *object;
	Object *layout;
	Object *buttons;
	const char *error;
	volatile U8 *wipe;
	ULONG mask;
	U32 event;
	U32 index;
	U16 code;
	S32 changed;

	memset( &fields, 0, sizeof( fields ) );

	layout = NULL;
	changed = FALSE;
	mask = 0;

	object = NewObject( editor->Classes[EDITOR_WINDOW], NULL, WA_PubScreen, editor->Screen,
		WA_Title, adapter ? "Wi-Fi Device" : "Wi-Fi Network",
		WA_Width, editor->Screen->Width < 460 ? editor->Screen->Width : 460,
		WA_DragBar, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_AutoAdjust, TRUE,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP, WINDOW_Position, WPOS_CENTERSCREEN, TAG_END );

	if ( ! object )
	{
		return ( FALSE );
	}

	layout = NewObject( editor->Classes[EDITOR_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ( ! layout ) || ( ! BuildFields( editor, layout, &fields, adapter, entry ) ) )
	{
		goto done;
	}

	buttons = WiFi_AddEditorChild( layout, NewObject( editor->Classes[EDITOR_LAYOUT], NULL,
		LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_EvenSize, TRUE, LAYOUT_SpaceInner, TRUE, TAG_END ), NULL );

	if ( ( ! buttons ) || ( ! WiFi_AddEditorChild( buttons, WiFi_EditorButton( editor, EDITOR_USE, "_OK" ), NULL ) )
		|| ( ! WiFi_AddEditorChild( buttons, WiFi_EditorButton( editor, EDITOR_CANCEL, "_Cancel" ), NULL ) ) )
	{
		goto done;
	}

	SetAttrs( object, WINDOW_Layout, layout, TAG_END );

	layout = NULL;

	window = (struct Window *)IDoMethod( object, WM_OPEN, NULL );

	if ( ( ! window ) || ( ! GetAttr( WINDOW_SigMask, object, &mask ) ) || ( ! mask ) )
	{
		goto done;
	}

	while ( ! ( Wait( mask | SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C ) )
	{
		while ( ( event = IDoMethod( object, WM_HANDLEINPUT, &code ) ) != WMHI_LASTMSG )
		{
			if ( ( ( event & WMHI_CLASSMASK ) == WMHI_CLOSEWINDOW ) || ( event == ( WMHI_GADGETUP | EDITOR_CANCEL ) ) )
			{
				goto done;
			}

			if ( event == ( WMHI_GADGETUP | EDITOR_USE ) )
			{
				error = StoreEntry( &editor->Database, &fields, adapter, entry );

				if ( ! error )
				{
					changed = TRUE;
					goto done;
				}

				SetWindowTitles( window, error, (CONST_STRPTR)-1 );
			}
		}
	}

done:

	DisposeObject( object );

	if ( layout )
	{
		DisposeObject( layout );
	}

	wipe = (volatile U8 *)&fields;

	for ( index = 0; index < sizeof( fields ); index++ )
	{
		wipe[index] = 0;
	}

	return ( changed );
}

// --
