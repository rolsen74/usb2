
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <classes/window.h>
#include <devices/usb2defaults.h>
#include <devices/usb2audio.h>
#include <usb2wifi.h>
#include <gadgets/button.h>
#include <gadgets/chooser.h>
#include <gadgets/clicktab.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <images/bevel.h>
#include <images/label.h>
#include <interfaces/bluetooth2.h>
#include <interfaces/layout.h>
#include <interfaces/listbrowser.h>
#include <interfaces/usb2media.h>
#include <interfaces/usb2eth.h>
#include <interfaces/usb2port.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

// --

enum
{
	PAGE_AUDIO,
	PAGE_BLUETOOTH,
	PAGE_ETHERNET,
	PAGE_PARALLEL,
	PAGE_PRINTER,
	PAGE_SERIAL,
	PAGE_VIDEO,
	PAGE_COUNT
};

enum
{
	MODE_AUTO,
	MODE_FIRST,
	MODE_CUSTOM,
	MODE_COUNT
};

enum
{
	CLASS_WINDOW,
	CLASS_LAYOUT,
	CLASS_TABS,
	CLASS_LIST,
	CLASS_BUTTON,
	CLASS_CHOOSER,
	CLASS_LABEL,
	CLASS_COUNT
};

enum
{
	GADGET_TABS = 1,
	GADGET_DEFAULT,
	GADGET_CUSTOM,
	GADGET_PICK,
	GADGET_PICK_CANCEL,
	GADGET_SAVE,
	GADGET_USE,
	GADGET_CANCEL,
	GADGET_WIFI_PANEL = 1000
};

struct PrefsPage
{
	Object *UnitChooser;
	Object *Custom;
	U32 Mode;
	char Value[USB2_PREF_SELECTOR_SIZE];
	char ButtonText[48];
};

struct PickerItem
{
	struct Node *Node;
	char Value[USB2_PREF_SELECTOR_SIZE];
};

struct Manager
{
	struct ClassLibrary *Bases[CLASS_COUNT];
	struct IClass *Classes[CLASS_COUNT];
	struct ListBrowserIFace *ListBrowser;
	struct IClass *PageClass;
	struct MsgPort *Port;
	struct IOStdReq *IO;
	struct Window *Window;
	struct Library *WiFiBase;
	struct USB2WiFiIFace *WiFi;
	struct USB2WiFiPanel *WiFiPanel;
	S32 WiFiVisited;
	struct PrefsPage Pages[PAGE_COUNT];
	Object *Object;
	Object *Tabs;
};

struct IntuitionIFace *IIntuition;

static const char Version[] __attribute__((used)) = "\0$VER: USB2Prefs 1.6 (25.11.2026)";
static const char *const ClassNames[] =
{
	"window.class",
	"gadgets/layout.gadget",
	"gadgets/clicktab.gadget",
	"gadgets/listbrowser.gadget",
	"gadgets/button.gadget",
	"gadgets/chooser.gadget",
	"images/label.image"
};

static const char *const TabNames[] = { "Audio", "Bluetooth", "Ethernet", "Parallel", "Printer", "Serial", "Video", NULL };
static const char *const DefaultGroupTitles[PAGE_COUNT] =
{
	[PAGE_AUDIO] = "Default Audio",
	[PAGE_BLUETOOTH] = "Default Bluetooth",
	[PAGE_ETHERNET] = "Default Ethernet",
	[PAGE_PARALLEL] = "Default Parallel",
	[PAGE_PRINTER] = "Default Printer",
	[PAGE_SERIAL] = "Default Serial",
	[PAGE_VIDEO] = "Default Video"
};
static const char *const Modes[] = { "Automatic", "First Device", "Custom Device", NULL };
static const char *const DeviceNames[] =
{
	AUDIO_DEVICE_NAME,
	BLUETOOTH2_NAME,
	USB2_ETH_DEVICE,
	USB2_PARALLEL_DEVICE,
	USB2_PRINTER_DEVICE,
	USB2_SERIAL_DEVICE,
	"usb2video.device"
};

static const U32 UnitPages[] =
{
	[USB2_DEFAULT_SERIAL] = PAGE_SERIAL,
	[USB2_DEFAULT_PARALLEL] = PAGE_PARALLEL,
	[USB2_DEFAULT_VIDEO] = PAGE_VIDEO,
	[USB2_DEFAULT_PRINTER] = PAGE_PRINTER,
	[USB2_DEFAULT_BLUETOOTH] = PAGE_BLUETOOTH
};

