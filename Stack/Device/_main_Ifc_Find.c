
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static void
FindFirstInterface( struct USB2_Node *node,
	struct RealFunctionNode **ptr_fn,
	struct USB2_InterfaceGroup **ptr_ig,
	struct USB2_InterfaceHeader **ptr_ih )
{
struct RealFunctionNode *fn;
struct USB2_ConfigNode *cn;
struct USB2_InterfaceGroup *ig;
struct USB2_InterfaceHeader *ih;

	// Keep scanning past devices with no active configuration.
	while( node )
	{
		fn = node->un_Data;
		cn = ( ( fn ) && ( fn->fkt_Initialized ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe ) ) ?
			fn->fkt_Config_Active : NULL;
		ig = cn ? cn->cfg_InterfaceGroups.uh_Head : NULL;

		while( ig )
		{
			ih = ig->ig_Group.uh_Head;

			if ( ih )
			{
				*ptr_fn = fn;
				*ptr_ig = ig;
				*ptr_ih = ih;

				return;
			}

			ig = ig->ig_Node.un_Next;
		}

		node = node->un_Prev;
	}

	*ptr_fn = NULL;
	*ptr_ig = NULL;
	*ptr_ih = NULL;
}

// --

SEC_CODE static void
_myIFC_GetPrev( struct RealFunctionNode **ptr_fn,
	struct USB2_InterfaceGroup **ptr_ig,
	struct USB2_InterfaceHeader **ptr_ih )
{
struct USB2_InterfaceGroup *ig;
struct USB2_InterfaceHeader *ih;

	ih = (*ptr_ih)->ih_Node.un_Next;

	if ( ih )
	{
		*ptr_ih = ih;

		return;
	}

	ig = (*ptr_ig)->ig_Node.un_Next;

	while( ig )
	{
		ih = ig->ig_Group.uh_Head;

		if ( ih )
		{
			*ptr_ig = ig;
			*ptr_ih = ih;

			return;
		}

		ig = ig->ig_Node.un_Next;
	}

	FindFirstInterface( (*ptr_fn)->fkt_GlobalNode.un_Prev, ptr_fn, ptr_ig, ptr_ih );
}

// --

/* -- Main Interface -- */

static struct USB2_Interface * VARARGS68K _main_Ifc_FindTags( struct USB2IFace *Self, ... )
{
va_list ap;
PTR ifc;

//	USBERROR( "USB2 Stack : _main_FindInterfaceTags" );

	va_start( ap, Self );

	ifc = Self->USB2_Ifc_FindList( va_getlinearva( ap, struct TagItem * ));

	va_end( ap );

	return( ifc );
}

// --

static struct USB2_Interface *_main_Ifc_FindList( struct USB2IFace *Self, struct TagItem *taglist )
{
struct USB2_InterfaceHeader *ih;
struct USB2_InterfaceGroup *ig;
struct RealFunctionNode *fn;
struct USBBase *usbbase;
struct TagItem *tag;
S32 SeeClaimed;
S32 DoSubClass;
S32 DoClaimed;
S32 DoVendor;
S32 DoDevice;
S32 SubClass;
S32 DoClass;
S32 DoIndex;
U16 Vendor;
U16 Device;
PTR retval;
U32 Index;
S32 found;
S32 Class;
U32 cnt;

	usbbase = (PTR) Self->Data.LibBase;

	USBERROR( "USB2 Stack : _main_FindInterfaceList" );

	retval = NULL;

	// -- Parse Tags

 	DoVendor	= FALSE;
 	DoDevice	= FALSE;
	DoClass		= FALSE;
	DoSubClass	= FALSE;
 	DoClaimed	= FALSE;
	DoIndex		= FALSE;

	// --

	Vendor		= 0;
 	Device		= 0;
	Class		= 0;
	SubClass	= 0;
	SeeClaimed	= FALSE;
	Index		= 0;

	// --

