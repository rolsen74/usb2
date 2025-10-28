
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

SEC_CODE static U32 _main_Attr_Get( struct USB2_IFace *Self, U64 id, U32 tag, PTR buffer, U32 buffsersize )
{
	return( 0 );
}

// --

#if 0


static struct USB2_HCDNode *			Find_HCD(		struct USBBase *usbbase, uint64 id );
static struct USB2_FunctionNode *	Find_Function(	struct USBBase *usbbase, uint64 id );
static struct USB2_InterfaceNode *	Find_Interface( struct USBBase *usbbase, uint64 id );


/* -- Stack Attriubute -- */

/// Stack Get Attribute

U32 _main_Stack_GetAttribute( struct USB2_IFace *Self, uint64 id, U32 tag, APTR buffer, U32 buffersize )
{
struct USB2_ConfigNode *cfgnode;
struct USB2_InterfaceNode *ifcnode;
struct USB2_FunctionNode *fn;
struct USB2_HCDNode *hn;
struct USBBase *usbbase;
U32 retval;
U32 len;
U32 l;
APTR data;

	usbbase = (APTR)Self->Data.LibBase;

	retval	= 0;

	data	= NULL;

	len		= 0;

	fn		= NULL;

	l		= 0;

	ifcnode	= NULL;

	if ( buffer == NULL )
	{
		SHOWMSG( "Storage Pointer is missing" );
		goto bailout;
	}

	switch( tag )
	{
		case USB2Tag_Stack_StartupTime:
		{
			SHOWMSG( "USB2Tag_Stack_StartupTime" );

			len		= sizeof( struct TimeVal );
			data	= &usbbase->usb_StartupTime;
			break;
		}

		case USB2Tag_Stack_Status:
		{
			SHOWMSG( "USB2Tag_Stack_Status" );

			len		= sizeof( U32 );
			data	= &usbbase->usb_StackStatus;
			break;
		}

		case USB2Tag_HCD_Type:
		{
			SHOWMSG( "USB2Tag_HCD_Type" );

			hn = Find_HCD( usbbase, id );

			if ( hn )
			{
				len		= sizeof( U32 );
				data	= &hn->hn_HCDType;
			}
			break;
		}

		case USB2Tag_HCD_Index:
		{
			SHOWMSG( "USB2Tag_HCD_Index" );

			hn = Find_HCD( usbbase, id );

			if ( hn )
			{
				len		= sizeof( U32 );
				data	= &hn->hn_HCDIndex;
			}
			break;
		}

		case USB2Tag_FKT_DeviceDescriptor:
		{
			SHOWMSG( "USB2Tag_FKT_DeviceDescriptor" );

			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len		= sizeof( struct USB2_DeviceDescriptor );
				data	= fn->fkt_DeviceDescriptor;
			}
			break;
		}

		case USB2Tag_FKT_HUBDescriptor:
		{
			SHOWMSG( "USB2Tag_FKT_HUBDescriptor" );

			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len		= sizeof( struct USB2_HUBDescriptor );
				data	= fn->fkt_HUBDescriptor;
			}
			break;
		}

		case USB2Tag_FKT_ConfigDescriptor:
		{
			SHOWMSG( "USB2Tag_FKT_ConfigDescriptor" );

			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len		= fn->fkt_ConfigDescriptorSize;
				data	= fn->fkt_ConfigDescriptor;
			}
			break;
		}

		case USB2Tag_FKT_ConfigDescriptorSize:
		{
			SHOWMSG( "USB2Tag_FKT_ConfigDescriptorSize" );

			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len		= sizeof( U32 );
				data	= &fn->fkt_ConfigDescriptorSize;
			}
			break;
		}

		case USB2Tag_FKT_ManufacturerStr:
		{
			SHOWMSG( "USB2Tag_FKT_ManufacturerStr" );

			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len		= fn->fkt_ManufacturerStrLen;
				data	= fn->fkt_ManufacturerStr;
			}
			break;
		}

		case USB2Tag_FKT_ProductStr:
		{
			SHOWMSG( "USB2Tag_FKT_ProductStr" );

			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len		= fn->fkt_ProductStrLen;
				data	= fn->fkt_ProductStr;
			}
			break;
		}

		case USB2Tag_FKT_SerialStr:
		{
			SHOWMSG( "USB2Tag_FKT_SerialStr" );

			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len		= fn->fkt_SerialStrLen;
				data	= fn->fkt_SerialStr;
			}
			break;
		}

		case USB2Tag_CFG_String:
		{
			SHOWMSG( "USB2Tag_CFG_String" );

			fn = Find_Function( usbbase, id );
			cfgnode = ( fn ) ? fn->fkt_ConfigActive : NULL;

			if ( cfgnode )
			{
				len		= cfgnode->cfg_StringLen;
				data	= cfgnode->cfg_String;
			}
			break;
		}

		case USB2Tag_IFC_String:
		{
			SHOWMSG( "USB2Tag_IFC_String" );

			ifcnode = Find_Interface( usbbase, id );

			if ( ifcnode )
			{
				len		= ifcnode->ifc_StringLen;
				data	= ifcnode->ifc_String;
			}
			break;
		}

