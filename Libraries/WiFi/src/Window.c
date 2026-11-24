
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

#include <classes/window.h>
#include <devices/inputevent.h>
#include <gadgets/listbrowser.h>
#include <interfaces/listbrowser.h>
#include <interfaces/usb2eth.h>
#include <gadgets/layout.h>
#include <gadgets/string.h>
#include <images/label.h>
#include <libraries/keymap.h>
#include <proto/intuition.h>
#include <stdio.h>

#include "WiFiScan.h"

// --

struct IntuitionIFace *IIntuition;
static struct Library *WiFiIntuitionBase;

enum WiFiClass
{
	WIFI_WINDOW,
	WIFI_LAYOUT,
	WIFI_STRING,
	WIFI_LISTBROWSER,
	WIFI_BUTTON,
	WIFI_LABEL,
	WIFI_CLASSES
};

enum WiFiControl
{
	WIFI_CONNECT = 2,
	WIFI_NETWORK_LIST,
	WIFI_SSID,
	WIFI_PASSWORD
};

#define WIFI_ACCEPT_FIELD 0x5743

struct WiFiControls
{
	Class *Classes[WIFI_CLASSES];
	Class *StringInput;
	Object *Layout;
	Object *SSID;
	Object *Networks;
	struct ListBrowserIFace *ListBrowser;
	struct List NetworkRows;
	struct Node *Rows[WIFI_SCAN_NETWORKS];
	char SecurityLabels[WIFI_SCAN_NETWORKS][sizeof( "Open WPA WPA2 WPA3" )];
	struct ColumnInfo Columns[3];
	Object *Password;
	struct USB2WiFiProfile Autofill;
	U32 Adapter;
	U32 Unit;
	S32 Retry;
	char PasswordBuffer[64];
	char UndoBuffer[64];
	char WorkBuffer[64];
};

struct WiFiWindowInput
{
	struct WiFiControls *Controls;
	struct USB2WiFiProfile *Profile;
	struct WiFiScan *Scan;
	struct Window *Window;
	Object *Object;
	U32 *Action;
	S32 Setup;
	S32 Result;
};

// --

static ULONG
StringInputDispatch( struct Hook *hook, APTR object, APTR message )
{
	struct gpInput *input;
	struct InputEvent *event;
	Class *class;

	class = (Class *) hook;
	input = message;

	if ( ( input->MethodID == GM_HANDLEINPUT ) && ( input->gpi_IEvent ) &&
		( input->gpi_IEvent->ie_Class == IECLASS_RAWKEY ) )
	{
		event = input->gpi_IEvent;

		// Report navigation keys through gadget-up while a string owns keyboard input.
		if ( ( event->ie_Code == RAWKEY_RETURN ) || ( event->ie_Code == RAWKEY_ENTER ) ||
			( event->ie_Code == RAWKEY_ESC ) )
		{
			if ( event->ie_Qualifier & IEQUALIFIER_REPEAT )
			{
				return ( GMR_MEACTIVE );
			}

			*input->gpi_Termination = ( event->ie_Code == RAWKEY_ESC ) ? RAWKEY_ESC : WIFI_ACCEPT_FIELD;

			return ( GMR_NOREUSE | GMR_VERIFY );
		}

		if ( ( class->cl_UserData ) && ( ( event->ie_Code == CURSORUP ) || ( event->ie_Code == CURSORDOWN ) ) )
		{
			*input->gpi_Termination = event->ie_Code;

			return ( GMR_NOREUSE | GMR_VERIFY );
		}
	}

	return ( IDoSuperMethodA( class, object, message ) );
}

// --

S32
WiFi_ObtainIntuition( void )
{
	S32 opened;

	ObtainSemaphore( & WiFiLock );

	if ( ! IIntuition )
	{
		WiFiIntuitionBase = OpenLibrary( "intuition.library", 50 );

		if ( WiFiIntuitionBase )
		{
			IIntuition = (struct IntuitionIFace *) GetInterface( WiFiIntuitionBase, "main", 1, NULL );

			if ( ! IIntuition )
			{
				CloseLibrary( WiFiIntuitionBase );

				WiFiIntuitionBase = NULL;
			}
		}
	}

	opened = IIntuition != NULL;

	ReleaseSemaphore( & WiFiLock );

	return ( opened );
}

// --

void
WiFi_CloseIntuition( void )
{
	if ( IIntuition )
	{
		DropInterface( (struct Interface *) IIntuition );

		CloseLibrary( WiFiIntuitionBase );
	}
}

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
ValidPassword( const char *password, U32 minimum )
{
	U32 length;

	length = 0;

	while( password[length] )
	{
		if ( ( length == 63 ) || ( (U8) password[length] < 32 ) || ( (U8) password[length] > 126 ) )
		{
			return ( FALSE );
		}

		length++;
	}

	return ( length >= minimum );
}

