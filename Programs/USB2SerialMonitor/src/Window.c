
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Monitor.h"

#include <classes/window.h>
#include <devices/usb2defaults.h>
#include <gadgets/button.h>
#include <gadgets/chooser.h>
#include <gadgets/integer.h>
#include <gadgets/layout.h>
#include <gadgets/scroller.h>
#include <gadgets/texteditor.h>
#include <images/label.h>
#include <stdio.h>
#include <string.h>

// --

enum
{
	GADGET_CAPTURE = 1,
	GADGET_SETUP,
	GADGET_SAVE,
	GADGET_CLEAR,
	GADGET_ACCEPT,
	GADGET_CANCEL
};

// --

static Object *
AddChild( Object *layout, Object *child, Object *label, BOOL fixedHeight, BOOL fixedWidth )
{
	struct TagItem tags[] =
	{
		{ CHILD_Label, (Tag) label },
		{ fixedHeight ? CHILD_WeightedHeight : TAG_IGNORE, 0 },
		{ fixedWidth ? CHILD_WeightedWidth : TAG_IGNORE, 0 },
		{ TAG_END, 0 }
	};

	if ( ( child ) && ( IDoMethod( layout, LM_ADDCHILD, NULL, child, tags ) ) )
	{
		return ( child );
	}

	DisposeObject( child );

	DisposeObject( label );

	return ( NULL );
}

// --

static Object *
Layout( struct Monitor *monitor, BOOL vertical )
{
	return ( NewObject( monitor->Classes[CLASS_LAYOUT], NULL,
		LAYOUT_Orientation, vertical ? LAYOUT_ORIENT_VERT : LAYOUT_ORIENT_HORIZ,
		LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, vertical, TAG_END ) );
}

// --

static Object *
Button( struct Monitor *monitor, Object *layout, U32 id, const char *text )
{
	return ( AddChild( layout, NewObject( monitor->Classes[CLASS_BUTTON], NULL,
		GA_ID, id, GA_RelVerify, TRUE, GA_Text, text, TAG_END ), NULL, FALSE, FALSE ) );
}

// --

static Object *
WindowObject( struct Monitor *monitor, Object *layout, const char *title, BOOL main )
{
	Object *object;

	object = NewObject( monitor->Classes[CLASS_WINDOW], NULL,
		WA_Title, title, WA_DragBar, TRUE, WA_DepthGadget, TRUE,
		WA_CloseGadget, TRUE, WA_SizeGadget, main, WA_Activate, TRUE,
		WA_InnerWidth, main ? 640 : 360, main ? WA_InnerHeight : TAG_IGNORE, 360,
		WA_AutoAdjust, TRUE, WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_RAWKEY,
		WINDOW_Position, main ? WPOS_CENTERSCREEN : WPOS_CENTERWINDOW,
		WINDOW_RefWindow, monitor->Window, WINDOW_ParentGroup, layout, TAG_END );

	if ( ! object )
	{
		DisposeObject( layout );
	}

	return ( object );
}

// --

