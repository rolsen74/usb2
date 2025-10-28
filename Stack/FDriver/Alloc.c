
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE struct USB2_FktDriverNode *__FDriver_Alloc( struct USBBase *usbbase, STR file UNUSED )

#else

SEC_CODE struct USB2_FktDriverNode *__FDriver_Alloc( struct USBBase *usbbase )

#endif

{
struct USB2_FktDriverNode *fdn;
S32 error;

	error = TRUE;

	USBDEBUG( "__FDriver_Alloc          : (%s)", (file)?file:"<NULL>" );

	// --

	fdn = MEMORY_ALLOC( MEMID_USBFktDriver, TRUE );

	if ( ! fdn )
	{
		USBERROR( "__FDriver_Alloc : Error allocating memory" );
		goto bailout;
	}

	// Set Default
	fdn->fdn_StructID	= ID_USB2_FDN;
	fdn->fdn_Type		= USB2Val_FDriver_Interface;
	fdn->fdn_Class		= FDCLASS_Any;
	fdn->fdn_SubClass	= FDSUBCLASS_Any;
	fdn->fdn_Protocol	= FDPROTOCOL_Any;

	// --

	error = FALSE;

bailout:

	if (( fdn ) && ( error ))
	{
		FDRIVER_FREE( fdn );
		fdn = NULL;
	}

	USBDEBUG( "__FDriver_Alloc          : FDN %p : (new)", fdn );
	
	return( fdn );
}

// --