// --

static Object *
AddControl( struct WiFiControls *controls, Object *child, const char *caption, S32 fixedHeight )
{
	struct lmAddChild add;
	struct TagItem tags[3];

	Object *label;

	if ( ! child )
	{
		return ( NULL );
	}

	label = NULL;

	if ( caption )
	{
		label = NewObject( controls->Classes[WIFI_LABEL], NULL, LABEL_Text, caption, TAG_END );

		if ( ! label )
		{
			DisposeObject( child );

			return ( NULL );
		}
	}

	tags[0].ti_Tag = label ? CHILD_Label : TAG_IGNORE;
	tags[0].ti_Data = (uintptr_t) label;
	tags[1].ti_Tag = fixedHeight ? CHILD_WeightedHeight : TAG_IGNORE;
	tags[1].ti_Data = 0;
	tags[2].ti_Tag = TAG_END;
	tags[2].ti_Data = 0;

	add.MethodID = LM_ADDCHILD;
	add.lm_Window = NULL;
	add.lm_Object = child;
	add.lm_ObjectAttrs = tags;

	if ( ! IDoMethodA( controls->Layout, (Msg) & add ) )
	{
		if ( label )
		{
			DisposeObject( label );
		}

		DisposeObject( child );

		return ( NULL );
	}

	return ( child );
}

// --

static const char *
SavedNetworkPassword( struct WiFiControls *controls, const struct USB2WiFiProfile *profile,
	const struct WiFiScan *scan, U32 index )
{
	const struct WiFiNetwork *network;
	struct USB2WiFiProfile *autofill;

	autofill = & controls->Autofill;

	Erase( autofill, sizeof( *autofill ) );

	if ( index >= scan->Count )
	{
		return ( "" );
	}

	network = & scan->Network[index];

	if ( ( controls->Retry ) && ( ! strcmp( network->SSID, profile->SSID ) ) )
	{
		return ( "" );
	}

	autofill->Security = network->Security;

	strcpy( autofill->SSID, network->SSID );

	if ( ( network->Security == profile->Security ) && ( ! strcmp( network->SSID, profile->SSID ) ) &&
		( profile->Password[0] ) )
	{
		strcpy( autofill->Password, profile->Password );
	}
	else
	{
		WiFi_FindPassword( autofill );
	}

	return ( autofill->Password );
}

// --

static void
SelectNetwork( struct WiFiControls *controls, const struct USB2WiFiProfile *profile,
	const struct WiFiScan *scan, struct Window *window, U32 index )
{
	const char *password;

	if ( index >= scan->Count )
	{
		return;
	}

	password = SavedNetworkPassword( controls, profile, scan, index );

	RefreshSetGadgetAttrs( (struct Gadget *) controls->SSID, window, NULL,
		STRINGA_TextVal, scan->Network[index].SSID, TAG_END );

	RefreshSetGadgetAttrs( (struct Gadget *) controls->Password, window, NULL,
		STRINGA_TextVal, password, TAG_END );

	Erase( controls->UndoBuffer, sizeof( controls->UndoBuffer ) );

	Erase( controls->WorkBuffer, sizeof( controls->WorkBuffer ) );
}

// --

static void
ActivatePassword( const struct WiFiControls *controls, Object *object, struct Window *window )
{
	ULONG value;

	value = 0;

	GetAttr( STRINGA_TextVal, controls->Password, & value );

	IDoMethod( object, WM_ACTIVATEGADGET, controls->Password );

	RefreshSetGadgetAttrs( (struct Gadget *) controls->Password, window, NULL,
		STRINGA_BufferPos, value ? strlen( (const char *) value ) : 0, TAG_END );
}

// --

static void
MoveNetworkSelection( struct WiFiControls *controls, const struct USB2WiFiProfile *profile,
	const struct WiFiScan *scan, Object *object, struct Window *window, U32 key )
{
	ULONG selected;
	U32 next;

	if ( ! scan->Count )
	{
		return;
	}

	selected = ~0UL;

	GetAttr( LISTBROWSER_Selected, controls->Networks, & selected );

	if ( selected >= scan->Count )
	{
		next = key == CURSORUP ? scan->Count - 1 : 0;
	}
	else
	{
		next = selected;

		if ( ( key == CURSORUP ) && ( selected > 0 ) )
		{
			next--;
		}
		else if ( ( key == CURSORDOWN ) && ( selected + 1 < scan->Count ) )
		{
			next++;
		}
	}

	RefreshSetGadgetAttrs( (struct Gadget *) controls->Networks, window, NULL,
		LISTBROWSER_Selected, next, LISTBROWSER_MakeVisible, next, TAG_END );

	SelectNetwork( controls, profile, scan, window, next );

	ActivatePassword( controls, object, window );
}