BOOL
Monitor_OpenMain( struct Monitor *monitor )
{
	Object *layout;
	Object *body;
	Object *buttons;
	Object *scroller;

	layout = Layout( monitor, TRUE );

	if ( ! layout )
	{
		return ( FALSE );
	}

	body = AddChild( layout, Layout( monitor, FALSE ), NULL, FALSE, FALSE );

	if ( ! body )
	{
		goto failed;
	}

	monitor->Editor = AddChild( body, NewObject( monitor->Classes[CLASS_EDITOR], NULL,
		GA_TEXTEDITOR_ReadOnly, TRUE, GA_TEXTEDITOR_FixedFont, TRUE,
		GA_TEXTEDITOR_ImportHook, GV_TEXTEDITOR_ImportHook_Plain, TAG_END ), NULL, FALSE, FALSE );

	scroller = AddChild( body, NewObject( monitor->Classes[CLASS_SCROLLER], NULL,
		SCROLLER_Orientation, SORIENT_VERT, SCROLLER_Arrows, TRUE, TAG_END ), NULL, FALSE, TRUE );

	if ( ( ! monitor->Editor ) || ( ! scroller ) )
	{
		goto failed;
	}

	SetAttrs( monitor->Editor, GA_TEXTEDITOR_VertScroller, scroller, TAG_END );

	buttons = AddChild( layout, Layout( monitor, FALSE ), NULL, TRUE, FALSE );

	if ( ! buttons )
	{
		goto failed;
	}

	SetAttrs( buttons, LAYOUT_EvenSize, TRUE, TAG_END );

	monitor->CaptureButton = Button( monitor, buttons, GADGET_CAPTURE, "Start" );

	if ( ( ! monitor->CaptureButton ) ||
		( ! Button( monitor, buttons, GADGET_SETUP, "Setup..." ) ) ||
		( ! Button( monitor, buttons, GADGET_SAVE, "Save Capture..." ) ) ||
		( ! Button( monitor, buttons, GADGET_CLEAR, "Clear" ) ) )
	{
		goto failed;
	}

	monitor->Status = AddChild( layout, NewObject( monitor->Classes[CLASS_BUTTON], NULL,
		GA_Text, "Stopped", GA_ReadOnly, TRUE, BUTTON_BevelStyle, BVS_NONE,
		BUTTON_Transparent, TRUE, TAG_END ), NULL, TRUE, FALSE );

	if ( ! monitor->Status )
	{
		goto failed;
	}

	monitor->Main = WindowObject( monitor, layout, "USB2 Serial Monitor", TRUE );

	if ( monitor->Main )
	{
		monitor->Window = (PTR) IDoMethod( monitor->Main, WM_OPEN, NULL );
	}

	return ( monitor->Window != NULL );

failed:
	DisposeObject( layout );

	return ( FALSE );
}

// --

static Object *
Field( struct Monitor *monitor, Object *layout, Object *gadget, const char *text )
{
	Object *label;

	label = NewObject( monitor->Classes[CLASS_LABEL], NULL, LABEL_Text, text, TAG_END );

	if ( ! label )
	{
		DisposeObject( gadget );

		return ( NULL );
	}

	return ( AddChild( layout, gadget, label, TRUE, FALSE ) );
}

// --

static Object *
NumberField( struct Monitor *monitor, Object *layout, const char *text, U32 value, U32 min, U32 max )
{
	return ( Field( monitor, layout, NewObject( monitor->Classes[CLASS_INTEGER], NULL,
		GA_TabCycle, TRUE, INTEGER_Number, value, INTEGER_Minimum, min, INTEGER_Maximum, max,
		TAG_END ), text ) );
}

// --

static BOOL
SetupFields( struct Monitor *monitor, Object *layout )
{
	static STRPTR parity[] = { "None", "Odd", "Even", "Mark", "Space", NULL };

	monitor->Fields[0] = NumberField( monitor, layout, "Unit", monitor->Settings.Unit, 0, USB2_MAX_DEFAULT_UNIT );

	monitor->Fields[1] = NumberField( monitor, layout, "Baud rate", monitor->Settings.Baud, 1, 6000000 );

	monitor->Fields[2] = NumberField( monitor, layout, "Data bits", monitor->Settings.DataBits, 5, 8 );

	monitor->Fields[3] = NumberField( monitor, layout, "Stop bits", monitor->Settings.StopBits, 1, 2 );

	monitor->Fields[4] = Field( monitor, layout, NewObject( monitor->Classes[CLASS_CHOOSER], NULL,
		CHOOSER_PopUp, TRUE, CHOOSER_LabelArray, parity, CHOOSER_Selected, monitor->Settings.Parity,
		GA_RelVerify, TRUE, GA_TabCycle, TRUE, TAG_END ), "Parity" );

	return ( ( monitor->Fields[0] ) && ( monitor->Fields[1] ) && ( monitor->Fields[2] ) &&
		( monitor->Fields[3] ) && ( monitor->Fields[4] ) );
}

