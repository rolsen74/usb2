
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

#define SCREEN_PREFS_VAR "USB2Analyzer.Screen"

enum
{
	PREFS_TYPE = 1,
	PREFS_MODE,
	PREFS_SAVE,
	PREFS_USE,
	PREFS_CANCEL
};

struct ScreenPrefs Prefs_Screen = { SCRTYPE_Window, INVALID_ID, 0, 0 };
U32 Prefs_WindowBits;

static struct ScreenPrefs Draft;
static struct Window *Prefs_Window;
static Object *Prefs_Object;
static Object *Type_Chooser;
static Object *Mode_Button;
static char Mode_Label[48];

static const char *Screen_Types[] =
{
	[SCRTYPE_Window] = "Workbench window",
	[SCRTYPE_Screen] = "Custom screen",
	[SCRTYPE_WBClone] = "Workbench clone",
	NULL
};

// --

static void
Prefs_Error( struct Window *window, const char *message )
{
	struct EasyStruct error;

	error.es_StructSize = sizeof( error );
	error.es_Flags = 0;
	error.es_Title = "USB2Analyzer Preferences";
	error.es_TextFormat = message;
	error.es_GadgetFormat = "OK";

	EasyRequestArgs( window, & error, NULL, NULL );
}

// --

void
Prefs_Load( void )
{
	char text[96];
	char extra;
	unsigned type;
	unsigned mode;
	unsigned width;
	unsigned height;
	S32 length;

	length = GetVar( SCREEN_PREFS_VAR, text, sizeof( text ) - 1, GVF_GLOBAL_ONLY );

	if ( ( length <= 0 ) || ( (U32) length >= sizeof( text ) - 1 ) )
	{
		return;
	}

	text[length] = '\0';

	if ( ( sscanf( text, "%u %x %u %u %c", & type, & mode, & width, & height, & extra ) == 4 ) &&
		( type <= SCRTYPE_WBClone ) && ( width <= 32767 ) && ( height <= 32767 ) )
	{
		Prefs_Screen.Type = type;
		Prefs_Screen.Mode = mode;
		Prefs_Screen.Width = width;
		Prefs_Screen.Height = height;
	}
}

// --

static void
Format_Mode( void )
{
	if ( ( Draft.Mode == INVALID_ID ) || ( ! Draft.Width ) || ( ! Draft.Height ) )
	{
		strcpy( Mode_Label, "Select mode..." );
	}
	else
	{
		snprintf( Mode_Label, sizeof( Mode_Label ), "%u x %u (0x%08X)",
			(unsigned) Draft.Width, (unsigned) Draft.Height, (unsigned) Draft.Mode );
	}
}

// --

static void
Refresh_Mode( void )
{
	Format_Mode();

	RefreshSetGadgetAttrs( (struct Gadget *) Mode_Button, Prefs_Window, NULL,
		GA_Text, Mode_Label,
		GA_Disabled, Draft.Type != SCRTYPE_Screen,
		TAG_END
	);
}

// --

void
Prefs_CloseWindow( void )
{
	if ( Prefs_Window )
	{
		IDoMethod( Prefs_Object, WM_CLOSE );
	}

	if ( Prefs_Object )
	{
		DisposeObject( Prefs_Object );
	}

	Prefs_Window = NULL;
	Prefs_Object = NULL;
	Type_Chooser = NULL;
	Mode_Button = NULL;
	Prefs_WindowBits = 0;
}

// --