// --

static void
NetworkSecurityLabel( char *label, U8 modes )
{
	static const char *const names[] = { "Open", "WPA", "WPA2", "WPA3" };

	char *text;
	U32 security;

	text = label;

	for( security = 0; security < sizeof( names ) / sizeof( names[0] ); security++ )
	{
		if ( modes & ( 1U << security ) )
		{
			if ( text != label )
			{
				*text++ = ' ';
			}

			strcpy( text, names[security] );

			text += strlen( names[security] );
		}
	}

	if ( text == label )
	{
		strcpy( label, "Unsupported" );
	}
}

// --

static S32
AddNetworkRow( struct WiFiControls *controls, const struct WiFiNetwork *network, U32 index )
{
	struct Node *row;

	NetworkSecurityLabel( controls->SecurityLabels[index], network->SecurityModes );

	row = controls->ListBrowser->AllocListBrowserNode( 2,
		LBNA_Column, 0, LBNCA_Text, network->SSID,
		LBNA_Column, 1, LBNCA_Text, controls->SecurityLabels[index], TAG_END );

	if ( ! row )
	{
		return ( FALSE );
	}

	controls->Rows[index] = row;

	AddTail( & controls->NetworkRows, row );

	return ( TRUE );
}

// --

static S32
BuildControls( struct WiFiControls *controls, const struct USB2WiFiProfile *profile,
	const struct WiFiScan *scan, S32 setup )
{
	static const struct ColumnInfo columns[] = {
		{ 60, "SSID", 0 },
		{ 40, "Security", 0 },
		{ -1, NULL, 0 }
	};

	Object *button;
	const char *ssid;
	S32 selected;
	U32 index;

	if ( setup )
	{
		ssid = "";
		selected = -1;

		if ( ( controls->Retry ) && ( profile->Options & USB2WIFI_PROFILE_HIDDEN ) )
		{
			ssid = profile->SSID;
		}

		for( index = 0; index < scan->Count; index++ )
		{
			if ( ! AddNetworkRow( controls, & scan->Network[index], index ) )
			{
				return ( FALSE );
			}

			if ( ! strcmp( scan->Network[index].SSID, profile->SSID ) )
			{
				selected = index;
				ssid = scan->Network[index].SSID;
			}
		}

		memcpy( controls->Columns, columns, sizeof( columns ) );

		controls->Networks = AddControl( controls, NewObject( controls->Classes[WIFI_LISTBROWSER], NULL,
			GA_ID, WIFI_NETWORK_LIST, GA_RelVerify, TRUE, LISTBROWSER_ShowSelected, TRUE,
			LISTBROWSER_ColumnTitles, TRUE, LISTBROWSER_ColumnInfo, controls->Columns,
			LISTBROWSER_Labels, & controls->NetworkRows, LISTBROWSER_Selected, selected,
			LISTBROWSER_Top, selected < 0 ? 0 : selected,
			LISTBROWSER_MinVisible, 6, TAG_END ), NULL, FALSE );

		controls->SSID = AddControl( controls, NewObject( controls->StringInput, NULL,
			GA_ID, WIFI_SSID, GA_RelVerify, TRUE, GA_TabCycle, TRUE, STRINGA_MaxChars, 33, STRINGA_MinVisible, 22,
			STRINGA_TextVal, ssid, TAG_END ), "SSID", FALSE );

		if ( ( ! controls->Networks ) || ( ! controls->SSID ) )
		{
			return ( FALSE );
		}
	}

	controls->Password = AddControl( controls, NewObject( controls->StringInput, NULL,
		GA_ID, WIFI_PASSWORD, GA_RelVerify, TRUE, GA_TabCycle, TRUE,
		STRINGA_MaxChars, sizeof( controls->PasswordBuffer ), STRINGA_MinVisible, 22,
		STRINGA_HookType, SHK_PASSWORD, STRINGA_Buffer, controls->PasswordBuffer,
		STRINGA_UndoBuffer, controls->UndoBuffer, STRINGA_WorkBuffer, controls->WorkBuffer,
		STRINGA_TextVal, "", STRINGA_DisablePopup, TRUE, STRINGA_AllowMarking, FALSE, TAG_END ), "Password", ! setup );

	if ( ! controls->Password )
	{
		return ( FALSE );
	}

	if ( ! setup )
	{
		return ( TRUE );
	}

	button = AddControl( controls, NewObject( controls->Classes[WIFI_BUTTON], NULL,
		GA_Text, "Connect", GA_ID, WIFI_CONNECT, GA_RelVerify, TRUE, TAG_END ), NULL, FALSE );

	return ( button != NULL );
}

