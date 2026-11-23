
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

/* -- Main Interface -- */

static struct USB2_Function * VARARGS68K
_main_Fkt_FindTags ( struct USB2IFace * Self, ... )
{
	va_list ap;
	PTR		fn;

	//	USBERROR( "USB2 Stack : _main_FindFunctionTags" );

	va_start ( ap, Self );

	fn = Self->USB2_Fkt_FindList ( va_getlinearva ( ap, struct TagItem * ) );

	va_end ( ap );

	return ( fn );
}

// --

static struct USB2_Function *
_main_Fkt_FindList ( struct USB2IFace * Self, struct TagItem * taglist )
{
	struct RealFunctionNode * fn;
	struct USB2_Node *		  node;
	struct USBBase *		  usbbase;
	struct USB2_Function *	  copy;
	const USB2_ID *			  function_id;
	struct TagItem *		  tag;
	S32						  DoSubClass;
	S32						  SeeClaimed;
	S32						  DoClaimed;
	S32						  DoDevice;
	S32						  DoVendor;
	S32						  SubClass;
	S32						  DoIndex;
	S32						  DoClass;
	U16						  Device;
	U16						  Vendor;
	PTR						  retval;
	U32						  Index;
	S32						  Class;
	U32						  cnt;

	usbbase = (PTR)Self->Data.LibBase;

	retval		= NULL;
	copy		= NULL;
	function_id = NULL;

	// -- Parse Tags

	DoVendor   = FALSE;
	DoDevice   = FALSE;
	DoClass	   = FALSE;
	DoSubClass = FALSE;
	DoClaimed  = FALSE;
	DoIndex	   = FALSE;

	// --

	Vendor	   = 0;
	Device	   = 0;
	Class	   = 0;
	SubClass   = 0;
	SeeClaimed = FALSE;
	Index	   = 0;

	// --

	while ( ( tag = MISC_NEXTTAGITEM ( &taglist ) ) )
	{
		switch ( tag->ti_Tag )
		{
			case USB2Tag_Find_VendorID:
			{
				USBINFO ( "Find VendorID  : $%04lx", tag->ti_Data );
				Vendor	 = tag->ti_Data;
				DoVendor = TRUE;
				break;
			}

			case USB2Tag_Find_DeviceID:
			{
				USBINFO ( "Find DeviceID : $%04lx", tag->ti_Data );
				Device	 = tag->ti_Data;
				DoDevice = TRUE;
				break;
			}

			case USB2Tag_Find_Class:
			{
				USBINFO ( "Find Class     : %ld", tag->ti_Data );
				Class	= tag->ti_Data;
				DoClass = TRUE;
				break;
			}

			case USB2Tag_Find_SubClass:
			{
				USBINFO ( "Find SubClass  : %ld", tag->ti_Data );
				SubClass   = tag->ti_Data;
				DoSubClass = TRUE;
				break;
			}

			case USB2Tag_Find_SeeClaimed:
			{
				USBINFO ( "Find SeeClaimed: %s", ( tag->ti_Data ) ? "Yes" : "No" );
				SeeClaimed = ( tag->ti_Data ) ? TRUE : FALSE;
				DoClaimed  = TRUE;
				break;
			}

			case USB2Tag_Find_Index:
			{
				USBINFO ( "Find Index     : %ld", tag->ti_Data );
				Index	= tag->ti_Data;
				DoIndex = TRUE;
				break;
			}

			case USB2Tag_Find_Copy:
			{
				copy = (PTR)tag->ti_Data;
				break;
			}

			case USB2Tag_Find_FunctionID:
			{
				function_id = (PTR)tag->ti_Data;
				break;
			}

			default:
			{
				USBINFO ( "Unknown Tag    : $%04lx:%04lx", tag->ti_Tag, tag->ti_Data );
				break;
			}
		}
	}

	// -- Scan

	SEMAPHORE_OBTAIN ( &usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	cnt = 0;

	for ( node = usbbase->usb_Fkt_Header.uh_Tail; node; node = node->un_Prev )
	{
		fn = node->un_Data;

		if ( ( ! fn ) || ( ! fn->fkt_Initialized ) || ( fn->fkt_Detach ) || ( fn->fkt_FreeMe )
			 || ( ! fn->fkt_DeviceDescriptor ) )
		{
			continue;
		}

		if ( ( ( function_id ) && ( *function_id != fn->fkt_NotifyID ) )
			 || ( ( DoVendor ) && ( LE_SWAP16 ( fn->fkt_DeviceDescriptor->VendorID ) != Vendor ) )
			 || ( ( DoDevice ) && ( LE_SWAP16 ( fn->fkt_DeviceDescriptor->DeviceID ) != Device ) )
			 || ( ( DoClass ) && ( fn->fkt_Class != Class ) ) || ( ( DoSubClass ) && ( fn->fkt_SubClass != SubClass ) )
			 || ( ( DoClaimed ) && ( ! SeeClaimed ) && ( fn->fkt_Owner ) ) )
		{
			continue;
		}

		if ( ( ! DoIndex ) || ( cnt == Index ) )
		{
			if ( copy )
			{
				*copy = fn->fkt_Public;
			}

			retval = ( copy ) ? copy : &fn->fkt_Public;
			break;
		}

		cnt++;
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE ( &usbbase->usb_Fkt_Semaphore );

	return ( retval );
}

// --
