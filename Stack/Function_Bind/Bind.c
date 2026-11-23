
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

//#include "Bind__Entry.c"

enum DriverDeviceMatch
{
	DriverMatch_None,
	DriverMatch_Class,
	DriverMatch_ID
};

// --

SEC_CODE static enum DriverDeviceMatch
Match_Device_IDs( U16 vendor, U16 product, U16 filter_vendor, U16 filter_product )
{
	if ( ( ( filter_vendor ) && ( filter_vendor != vendor ) ) ||
		( ( filter_product ) && ( filter_product != product ) ) )
	{
		return ( DriverMatch_None );
	}

	return ( ( ( filter_vendor ) || ( filter_product ) ) ? DriverMatch_ID : DriverMatch_Class );
}

// --

SEC_CODE static enum DriverDeviceMatch
Match_Driver_Device( struct USBBase *usbbase, struct USB2_FktDriverNode *fdn, const struct USB2_Device_Desc *desc )
{
	struct USB2_DeviceNode *entry;
	struct ExecIFace *IExec;
	enum DriverDeviceMatch match;
	enum DriverDeviceMatch best;

	U16 vendor;
	U16 product;

	IExec = usbbase->usb_IExec;
	vendor = LE_SWAP16( desc->VendorID );
	product = LE_SWAP16( desc->DeviceID );
	best = DriverMatch_None;

	entry = (PTR) IExec->GetHead( & fdn->fdn_Devices );

	if ( ! entry )
	{
		return ( Match_Device_IDs( vendor, product, fdn->fdn_VendorID, fdn->fdn_DeviceID ) );
	}

	while( entry )
	{
		match = Match_Device_IDs( vendor, product, entry->VendorID, entry->DeviceID );

		if ( match == DriverMatch_ID )
		{
			return ( match );
		}

		if ( match == DriverMatch_Class )
		{
			best = match;
		}

		entry = (PTR) IExec->GetSucc( & entry->Node );
	}

	return ( best );
}

// --

#include "Bind_Check_for_Device.c"
#include "Bind_Check_for_Function.c"
#include "Bind_Check_for_Interfaces.c"

// --

SEC_CODE S32 __Function_Bind( struct USBBase *usbbase, struct RealFunctionNode *fn, struct USB2_ASync *as )
{
enum FDSTAT stat;
S32 retval;

//	usbbase->usb_IExec->DebugPrintF( "\n##\n## __Function_Bind : FN    %p\n##\n\n", fn );

	// --

	#ifdef DO_PANIC

	if ( TASK_FIND() == usbbase->usb_Master_Task.tn_TaskAdr )
	{
		// if run from master task, we will get a dead lock when starting a driver
		USBPANIC( "__Function_Bind          : Do NOT run from Master Task" );
	}

	#endif

	// --

	stat = FDSTAT_Not_Found;

	if ( FUNCTION_LOCK( fn ) == LSTAT_Okay )
	{
		#ifdef USB2_BIND_DEBUG
		if ( fn->fkt_DeviceDescriptor )
		{
			usbbase->usb_IExec->DebugPrintF( "USB2 bind: %04lX:%04lX address %lu, active configuration %lu, owner %p\n",
				(U32) LE_SWAP16( fn->fkt_DeviceDescriptor->VendorID ), (U32) LE_SWAP16( fn->fkt_DeviceDescriptor->DeviceID ),
				(U32) fn->fkt_Address, fn->fkt_Config_Active ? fn->fkt_Config_Active->cfg_Number : 0, fn->fkt_Owner );
		}
		#endif

		// Fast check, if someone owns this device.. 
		// no need to check for any drivers
		if ( ! fn->fkt_Owner )
		{
			SEMAPHORE_OBTAIN( & usbbase->usb_FDriver_Semaphore );

			do
			{
				if ( stat == FDSTAT_Not_Found )
				{
					stat = _Check_for_Devices( usbbase, fn, as );
				}

				if ( stat == FDSTAT_Error )
				{
					USBDEBUG( "__Function_Bind : Checking for Devices failed" );
					break;
				}

				if ( stat == FDSTAT_Not_Found )
				{
					stat = _Check_for_Functions( usbbase, fn, as );
				}

				if ( stat == FDSTAT_Error )
				{
					USBDEBUG( "__Function_Bind : Checking for Functions failed" );
					break;
				}

				if ( stat == FDSTAT_Not_Found )
				{
					stat = _Check_for_Interfaces( usbbase, fn, as );
				}

				if ( stat == FDSTAT_Error )
				{
					USBDEBUG( "__Function_Bind : Checking for Interfaces failed" );
					break;
				}
			}
			while( FALSE );

			SEMAPHORE_RELEASE( & usbbase->usb_FDriver_Semaphore );
		}
		else
		{
			// Allready claimed
//			USBERROR( "Skipping Function (claimed) : FN    %p : Class %ld", fn, fn->fkt_Class );
		}

		FUNCTION_UNLOCK( fn );
	}

	retval = ( stat != FDSTAT_Error ) ? TRUE : FALSE ;

//	usbbase->usb_IExec->DebugPrintF( "\n##\n## __Function_Bind 222 : Bind State %ld\n##\n\n", (S32) retval );

	return( retval );
}

// --