// --

static S32
ReadControls( const struct WiFiControls *controls, struct USB2WiFiProfile *profile,
	struct Window *window, const struct WiFiScan *scan, S32 setup, U32 *action )
{
	const struct WiFiNetwork *network;

	const char *ssid;
	const char *password;
	const char *error;
	ULONG value;
	U32 index;
	U32 channel;
	U32 security;
	U32 options;
	S32 keepPassword;

	ssid = profile->SSID;
	security = profile->Security;
	channel = profile->Channel;
	options = profile->Options;
	keepPassword = FALSE;
	value = 0;
	error = NULL;

	if ( setup )
	{
		GetAttr( STRINGA_TextVal, controls->SSID, & value );

		ssid = (const char *) value;
		options = profile->Options & USB2WIFI_PROFILE_AUTO;

		if ( ( ! ssid ) || ( ! *ssid ) || ( strlen( ssid ) >= sizeof( profile->SSID ) ) )
		{
			SetWindowTitles( window, "Enter a network name of 1 to 32 bytes", (CONST_STRPTR) -1 );

			return ( FALSE );
		}

		for( index = 0; index < scan->Count; index++ )
		{
			network = & scan->Network[index];

			if ( ! strcmp( ssid, network->SSID ) )
			{
				break;
			}
		}

		if ( index < scan->Count )
		{
			security = scan->Network[index].Security;
			channel = scan->Network[index].Channel;

			if ( security == WIFI_SECURITY_UNSUPPORTED )
			{
				SetWindowTitles( window, "This adapter cannot use the selected network's security or rates", (CONST_STRPTR) -1 );

				return ( FALSE );
			}
		}
		else
		{
			options |= USB2WIFI_PROFILE_HIDDEN;

			if ( strcmp( ssid, profile->SSID ) )
			{
				security = WIFI_SECURITY_UNSUPPORTED;
				channel = 0;
			}
		}

		keepPassword = security == profile->Security && ! strcmp( ssid, profile->SSID ) && profile->Password[0];
	}

	value = 0;

	GetAttr( STRINGA_TextVal, controls->Password, & value );

	password = value ? (const char *) value : "";

	if ( ( ! setup ) && ( ! *password ) )
	{
		return ( USB2WIFI_BACK_TO_PICKER );
	}

	// A manually entered SSID must never inherit the password filled from a list row.
	if ( ( controls->Autofill.Password[0] ) && ( ! strcmp( password, controls->Autofill.Password ) ) &&
		( ( security != controls->Autofill.Security ) || ( strcmp( ssid, controls->Autofill.SSID ) ) ) )
	{
		password = "";
	}

	if ( ( security == USB2WIFI_OPEN ) && ( *password ) )
	{
		error = "Clear the password for an open network";
	}
	else if ( ( security != USB2WIFI_OPEN ) && ( ( ! setup ) || ( controls->Retry ) || ( *password ) ) &&
		( ! ValidPassword( password, ( security == USB2WIFI_WPA3 || security == WIFI_SECURITY_UNSUPPORTED ) ? 1 : 8 ) ) )
	{
		error = ( security == USB2WIFI_WPA3 || security == WIFI_SECURITY_UNSUPPORTED ) ?
			"Enter 1 to 63 printable characters" : "Enter 8 to 63 printable characters";
	}

	if ( error )
	{
		SetWindowTitles( window, error, (CONST_STRPTR) -1 );

		return ( FALSE );
	}

	if ( setup )
	{
		strcpy( profile->SSID, ssid );

		profile->Security = security;
		profile->Channel = channel;
		profile->Options = options;
	}

	if ( ( *password ) || ( ! keepPassword ) )
	{
		Erase( profile->Password, sizeof( profile->Password ) );

		strcpy( profile->Password, password );
	}

	*action = profile->Password[0] ? USB2WIFI_SAVE_PASSWORD : USB2WIFI_KEEP_PASSWORD;

	return ( TRUE );
}

// --