// --

void
Monitor_CloseDialog( struct Monitor *monitor )
{
	if ( monitor->Dialog )
	{
		DisposeObject( monitor->Dialog );

		monitor->Dialog = NULL;
	}
}

// --

static void
OpenDialog( struct Monitor *monitor, enum MonitorDialog kind )
{
	Object *layout;
	Object *buttons;

	BOOL setup;

	Monitor_CloseDialog( monitor );

	setup = ( kind == DIALOG_SETUP );
	layout = Layout( monitor, TRUE );

	if ( ! layout )
	{
		goto failed;
	}

	if ( setup )
	{
		if ( ! SetupFields( monitor, layout ) )
		{
			goto failed;
		}
	}
	else
	{
		monitor->Fields[0] = Field( monitor, layout, NewObject( monitor->Classes[CLASS_STRING], NULL,
			GA_TabCycle, TRUE, STRINGA_TextVal, monitor->SavePath,
			STRINGA_MaxChars, sizeof( monitor->SavePath ) - 1, TAG_END ), "File" );

		if ( ! monitor->Fields[0] )
		{
			goto failed;
		}
	}

	buttons = AddChild( layout, Layout( monitor, FALSE ), NULL, TRUE, FALSE );

	if ( ( ! buttons ) || ( ! Button( monitor, buttons, GADGET_ACCEPT, setup ? "Apply" : "Save" ) ) ||
		( ! Button( monitor, buttons, GADGET_CANCEL, "Cancel" ) ) )
	{
		goto failed;
	}

	monitor->Dialog = WindowObject( monitor, layout, setup ? "Serial Setup" : "Save Capture", FALSE );
	layout = NULL;

	if ( ( ! monitor->Dialog ) || ( ! IDoMethod( monitor->Dialog, WM_OPEN, NULL ) ) )
	{
		goto failed;
	}

	monitor->DialogKind = kind;

	return;

failed:
	DisposeObject( layout );

	Monitor_CloseDialog( monitor );

	monitor->Notice = "Could not open the window.";
	monitor->Failure = NULL;
	monitor->Dirty = TRUE;
}

// --

static void
ApplySetup( struct Monitor *monitor )
{
	struct LineSettings next;

	ULONG data;
	S32 error;
	const char *failure;

	GetAttr( INTEGER_Number, monitor->Fields[0], & data );

	next.Unit = data;

	GetAttr( INTEGER_Number, monitor->Fields[1], & data );

	next.Baud = data;

	GetAttr( INTEGER_Number, monitor->Fields[2], & data );

	next.DataBits = data;

	GetAttr( INTEGER_Number, monitor->Fields[3], & data );

	next.StopBits = data;

	GetAttr( CHOOSER_Selected, monitor->Fields[4], & data );

	next.Parity = data;

	if ( ( monitor->Capturing ) && ( memcmp( & next, & monitor->Settings, sizeof( next ) ) ) )
	{
		Capture_Stop( monitor );

		if ( monitor->Length < CAPTURE_LIMIT )
		{
			error = Capture_Start( monitor, & next );

			if ( error )
			{
				failure = monitor->Failure;

				if ( ! Capture_Start( monitor, & monitor->Settings ) )
				{
					monitor->Failure = failure;
					monitor->Error = error;
				}

				return;
			}
		}
	}

	monitor->Settings = next;
	monitor->Failure = NULL;
	monitor->Notice = NULL;
	monitor->Dirty = TRUE;
}

// --

