
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

#include <gadgets/clicktab.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <interfaces/layout.h>
#include <stdio.h>

// --

struct EditorLists
{
	struct List Networks;
	struct List Adapters;
	struct ColumnInfo NetworkColumns[3];
	struct ColumnInfo AdapterColumns[5];
	Object *NetworkBrowser;
	Object *AdapterBrowser;
	Object *Tabs;
	U32 FirstProfileTab;
};

struct USB2WiFiPanel
{
	struct ProfileEditor Editor;
	struct EditorLists Lists;
	struct ClassLibrary *Bases[EDITOR_CLASSES];
	char *Original;
	U32 OriginalLength;
	S32 Dirty;
};

static const struct TagItem FixedHeight[] = { { CHILD_WeightedHeight, 0 }, { TAG_END, 0 } };

// --

Object *
WiFi_AddEditorChild( Object *layout, Object *child, const struct TagItem *tags )
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

Object *
WiFi_EditorButton( const struct ProfileEditor *editor, U32 id, const char *text )
{
	return ( NewObject( editor->Classes[EDITOR_BUTTON], NULL, GA_ID, id,
		GA_RelVerify, TRUE, GA_Text, text, GA_Underscore, '_', TAG_END ) );
}

// --

static Object *
BuildPage( const struct ProfileEditor *editor, struct List *rows, struct ColumnInfo *columns, S32 adapter,
	Object **browser )
{
	Object *page;
	Object *buttons;
	U32 add;
	U32 remove;
	U32 edit;

	page = NewObject( editor->Classes[EDITOR_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ! page )
	{
		return ( NULL );
	}

	*browser = WiFi_AddEditorChild( page, NewObject( editor->Classes[EDITOR_LIST], NULL,
		GA_ID, editor->GadgetBase + ( adapter ? EDITOR_ADAPTERS : EDITOR_NETWORKS ), GA_RelVerify, TRUE, LISTBROWSER_ShowSelected, TRUE,
		LISTBROWSER_ColumnTitles, TRUE, LISTBROWSER_ColumnInfo, columns, LISTBROWSER_Labels, rows,
		LISTBROWSER_Selected, -1, LISTBROWSER_MinVisible, 5, TAG_END ), NULL );

	buttons = WiFi_AddEditorChild( page, NewObject( editor->Classes[EDITOR_LAYOUT], NULL,
		LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_SpaceInner, TRUE, LAYOUT_EvenSize, TRUE, TAG_END ), FixedHeight );

	add = adapter ? EDITOR_ADD_ADAPTER : EDITOR_ADD_NETWORK;
	remove = adapter ? EDITOR_REMOVE_ADAPTER : EDITOR_REMOVE_NETWORK;
	edit = adapter ? EDITOR_EDIT_ADAPTER : EDITOR_EDIT_NETWORK;

	if ( ( ! *browser ) || ( ! buttons )
		|| ( ! WiFi_AddEditorChild( buttons, WiFi_EditorButton( editor, editor->GadgetBase + add, "Add" ), NULL ) )
		|| ( ! WiFi_AddEditorChild( buttons, WiFi_EditorButton( editor, editor->GadgetBase + remove, "Remove" ), NULL ) )
		|| ( ! WiFi_AddEditorChild( buttons, WiFi_EditorButton( editor, editor->GadgetBase + edit, "Edit" ), NULL ) ) )
	{
		DisposeObject( page );

		return ( NULL );
	}

	return ( page );
}

// --

static S32
BuildTabs( const struct ProfileEditor *editor, struct EditorLists *lists, Object *layout, Object *defaults )
{
	static const char *const tabs[] = { "Default", "SSIDs", "Devices", NULL };
	static const struct ColumnInfo networkColumns[] = { { 80, "SSID", 0 }, { 20, "Hidden", 0 }, { -1, NULL, 0 } };
	static const struct ColumnInfo adapterColumns[] = { { 31, "Adapter name", 0 }, { 23, "MAC or unit", 0 },
		{ 30, "SSID", 0 }, { 16, "Autoconnect", 0 }, { -1, NULL, 0 } };

	Object *networks;
	Object *adapters;
	Object *pages;
	Object *object;

	networks = NULL;
	adapters = NULL;
	lists->FirstProfileTab = defaults ? 1 : 0;

	memcpy( lists->NetworkColumns, networkColumns, sizeof( networkColumns ) );

	memcpy( lists->AdapterColumns, adapterColumns, sizeof( adapterColumns ) );

	networks = BuildPage( editor, &lists->Networks, lists->NetworkColumns, FALSE, &lists->NetworkBrowser );

	if ( ! networks )
	{
		goto failed;
	}

	adapters = BuildPage( editor, &lists->Adapters, lists->AdapterColumns, TRUE, &lists->AdapterBrowser );

	if ( ! adapters )
	{
		goto failed;
	}

	pages = NewObject( editor->PageClass, NULL, defaults ? PAGE_Add : TAG_IGNORE, defaults,
		PAGE_Add, networks, PAGE_Add, adapters, TAG_END );

	if ( ! pages )
	{
		goto failed;
	}

	object = NewObject( editor->Classes[EDITOR_TABS], NULL,
		GA_Text, tabs + ( defaults ? 0 : 1 ), CLICKTAB_Current, 0, CLICKTAB_PageGroup, pages, TAG_END );

	if ( ! object )
	{
		DisposeObject( pages );

		return ( FALSE );
	}

	lists->Tabs = WiFi_AddEditorChild( layout, object, NULL );

	return ( lists->Tabs != NULL );

failed:
	if ( adapters )
	{
		DisposeObject( adapters );
	}

	if ( networks )
	{
		DisposeObject( networks );
	}

	if ( defaults )
	{
		DisposeObject( defaults );
	}

	return ( FALSE );
}

// --

static void
RefreshRows( const struct ProfileEditor *editor, struct EditorLists *lists, struct Window *window )
{
	const struct ProfileNetwork *network;
	const struct ProfileAdapter *adapter;
	struct Node *node;

	char identity[3 * WIFI_STATION_SIZE];
	ULONG active;
	S32 complete;

	active = 0;
	complete = TRUE;

	GetAttr( CLICKTAB_Current, lists->Tabs, &active );

	SetAttrs( lists->NetworkBrowser, LISTBROWSER_Labels, ~0UL, TAG_END );

	SetAttrs( lists->AdapterBrowser, LISTBROWSER_Labels, ~0UL, TAG_END );

	editor->ListBrowser->FreeListBrowserList( &lists->Networks );

	editor->ListBrowser->FreeListBrowserList( &lists->Adapters );

	for ( network = editor->Database.Networks; network; network = network->Next )
	{
		node = editor->ListBrowser->AllocListBrowserNode( 2, LBNA_UserData, network,
			LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, network->Profile.SSID,
			LBNA_Column, 1, LBNCA_Text, ( network->Profile.Options & USB2WIFI_PROFILE_HIDDEN ) ? "Yes" : "No", TAG_END );

		if ( ! node )
		{
			complete = FALSE;
			break;
		}

		AddTail( &lists->Networks, node );
	}

	for ( adapter = editor->Database.Adapters; adapter; adapter = adapter->Next )
	{
		if ( WiFi_HasStation( adapter->Station ) )
		{
			WiFi_FormatStation( identity, adapter->Station );
		}
		else
		{
			snprintf( identity, sizeof( identity ), "%lu", (unsigned long) adapter->Unit );
		}

		node = editor->ListBrowser->AllocListBrowserNode( 4, LBNA_UserData, adapter,
			LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, adapter->Device,
			LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, identity,
			LBNA_Column, 2, LBNCA_CopyText, TRUE, LBNCA_Text, adapter->Profile.SSID,
			LBNA_Column, 3, LBNCA_Text, ( adapter->Profile.Options & USB2WIFI_PROFILE_AUTO ) ? "Yes" : "No", TAG_END );

		if ( ! node )
		{
			complete = FALSE;
			break;
		}

		AddTail( &lists->Adapters, node );
	}

	SetAttrs( lists->NetworkBrowser, LISTBROWSER_Labels, &lists->Networks, LISTBROWSER_Selected, -1, TAG_END );

	SetAttrs( lists->AdapterBrowser, LISTBROWSER_Labels, &lists->Adapters, LISTBROWSER_Selected, -1, TAG_END );

	if ( window )
	{
		if ( active >= lists->FirstProfileTab )
		{
			RefreshGList( (struct Gadget *)( active == lists->FirstProfileTab ?
				lists->NetworkBrowser : lists->AdapterBrowser ), window, NULL, 1 );
		}

		if ( ! complete )
		{
			SetWindowTitles( window, "Wi-Fi Profiles - could not display all entries", (CONST_STRPTR)-1 );
		}
	}
}

// --

static APTR
SelectedEntry( const struct ProfileEditor *editor, Object *browser )
{
	ULONG node;
	ULONG entry;

	node = 0;
	entry = 0;

	GetAttr( LISTBROWSER_SelectedNode, browser, &node );

	if ( node )
	{
		editor->ListBrowser->GetListBrowserNodeAttrs( (struct Node *)node, LBNA_UserData, &entry, TAG_END );
	}

	return ( (APTR)entry );
}

// --

static S32
RemoveEntry( struct ProfileDatabase *db, S32 device, APTR entry )
{
	struct ProfileDatabase removed;
	struct ProfileAdapter **adapters;
	struct ProfileNetwork **networks;
	struct ProfileAdapter *adapter;
	struct ProfileNetwork *network;

	memset( &removed, 0, sizeof( removed ) );

	if ( device )
	{
		adapters = &db->Adapters;

		while ( *adapters != entry )
		{
			if ( ! *adapters )
			{
				return ( FALSE );
			}

			adapters = &(*adapters)->Next;
		}

		adapter = *adapters;
		*adapters = adapter->Next;
		adapter->Next = NULL;
		removed.Adapters = adapter;
	}
	else
	{
		network = entry;

		for ( adapter = db->Adapters; adapter; adapter = adapter->Next )
		{
			if ( ! strcmp( adapter->Profile.SSID, network->Profile.SSID ) )
			{
				return ( FALSE );
			}
		}

		networks = &db->Networks;

		while ( *networks != network )
		{
			if ( ! *networks )
			{
				return ( FALSE );
			}

			networks = &(*networks)->Next;
		}

		*networks = network->Next;
		network->Next = NULL;
		removed.Networks = network;
	}

	WiFi_FreeProfiles( &removed );

	return ( TRUE );
}

// --

static S32
HandleEntry( struct ProfileEditor *editor, struct EditorLists *lists, struct Window *window, U32 gadget )
{
	Object *browser;
	APTR entry;
	ULONG release;
	S32 adapter;

	adapter = gadget == EDITOR_ADAPTERS || gadget >= EDITOR_ADD_ADAPTER;
	browser = adapter ? lists->AdapterBrowser : lists->NetworkBrowser;

	if ( ( gadget == EDITOR_ADD_NETWORK ) || ( gadget == EDITOR_ADD_ADAPTER ) )
	{
		return ( WiFi_EditProfileEntry( editor, adapter, NULL ) );
	}

	if ( ( gadget == EDITOR_NETWORKS ) || ( gadget == EDITOR_ADAPTERS ) )
	{
		release = 0;

		GetAttr( LISTBROWSER_RelEvent, browser, &release );

		if ( ! ( release & LBRE_DOUBLECLICK ) )
		{
			return ( FALSE );
		}
	}

	entry = SelectedEntry( editor, browser );

	if ( ! entry )
	{
		SetWindowTitles( window, "Wi-Fi Profiles - select an entry first", (CONST_STRPTR)-1 );

		return ( FALSE );
	}

	if ( ( gadget == EDITOR_REMOVE_NETWORK ) || ( gadget == EDITOR_REMOVE_ADAPTER ) )
	{
		if ( ! RemoveEntry( &editor->Database, adapter, entry ) )
		{
			SetWindowTitles( window, "Wi-Fi Profiles - a device still references this SSID", (CONST_STRPTR)-1 );

			return ( FALSE );
		}

		return ( TRUE );
	}

	return ( WiFi_EditProfileEntry( editor, adapter, entry ) );
}

// --

static S32
SaveEditor( struct USB2WiFiPanel *panel, struct Window *window, S32 permanent )
{
	enum ProfileEditResult result;

	result = WiFi_SaveProfileEdit( &panel->Editor.Database, &panel->Original, &panel->OriginalLength, permanent );

	switch ( result )
	{
		case PROFILE_EDIT_SAVED:
		{
			panel->Dirty = FALSE;

			return ( TRUE );
		}

		case PROFILE_EDIT_INVALID:
		{
			SetWindowTitles( window, "Wi-Fi Profiles - check the SSID and device entries", (CONST_STRPTR)-1 );

			break;
		}

		case PROFILE_EDIT_CHANGED:
		{
			SetWindowTitles( window, "Wi-Fi Profiles - database changed since opening", (CONST_STRPTR)-1 );

			break;
		}

		default:
		{
			SetWindowTitles( window, permanent ? "Wi-Fi Profiles - could not save profiles" :
				"Wi-Fi Profiles - could not apply profiles to ENV:", (CONST_STRPTR)-1 );

			break;
		}
	}

	return ( FALSE );
}

// --

static void
FreePanel( struct USB2WiFiPanel *panel )
{
	struct ProfileEditor *editor;

	U32 index;

	editor = &panel->Editor;

	if ( editor->ListBrowser )
	{
		editor->ListBrowser->FreeListBrowserList( &panel->Lists.Networks );

		editor->ListBrowser->FreeListBrowserList( &panel->Lists.Adapters );

		DropInterface( (struct Interface *)editor->ListBrowser );
	}

	for( index = EDITOR_CLASSES; index; index-- )
	{
		if ( panel->Bases[index - 1] )
		{
			CloseClass( panel->Bases[index - 1] );
		}
	}

	if ( editor->Screen )
	{
		UnlockPubScreen( NULL, editor->Screen );
	}

	WiFi_FreeProfiles( &editor->Database );

	WiFi_FreeProfileText( panel->Original, panel->OriginalLength );

	FreeVec( panel );
}

// --

static struct USB2WiFiPanel *
AllocPanel( U32 gadgetBase )
{
	static const char *const classNames[EDITOR_CLASSES] =
	{
		"window.class", "gadgets/layout.gadget", "gadgets/listbrowser.gadget", "gadgets/clicktab.gadget",
		"gadgets/button.gadget", "gadgets/string.gadget", "gadgets/checkbox.gadget",
		"gadgets/chooser.gadget", "images/label.image"
	};

	struct USB2WiFiPanel *panel;
	struct ProfileEditor *editor;
	struct LayoutIFace *layoutInterface;

	U32 index;

	panel = AllocVecTags( sizeof( *panel ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! panel )
	{
		return ( NULL );
	}

	editor = &panel->Editor;
	editor->GadgetBase = gadgetBase;

	NewList( &panel->Lists.Networks );

	NewList( &panel->Lists.Adapters );

	if ( WiFi_LoadProfileEdit( &editor->Database, &panel->Original, &panel->OriginalLength ) == USB2WIFI_ERROR )
	{
		FPrintf( ErrorOutput(), "Could not read ENV:usb2wifi. Check the SSID and ETH records.\n" );

		goto failed;
	}

	if ( ! WiFi_ObtainIntuition() )
	{
		goto failed;
	}

	editor->Screen = LockPubScreen( NULL );

	if ( ! editor->Screen )
	{
		goto failed;
	}

	for( index = 0; index < EDITOR_CLASSES; index++ )
	{
		panel->Bases[index] = OpenClass( classNames[index], 50, &editor->Classes[index] );

		if ( ! panel->Bases[index] )
		{
			goto failed;
		}
	}

	editor->ListBrowser = (APTR)GetInterface( (APTR)panel->Bases[EDITOR_LIST], "main", 1, NULL );

	if ( ! editor->ListBrowser )
	{
		goto failed;
	}

	layoutInterface = (APTR)GetInterface( (APTR)panel->Bases[EDITOR_LAYOUT], "main", 1, NULL );

	if ( ! layoutInterface )
	{
		goto failed;
	}

	editor->PageClass = layoutInterface->PAGE_GetClass();

	DropInterface( (APTR)layoutInterface );

	if ( editor->PageClass )
	{
		return ( panel );
	}

failed:
	FreePanel( panel );

	return ( NULL );
}

// --

static struct USB2WiFiPanel *
AddPanel( Object *layout, Object *defaults, U32 gadgetBase )
{
	struct USB2WiFiPanel *panel;

	panel = NULL;

	if ( ( layout ) && ( gadgetBase ) && ( gadgetBase <= 65519 ) )
	{
		panel = AllocPanel( gadgetBase );
	}

	if ( ! panel )
	{
		if ( defaults )
		{
			DisposeObject( defaults );
		}

		return ( NULL );
	}

	if ( ! BuildTabs( &panel->Editor, &panel->Lists, layout, defaults ) )
	{
		FreePanel( panel );

		return ( NULL );
	}

	RefreshRows( &panel->Editor, &panel->Lists, NULL );

	return ( panel );
}

// --

struct USB2WiFiPanel * APICALL
WiFi_AddProfilePanel( struct USB2WiFiIFace *self __attribute__((unused)), APTR layout, U32 gadgetBase )
{
	return ( AddPanel( layout, NULL, gadgetBase ) );
}

// --

struct USB2WiFiPanel * APICALL
WiFi_AddProfilePanelWithDefaults( struct USB2WiFiIFace *self __attribute__((unused)),
	APTR layout, APTR defaultPage, U32 gadgetBase )
{
	return ( AddPanel( layout, defaultPage, gadgetBase ) );
}

// --

void APICALL
WiFi_HandleProfilePanel( struct USB2WiFiIFace *self __attribute__((unused)), struct USB2WiFiPanel *panel,
	struct Window *window, U32 gadget )
{
	gadget -= panel->Editor.GadgetBase;

	if ( ( gadget >= EDITOR_NETWORKS ) && ( gadget <= EDITOR_EDIT_ADAPTER ) &&
		( HandleEntry( &panel->Editor, &panel->Lists, window, gadget ) ) )
	{
		panel->Dirty = TRUE;

		RefreshRows( &panel->Editor, &panel->Lists, window );
	}
}

// --

S32 APICALL
WiFi_ApplyProfilePanel( struct USB2WiFiIFace *self __attribute__((unused)), struct USB2WiFiPanel *panel,
	struct Window *window, S32 permanent )
{
	if ( ( ! panel->Dirty ) && ( ( ! permanent ) || ( ! panel->Original ) ) )
	{
		return ( USB2WIFI_MISSING );
	}

	return ( SaveEditor( panel, window, permanent ) ? USB2WIFI_OK : USB2WIFI_ERROR );
}

// --

void APICALL
WiFi_FreeProfilePanel( struct USB2WiFiIFace *self __attribute__((unused)), struct USB2WiFiPanel *panel )
{
	FreePanel( panel );
}

// --

S32 APICALL
WiFi_EditProfiles( struct USB2WiFiIFace *self __attribute__((unused)) )
{
	// Keep the retired standalone editor's interface slot callable.
	return ( RETURN_FAIL );
}

// --