static S32
UpdateNetworkRow( struct WiFiWindowInput *input )
{
	struct WiFiControls *controls;
	const struct USB2WiFiProfile *profile;
	const struct WiFiNetwork *network;
	struct Node *row;

	const char *typedSSID;
	const char *typedPassword;
	ULONG selected;
	ULONG top;
	ULONG ssid;
	ULONG password;
	U32 index;
	S32 choose;
	S32 focusPassword;

	controls = input->Controls;
	profile = input->Profile;
	index = input->Scan->Changed - 1;
	input->Scan->Changed = 0;
	network = & input->Scan->Network[index];
	row = controls->Rows[index];
	selected = ~0UL;
	top = 0;
	choose = FALSE;

	GetAttr( LISTBROWSER_Selected, controls->Networks, & selected );

	GetAttr( LISTBROWSER_Top, controls->Networks, & top );

	if ( ( selected == index ) || ( selected >= input->Scan->Count ) )
	{
		ssid = 0;
		password = 0;

		GetAttr( STRINGA_TextVal, controls->SSID, & ssid );

		GetAttr( STRINGA_TextVal, controls->Password, & password );

		typedSSID = ssid ? (const char *) ssid : "";
		typedPassword = password ? (const char *) password : "";

		if ( ! strcmp( typedPassword, controls->Autofill.Password ) )
		{
			if ( selected == index )
			{
				choose = ! strcmp( typedSSID, network->SSID );
			}
			else if ( ! typedSSID[0] )
			{
				choose = ! strcmp( profile->SSID, network->SSID );
			}
		}
	}

	SetAttrs( controls->Networks, LISTBROWSER_Labels, ~0UL, TAG_END );

	if ( row )
	{
		NetworkSecurityLabel( controls->SecurityLabels[index], network->SecurityModes );

		controls->ListBrowser->SetListBrowserNodeAttrs( row, LBNA_Column, 1,
			LBNCA_Text, controls->SecurityLabels[index], TAG_END );
	}
	else if ( ! AddNetworkRow( controls, network, index ) )
	{
		input->Result = USB2WIFI_ERROR;

		return ( FALSE );
	}

	controls->StringInput->cl_UserData = input->Scan->Count;
	focusPassword = choose && ( selected != index );

	if ( choose )
	{
		selected = index;

		SelectNetwork( controls, profile, input->Scan, input->Window, index );
	}

	RefreshSetGadgetAttrs( (struct Gadget *) controls->Networks, input->Window, NULL,
		LISTBROWSER_Labels, & controls->NetworkRows, LISTBROWSER_Top, top, LISTBROWSER_Selected, selected,
		( choose ? LISTBROWSER_MakeVisible : TAG_IGNORE ), index, TAG_END );

	if ( focusPassword )
	{
		ActivatePassword( controls, input->Object, input->Window );
	}

	return ( TRUE );
}

// --