static BOOL
SaveCapture( struct Monitor *monitor )
{
	struct EasyStruct replace =
	{
		.es_StructSize = sizeof( struct EasyStruct ),
		.es_Title = "Save Capture",
		.es_TextFormat = "Replace the existing capture file?",
		.es_GadgetFormat = "Replace|Cancel"
	};

	ULONG path;
	BPTR lock;

	path = 0;

	GetAttr( STRINGA_TextVal, monitor->Fields[0], & path );

	if ( ( ! path ) || ( ! *( (const char *) path ) ) )
	{
		monitor->Notice = "Enter a file path.";
		monitor->Failure = NULL;
		monitor->Dirty = TRUE;

		return ( FALSE );
	}
	else
	{
		snprintf( monitor->SavePath, sizeof( monitor->SavePath ), "%s", (const char *) path );

		lock = Lock( monitor->SavePath, ACCESS_READ );

		if ( lock )
		{
			UnLock( lock );

			if ( ! EasyRequestArgs( monitor->Window, & replace, NULL, NULL ) )
			{
				return ( FALSE );
			}
		}

		if ( Monitor_Save( monitor, monitor->SavePath ) )
		{
			monitor->Notice = "Capture saved.";
		}
		else
		{
			monitor->Notice = "Could not save the capture.";
		}
	}

	monitor->Failure = NULL;
	monitor->Dirty = TRUE;

	return ( TRUE );
}

// --

static BOOL
CloseEvent( ULONG event, U16 code )
{
	return ( ( ( event & WMHI_CLASSMASK ) == WMHI_CLOSEWINDOW ) ||
		( ( ( event & WMHI_CLASSMASK ) == WMHI_RAWKEY ) && ( code == 0x45 ) ) );
}

// --

void
Monitor_HandleWindows( struct Monitor *monitor )
{
	ULONG event;
	U16 code;

	while( ( event = IDoMethod( monitor->Main, WM_HANDLEINPUT, & code ) ) != WMHI_LASTMSG )
	{
		if ( CloseEvent( event, code ) )
		{
			monitor->Run = FALSE;

			return;
		}

		if ( ( event & WMHI_CLASSMASK ) != WMHI_GADGETUP )
		{
			continue;
		}

		switch( event & WMHI_GADGETMASK )
		{
			case GADGET_CAPTURE:
			{
				if ( monitor->Capturing )
				{
					Capture_Stop( monitor );
				}
				else
				{
					Capture_Start( monitor, & monitor->Settings );
				}
				break;
			}

			case GADGET_SETUP:
			{
				OpenDialog( monitor, DIALOG_SETUP );
				break;
			}

			case GADGET_SAVE:
			{
				if ( monitor->Length )
				{
					OpenDialog( monitor, DIALOG_SAVE );
				}
				break;
			}

			case GADGET_CLEAR:
			{
				Monitor_CloseDialog( monitor );

				monitor->Length = 0;
				monitor->Overruns = 0;
				monitor->Failure = NULL;
				monitor->Notice = NULL;
				monitor->Dirty = TRUE;
				break;
			}
		}
	}

	while( ( monitor->Dialog ) && ( ( event = IDoMethod( monitor->Dialog, WM_HANDLEINPUT, & code ) ) != WMHI_LASTMSG ) )
	{
		if ( ( CloseEvent( event, code ) ) ||
			( ( event & ( WMHI_CLASSMASK | WMHI_GADGETMASK ) ) == ( WMHI_GADGETUP | GADGET_CANCEL ) ) )
		{
			Monitor_CloseDialog( monitor );
		}
		else if ( ( event & ( WMHI_CLASSMASK | WMHI_GADGETMASK ) ) == ( WMHI_GADGETUP | GADGET_ACCEPT ) )
		{
			if ( monitor->DialogKind == DIALOG_SETUP )
			{
				ApplySetup( monitor );

				Monitor_CloseDialog( monitor );
			}
			else if ( SaveCapture( monitor ) )
			{
				Monitor_CloseDialog( monitor );
			}
		}
	}
}

// --

U32
Monitor_Signals( struct Monitor *monitor )
{
	ULONG mask;
	ULONG dialog;

	mask = 0;
	dialog = 0;

	GetAttr( WINDOW_SigMask, monitor->Main, & mask );

	if ( monitor->Dialog )
	{
		GetAttr( WINDOW_SigMask, monitor->Dialog, & dialog );
	}

	return ( mask | dialog );
}

// --
