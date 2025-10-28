
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

#define CFGBUF_SIZE	1024*4

static S32 __myProcess_1_Get_Config( struct PrinterUnit *unit )
{
struct RDArgs *rda2;
struct RDArgs *rda;
STR filename;
STR filebuf;
S32 retval;
S32 stat;
S32 len;

	USBERROR( "Printer : __myProcess_1_Get_Config" );

	rda		= NULL;
	rda2	= NULL;
	filebuf	= NULL;
	retval	= FALSE;

	unit->unit_USB_VendorID = -1;
	unit->unit_USB_DeviceID = -1;
	unit->unit_Buffer_Size = 16;	// Default Buffer 16kb
//	unit->unit_Buffer_Size = 0;		// Testing Buffering off

	// --

	filename = ASPrintf( "%s/unit%ld", DEVNAME, unit->unit_UnitNr );

	if ( ! filename )
	{
		USBERROR( "Printer : Error allocating memory" );
		goto bailout;
	}

	filebuf = AllocVecTags( CFGBUF_SIZE, TAG_END );

	if ( ! filebuf )
	{
		USBERROR( "Printer : Error allocating memory" );
		goto bailout;
	}

	stat = GetVar( filename, filebuf, CFGBUF_SIZE - 2, 0 );

	if ( stat != -1 )
	{
		USBERROR( "Printer : Error GetVar failed" );
		goto bailout;
	}

	if ( stat <= 0 )
	{
		// Not an error
		retval = TRUE;
		goto bailout;
	}

	len = strlen( filebuf );
	filebuf[len++] = '\n';
	filebuf[len++] = 0;

	rda = AllocDosObjectTagList( DOS_RDARGS, NULL );

	if ( ! rda )
	{
		USBERROR( "Printer : Error allocating read args" );
		goto bailout;
	}


	memset( & unit->unit_Options, 0, sizeof( struct ConfigOptions ));

	rda->RDA_Source.CS_Buffer = filebuf;
	rda->RDA_Source.CS_Length = len;
	rda->RDA_Source.CS_CurChr = 0;
	rda->RDA_Flags |= RDAF_NOPROMPT;

	rda2 = ReadArgs( CONFIGTEMPLATE, (PTR) & unit->unit_Options, rda );

	if ( ! rda2 )
	{
		USBERROR( "Printer : Error parsing config file" );

		if ( ! unit->unit_CfgError )
		{
			// Only print one pr Opener
			unit->unit_CfgError = TRUE;	

			TimedDosRequesterTags( 
				TDR_TitleString, DEVNAME,
				TDR_GadgetString, "_Okay",
				TDR_ImageType, TDRIMAGE_ERROR,
				TDR_FormatString, "Error parsing config file",
				TAG_END
			);
		}
		goto bailout;
	}

	if ( unit->unit_Options.VendorID )
	{
		unit->unit_USB_VendorID = *unit->unit_Options.VendorID;
	}

	if ( unit->unit_Options.DeviceID )
	{
		unit->unit_USB_DeviceID = *unit->unit_Options.DeviceID;
	}

	if ( unit->unit_Options.BufferSize )
	{
		unit->unit_Buffer_Size = *unit->unit_Options.BufferSize;
	}

	retval = TRUE;

bailout:

	if ( rda2 )
	{
		FreeArgs( rda2 );
		rda2 = NULL;
	}

	if ( rda )
	{
		FreeDosObject( DOS_RDARGS, rda );
		rda = NULL;
	}

	if ( filebuf )
	{
		FreeVec( filebuf );
		filebuf = NULL;
	}

	if ( filename )
	{
		FreeVec( filename );
		filename = NULL;
	}

	if ( unit->unit_Buffer_Size > 1024 )
	{
		USBINFO( "Printer : Limiting Buffser size to 1024" );
		unit->unit_Buffer_Size = 1024;
	}

	USBERROR( "Printer : Vendor ID ....... : $%04lx", unit->unit_USB_VendorID );
	USBERROR( "Printer : Device ID ....... : $%04lx", unit->unit_USB_DeviceID );
	USBERROR( "Printer : Buffer Size ..... : %lu (%lu bytes)", unit->unit_Buffer_Size, unit->unit_Buffer_Size*1024 );

	// Convert into real bytes .. for speed
	unit->unit_Buffer_Size *= 1024;

	return( retval );
}

// --