static S32
HandleWindowInput( void *context )
{
	struct WiFiWindowInput *input;
	struct WiFiControls *controls;
	struct USB2WiFiProfile *profile;
	struct WiFiScan *scan;
	struct Window *window;
	struct InputEvent *keyEvent;

	Object *object;
	ULONG inputValue;
	U32 event;
	U32 key;
	U16 code;
	S32 setup;
	S32 status;
	S32 stringEvent;

	input = context;
	controls = input->Controls;
	profile = input->Profile;
	scan = input->Scan;
	window = input->Window;
	object = input->Object;
	setup = input->Setup;

	if ( ( scan->Changed ) && ( ! UpdateNetworkRow( input ) ) )
	{
		return ( FALSE );
	}

	for( ;; )
	{
		code = 0;

		event = IDoMethod( object, WM_HANDLEINPUT, & code );

		if ( event == WMHI_LASTMSG )
		{
			break;
		}

		key = 0;
		keyEvent = NULL;
		stringEvent = ( event == ( WMHI_GADGETUP | WIFI_SSID ) ) || ( event == ( WMHI_GADGETUP | WIFI_PASSWORD ) );

		if ( ( event & WMHI_CLASSMASK ) == WMHI_RAWKEY )
		{
			key = event & WMHI_KEYMASK;
			inputValue = 0;

			GetAttr( WINDOW_InputEvent, object, & inputValue );

			keyEvent = (struct InputEvent *) inputValue;

			if ( ( keyEvent ) && ( keyEvent->ie_Qualifier & IEQUALIFIER_REPEAT ) &&
				( ( key == RAWKEY_ESC ) || ( key == RAWKEY_RETURN ) || ( key == RAWKEY_ENTER ) ) )
			{
				continue;
			}
		}

		if ( ( ( event & WMHI_CLASSMASK ) == WMHI_CLOSEWINDOW ) ||
			( key == RAWKEY_ESC ) || ( ( stringEvent ) && ( code == RAWKEY_ESC ) ) )
		{
			input->Result = setup ? USB2WIFI_MISSING : USB2WIFI_BACK_TO_PICKER;

			return ( FALSE );
		}

		if ( ( event == ( WMHI_GADGETUP | WIFI_NETWORK_LIST ) ) && ( controls->SSID ) )
		{
			ULONG selected;

			selected = ~0UL;

			GetAttr( LISTBROWSER_Selected, controls->Networks, & selected );

			SelectNetwork( controls, profile, scan, window, selected );
		}

		if ( ( event == ( WMHI_GADGETUP | WIFI_SSID ) ) && ( code == WIFI_ACCEPT_FIELD ) )
		{
			ActivatePassword( controls, object, window );
		}

		if ( ( stringEvent ) &&
			( ( code == CURSORUP ) || ( code == CURSORDOWN ) ) )
		{
			MoveNetworkSelection( controls, profile, scan, object, window, code );
		}

		if ( setup )
		{
			if ( key == RAWKEY_TAB )
			{
				IDoMethod( object, WM_ACTIVATEGADGET,
					( ( keyEvent ) && ( keyEvent->ie_Qualifier & ( IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT ) ) ) ?
						controls->Password : controls->SSID );
			}
			else if ( ( key == CURSORUP ) || ( key == CURSORDOWN ) )
			{
				MoveNetworkSelection( controls, profile, scan, object, window, key );
			}
		}

		if ( ( event == ( WMHI_GADGETUP | WIFI_CONNECT ) ) ||
			( ( event == ( WMHI_GADGETUP | WIFI_PASSWORD ) ) && ( code == WIFI_ACCEPT_FIELD ) ) ||
			( ( ! setup ) && ( ( key == RAWKEY_RETURN ) || ( key == RAWKEY_ENTER ) ) ) )
		{
			status = ReadControls( controls, profile, window, scan, setup, input->Action );

			if ( ! status )
			{
				continue;
			}

			input->Result = setup ? USB2WIFI_CONNECT_NOW : status;

			if ( setup )
			{
				DebugPrintF( "USB2 Wi-Fi: picker accepted %s for %s unit %lu.\n",
					event == ( WMHI_GADGETUP | WIFI_CONNECT ) ? "Connect button" : "Enter",
					controls->Adapter == USB2WIFI_RT2870 ? "RT2870" :
						( controls->Adapter == USB2WIFI_RTL8811AU ? "RTL8811AU" :
			( controls->Adapter == USB2WIFI_RTL8821CU ? "RTL8821CU" : "RTL8188EU" ) ), controls->Unit );
			}

			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static void
ReadAdapterTitle( U32 adapter, U32 unit, char *title )
{
	struct IORequest request;
	struct USB2EthIFace *ethernet;

	ULONG expected;

	title[0] = 0;

	if ( unit >= USB2_ETH_MAX_UNITS )
	{
		return;
	}

	memset( & request, 0, sizeof( request ) );

	request.io_Message.mn_Length = sizeof( request );
	expected = WiFi_AdapterProtocol( adapter );

	if ( OpenDevice( USB2_ETH_DEVICE, USB2_ETH_BACKEND_UNIT, & request, 0 ) )
	{
		return;
	}

	if ( request.io_Device->dd_Library.lib_Revision >= USB2ETH_TITLE_REVISION )
	{
		ethernet = (APTR) GetInterface( (APTR) request.io_Device, "main", 1, NULL );

		if ( ethernet )
		{
			if ( ethernet->GetAdapterTitle( unit, title ) != expected )
			{
				title[0] = 0;
			}

			DropInterface( (APTR) ethernet );
		}
	}

	CloseDevice( & request );
}

// --

S32
WiFi_Window( struct USB2WiFiProfile *profile, U32 adapter, U32 unit, S32 setup, S32 retry, U32 *action )
{
	static const char *names[] = {
		"window.class", "gadgets/layout.gadget", "gadgets/string.gadget", "gadgets/listbrowser.gadget",
		"gadgets/button.gadget", "images/label.image"
	};

	struct ClassLibrary *bases[WIFI_CLASSES];
	struct WiFiControls controls;
	struct WiFiScan scan;
	struct WiFiWindowInput input;
	struct Screen *screen;
	struct Window *window;

	Object *object;
	const char *adapterLabel;
	const char *passwordLabel;
	char adapterTitle[USB2ETH_TITLE_SIZE];
	char title[192];
	ULONG mask;
	ULONG registration;
	U32 index;
	S32 preferredHeight;
	S32 width;
	S32 height;
	S32 result;

	memset( & controls, 0, sizeof( controls ) );

	controls.Adapter = adapter;
	controls.Unit = unit;
	controls.Retry = retry;

	memset( bases, 0, sizeof( bases ) );

	memset( & scan, 0, sizeof( scan ) );

	NewList( & controls.NetworkRows );

	object = NULL;
	screen = NULL;
	result = USB2WIFI_ERROR;
	mask = 0;

	ReadAdapterTitle( adapter, unit, adapterTitle );

	adapterLabel = adapterTitle[0] ? adapterTitle : ( adapter == USB2WIFI_RT2870 ? "RT2870" :
		( adapter == USB2WIFI_RTL8811AU ? "RTL8811AU" :
			( adapter == USB2WIFI_RTL8821CU ? "RTL8821CU" : "RTL8188EU" ) ) );

	if ( ! WiFi_ObtainIntuition() )
	{
		FPrintf( ErrorOutput(), "Could not open intuition.library.\n" );

		return ( USB2WIFI_ERROR );
	}

	// AUTO may start before Workbench. Only the background manager waits here.
	for( index = 0; index < 120; index++ )
	{
		if ( SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
		{
			result = USB2WIFI_MISSING;
			goto done;
		}

		screen = LockPubScreen( setup ? NULL : "Workbench" );

		if ( screen )
		{
			break;
		}

		Delay( 25 );
	}

	if ( ! screen )
	{
		FPrintf( ErrorOutput(), "No screen available for Wi-Fi settings or password entry.\n" );

		goto done;
	}

	for( index = 0; index < WIFI_CLASSES; index++ )
	{
		if ( ( ! setup ) && ( ( index == WIFI_LISTBROWSER ) || ( index == WIFI_BUTTON ) ) )
		{
			continue;
		}

		bases[index] = OpenClass( names[index], 50, & controls.Classes[index] );

		if ( ! bases[index] )
		{
			FPrintf( ErrorOutput(), "Could not open %s.\n", names[index] );

			goto done;
		}
	}

	if ( setup )
	{
		controls.ListBrowser = (struct ListBrowserIFace *) GetInterface( (struct Library *) bases[WIFI_LISTBROWSER],
			"main", 1, NULL );

		if ( ! controls.ListBrowser )
		{
			goto done;
		}

		snprintf( title, sizeof( title ), "%s - %s",
			retry ? "Wi-Fi authentication did not complete" : "Wi-Fi Settings",
			adapterLabel );
	}
	else
	{
		passwordLabel = retry ? "Wi-Fi authentication did not complete" : "Wi-Fi password required";

		if ( adapterTitle[0] )
		{
			snprintf( title, sizeof( title ), "%s - %s - %s", passwordLabel,
				adapterLabel, profile->SSID );
		}
		else
		{
			snprintf( title, sizeof( title ), "%s - %s", passwordLabel, profile->SSID );
		}
	}

	controls.StringInput = MakeClass( NULL, NULL, controls.Classes[WIFI_STRING], 0, 0 );

	if ( ! controls.StringInput )
	{
		goto done;
	}

	controls.StringInput->cl_Dispatcher.h_Entry = StringInputDispatch;

	if ( setup )
	{
		WiFi_RestoreScan( adapter, unit, & scan );
	}

	controls.StringInput->cl_UserData = scan.Count;

	controls.Layout = NewObject( controls.Classes[WIFI_LAYOUT], NULL,
		LAYOUT_Orientation, LAYOUT_ORIENT_VERT, LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE,
		LAYOUT_VertAlignment, setup ? LAYOUT_ALIGN_TOP : LAYOUT_ALIGN_CENTER, TAG_END );

	if ( ( ! controls.Layout ) || ( ! BuildControls( & controls, profile, & scan, setup ) ) )
	{
		FPrintf( ErrorOutput(), "Could not create the Wi-Fi window controls.\n" );

		goto done;
	}

	width = screen->Width - screen->WBorLeft - screen->WBorRight - 4;
	height = screen->Height - screen->WBorTop - screen->WBorBottom - screen->BarHeight - 4;
	preferredHeight = setup ? 220 : 44;

	if ( width < 1 )
	{
		width = 1;
	}

	if ( height < 1 )
	{
		height = 1;
	}

	object = NewObject( controls.Classes[WIFI_WINDOW], NULL,
		WA_PubScreen, screen, WA_Title, title, WA_Activate, TRUE, WA_DragBar, TRUE,
		WA_CloseGadget, TRUE, WA_DepthGadget, TRUE, WA_AutoAdjust, TRUE,
		WA_InnerWidth, width < 440 ? width : 440,
		WA_InnerHeight, height < preferredHeight ? height : preferredHeight,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_RAWKEY,
		WINDOW_Layout, controls.Layout, WINDOW_Position, WPOS_CENTERSCREEN, TAG_END );

	if ( ! object )
	{
		FPrintf( ErrorOutput(), "Could not create the Wi-Fi window object.\n" );

		goto done;
	}

	window = (struct Window *) IDoMethod( object, WM_OPEN, NULL );

	if ( ! window )
	{
		FPrintf( ErrorOutput(), "Could not open the Wi-Fi window on the selected screen.\n" );

		goto done;
	}

	if ( ( ! GetAttr( WINDOW_SigMask, object, & mask ) ) || ( ! mask ) )
	{
		FPrintf( ErrorOutput(), "The Wi-Fi window has no input signal.\n" );

		goto done;
	}

	input.Controls = & controls;
	input.Profile = profile;
	input.Scan = & scan;
	input.Window = window;
	input.Object = object;
	input.Action = action;
	input.Setup = setup;
	input.Result = USB2WIFI_MISSING;

	if ( setup )
	{
		ULONG ssid;

		ssid = 0;

		GetAttr( STRINGA_TextVal, controls.SSID, & ssid );

		if ( ( ssid ) && ( *(const char *) ssid ) )
		{
			ActivatePassword( & controls, object, window );
		}

		DebugPrintF( "USB2 Wi-Fi: opened the picker for %s unit %lu.\n",
			adapterLabel, unit );

		scan.InputMask = mask;
		scan.Update = HandleWindowInput;
		scan.Context = & input;

		registration = WiFi_ClaimedRegistration( adapter, unit );

		if ( ( registration ) || ( WiFi_LiveRegistration( unit, &registration ) ) )
		{
			result = WiFi_PickNetwork( adapter, unit, registration, & scan );
		}
		else
		{
			result = USB2WIFI_MISSING;
		}

		result = ( result == USB2WIFI_OK ) ? input.Result : result;

		goto done;
	}

	ActivatePassword( & controls, object, window );

	while( HandleWindowInput( & input ) )
	{
		if ( Wait( mask | SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
		{
			break;
		}
	}

	result = input.Result;

done:

	if ( result == USB2WIFI_CONNECT_NOW )
	{
		WiFi_RememberScan( adapter, unit, & scan );
	}

	if ( object )
	{
		DisposeObject( object );
	}
	else if ( controls.Layout )
	{
		DisposeObject( controls.Layout );
	}

	if ( controls.StringInput )
	{
		FreeClass( controls.StringInput );
	}

	if ( controls.ListBrowser )
	{
		controls.ListBrowser->FreeListBrowserList( & controls.NetworkRows );

		DropInterface( (struct Interface *) controls.ListBrowser );
	}

	Erase( controls.PasswordBuffer, sizeof( controls.PasswordBuffer ) );

	Erase( controls.UndoBuffer, sizeof( controls.UndoBuffer ) );

	Erase( controls.WorkBuffer, sizeof( controls.WorkBuffer ) );

	Erase( & controls.Autofill, sizeof( controls.Autofill ) );

	for( index = WIFI_CLASSES; index > 0; index-- )
	{
		if ( bases[index - 1] )
		{
			CloseClass( bases[index - 1] );
		}
	}

	if ( screen )
	{
		UnlockPubScreen( NULL, screen );
	}

	return ( result );
}

// --
static S32
SetupWindow( struct USB2WiFiProfile *profile, U32 adapter, U32 unit, S32 retry, U32 *action )
{
	if ( ( ! profile ) || ( ! action ) || ( ! WiFi_AdapterProtocol( adapter ) ) ||
		( ! memchr( profile->SSID, 0, sizeof( profile->SSID ) ) ) ||
		( ! memchr( profile->Password, 0, sizeof( profile->Password ) ) ) ||
		( profile->Security > USB2WIFI_WPA3 ) ||
		( adapter == USB2WIFI_RT2870 && profile->Security == USB2WIFI_OPEN ) )
	{
		return ( USB2WIFI_ERROR );
	}

	return ( WiFi_Window( profile, adapter, unit, TRUE, retry, action ) );
}

// --

S32
WiFi_Setup( struct USB2WiFiIFace *self __attribute__((unused)), struct USB2WiFiProfile *profile,
	U32 adapter, U32 unit, U32 *action )
{
	return ( SetupWindow( profile, adapter, unit, FALSE, action ) );
}

// --

S32
WiFi_SetupRetry( struct USB2WiFiIFace *self __attribute__((unused)), struct USB2WiFiProfile *profile,
	U32 adapter, U32 unit, U32 *action )
{
	return ( SetupWindow( profile, adapter, unit, TRUE, action ) );
}

// --