static const struct TagItem FixedHeight[] = { { CHILD_WeightedHeight, 0 }, { TAG_END, 0 } };

// --

static void
ShowError( struct Manager *manager, const char *message )
{
	struct EasyStruct request;

	request.es_StructSize = sizeof( request );
	request.es_Flags = 0;
	request.es_Title = "USB2Prefs";
	request.es_TextFormat = message;
	request.es_GadgetFormat = "OK";

	EasyRequestArgs( manager->Window, & request, NULL, NULL );
}

// --

static void
LoadDefaults( struct Manager *manager )
{
	struct USB2Prefs prefs;
	struct PrefsPage *page;

	char legacy[AUDIO_ENDPOINT_ID_SIZE + 1];
	const char *values[2];
	S32 length;
	S32 unit;
	U32 index;

	if ( ! USB2_ReadPrefs( IDOS, & prefs ) )
	{
		length = GetVar( AUDIO_DEFAULT_VAR, legacy, sizeof( legacy ), GVF_GLOBAL_ONLY );

		if ( ( length > 0 ) && ( length < AUDIO_ENDPOINT_ID_SIZE ) )
		{
			legacy[length] = 0;

			strcpy( prefs.Audio, legacy );
		}
	}

	for( index = 0; index < USB2_DEFAULT_COUNT; index++ )
	{
		page = & manager->Pages[UnitPages[index]];
		unit = prefs.Units[index];

		if ( unit >= 0 )
		{
			page->Mode = MODE_CUSTOM;

			snprintf( page->Value, sizeof( page->Value ), "%ld", (long) unit );
		}
		else
		{
			page->Mode = ( unit == USB2_UNIT_FIRST ) ? MODE_FIRST : MODE_AUTO;
		}
	}

	values[0] = prefs.Audio;
	values[1] = prefs.Ethernet;

	for( index = 0; index < 2; index++ )
	{
		page = & manager->Pages[index ? PAGE_ETHERNET : PAGE_AUDIO];

		if ( strcmp( values[index], USB2_PREF_AUTO ) && values[index][0] )
		{
			if ( ! strcmp( values[index], USB2_PREF_FIRST ) )
			{
				page->Mode = MODE_FIRST;
			}
			else
			{
				page->Mode = MODE_CUSTOM;

				strcpy( page->Value, values[index] );
			}
		}
	}
}

// --

static const char *
PreferenceValue( const struct PrefsPage *page, const char *automatic, const char *first )
{
	if ( page->Mode == MODE_FIRST )
	{
		return ( first );
	}

	return ( ( page->Mode == MODE_CUSTOM ) ? page->Value : automatic );
}

// --

