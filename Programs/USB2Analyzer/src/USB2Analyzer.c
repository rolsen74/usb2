
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

const char		myVersion[]		= VERSTAG;
struct Task *	Main_Task		= NULL;
S32				Restart			= FALSE;
S32				Running			= FALSE;

// --

static S32	Main_Alloc( S32 gui );
static S32	Main_Start( void );
static void Main_Stop( void );
static void Main_Free( void );
static void Main_Loop( void );

// --

int main( int argc, char **argv )
{
	const char *path;
	S32 retval;

	path = NULL;
	retval = RETURN_FAIL;

	if ( argc > 1 )
	{
		if ( ( argc != 3 ) || ( strcasecmp( argv[1], "SAVETREE" ) ) || ( ! argv[2][0] ) )
		{
			fprintf( stderr, "Usage: USB2Analyzer [SAVETREE <file>]\n" );

			return ( RETURN_FAIL );
		}

		path = argv[2];
	}

	if ( Main_Alloc( ! path ) )
	{
		if ( path )
		{
			if ( Save_Device_Tree( path ) )
			{
				retval = RETURN_OK;
			}
			else
			{
				fprintf( stderr, "USB2Analyzer: Could not write '%s'.\n", path );
			}
		}
		else
		{
			do
			{
				Restart = FALSE;

				if ( Main_Start() )
				{
					Main_Loop();

					retval = RETURN_OK;
				}

				Main_Stop();
			}
			while( Restart );
		}
	}

	Main_Free();

	return ( retval );
}

// --

static S32 Main_Alloc( S32 gui )
{
S32 retval;

	retval = FALSE;

	// Open Libraries

	Main_Task = FindTask( NULL );

	NewList( & DeviceTreeUSBList );
	NewList( & DeviceTreeGUIList );
	NewList( & PropertiesGUIList );
	NewList( & IORequestGUIList );
	NewList( & CaptureGUIList );

	if ( ( gui ) && ( ! myOpenResources( 16384 ) ) )
	{
		printf( "myOpenResources() failed\n" );
		goto bailout;
	}

	if ( ! USB_Setup( gui ) )
	{
		printf( "USB_Setup() failed\n" );
		goto bailout;
	}

//	if ( Main_OpenTimerDevice() == FALSE )
//	{
//		goto bailout;
//	}

//	if ( Main_CreateMsgPorts() == FALSE )
//	{
//		goto bailout;
//	}

	if ( gui )
	{
		Prefs_Load();
	}

//	if ( Init_Databases() == FALSE )
//	{
//		goto bailout;
//	}

//	if ( Main_Init1() == FALSE )
//	{
//		goto bailout;
//	}

	if ( ( gui ) && ( ! Main_BuildWindow() ) )
	{
		printf( "Main_BuildWindow() failed\n" );
		goto bailout;
	}

	retval = TRUE;

bailout:

	return(	retval );
}

// --

static void Main_Free( void )
{
	PTR node;

//	Main_Free1();

	if ( IListBrowser )
	{
		Main_DestoryWindow();
	}

//--	Free_Databases();

//--	Main_CloseTimer();

//	if ( PHandle )
//	{
//		IUSB2Config->USBEnv_Delete( PHandle );
//		PHandle = NULL;
//	}

//	Main_DeleteMsgPorts();

//	Main_CloseTimerDevice();

	USB_Cleanup();

	while( ( node = RemHead( & DeviceTreeUSBList ) ) )
	{
		FktNode_Delete( node );
	}

	myCloseResources();
}

// --

static S32 Main_Start( void )
{
S32 retval;

	retval = FALSE;

	if ( ! Main_OpenScreen() )
	{
		printf( "Main_OpenScreen() failed\n" );
		goto bailout;
	}

	if ( ! Main_OpenWindow() )
	{
		printf( "Main_OpenWindow() failed\n" );
		goto bailout;
	}

//	Main_StartTimer();

	retval = TRUE;

bailout:

	return(	retval );
}

// --

static void Main_Stop( void )
{
//	Main_RemNotify();

	Prefs_CloseWindow();

	Main_CloseWindow();

	Main_CloseScreen();
}

// --

static void Main_Loop( void )
{
U32 mask;

	Running = TRUE;

	while( Running )
	{
		mask = Wait(
//			CmdMsgPortBit |
			Main_WindowBits |
//			DBBrowserWindowBits |
//			DBInfoWindowBits |
//			MemWindowBits |
			Prefs_WindowBits |
			WindowTimerMask |
//			USBNotifyMsgPortBit |
//			USBWatchMsgPortBit |
			USBNotifyBit |
			SIGBREAKF_CTRL_C
		);

		if ( mask & Main_WindowBits )
		{
			Main_HandleWindow();
		}

//		if ( mask & DBBrowserWindowBits )
//		{
//			DBBrowser_HandleWindow();
//		}
//
//		if ( mask & DBInfoWindowBits )
//		{
//			DBInfo_HandleWindow();
//		}

//		if ( mask & MemWindowBits )
//		{
//			Mem_HandleWindow();
//		}

		if ( ( Running ) && ( mask & Prefs_WindowBits ) )
		{
			Prefs_HandleWindow();
		}

		if ( ( Running ) && ( mask & WindowTimerMask ) )
		{
			Handle_Window_Timer();
		}

//		if ( mask & CmdMsgPortBit )
//		{
//			Handle_Commands();
//		}

//		if ( mask & USBNotifyMsgPortBit )
//		{
//			Handle_Notify();
//		}

//		if ( mask & USBWatchMsgPortBit )
//		{
//			Handle_USBWatch();
//		}

		if ( mask & USBNotifyBit )
		{
			Handle_Notify();
		}

		if ( mask & SIGBREAKF_CTRL_C )
		{
			Running = FALSE;
		}
	}
}

// --