	while(( tag = MISC_NEXTTAGITEM( & taglist )))
	{
		switch ( tag->ti_Tag )
		{
			case USB2Tag_Find_VendorID:
			{
				USBDEBUG( "Find VendorID  : $%04lx", tag->ti_Data );
				Vendor = tag->ti_Data;
				DoVendor = TRUE;
				break;
			}

			case USB2Tag_Find_DeviceID:
			{
				USBDEBUG( "Find DeviceID : $%04lx", tag->ti_Data );
				Device = tag->ti_Data;
				DoDevice = TRUE;
				break;
			}

			case USB2Tag_Find_Class:
			{
				USBINFO( "Find Class     : %ld", tag->ti_Data );
				Class = tag->ti_Data;
				DoClass = TRUE;
				break;
			}

			case USB2Tag_Find_SubClass:
			{
				USBINFO( "Find SubClass  : %ld", tag->ti_Data );
				SubClass = tag->ti_Data;
				DoSubClass = TRUE;
				break;
			}

			case USB2Tag_Find_SeeClaimed:
			{
				USBINFO( "Find SeeClaimed: %s", ( tag->ti_Data ) ? "Yes" : "No" );
				SeeClaimed = ( tag->ti_Data ) ? TRUE : FALSE ;
				DoClaimed = TRUE;
				break;
			}

			case USB2Tag_Find_Index:
			{
				USBINFO( "Find Index     : %ld", tag->ti_Data );
				Index = tag->ti_Data;
				DoIndex = TRUE;
				break;
			}

			default:
			{
				USBINFO( "Unknown Tag    : $%04lx:%04lx", tag->ti_Tag, tag->ti_Data );
				break;
			}
		}
	}

	// -- Scan

	SEMAPHORE_OBTAIN( & usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	cnt = 0;

	// Functions are added to Head of list
	// so scan from bottom up so Index are more reliable
	FindFirstInterface( usbbase->usb_Fkt_Header.uh_Tail, & fn, & ig, & ih );

	while(( fn ) && ( ig ) && ( ih ))
	{
		USBERROR( "USB2 Stack : _main_FindInterfaceList : FN    %p : IG    %p : IH    %p : Ifc Class %lu", fn, ig, ih, ig->ig_Class );

		found = TRUE;

		// Always Ignore Detached Devices
		if ( fn->fkt_Detach )
		{
//			USBERROR( "fkt Detach" );
			found = FALSE;
		}

		// Check Vendor ID
		if ( DoVendor )
		{
			if ( LE_SWAP16( fn->fkt_DeviceDescriptor->VendorID ) != Vendor )
			{
//				USBERROR( "fkt Vendor : $%04lx", LE_SWAP16( fn->fkt_DeviceDescriptor->VendorID  ));
				found = FALSE;
			}
		}

		// Check Device ID
		if ( DoDevice )
		{
			if ( LE_SWAP16( fn->fkt_DeviceDescriptor->DeviceID ) != Device )
			{
//				USBERROR( "fkt Device : $%04lx", LE_SWAP16( fn->fkt_DeviceDescriptor->DeviceID  ));
				found = FALSE;
			}
		}

		// Check Interface Class
		if ( DoClass )
		{
			if ( ig->ig_Class != Class )
			{ 
//				USBERROR( "ifc Class : %lu", (U32) ig->ig_Class );
				found = FALSE;
			}
		}

		// Check Interface SubClass
		if ( DoSubClass )
		{
			if ( ig->ig_SubClass != SubClass )
			{ 
//				USBERROR( "ifc SubClass : %lu", (U32) ig->ig_SubClass );
				found = FALSE;
			}
		}

		// Check SeeClaimed Devices
		if ( DoClaimed )
		{
			if ( ! SeeClaimed )
			{
				/**/ if ( fn->fkt_Owner )
				{
//					USBERROR( "fkt Claimed" );
					found = FALSE;
				}
				else if ( ih->ih_Owner )
				{
//					USBERROR( "ifc Claimed" );
					found = FALSE;
				}
			}
		}


		// Did we find it?
		if ( found )
		{
			if ( DoIndex )
			{
				if ( cnt == Index )
				{
					retval = & ih->ih_Public ;
					break;
				}
				else
				{
					cnt++;
				}
			}
			else
			{
				retval = & ih->ih_Public ;
				break;
			}
		}

		_myIFC_GetPrev( & fn, & ig, & ih );
	}

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE( & usbbase->usb_Fkt_Semaphore );

	return( retval );
}

// --
