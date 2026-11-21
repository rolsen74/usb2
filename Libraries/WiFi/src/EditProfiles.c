
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
#include <gadgets/clicktab.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/space.h>
#include <images/bevel.h>
#include <interfaces/layout.h>
#include <stdio.h>

// --

struct EditorLists
{
	struct List Networks;
	struct List Adapters;
	struct ColumnInfo NetworkColumns[4];
	struct ColumnInfo AdapterColumns[5];
	Object *NetworkBrowser;
	Object *AdapterBrowser;
	Object *Tabs;
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
		GA_ID, adapter ? EDITOR_ADAPTERS : EDITOR_NETWORKS, GA_RelVerify, TRUE, LISTBROWSER_ShowSelected, TRUE,
		LISTBROWSER_ColumnTitles, TRUE, LISTBROWSER_ColumnInfo, columns, LISTBROWSER_Labels, rows,
		LISTBROWSER_Selected, -1, LISTBROWSER_MinVisible, 5, TAG_END ), NULL );

	buttons = WiFi_AddEditorChild( page, NewObject( editor->Classes[EDITOR_LAYOUT], NULL,
		LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_SpaceInner, TRUE, LAYOUT_EvenSize, TRUE, TAG_END ), FixedHeight );

	add = adapter ? EDITOR_ADD_ADAPTER : EDITOR_ADD_NETWORK;
	remove = adapter ? EDITOR_REMOVE_ADAPTER : EDITOR_REMOVE_NETWORK;
	edit = adapter ? EDITOR_EDIT_ADAPTER : EDITOR_EDIT_NETWORK;

	if ( ( ! *browser ) || ( ! buttons )
		|| ( ! WiFi_AddEditorChild( buttons, WiFi_EditorButton( editor, add, "Add" ), NULL ) )
		|| ( ! WiFi_AddEditorChild( buttons, WiFi_EditorButton( editor, remove, "Remove" ), NULL ) )
		|| ( ! WiFi_AddEditorChild( buttons, WiFi_EditorButton( editor, edit, "Edit" ), NULL ) ) )
	{
		DisposeObject( page );

		return ( NULL );
	}

	return ( page );
}

// --

static S32
BuildTabs( const struct ProfileEditor *editor, struct EditorLists *lists, Object *layout )
{
	static const char *const tabs[] = { "SSIDs", "Devices", NULL };
	static const struct ColumnInfo networkColumns[] = { { 55, "SSID", 0 }, { 25, "Security", 0 },
		{ 20, "Hidden", 0 }, { -1, NULL, 0 } };
	static const struct ColumnInfo adapterColumns[] = { { 35, "Device filename", 0 }, { 10, "Unit", 0 },
		{ 32, "SSID", 0 }, { 23, "Autoconnect", 0 }, { -1, NULL, 0 } };

	Object *networks;
	Object *adapters;
	Object *pages;
	Object *object;

	memcpy( lists->NetworkColumns, networkColumns, sizeof( networkColumns ) );

	memcpy( lists->AdapterColumns, adapterColumns, sizeof( adapterColumns ) );

	networks = BuildPage( editor, &lists->Networks, lists->NetworkColumns, FALSE, &lists->NetworkBrowser );

	if ( ! networks )
	{
		return ( FALSE );
	}

	adapters = BuildPage( editor, &lists->Adapters, lists->AdapterColumns, TRUE, &lists->AdapterBrowser );

	if ( ! adapters )
	{
		DisposeObject( networks );

		return ( FALSE );
	}

	pages = NewObject( editor->PageClass, NULL, PAGE_Add, networks, PAGE_Add, adapters, TAG_END );

	if ( ! pages )
	{
		DisposeObject( adapters );

		DisposeObject( networks );

		return ( FALSE );
	}

	object = NewObject( editor->Classes[EDITOR_TABS], NULL, GA_Text, tabs, CLICKTAB_PageGroup, pages, TAG_END );

	if ( ! object )
	{
		DisposeObject( pages );

		return ( FALSE );
	}

	lists->Tabs = WiFi_AddEditorChild( layout, object, NULL );

	return ( lists->Tabs != NULL );
}

// --

