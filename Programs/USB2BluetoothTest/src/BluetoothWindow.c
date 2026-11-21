
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <string.h>
#include <classes/window.h>
#include <dos/var.h>
#include <gadgets/getfile.h>
#include <gadgets/integer.h>
#include <gadgets/layout.h>
#include <gadgets/listbrowser.h>
#include <gadgets/string.h>
#include <images/bevel.h>
#include <interfaces/listbrowser.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include "BluetoothWindow.h"

// --

struct IntuitionIFace *IIntuition;

enum BTWindowClass
{
	CLASS_WINDOW,
	CLASS_LAYOUT,
	CLASS_LIST,
	CLASS_INTEGER,
	CLASS_STRING,
	CLASS_FILE,
	CLASS_BUTTON,
	CLASS_COUNT,
};

enum BTWindowGadget
{
	GADGET_UNIT = 1,
	GADGET_PEERS,
	GADGET_ADDRESS,
	GADGET_FILE,
	GADGET_CHANNEL,
	GADGET_SCAN,
	GADGET_SEND,
	GADGET_PLAY,
	GADGET_AHI,
	GADGET_CANCEL,
	GADGET_COUNT,
};

struct BTWindow
{
	struct BTWindowJob Job;
	struct MsgPort *Reply;
	struct Window *Window;
	struct ListBrowserIFace *ListBrowser;
	struct List Peers;
	Class *Classes[CLASS_COUNT];
	Object *Gadgets[GADGET_COUNT];
	ULONG WindowMask;
	ULONG JobMask;
	LONG Busy;
	LONG Closing;
};

static const char *const ClassNames[] =
{
	"window.class",
	"gadgets/layout.gadget",
	"gadgets/listbrowser.gadget",
	"gadgets/integer.gadget",
	"gadgets/string.gadget",
	"gadgets/getfile.gadget",
	"gadgets/button.gadget",
};

static const struct TagItem FixedHeight[] = { { CHILD_WeightedHeight, 0 }, { TAG_END, 0 } };

// --

