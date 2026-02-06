
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

extern PTR INewlib;
extern PTR NewlibBase;

S32 Resources_Setup( struct libBase *libBase )
{
S32 retval;

	retval = FALSE;

	// --

	NewlibBase = OpenLibrary( "newlib.library", 52 );
	INewlib = GetInterface( NewlibBase, "main", 1, NULL );

	if ( ! INewlib )
	{
		#ifdef DEBUG
		DebugPrintF( "Error opening Newlib library (v52)\n" );
		#endif
		goto bailout;
	}

	// --


	// struct ExecIFace *IExec;
// int32 err;

// 	IExec = libBase->lib_IExec;

// 	NewlibBase = IExec->OpenLibrary( "newlib.library", 52 );
// 	INewlib = IExec->GetInterface( NewlibBase, "main", 1, NULL );

// 	if ( INewlib == NULL )
// 	{
// 		#ifdef DEBUG
// 		IExec->DebugPrintF( "[AILib] Error opening Newlib v52...\n" );
// 		#endif
// 		return( FALSE );
// 	}

// 	libBase->lib_AIN_Base = IExec->OpenLibrary( "AmigaInput.library", 53 );
// 	libBase->lib_IAIN = (struct AIN_IFace *)IExec->GetInterface( libBase->lib_AIN_Base, "main", 1, NULL );

// 	if ( libBase->lib_IAIN == NULL )
//     {
// 	    #ifdef DEBUG
// 		IExec->DebugPrintF( "[AILib] Error opening AIN library v53...\n" );
// 	    #endif
// 	    return( FALSE );
//     }

// 	libBase->lib_UtilityBase = IExec->OpenLibrary( "utility.library", 53 );
// 	libBase->lib_IUtility = (struct UtilityIFace *)IExec->GetInterface( libBase->lib_AIN_Base, "main", 1, NULL );

// 	if ( libBase->lib_IUtility == NULL )
//     {
// 	    #ifdef DEBUG
// 		IExec->DebugPrintF( "[AILib] Error opening Utility library v53...\n" );
// 	    #endif
// 	    return( FALSE );
//     }



	// --

	USBResourceBase = OpenLibrary( "usbresource.library", 50 );
	IUSBResource = (PTR) GetInterface( USBResourceBase, "main", 1, NULL );

	if ( ! IUSBResource )
	{
		#ifdef DEBUG
		DebugPrintF( "Error opening USBResource library (v50)\n" );
		#endif
		goto bailout;
	}

	libBase->lib_fdkey = USBResRegisterFD(
		USBA_FD_Name,			LIBNAME,
		USBA_FD_Title,			VSTRING,
		USBA_FD_FunctionDriver,	TRUE,
		USBA_VendorID,			0x0079,
		USBA_DeviceID,			0x181c,
//		USBA_ErrorCode,			&err,
		TAG_END
	);

	retval = TRUE;

bailout:

	return( retval );
}

// --