static S32
BuildFooter( const struct ProfileEditor *editor, Object *layout )
{
	static const char *const labels[] = { "_Save", "_Use", "_Cancel" };

	Object *row;
	Object *spacer;
	U32 index;

	if ( ! WiFi_AddEditorChild( layout, NewObject( editor->Classes[EDITOR_SPACE], NULL,
		SPACE_MinHeight, 3, SPACE_BevelStyle, BVS_SBAR_VERT, TAG_END ), FixedHeight ) )
	{
		return ( FALSE );
	}

	row = WiFi_AddEditorChild( layout, NewObject( editor->Classes[EDITOR_LAYOUT], NULL,
		LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_EvenSize, TRUE, LAYOUT_SpaceInner, TRUE, TAG_END ), FixedHeight );

	if ( ! row )
	{
		return ( FALSE );
	}

	for ( index = 0; index < sizeof( labels ) / sizeof( *labels ); index++ )
	{
		if ( index )
		{
			spacer = NewObject( editor->Classes[EDITOR_SPACE], NULL, SPACE_Transparent, TRUE, TAG_END );

			if ( ! WiFi_AddEditorChild( row, spacer, NULL ) )
			{
				return ( FALSE );
			}
		}

		if ( ! WiFi_AddEditorChild( row, WiFi_EditorButton( editor, EDITOR_SAVE + index, labels[index] ), NULL ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static void
RefreshRows( const struct ProfileEditor *editor, struct EditorLists *lists, struct Window *window )
{
	const struct ProfileNetwork *network;
	const struct ProfileAdapter *adapter;
	struct Node *node;

	char unit[12];
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
		node = editor->ListBrowser->AllocListBrowserNode( 3, LBNA_UserData, network,
			LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, network->Profile.SSID,
			LBNA_Column, 1, LBNCA_Text, WiFi_ProfileSecurityNames[network->Profile.Security],
			LBNA_Column, 2, LBNCA_Text, ( network->Profile.Options & USB2WIFI_PROFILE_HIDDEN ) ? "Yes" : "No", TAG_END );

		if ( ! node )
		{
			complete = FALSE;
			break;
		}

		AddTail( &lists->Networks, node );
	}

	for ( adapter = editor->Database.Adapters; adapter; adapter = adapter->Next )
	{
		snprintf( unit, sizeof( unit ), "%lu", (unsigned long)adapter->Unit );

		node = editor->ListBrowser->AllocListBrowserNode( 4, LBNA_UserData, adapter,
			LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, adapter->Device,
			LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, unit,
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

	RefreshGList( (struct Gadget *)( active ? lists->AdapterBrowser : lists->NetworkBrowser ), window, NULL, 1 );

	SetWindowTitles( window, complete ? "USB2 Wi-Fi Profiles" : "Wi-Fi Profiles - could not display all entries", (CONST_STRPTR)-1 );
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
			if ( ( adapter->Profile.Security == network->Profile.Security ) &&
				( ! strcmp( adapter->Profile.SSID, network->Profile.SSID ) ) )
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
SaveEditor( const struct ProfileDatabase *db, struct Window *window, const char *original, U32 originalLength, S32 permanent )
{
	enum ProfileEditResult result;

	result = WiFi_SaveProfileEdit( db, original, originalLength, permanent );

	switch ( result )
	{
		case PROFILE_EDIT_SAVED:
		{
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

S32
WiFi_EditProfiles( struct USB2WiFiIFace *self __attribute__((unused)) )
{
	static const char *const classNames[EDITOR_CLASSES] =
	{
		"window.class", "gadgets/layout.gadget", "gadgets/listbrowser.gadget", "gadgets/clicktab.gadget",
		"gadgets/button.gadget", "gadgets/space.gadget", "gadgets/string.gadget", "gadgets/checkbox.gadget",
		"gadgets/chooser.gadget", "images/label.image"
	};

	struct ProfileEditor editor;
	struct EditorLists lists;
	struct ClassLibrary *bases[EDITOR_CLASSES];
	struct LayoutIFace *layoutInterface;
	struct Window *window;

	Object *object;
	Object *layout;
	char *original;
	U32 originalLength;
	U32 index;
	U32 event;
	U32 gadget;
	ULONG mask;
	U16 code;
	S32 result;

	memset( &editor, 0, sizeof( editor ) );

	memset( &lists, 0, sizeof( lists ) );

	memset( bases, 0, sizeof( bases ) );

	NewList( &lists.Networks );

	NewList( &lists.Adapters );

	object = NULL;
	layout = NULL;
	result = RETURN_FAIL;
	mask = 0;

	if ( WiFi_LoadProfileEdit( &editor.Database, &original, &originalLength ) == USB2WIFI_ERROR )
	{
		FPrintf( ErrorOutput(), "Could not read ENV:usb2wifi. Check the SSID and ETH records.\n" );

		goto done;
	}

	if ( ! WiFi_ObtainIntuition() )
	{
		goto done;
	}

	editor.Screen = LockPubScreen( NULL );

	if ( ! editor.Screen )
	{
		goto done;
	}

	for ( index = 0; index < EDITOR_CLASSES; index++ )
	{
		bases[index] = OpenClass( classNames[index], 50, &editor.Classes[index] );

		if ( ! bases[index] )
		{
			FPrintf( ErrorOutput(), "Could not open %s.\n", classNames[index] );

			goto done;
		}
	}

	editor.ListBrowser = (struct ListBrowserIFace *)GetInterface( (struct Library *)bases[EDITOR_LIST], "main", 1, NULL );

	if ( ! editor.ListBrowser )
	{
		goto done;
	}

	layoutInterface = (struct LayoutIFace *)GetInterface( (struct Library *)bases[EDITOR_LAYOUT], "main", 1, NULL );

	if ( ! layoutInterface )
	{
		goto done;
	}

	editor.PageClass = layoutInterface->PAGE_GetClass();

	DropInterface( (struct Interface *)layoutInterface );

	if ( ! editor.PageClass )
	{
		goto done;
	}

	object = NewObject( editor.Classes[EDITOR_WINDOW], NULL, WA_PubScreen, editor.Screen, WA_Title, "USB2 Wi-Fi Profiles",
		WA_Width, editor.Screen->Width < 660 ? editor.Screen->Width : 660,
		WA_Height, editor.Screen->Height < 350 ? editor.Screen->Height : 350,
		WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_SizeGadget, TRUE,
		WA_Activate, TRUE, WA_AutoAdjust, TRUE, WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP,
		WINDOW_Position, WPOS_CENTERSCREEN, TAG_END );

	layout = NewObject( editor.Classes[EDITOR_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ( ! object ) || ( ! layout ) || ( ! BuildTabs( &editor, &lists, layout ) ) || ( ! BuildFooter( &editor, layout ) ) )
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

	RefreshRows( &editor, &lists, window );

	result = RETURN_OK;

	while ( ! ( Wait( mask | SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C ) )
	{
		while ( ( event = IDoMethod( object, WM_HANDLEINPUT, &code ) ) != WMHI_LASTMSG )
		{
			if ( ( event & WMHI_CLASSMASK ) == WMHI_CLOSEWINDOW )
			{
				goto done;
			}

			if ( ( event & WMHI_CLASSMASK ) != WMHI_GADGETUP )
			{
				continue;
			}

			gadget = event & WMHI_GADGETMASK;

			if ( gadget == EDITOR_CANCEL )
			{
				goto done;
			}

			if ( ( gadget == EDITOR_USE ) || ( gadget == EDITOR_SAVE ) )
			{
				if ( SaveEditor( &editor.Database, window, original, originalLength, gadget == EDITOR_SAVE ) )
				{
					goto done;
				}
			}
			else if ( ( gadget >= EDITOR_NETWORKS ) && ( gadget <= EDITOR_EDIT_ADAPTER )
				&& ( HandleEntry( &editor, &lists, window, gadget ) ) )
			{
				RefreshRows( &editor, &lists, window );
			}
		}
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

	if ( editor.ListBrowser )
	{
		editor.ListBrowser->FreeListBrowserList( &lists.Networks );

		editor.ListBrowser->FreeListBrowserList( &lists.Adapters );

		DropInterface( (struct Interface *)editor.ListBrowser );
	}

	for ( index = EDITOR_CLASSES; index; index-- )
	{
		if ( bases[index - 1] )
		{
			CloseClass( bases[index - 1] );
		}
	}

	if ( editor.Screen )
	{
		UnlockPubScreen( NULL, editor.Screen );
	}

	WiFi_FreeProfiles( &editor.Database );

	WiFi_FreeProfileText( original, originalLength );

	return ( result );
}

// --