static void
ShowStatus( struct BTWindow *gui, const char *text )
{
	SetWindowTitles( gui->Window, text, (CONST_STRPTR) -1 );
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
MakeGroup( struct BTWindow *gui, Object *layout, const char *label )
{
	Object *group;

	group = NewObject( gui->Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
		LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, LAYOUT_BevelStyle, BVS_GROUP,
		LAYOUT_Label, label, TAG_END );

	return ( AddChild( layout, group, FixedHeight ) );
}

// --

static LONG
AddButton( struct BTWindow *gui, Object *row, ULONG id, const char *text )
{
	Object *button;

	button = NewObject( gui->Classes[CLASS_BUTTON], NULL, GA_ID, id, GA_RelVerify, TRUE,
		GA_Text, text, GA_Underscore, '_', TAG_END );

	gui->Gadgets[id] = AddChild( row, button, NULL );

	return ( gui->Gadgets[id] != NULL );
}

// --

static LONG
BuildControls( struct BTWindow *gui, Object *layout, ULONG unit )
{
	Object *group;
	Object *control;

	group = MakeGroup( gui, layout, "Adapter unit" );

	if ( ! group )
	{
		return ( FALSE );
	}

	control = NewObject( gui->Classes[CLASS_INTEGER], NULL, GA_ID, GADGET_UNIT, GA_TabCycle, TRUE,
		INTEGER_Number, unit, INTEGER_Minimum, 0, INTEGER_Maximum, 2147483647L, INTEGER_MaxChars, 10, TAG_END );

	gui->Gadgets[GADGET_UNIT] = AddChild( group, control, NULL );

	if ( ( ! gui->Gadgets[GADGET_UNIT] ) || ( ! AddButton( gui, group, GADGET_SCAN, "_Scan Classic" ) ) )
	{
		return ( FALSE );
	}

	control = NewObject( gui->Classes[CLASS_LIST], NULL, GA_ID, GADGET_PEERS, GA_RelVerify, TRUE,
		LISTBROWSER_ShowSelected, TRUE, LISTBROWSER_Labels, & gui->Peers, LISTBROWSER_MinVisible, 6, TAG_END );

	gui->Gadgets[GADGET_PEERS] = AddChild( layout, control, NULL );

	group = MakeGroup( gui, layout, "Peer address" );

	if ( ( ! gui->Gadgets[GADGET_PEERS] ) || ( ! group ) )
	{
		return ( FALSE );
	}

	control = NewObject( gui->Classes[CLASS_STRING], NULL, GA_ID, GADGET_ADDRESS, GA_TabCycle, TRUE,
		STRINGA_MaxChars, 18, STRINGA_TextVal, "", TAG_END );

	gui->Gadgets[GADGET_ADDRESS] = AddChild( group, control, NULL );

	group = MakeGroup( gui, layout, "Local file" );

	if ( ( ! gui->Gadgets[GADGET_ADDRESS] ) || ( ! group ) )
	{
		return ( FALSE );
	}

	control = NewObject( gui->Classes[CLASS_FILE], NULL, GA_ID, GADGET_FILE, GA_RelVerify, TRUE,
		GETFILE_ReadOnly, TRUE, GETFILE_TitleText, "Choose a file", GETFILE_FullFile, "", TAG_END );

	gui->Gadgets[GADGET_FILE] = AddChild( group, control, NULL );

	group = MakeGroup( gui, layout, "RFCOMM channel (0 = auto)" );

	if ( ( ! gui->Gadgets[GADGET_FILE] ) || ( ! group ) )
	{
		return ( FALSE );
	}

	control = NewObject( gui->Classes[CLASS_INTEGER], NULL, GA_ID, GADGET_CHANNEL, GA_TabCycle, TRUE,
		INTEGER_Number, 0, INTEGER_Minimum, 0, INTEGER_Maximum, 30, INTEGER_MaxChars, 2, TAG_END );

	gui->Gadgets[GADGET_CHANNEL] = AddChild( group, control, NULL );

	group = MakeGroup( gui, layout, "Actions" );

	return ( ( gui->Gadgets[GADGET_CHANNEL] ) && ( group ) &&
		( AddButton( gui, group, GADGET_SEND, "_Send file" ) ) &&
		( AddButton( gui, group, GADGET_PLAY, "_Play WAV" ) ) &&
		( AddButton( gui, group, GADGET_AHI, "Use for _AHI" ) ) &&
		( AddButton( gui, group, GADGET_CANCEL, "_Stop" ) ) );
}

// --

static void
SetBusy( struct BTWindow *gui, LONG busy )
{
	ULONG id;

	gui->Busy = busy;

	for( id = GADGET_UNIT; id < GADGET_COUNT; id++ )
	{
		RefreshSetGadgetAttrs( (struct Gadget *) gui->Gadgets[id], gui->Window, NULL,
			GA_Disabled, id == GADGET_CANCEL ? ! busy : busy, TAG_END );
	}
}

// --

static void
FormatAddress( char *address, const uint8_t *bytes )
{
	snprintf( address, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
		(unsigned) bytes[0], (unsigned) bytes[1], (unsigned) bytes[2],
		(unsigned) bytes[3], (unsigned) bytes[4], (unsigned) bytes[5] );
}

// --

static void
ShowPeers( struct BTWindow *gui )
{
	struct Node *node;

	char address[18];
	char row[48];
	ULONG index;

	SetAttrs( gui->Gadgets[GADGET_PEERS], LISTBROWSER_Labels, ~0UL, TAG_END );

	gui->ListBrowser->FreeListBrowserList( & gui->Peers );

	for( index = 0; index < gui->Job.Count; index++ )
	{
		FormatAddress( address, gui->Job.Peers[index].Address );

		snprintf( row, sizeof( row ), "%s    Class $%06lX",
			address, (unsigned long) gui->Job.Peers[index].ClassOfDevice );

		node = gui->ListBrowser->AllocListBrowserNode( 1, LBNA_UserData, index,
			LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, row, TAG_END );

		if ( ! node )
		{
			break;
		}

		AddTail( & gui->Peers, node );
	}

	RefreshSetGadgetAttrs( (struct Gadget *) gui->Gadgets[GADGET_PEERS], gui->Window, NULL,
		LISTBROWSER_Labels, & gui->Peers, LISTBROWSER_Selected, -1, TAG_END );
}

// --

static void
SelectPeer( struct BTWindow *gui )
{
	struct Node *node;

	char address[18];
	ULONG selected;
	ULONG index;

	selected = 0;

	if ( ( ! GetAttr( LISTBROWSER_SelectedNode, gui->Gadgets[GADGET_PEERS], & selected ) ) || ( ! selected ) )
	{
		return;
	}

	node = (struct Node *) selected;

	gui->ListBrowser->GetListBrowserNodeAttrs( node, LBNA_UserData, & index, TAG_END );

	if ( index < BT_WINDOW_PEERS )
	{
		FormatAddress( address, gui->Job.Peers[index].Address );

		RefreshSetGadgetAttrs( (struct Gadget *) gui->Gadgets[GADGET_ADDRESS], gui->Window, NULL,
			STRINGA_TextVal, address, TAG_END );
	}
}

// --

static void
ChooseFile( struct BTWindow *gui )
{
	Object *control;

	char path[BT_WINDOW_PATH];
	ULONG drawer;
	ULONG file;

	control = gui->Gadgets[GADGET_FILE];
	drawer = 0;
	file = 0;

	if ( ! gfRequestFile( control, gui->Window ) )
	{
		return;
	}

	GetAttr( GETFILE_Drawer, control, & drawer );

	GetAttr( GETFILE_File, control, & file );

	if ( ( ! drawer ) || ( ! file ) || ( ! *(const char *) file ) ||
		( strlcpy( path, (const char *) drawer, sizeof( path ) ) >= sizeof( path ) ) ||
		( ! AddPart( path, (const char *) file, sizeof( path ) ) ) )
	{
		ShowStatus( gui, "USB2 Bluetooth - Could not use the selected file" );

		return;
	}

	strcpy( gui->Job.Path, path );

	RefreshSetGadgetAttrs( (struct Gadget *) control, gui->Window, NULL,
		GETFILE_FullFile, gui->Job.Path, TAG_END );
}

// --

static LONG
GetPeerAddress( struct BTWindow *gui, char *formatted )
{
	uint8_t bytes[6];
	ULONG address;

	address = 0;

	GetAttr( STRINGA_TextVal, gui->Gadgets[GADGET_ADDRESS], & address );

	if ( ( ! address ) || ( ! ParseAddress( (const char *) address, bytes ) ) )
	{
		ShowStatus( gui, "USB2 Bluetooth - Choose a peer or enter its address" );

		return ( FALSE );
	}

	FormatAddress( formatted, bytes );

	return ( TRUE );
}

// --

static void
UseForAHI( struct BTWindow *gui )
{
	char address[18];
	char setting[32];
	ULONG unit;
	LONG length;

	if ( ! GetPeerAddress( gui, address ) )
	{
		return;
	}

	unit = 0;

	GetAttr( INTEGER_Number, gui->Gadgets[GADGET_UNIT], & unit );

	length = snprintf( setting, sizeof( setting ), "%lu %s", (unsigned long) unit, address );

	if ( ( length < 0 ) || ( (ULONG) length >= sizeof( setting ) ) ||
		( ! SetVar( "Bluetooth2-AHI", setting, length, GVF_GLOBAL_ONLY ) ) )
	{
		ShowStatus( gui, "USB2 Bluetooth - Could not set the AHI speaker" );

		return;
	}

	ShowStatus( gui, "USB2 Bluetooth - AHI speaker set for this session" );
}

// --

static void
StartAction( struct BTWindow *gui, enum BTWindowAction action )
{
	if ( action != BT_WINDOW_SCAN )
	{
		if ( ! GetPeerAddress( gui, gui->Job.Address ) )
		{
			return;
		}

		if ( ! gui->Job.Path[0] )
		{
			ShowStatus( gui, "USB2 Bluetooth - Choose a local file" );

			return;
		}

		GetAttr( INTEGER_Number, gui->Gadgets[GADGET_CHANNEL], & gui->Job.Channel );
	}

	GetAttr( INTEGER_Number, gui->Gadgets[GADGET_UNIT], & gui->Job.Unit );

	gui->Job.Action = action;

	if ( ! StartBluetoothJob( & gui->Job, gui->Reply ) )
	{
		ShowStatus( gui, "USB2 Bluetooth - Could not start the operation" );

		return;
	}

	SetBusy( gui, TRUE );

	switch( action )
	{
		case BT_WINDOW_SCAN:
			ShowStatus( gui, "USB2 Bluetooth - Scanning" );
			break;
		case BT_WINDOW_SEND:
			ShowStatus( gui, "USB2 Bluetooth - Sending file" );
			break;
		case BT_WINDOW_PLAY:
			ShowStatus( gui, "USB2 Bluetooth - Playing WAV" );
			break;
	}
}

// --

static void
FinishAction( struct BTWindow *gui )
{
	const char *status;

	if ( gui->Job.Action == BT_WINDOW_SCAN )
	{
		ShowPeers( gui );

		if ( gui->Job.Cancelled )
		{
			status = "USB2 Bluetooth - Scan stopped";
		}
		else if ( gui->Job.Action == BT_WINDOW_PLAY )
	{
		if ( gui->Job.Result == RETURN_OK )
		{
			status = "USB2 Bluetooth - Playback finished";
		}
		else
		{
			status = gui->Job.Result == RETURN_WARN ? "USB2 Bluetooth - Playback stopped" :
				"USB2 Bluetooth - Playback failed (see Shell)";
		}
	}
	else if ( gui->Job.Result == RETURN_OK )
		{
			status = gui->Job.Count ? "USB2 Bluetooth - Select a peer" : "USB2 Bluetooth - No discoverable peers";
		}
		else
		{
			status = gui->Job.DiscoveryResult == BT_HCI_NO_CONTROLLER ?
				"USB2 Bluetooth - Adapter unit unavailable" : "USB2 Bluetooth - Scan failed";
		}
	}
	else if ( gui->Job.Result == RETURN_OK )
	{
		status = "USB2 Bluetooth - File accepted";
	}
	else
	{
		status = gui->Job.Result == RETURN_WARN ? "USB2 Bluetooth - Send stopped" :
			"USB2 Bluetooth - Send failed (see Shell)";
	}

	SetBusy( gui, FALSE );

	ShowStatus( gui, status );
}

// --

static void
RunWindow( struct BTWindow *gui, Object *object )
{
	ULONG signals;
	ULONG event;
	UWORD code;

	while( ( ! gui->Closing ) || ( gui->Busy ) )
	{
		signals = Wait( gui->WindowMask | gui->JobMask | SIGBREAKF_CTRL_C );

		if ( ( signals & gui->JobMask ) && ( GetMsg( gui->Reply ) ) )
		{
			FinishAction( gui );
		}

		if ( signals & SIGBREAKF_CTRL_C )
		{
			gui->Closing = TRUE;
		}

		while( ( event = IDoMethod( object, WM_HANDLEINPUT, & code ) ) != WMHI_LASTMSG )
		{
			if ( ( event & WMHI_CLASSMASK ) == WMHI_CLOSEWINDOW )
			{
				gui->Closing = TRUE;

				continue;
			}

			if ( ( gui->Closing ) || ( ( event & WMHI_CLASSMASK ) != WMHI_GADGETUP ) )
			{
				continue;
			}

			if ( gui->Busy )
			{
				if ( ( event & WMHI_GADGETMASK ) == GADGET_CANCEL )
				{
					CancelBluetoothJob( & gui->Job );

					ShowStatus( gui, "USB2 Bluetooth - Stopping" );
				}

				continue;
			}

			switch( event & WMHI_GADGETMASK )
			{
				case GADGET_PEERS:
					SelectPeer( gui );
					break;
				case GADGET_FILE:
					ChooseFile( gui );
					break;
				case GADGET_SCAN:
					StartAction( gui, BT_WINDOW_SCAN );
					break;
				case GADGET_SEND:
					StartAction( gui, BT_WINDOW_SEND );
					break;
				case GADGET_PLAY:
					StartAction( gui, BT_WINDOW_PLAY );
					break;
				case GADGET_AHI:
					UseForAHI( gui );
					break;
			}
		}

		if ( ( gui->Closing ) && ( gui->Busy ) )
		{
			CancelBluetoothJob( & gui->Job );

			ShowStatus( gui, "USB2 Bluetooth - Stopping" );
		}
	}
}

// --

LONG
RunBluetoothWindow( ULONG unit )
{
	struct BTWindow gui;
	struct Library *intuition;
	struct ClassLibrary *bases[CLASS_COUNT];
	Object *object;
	Object *layout;

	ULONG index;
	LONG result;

	memset( & gui, 0, sizeof( gui ) );

	memset( bases, 0, sizeof( bases ) );

	object = NULL;
	layout = NULL;
	result = RETURN_FAIL;

	intuition = OpenLibrary( "intuition.library", 50 );

	if ( ! intuition )
	{
		return ( result );
	}

	IIntuition = (APTR) GetInterface( intuition, "main", 1, NULL );

	if ( ! IIntuition )
	{
		goto done;
	}

	NewList( & gui.Peers );

	InitSemaphore( & gui.Job.Lock );

	gui.Reply = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! gui.Reply )
	{
		goto done;
	}

	gui.JobMask = 1UL << gui.Reply->mp_SigBit;

	for( index = 0; index < CLASS_COUNT; index++ )
	{
		bases[index] = OpenClass( ClassNames[index], 50, & gui.Classes[index] );

		if ( ! bases[index] )
		{
			Printf( "Could not open %s.\n", ClassNames[index] );

			goto done;
		}
	}

	gui.ListBrowser = (APTR) GetInterface( (APTR) bases[CLASS_LIST], "main", 1, NULL );

	if ( ! gui.ListBrowser )
	{
		goto done;
	}

	layout = NewObject( gui.Classes[CLASS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
		LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ( ! layout ) || ( ! BuildControls( & gui, layout, unit ) ) )
	{
		goto done;
	}

	object = NewObject( gui.Classes[CLASS_WINDOW], NULL, WA_Title, "USB2 Bluetooth",
		WA_Width, 480, WA_AutoAdjust, TRUE, WA_Activate, TRUE, WA_DragBar, TRUE, WA_DepthGadget, TRUE,
		WA_CloseGadget, TRUE, WA_SizeGadget, TRUE, WINDOW_Position, WPOS_CENTERSCREEN,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP, TAG_END );

	if ( ! object )
	{
		goto done;
	}

	SetAttrs( object, WINDOW_Layout, layout, TAG_END );

	layout = NULL;

	gui.Window = (struct Window *) IDoMethod( object, WM_OPEN, NULL );

	if ( ( ! gui.Window ) || ( ! GetAttr( WINDOW_SigMask, object, & gui.WindowMask ) ) || ( ! gui.WindowMask ) )
	{
		goto done;
	}

	SetBusy( & gui, FALSE );

	RunWindow( & gui, object );

	result = RETURN_OK;

done:

	if ( object )
	{
		DisposeObject( object );
	}

	if ( layout )
	{
		DisposeObject( layout );
	}

	if ( gui.ListBrowser )
	{
		gui.ListBrowser->FreeListBrowserList( & gui.Peers );

		DropInterface( (APTR) gui.ListBrowser );
	}

	for( index = CLASS_COUNT; index; index-- )
	{
		if ( bases[index - 1] )
		{
			CloseClass( bases[index - 1] );
		}
	}

	if ( gui.Reply )
	{
		FreeSysObject( ASOT_PORT, gui.Reply );
	}

	if ( IIntuition )
	{
		DropInterface( (APTR) IIntuition );

		IIntuition = NULL;
	}

	CloseLibrary( intuition );

	if ( result != RETURN_OK )
	{
		Printf( "Could not open the USB2 Bluetooth window.\n" );
	}

	return ( result );
}

// --