//#define USB2Tag_Prefs_HUB_EnableTT	   ( USB2Tag_Dummy + 59 )   // v1.6  - U32

//#define USB2Tag_Prefs_HID_Protocol	   ( USB2Tag_Dummy + 60 )   // v1.6  - U32

		default:
		{
			SHOWMSG( "Unknown TagID (0x%08lx)", id );
			goto bailout;
		}
	}

	l = MIN( len, buffersize );

	SHOWMSG( "Data: %p, Len: %ld, Buffer: %p", data, l, buffer );

	if (( data ) && ( l ))
	{
		MEM_COPY( data, buffer, l );
	}

	retval = l;

bailout:

	if (( ifcnode ) || ( fn ))
	{
		SEMAPHORE_RELEASE( &usbbase->usb_USBFunctionSemaphore );
	}
	return(		retval );
}

///
/// Stack Set Attribute

U32 _main_Stack_SetAttribute( struct USB2_IFace *Self, uint64 id UNUSED, U32 tag, APTR buffer, U32 buffersize )
{
//struct ExecIFace *IExec;
struct USBBase *usbbase;
U32 retval;
U32 len;
U32 l;
APTR data;

	usbbase	= (APTR)Self->Data.LibBase;

//	  IExec	  = usbbase->usb_IExec;

	retval	= 0;

	len		= 0;

	if ( buffer == NULL )
	{
		SHOWMSG( "Storage Pointer is missing" );
		goto bailout;
	}

	switch( tag )
	{


		default:
		{
			SHOWMSG( "Unknown TagID (0x%08lx)", id );
			goto bailout;
		}
	}

	l = MIN( len, buffersize );

	if ( l )
	{
		MEM_COPY( data, buffer, l );
//		  IExec->CopyMem( data, buffer, l );
	}

	retval = l;

bailout:
	return(		retval );
}

///

/* -- Internal -- */

/// Find HCD

static struct USB2_HCDNode *Find_HCD( struct USBBase *usbbase, uint64 id )
{
struct USB2_HCDNode *hcd;
//struct ExecIFace *IExec;

//	  IExec = usbbase->usb_IExec;

	hcd = usbbase->usb_HCDHeader.uh_Head;

	while( hcd )
	{
		if ( hcd->hn_NotifyID == id )
		{
			break;
		}
		else
		{
			hcd = hcd->hn_Node.un_Next;
		}
	}

	return(	hcd );
}

static struct USB2_FunctionNode *Find_Function( struct USBBase *usbbase, uint64 id )
{
struct USB2_FunctionNode *fkt;

	SEMAPHORE_OBTAIN( &usbbase->usb_USBFunctionSemaphore );

	fkt = usbbase->usb_USBFunctionHeader.uh_Head;

	while( fkt )
	{
		if ( fkt->fkt_NotifyID == id )
		{
			break;
		}
		else
		{
			fkt = fkt->fkt_Node.un_Next;
		}
	}

	if ( fkt == NULL )
	{
		SEMAPHORE_RELEASE( &usbbase->usb_USBFunctionSemaphore );
	}

	return(	fkt );
}

static struct USB2_InterfaceNode *Find_Interface( struct USBBase *usbbase, uint64 id )
{
struct USB2_ConfigNode *cfg;
struct USB2_InterfaceHeader *ih;
struct USB2_InterfaceGroup *ig;
struct USB2_InterfaceNode *in;
struct USB2_InterfaceNode *retval;
struct USB2_FunctionNode *fkt;

	retval = NULL;

	SEMAPHORE_OBTAIN( &usbbase->usb_USBFunctionSemaphore );

	fkt = usbbase->usb_USBFunctionHeader.uh_Head;

	while( fkt )
	{
		cfg = fkt->fkt_Configs.uh_Head;

		while( cfg )
		{
			ig = cfg->cfg_InterfaceGroups.uh_Head;

			while( ig )
			{
				ih = ig->ifc_Group.uh_Head;

				while( ih )
				{
					in = ih->ifc_AltSettings.uh_Head;

					while( in )
					{
						if ( in->ifc_StringID == id )
						{
							retval = in;
							break;
						}
						else
						{
							in = Node_Next( in );
						}
					}
			
					if ( retval )
					{
						break;
					}

					ih = Node_Next( ih );
				}

				if ( retval )
				{
					break;
				}

				ig = Node_Next( ig );
			}

			if ( retval )
			{
				break;
			}

			cfg = Node_Next( cfg );
		}

		if ( retval )
		{
			break;
		}

		fkt = Node_Next( fkt );
	}

	if ( retval == NULL )
	{
		SEMAPHORE_RELEASE( &usbbase->usb_USBFunctionSemaphore );
	}

	return(	retval );
}

// --



#endif

// --