static S32
ApplyDefaults( struct Manager *manager, S32 save )
{
	char text[USB2_PREFS_TEXT_SIZE];
	const char *units[USB2_DEFAULT_COUNT];
	const char *audio;
	const char *ethernet;
	U32 index;
	S32 length;
	S32 wifi;

	for( index = 0; index < PAGE_COUNT; index++ )
	{
		if ( ( manager->Pages[index].Mode == MODE_CUSTOM ) && ( ! manager->Pages[index].Value[0] ) )
		{
			snprintf( text, sizeof( text ), "Choose a custom device on the %s page.", TabNames[index] );

			ShowError( manager, text );

			return ( FALSE );
		}
	}

	for( index = 0; index < USB2_DEFAULT_COUNT; index++ )
	{
		units[index] = PreferenceValue( & manager->Pages[UnitPages[index]], "-1", "-2" );
	}

	audio = PreferenceValue( & manager->Pages[PAGE_AUDIO], USB2_PREF_AUTO, USB2_PREF_FIRST );

	ethernet = PreferenceValue( & manager->Pages[PAGE_ETHERNET], USB2_PREF_AUTO, USB2_PREF_FIRST );

	length = snprintf( text, sizeof( text ), "%s %s %s %s %s %s %s",
		units[0], units[1], units[2], units[3], units[4], audio, ethernet );

	if ( ( length <= 0 ) || ( (U32) length >= sizeof( text ) ) )
	{
		ShowError( manager, "Could not format USB2 preferences." );

		return ( FALSE );
	}

	wifi = USB2WIFI_MISSING;

	if ( ( manager->WiFiPanel ) && ( manager->WiFiVisited ) )
	{
		wifi = manager->WiFi->ApplyProfilePanel( manager->WiFiPanel, manager->Window, save );

		if ( wifi == USB2WIFI_ERROR )
		{
			return ( FALSE );
		}
	}

	if ( ! SetVar( USB2_PREFS_VAR, text, length, GVF_GLOBAL_ONLY | ( save ? GVF_SAVE_VAR : 0 ) ) )
	{
		ShowError( manager, ( wifi == USB2WIFI_OK ) ?
			"Wi-Fi profiles were stored, but the USB device defaults could not be written." :
			"Could not store USB2 preferences." );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

static void
OpenWiFi( struct Manager *manager )
{
	manager->WiFiBase = OpenLibrary( USB2WIFI_NAME, USB2WIFI_VERSION );

	if ( ( manager->WiFiBase ) && ( ( manager->WiFiBase->lib_Version > USB2WIFI_VERSION ) ||
		( manager->WiFiBase->lib_Revision >= USB2WIFI_PANEL_REVISION ) ) )
	{
		manager->WiFi = (PTR) GetInterface( manager->WiFiBase, "main", 1, NULL );
	}
}

// --

static Object *
AddChild( Object *layout, Object *child, const struct TagItem *tags )
{
	if ( ! child )
	{
		return ( NULL );
	}

	if ( ! IDoMethod( layout, LM_ADDCHILD, NULL, child, tags ) )
	{
		DisposeObject( child );

		return ( NULL );
	}

	return ( child );
}

// --

static Object *
Button( struct Manager *manager, U32 id, const char *text )
{
	return ( NewObject( manager->Classes[CLASS_BUTTON], NULL,
		GA_ID, id, GA_RelVerify, TRUE, GA_Text, text, GA_Underscore, '_', TAG_END ) );
}

// --

static void
FormatButton( struct PrefsPage *page, U32 type )
{
	if ( ! page->Value[0] )
	{
		strcpy( page->ButtonText, "Select..." );
	}
	else if ( ( type == PAGE_AUDIO ) || ( type == PAGE_ETHERNET ) )
	{
		snprintf( page->ButtonText, sizeof( page->ButtonText ), "%.36s", page->Value );
	}
	else
	{
		snprintf( page->ButtonText, sizeof( page->ButtonText ), "Unit %.6s", page->Value );
	}
}

// --

static Object *
AddField( struct Manager *manager, Object *group, Object *gadget, const char *caption )
{
	struct TagItem tags[3];
	Object *label;

	if ( ! gadget )
	{
		return ( NULL );
	}

	label = NewObject( manager->Classes[CLASS_LABEL], NULL, LABEL_Text, caption, TAG_END );

	if ( ! label )
	{
		DisposeObject( gadget );

		return ( NULL );
	}

	tags[0].ti_Tag = CHILD_Label;
	tags[0].ti_Data = (Tag) label;
	tags[1].ti_Tag = CHILD_WeightedHeight;
	tags[1].ti_Data = 0;
	tags[2].ti_Tag = TAG_END;
	tags[2].ti_Data = 0;

	if ( ! AddChild( group, gadget, tags ) )
	{
		DisposeObject( label );

		return ( NULL );
	}

	return ( gadget );
}

// --

static Object *
BuildDefaultPage( struct Manager *manager, U32 type )
{
	struct PrefsPage *page;
	Object *layout;
	Object *group;
	Object *custom;

	page = & manager->Pages[type];

	layout = NewObject( manager->Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ! layout )
	{
		return ( NULL );
	}

	group = AddChild( layout, NewObject( manager->Classes[CLASS_LAYOUT], NULL,
		LAYOUT_Orientation, LAYOUT_ORIENT_VERT, LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE,
		LAYOUT_BevelStyle, BVS_GROUP, LAYOUT_Label, DefaultGroupTitles[type], TAG_END ), FixedHeight );

	if ( ! group )
	{
		goto failed;
	}

	page->UnitChooser = AddField( manager, group, NewObject( manager->Classes[CLASS_CHOOSER], NULL,
		GA_ID, GADGET_DEFAULT, GA_RelVerify, TRUE, GA_TabCycle, TRUE,
		CHOOSER_PopUp, TRUE, CHOOSER_LabelArray, Modes, CHOOSER_Selected, page->Mode, TAG_END ), "Unit" );

	FormatButton( page, type );

	custom = NewObject( manager->Classes[CLASS_BUTTON], NULL, GA_ID, GADGET_CUSTOM,
		GA_RelVerify, TRUE, GA_Text, page->ButtonText, GA_Disabled, page->Mode != MODE_CUSTOM, TAG_END );

	page->Custom = AddField( manager, group, custom, "Custom" );

	if ( ( ! page->UnitChooser ) || ( ! page->Custom ) ||
		( ! AddChild( layout, NewObject( manager->Classes[CLASS_BUTTON], NULL,
			GA_Text, "", GA_ReadOnly, TRUE, BUTTON_BevelStyle, BVS_NONE,
			BUTTON_Transparent, TRUE, TAG_END ), NULL ) ) )
	{
		goto failed;
	}

	return ( layout );

failed:
	DisposeObject( layout );

	return ( NULL );
}

// --

static Object *
BuildEthernetPage( struct Manager *manager, Object *defaults )
{
	Object *layout;

	const char *status;
	S32 nested;

	layout = NewObject( manager->Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT, TAG_END );

	if ( ! layout )
	{
		DisposeObject( defaults );

		return ( NULL );
	}

	nested = ( manager->WiFi ) && ( ( manager->WiFiBase->lib_Version > USB2WIFI_VERSION ) ||
		( manager->WiFiBase->lib_Revision >= USB2WIFI_DEFAULT_TAB_REVISION ) );

	if ( nested )
	{
		manager->WiFiPanel = manager->WiFi->AddProfilePanelWithDefaults( layout, defaults, GADGET_WIFI_PANEL );

		if ( manager->WiFiPanel )
		{
			return ( layout );
		}

		defaults = BuildDefaultPage( manager, PAGE_ETHERNET );
	}

	if ( ! AddChild( layout, defaults, FixedHeight ) )
	{
		goto failed;
	}

	if ( ( manager->WiFi ) && ( ! nested ) )
	{
		manager->WiFiPanel = manager->WiFi->AddProfilePanel( layout, GADGET_WIFI_PANEL );

		if ( manager->WiFiPanel )
		{
			return ( layout );
		}
	}

	status = manager->WiFi ? "Could not load Wi-Fi profiles" : "Wi-Fi profiles require usb2wifi.library 1.47";

	if ( ! AddChild( layout, NewObject( manager->Classes[CLASS_BUTTON], NULL,
		GA_Text, status, GA_Disabled, TRUE, TAG_END ), FixedHeight ) )
	{
		goto failed;
	}

	return ( layout );

failed:
	DisposeObject( layout );

	return ( NULL );
}

// --

static Object *
BuildTabs( struct Manager *manager )
{
	struct TagItem tags[PAGE_COUNT + 1];
	Object *pages;
	Object *child;
	Object *tabs;

	U32 index;

	for( index = 0; index < PAGE_COUNT; index++ )
	{
		child = BuildDefaultPage( manager, index );

		if ( ( child ) && ( index == PAGE_ETHERNET ) )
		{
			child = BuildEthernetPage( manager, child );
		}

		if ( ! child )
		{
			goto failed;
		}

		tags[index].ti_Tag = PAGE_Add;
		tags[index].ti_Data = (Tag) child;
	}

	tags[index].ti_Tag = TAG_END;
	tags[index].ti_Data = 0;

	pages = NewObjectA( manager->PageClass, NULL, tags );

	if ( ! pages )
	{
		goto failed;
	}

	tabs = NewObject( manager->Classes[CLASS_TABS], NULL, GA_ID, GADGET_TABS,
		GA_RelVerify, TRUE, GA_Text, TabNames, CLICKTAB_PageGroup, pages, TAG_END );

	if ( ! tabs )
	{
		DisposeObject( pages );
	}

	return ( tabs );

failed:
	while( index )
	{
		index--;

		DisposeObject( (PTR) tags[index].ti_Data );
	}

	return ( NULL );
}

// --

static S32
OpenManagerWindow( struct Manager *manager )
{
	Object *layout;
	Object *footer;

	layout = NewObject( manager->Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ! layout )
	{
		return ( FALSE );
	}

	manager->Tabs = AddChild( layout, BuildTabs( manager ), NULL );
	footer = AddChild( layout, NewObject( manager->Classes[CLASS_LAYOUT], NULL,
		LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_EvenSize, TRUE,
		LAYOUT_SpaceInner, TRUE, LAYOUT_InnerSpacing, 12, TAG_END ), FixedHeight );

	if ( ( ! manager->Tabs ) || ( ! footer ) ||
		( ! AddChild( footer, Button( manager, GADGET_SAVE, "_Save" ), NULL ) ) ||
		( ! AddChild( footer, Button( manager, GADGET_USE, "_Use" ), NULL ) ) ||
		( ! AddChild( footer, Button( manager, GADGET_CANCEL, "_Cancel" ), NULL ) ) )
	{
		goto failed;
	}

	manager->Object = NewObject( manager->Classes[CLASS_WINDOW], NULL,
		WA_Title, "USB2Prefs", WA_Width, 530, WA_AutoAdjust, TRUE, WA_Activate, TRUE,
		WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_SizeGadget, TRUE,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_RAWKEY,
		WINDOW_Position, WPOS_CENTERSCREEN, TAG_END );

	if ( ! manager->Object )
	{
		goto failed;
	}

	SetAttrs( manager->Object, WINDOW_Layout, layout, TAG_END );

	manager->Window = (PTR) IDoMethod( manager->Object, WM_OPEN, NULL );

	return ( manager->Window != NULL );

failed:
	DisposeObject( layout );

	return ( FALSE );
}

// --

static PTR
ReadChoices( struct Manager *manager, U32 type, U32 *count )
{
	struct IOStdReq *io;

	PTR entries;
	ULONG query_unit;
	U32 size;
	U32 stride;
	S32 error;

	io = manager->IO;
	entries = NULL;
	*count = 0;
	query_unit = USB2_PORT_BACKEND_UNIT;
	stride = sizeof( struct USB2UnitInfo );

	if ( ( type == PAGE_AUDIO ) || ( type == PAGE_VIDEO ) )
	{
		query_unit = USB2_MEDIA_QUERY_UNIT;
	}
	else if ( type == PAGE_BLUETOOTH )
	{
		query_unit = 0;
	}
	else if ( type == PAGE_ETHERNET )
	{
		query_unit = USB2_ETH_BACKEND_UNIT;
	}

	if ( OpenDevice( DeviceNames[type], query_unit, (PTR) io, 0 ) )
	{
		return ( NULL );
	}

	io->io_Command = USB2_CMD_ENUM_UNITS;
	io->io_Offset = USB2_UNIT_LIST_VERSION;

	if ( type == PAGE_AUDIO )
	{
		io->io_Command = AUDIO_CMD_ENUM_ENDPOINTS;
		io->io_Offset = AUDIO_ENDPOINT_VERSION;
		stride = sizeof( struct AudioEndpoint );
	}
	else if ( type == PAGE_ETHERNET )
	{
		io->io_Command = USB2_ETH_CMD_ENUM_ADAPTERS;
		io->io_Offset = USB2_ETH_ADAPTER_LIST_VERSION;
		stride = sizeof( struct USB2EthAdapterEntry );
	}

	io->io_Length = 0;
	io->io_Actual = 0;
	io->io_Data = NULL;

	error = DoIO( (PTR) io );

	size = io->io_Actual;

	if ( ( error == IOERR_BADLENGTH ) && ( size ) && ( size % stride == 0 ) &&
		( size / stride <= USB2_DEFAULT_UNIT ) )
	{
		entries = AllocVecTags( size, TAG_END );

		if ( entries )
		{
			io->io_Data = entries;
			io->io_Length = size;

			error = DoIO( (PTR) io );

			if ( ( ! error ) && ( io->io_Actual <= size ) && ( io->io_Actual % stride == 0 ) )
			{
				*count = io->io_Actual / stride;
			}
		}
	}

	CloseDevice( (PTR) io );

	return ( entries );
}

// --

static S32
FormatChoice( struct PickerItem *item, char *text, U32 size, const void *entries, U32 index, U32 type )
{
	const struct AudioEndpoint *audio;
	const struct USB2EthAdapterEntry *ethernet;
	const struct USB2UnitInfo *unit;

	if ( type == PAGE_AUDIO )
	{
		audio = & ((const struct AudioEndpoint *) entries)[index];

		if ( ( audio->Direction != AUDIO_ENDPOINT_OUTPUT ) || ( ! audio->ID[0] ) ||
			( audio->DefaultFormat.Encoding != AUDIO_ENCODING_PCM_S16LE ) ||
			( ( audio->DefaultFormat.Channels != 1 ) && ( audio->DefaultFormat.Channels != 2 ) ) )
		{
			return ( FALSE );
		}

		strcpy( item->Value, audio->ID );

		snprintf( text, size, "%s, unit %lu%s", audio->Name, (unsigned long) audio->LegacyUnit,
			( audio->Flags & AUDIO_ENDPOINT_BUSY ) ? " (busy)" : "" );
	}
	else if ( type == PAGE_ETHERNET )
	{
		ethernet = & ((const struct USB2EthAdapterEntry *) entries)[index];

		if ( ( ! ethernet->Info.Driver[0] ) || ( strchr( ethernet->Info.Driver, ' ' ) ) )
		{
			return ( FALSE );
		}

		strcpy( item->Value, ethernet->Info.Driver );

		snprintf( text, size, "%s (%s)",
			ethernet->Info.Title[0] ? ethernet->Info.Title : "Ethernet", ethernet->Info.Driver );
	}
	else
	{
		unit = & ((const struct USB2UnitInfo *) entries)[index];

		if ( unit->Number > USB2_MAX_DEFAULT_UNIT )
		{
			return ( FALSE );
		}

		snprintf( item->Value, sizeof( item->Value ), "%lu", (unsigned long) unit->Number );

		snprintf( text, size, "Unit %lu%s", (unsigned long) unit->Number,
			( unit->Flags & USB2_UNIT_BUSY ) ? " (busy)" : "" );
	}

	return ( TRUE );
}

// --

static S32
RunPicker( struct Manager *manager, Object *object, Object *browser, char *value )
{
	struct Node *node;
	struct PickerItem *item;

	ULONG mask;
	ULONG event;
	ULONG data;
	U16 code;

	mask = 0;

	GetAttr( WINDOW_SigMask, object, & mask );

	while( ( mask ) && ( ! ( Wait( mask | SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C ) ) )
	{
		while( ( event = IDoMethod( object, WM_HANDLEINPUT, & code ) ) != WMHI_LASTMSG )
		{
			if ( ( ( event & WMHI_CLASSMASK ) == WMHI_CLOSEWINDOW ) ||
				( ( ( event & WMHI_CLASSMASK ) == WMHI_RAWKEY ) && ( code == 0x45 ) ) ||
				( ( event & ( WMHI_CLASSMASK | WMHI_GADGETMASK ) ) == ( WMHI_GADGETUP | GADGET_PICK_CANCEL ) ) )
			{
				return ( FALSE );
			}

			if ( ( event & ( WMHI_CLASSMASK | WMHI_GADGETMASK ) ) == ( WMHI_GADGETUP | GADGET_PICK ) )
			{
				data = 0;

				GetAttr( LISTBROWSER_SelectedNode, browser, & data );

				node = (PTR) data;

				if ( node )
				{
					manager->ListBrowser->GetListBrowserNodeAttrs( node, LBNA_UserData, & data, TAG_END );

					item = (PTR) data;

					strcpy( value, item->Value );

					return ( TRUE );
				}
			}
		}
	}

	return ( FALSE );
}

// --

static void
PickDevice( struct Manager *manager, U32 type )
{
	struct PrefsPage *page;
	struct PickerItem *items;
	struct List rows;
	Object *object;
	Object *layout;
	Object *browser;
	Object *footer;

	PTR entries;
	char text[2 * USB2_PREF_SELECTOR_SIZE + 32];
	U32 count;
	U32 used;
	U32 index;
	U32 previous;
	S32 selected;

	page = & manager->Pages[type];

	if ( page->Mode != MODE_CUSTOM )
	{
		return;
	}

	object = NULL;
	layout = NULL;
	items = NULL;
	used = 0;
	selected = -1;

	NewList( & rows );

	entries = ReadChoices( manager, type, & count );

	if ( ! count )
	{
		ShowError( manager, "No selectable devices are available. Connect a device and reopen the list." );

		goto done;
	}

	items = AllocVecTags( count * sizeof( *items ), TAG_END );

	if ( ! items )
	{
		goto done;
	}

	for( index = 0; index < count; index++ )
	{
		if ( ! FormatChoice( & items[used], text, sizeof( text ), entries, index, type ) )
		{
			continue;
		}

		if ( type == PAGE_ETHERNET )
		{
			for( previous = 0; previous < used; previous++ )
			{
				if ( ! strcasecmp( items[previous].Value, items[used].Value ) )
				{
					break;
				}
			}

			if ( previous < used )
			{
				continue;
			}
		}

		items[used].Node = manager->ListBrowser->AllocListBrowserNode( 1, LBNA_UserData, & items[used],
			LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, text, TAG_END );

		if ( ! items[used].Node )
		{
			break;
		}

		AddTail( & rows, items[used].Node );

		if ( ( selected < 0 ) && ( ! strcmp( page->Value, items[used].Value ) ) )
		{
			selected = used;
		}

		used++;
	}

	if ( ! used )
	{
		ShowError( manager, "No selectable devices are available." );

		goto done;
	}

	layout = NewObject( manager->Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ! layout )
	{
		goto done;
	}

	browser = AddChild( layout, NewObject( manager->Classes[CLASS_LIST], NULL,
		GA_RelVerify, TRUE, LISTBROWSER_ShowSelected, TRUE, LISTBROWSER_Labels, & rows,
		LISTBROWSER_Selected, selected < 0 ? 0 : selected, LISTBROWSER_MinVisible, 5, TAG_END ), NULL );

	footer = AddChild( layout, NewObject( manager->Classes[CLASS_LAYOUT], NULL,
		LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_EvenSize, TRUE, TAG_END ), FixedHeight );

	if ( ( ! browser ) || ( ! footer ) || ( ! AddChild( footer, Button( manager, GADGET_PICK, "_Select" ), NULL ) ) ||
		( ! AddChild( footer, Button( manager, GADGET_PICK_CANCEL, "_Cancel" ), NULL ) ) )
	{
		goto done;
	}

	object = NewObject( manager->Classes[CLASS_WINDOW], NULL, WA_Title, TabNames[type],
		WA_Width, 480, WA_AutoAdjust, TRUE, WA_Activate, TRUE, WA_DragBar, TRUE, WA_CloseGadget, TRUE,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_RAWKEY,
		WINDOW_Position, WPOS_CENTERSCREEN, TAG_END );

	if ( ! object )
	{
		goto done;
	}

	SetAttrs( object, WINDOW_Layout, layout, TAG_END );

	layout = NULL;

	if ( ( IDoMethod( object, WM_OPEN, NULL ) ) && ( RunPicker( manager, object, browser, page->Value ) ) )
	{
		FormatButton( page, type );

		RefreshSetGadgetAttrs( (struct Gadget *) page->Custom, manager->Window, NULL,
			GA_Text, page->ButtonText, TAG_END );
	}

done:
	if ( object )
	{
		DisposeObject( object );
	}

	if ( layout )
	{
		DisposeObject( layout );
	}

	manager->ListBrowser->FreeListBrowserList( & rows );

	FreeVec( items );

	FreeVec( entries );
}

// --

static void
SelectMode( struct Manager *manager, U32 type )
{
	struct PrefsPage *page;

	ULONG mode;

	page = & manager->Pages[type];

	if ( ( ! GetAttr( CHOOSER_Selected, page->UnitChooser, & mode ) ) || ( mode >= MODE_COUNT ) )
	{
		return;
	}

	page->Mode = mode;

	RefreshSetGadgetAttrs( (struct Gadget *) page->Custom, manager->Window, NULL,
		GA_Disabled, page->Mode != MODE_CUSTOM, TAG_END );
}

// --

static void
RunWindow( struct Manager *manager )
{
	ULONG mask;
	ULONG event;
	ULONG type;
	U16 code;
	U32 gadget;

	mask = 0;
	type = PAGE_AUDIO;

	GetAttr( WINDOW_SigMask, manager->Object, & mask );

	if ( ! mask )
	{
		return;
	}

	for( ;; )
	{
		if ( Wait( mask | SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
		{
			return;
		}

		while( ( event = IDoMethod( manager->Object, WM_HANDLEINPUT, & code ) ) != WMHI_LASTMSG )
		{
			if ( ( ( event & WMHI_CLASSMASK ) == WMHI_CLOSEWINDOW ) ||
				( ( ( event & WMHI_CLASSMASK ) == WMHI_RAWKEY ) && ( code == 0x45 ) ) )
			{
				return;
			}

			if ( ( event & WMHI_CLASSMASK ) != WMHI_GADGETUP )
			{
				continue;
			}

			gadget = event & WMHI_GADGETMASK;

			GetAttr( CLICKTAB_Current, manager->Tabs, & type );

			if ( type == PAGE_ETHERNET )
			{
				manager->WiFiVisited = TRUE;

				if ( ( manager->WiFiPanel ) && ( gadget >= GADGET_WIFI_PANEL ) && ( gadget < GADGET_WIFI_PANEL + 16 ) )
				{
					manager->WiFi->HandleProfilePanel( manager->WiFiPanel, manager->Window, gadget );

					continue;
				}
			}

			switch( gadget )
			{
				case GADGET_DEFAULT:
					if ( type < PAGE_COUNT )
					{
						SelectMode( manager, type );
					}
					break;

				case GADGET_CUSTOM:
					if ( type < PAGE_COUNT )
					{
						PickDevice( manager, type );
					}
					break;

				case GADGET_SAVE:
				case GADGET_USE:
					if ( ApplyDefaults( manager, gadget == GADGET_SAVE ) )
					{
						return;
					}
					break;

				case GADGET_CANCEL:
					return;
			}
		}
	}
}

// --

int
main( void )
{
	struct Manager manager;
	struct Library *intuition;
	struct LayoutIFace *layout;

	S32 result;
	U32 index;

	result = RETURN_FAIL;

	memset( & manager, 0, sizeof( manager ) );

	intuition = OpenLibrary( "intuition.library", 50 );

	if ( ! intuition )
	{
		return ( result );
	}

	IIntuition = (PTR) GetInterface( intuition, "main", 1, NULL );

	if ( ! IIntuition )
	{
		goto done;
	}

	manager.Port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! manager.Port )
	{
		goto done;
	}

	manager.IO = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *manager.IO ),
		ASOIOR_ReplyPort, manager.Port, TAG_END );

	if ( ! manager.IO )
	{
		goto done;
	}

	for( index = 0; index < CLASS_COUNT; index++ )
	{
		manager.Bases[index] = OpenClass( ClassNames[index], 50, & manager.Classes[index] );

		if ( ! manager.Bases[index] )
		{
			Printf( "Could not open %s.\n", ClassNames[index] );

			goto done;
		}
	}

	manager.ListBrowser = (PTR) GetInterface( (PTR) manager.Bases[CLASS_LIST], "main", 1, NULL );

	layout = (PTR) GetInterface( (PTR) manager.Bases[CLASS_LAYOUT], "main", 1, NULL );

	if ( ! layout )
	{
		goto done;
	}

	manager.PageClass = layout->PAGE_GetClass();

	DropInterface( (PTR) layout );

	LoadDefaults( & manager );

	OpenWiFi( & manager );

	if ( ( manager.ListBrowser ) && ( manager.PageClass ) && ( OpenManagerWindow( & manager ) ) )
	{
		RunWindow( & manager );

		result = RETURN_OK;
	}

done:
	if ( manager.Object )
	{
		DisposeObject( manager.Object );
	}

	if ( manager.WiFiPanel )
	{
		manager.WiFi->FreeProfilePanel( manager.WiFiPanel );
	}

	if ( manager.WiFi )
	{
		DropInterface( (PTR) manager.WiFi );
	}

	if ( manager.WiFiBase )
	{
		CloseLibrary( manager.WiFiBase );
	}

	if ( manager.ListBrowser )
	{
		DropInterface( (PTR) manager.ListBrowser );
	}

	for( index = CLASS_COUNT; index; index-- )
	{
		if ( manager.Bases[index - 1] )
		{
			CloseClass( manager.Bases[index - 1] );
		}
	}

	if ( manager.IO )
	{
		FreeSysObject( ASOT_IOREQUEST, manager.IO );
	}

	if ( manager.Port )
	{
		FreeSysObject( ASOT_PORT, manager.Port );
	}

	if ( IIntuition )
	{
		DropInterface( (PTR) IIntuition );
	}

	CloseLibrary( intuition );

	return ( result );
}

// --