void
Prefs_OpenWindow( struct Window *parent )
{
	if ( Prefs_Window )
	{
		WindowToFront( Prefs_Window );

		ActivateWindow( Prefs_Window );

		return;
	}

	Draft = Prefs_Screen;

	Format_Mode();

	Prefs_Object = NewObject( WindowClass, NULL,
		WA_Title, "Preferences",
		WA_Activate, TRUE,
		WA_CloseGadget, TRUE,
		WA_DepthGadget, TRUE,
		WA_DragBar, TRUE,
		WA_CustomScreen, Main_ScreenAdr,
		WA_IDCMP, IDCMP_RAWKEY,
		WA_Width, 380,
		WINDOW_Position, WPOS_CENTERSCREEN,
		WINDOW_ParentGroup, NewObject( LayoutClass, NULL,
			LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
			LAYOUT_SpaceOuter, TRUE,
			LAYOUT_AddChild, Type_Chooser = NewObject( ChooserClass, NULL,
				GA_ID, PREFS_TYPE,
				GA_RelVerify, TRUE,
				CHOOSER_LabelArray, Screen_Types,
				CHOOSER_Selected, Draft.Type,
				TAG_END ),
			CHILD_Label, NewObject( LabelClass, NULL, LABEL_Text, "Screen type", TAG_END ),
			CHILD_WeightedHeight, 0,
			LAYOUT_AddChild, Mode_Button = NewObject( ButtonClass, NULL,
				GA_ID, PREFS_MODE,
				GA_RelVerify, TRUE,
				GA_Text, Mode_Label,
				GA_Disabled, Draft.Type != SCRTYPE_Screen,
				TAG_END ),
			CHILD_Label, NewObject( LabelClass, NULL, LABEL_Text, "Screen mode", TAG_END ),
			CHILD_WeightedHeight, 0,
			LAYOUT_AddChild, NewObject( LayoutClass, NULL,
				LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
				LAYOUT_EvenSize, TRUE,
				LAYOUT_AddChild, NewObject( ButtonClass, NULL,
					GA_ID, PREFS_SAVE, GA_RelVerify, TRUE, GA_Text, "Save", TAG_END ),
				LAYOUT_AddChild, NewObject( ButtonClass, NULL,
					GA_ID, PREFS_USE, GA_RelVerify, TRUE, GA_Text, "Use", TAG_END ),
				LAYOUT_AddChild, NewObject( ButtonClass, NULL,
					GA_ID, PREFS_CANCEL, GA_RelVerify, TRUE, GA_Text, "Cancel", TAG_END ),
				TAG_END ),
			CHILD_WeightedHeight, 0,
			TAG_END ),
		TAG_END
	);

	if ( ( Prefs_Object ) && ( Type_Chooser ) && ( Mode_Button ) )
	{
		Prefs_Window = (PTR) IDoMethod( Prefs_Object, WM_OPEN );
	}

	if ( ! Prefs_Window )
	{
		Prefs_CloseWindow();

		Prefs_Error( parent, "Could not open the Preferences window." );

		return;
	}

	GetAttrs( Prefs_Object, WINDOW_SigMask, & Prefs_WindowBits, TAG_END );
}

// --

static void
Apply_Prefs( S32 save )
{
	char text[96];
	S32 length;
	S32 reopen;

	if ( ( Draft.Type == SCRTYPE_Screen ) && ( ModeNotAvailable( Draft.Mode ) ) )
	{
		if ( ! Request_Screen_Mode( Prefs_Window, & Draft ) )
		{
			return;
		}

		Refresh_Mode();
	}

	length = snprintf( text, sizeof( text ), "%u %08X %u %u\n",
		(unsigned) Draft.Type, (unsigned) Draft.Mode, (unsigned) Draft.Width, (unsigned) Draft.Height );

	if ( ! SetVar( SCREEN_PREFS_VAR, text, length, GVF_GLOBAL_ONLY | ( save ? GVF_SAVE_VAR : 0 ) ) )
	{
		Prefs_Error( Prefs_Window, "Could not store the screen preferences." );

		return;
	}

	reopen = ( Draft.Type != Main_ScreenType ) ||
		( ( Draft.Type == SCRTYPE_Screen ) && ( ( Draft.Mode != Prefs_Screen.Mode ) ||
			( Draft.Width != Prefs_Screen.Width ) || ( Draft.Height != Prefs_Screen.Height ) ) );
	Prefs_Screen = Draft;

	Prefs_CloseWindow();

	if ( reopen )
	{
		Restart = TRUE;
		Running = FALSE;
	}
}

// --

void
Prefs_HandleWindow( void )
{
	U32 result;
	U32 type;
	U16 code;

	while( ( Prefs_Object ) && ( ( result = IDoMethod( Prefs_Object, WM_HANDLEINPUT, & code ) ) != WMHI_LASTMSG ) )
	{
		switch( result & WMHI_CLASSMASK )
		{
			case WMHI_CLOSEWINDOW:
			{
				Prefs_CloseWindow();

				return;
			}

			case WMHI_RAWKEY:
			{
				if ( code == 0x45 )
				{
					Prefs_CloseWindow();

					return;
				}
				break;
			}

			case WMHI_GADGETUP:
			{
				switch( result & WMHI_GADGETMASK )
				{
					case PREFS_TYPE:
					{
						type = Draft.Type;

						GetAttrs( Type_Chooser, CHOOSER_Selected, & type, TAG_END );

						Draft.Type = type;

						Refresh_Mode();

						break;
					}

					case PREFS_MODE:
					{
						if ( Request_Screen_Mode( Prefs_Window, & Draft ) )
						{
							Refresh_Mode();
						}
						break;
					}

					case PREFS_SAVE:
					case PREFS_USE:
					{
						Apply_Prefs( ( result & WMHI_GADGETMASK ) == PREFS_SAVE );

						break;
					}

					case PREFS_CANCEL:
					{
						Prefs_CloseWindow();

						return;
					}
				}
				break;
			}
		}
	}
}

// --
