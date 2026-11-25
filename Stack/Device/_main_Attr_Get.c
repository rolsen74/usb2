
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

static struct RealFunctionNode *Find_Function( struct USBBase *usbbase, USB2_ID id );

// --

SEC_CODE static S32
PortPath( struct RealFunctionNode *fn, struct USB2_PortPath *path )
{
	struct USB2_HCDNode *hn;
	struct PCIDevice *pci;

	U8 ports[USB2_PORT_PATH_DEPTH];
	U8 bus;
	U8 device;
	U8 function;
	U8 depth;
	U8 index;

	hn = fn->fkt_HCD;
	depth = 0;

	while( fn->fkt_Parent )
	{
		if ( ( depth == USB2_PORT_PATH_DEPTH ) || ( ! fn->fkt_PortNr ) || ( fn->fkt_PortNr > 255 ) )
		{
			return ( FALSE );
		}

		ports[depth++] = fn->fkt_PortNr;
		fn = fn->fkt_Parent;
	}

	if ( ( ! hn ) || ( ! depth ) || ( fn->fkt_Tier != 0 ) )
	{
		return ( FALSE );
	}

	pci = hn->hn_PCIDevice;
	path->IsPCI = ( pci != NULL );
	path->Controller = hn->hn_PCIDevIOBase;
	path->Depth = depth;

	if ( pci )
	{
		pci->GetAddress( & bus, & device, & function );

		path->Controller = ( (U32) bus << 16 ) | ( (U32) device << 8 ) | function;
	}

	for( index = 0; index < depth; index++ )
	{
		path->Ports[index] = ports[depth - index - 1];
	}

	return ( TRUE );
}

// --

SEC_CODE static U32 _main_Attr_Get( struct USB2IFace *Self, USB2_ID id, U32 tag, PTR buffer, U32 buffersize )
{
struct RealFunctionNode *fn;
struct USB2_PortPath path;
struct USBBase *usbbase;
U32 value;
U32 len;
PTR mem;

	len = 0;
	mem = NULL;
	fn = NULL;

	usbbase = (PTR) Self->Data.LibBase;

	switch( tag )
	{
		case USB2Tag_Attr_DeviceSize:
		{
			fn = Find_Function( usbbase, id );

			if ( fn )	// Just Validating ID
			{
				value = sizeof( struct USB2_Device_Desc );
				len	= sizeof( U32 );
				mem = & value ;
			}
			break;
		}

		case USB2Tag_Attr_DeviceDesc:
		{
			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len	= sizeof( struct USB2_Device_Desc );
				mem = fn->fkt_DeviceDescriptor;
			}
			break;
		}

		case USB2Tag_Attr_HUBSize:
		{
			fn = Find_Function( usbbase, id );

			if ( fn )	// Just Validating ID
			{
				value = sizeof( struct USB2_HUB_Desc );
				len	= sizeof( U32 );
				mem = & value ;
			}
			break;
		}

		case USB2Tag_Attr_HUBDesc:
		{
			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len	= sizeof( struct USB2_HUB_Desc );
				mem = fn->fkt_HUBDescriptor;
			}
			break;
		}

		case USB2Tag_Attr_HUBPorts:
		{
			fn = Find_Function( usbbase, id );

			if ( ( fn ) && ( fn->fkt_Class == USBCLASS_HUB ) )
			{
				value = fn->fkt_HUBDescriptor ? fn->fkt_HUBDescriptor->NbrPorts : 0;

				if ( ( ! value ) && ( fn->fkt_Tier == 0 ) && ( fn->fkt_HCD ) )
				{
					value = fn->fkt_HCD->hn_HUB_NumPorts;
				}

				len = sizeof( U32 );
				mem = &value;
			}
			break;
		}

		case USB2Tag_Attr_Speed:
		{
			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				value = fn->fkt_Speed;
				len = sizeof( value );
				mem = & value;
			}
			break;
		}

		case USB2Tag_Attr_HCDType:
		{
			fn = Find_Function( usbbase, id );

			if ( ( fn ) && ( fn->fkt_HCD ) )
			{
				value = fn->fkt_HCD->hn_HCDType;
				len = sizeof( value );
				mem = &value;
			}
			break;
		}

		case USB2Tag_Attr_PortPath:
		{
			path = (struct USB2_PortPath) { 0 };

			fn = Find_Function( usbbase, id );

			if ( ( fn ) && ( PortPath( fn, & path ) ) )
			{
				len = sizeof( path );
				mem = & path;
			}
			break;
		}

		case USB2Tag_Attr_ConfigSize:
		{
			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				value = fn->fkt_Config_Desc_Size;
				len	= sizeof( U32 );
				mem = & value ;
			}
			break;
		}

		case USB2Tag_Attr_ConfigDesc:
		{
			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				len	= fn->fkt_Config_Desc_Size;
				mem = fn->fkt_Config_Desc_Buf;
			}
			break;
		}

		case USB2Tag_Attr_ManufacturerStr:
		case USB2Tag_Attr_ProductStr:
		case USB2Tag_Attr_SerialStr:
		{
			fn = Find_Function( usbbase, id );

			if ( fn )
			{
				if ( tag == USB2Tag_Attr_ManufacturerStr )
				{
					len = fn->fkt_ManufacturerStrLen;
					mem = fn->fkt_ManufacturerStr;
				}
				else if ( tag == USB2Tag_Attr_ProductStr )
				{
					len = fn->fkt_DeviceStrLen;
					mem = fn->fkt_DeviceStr;
				}
				else
				{
					len = fn->fkt_SerialStrLen;
					mem = fn->fkt_SerialStr;
				}
			}
			break;
		}

		default:
		{
			break;
		}
	}

	if (( mem ) && ( buffer ))
	{
		len = MIN( buffersize, len );

		if ( len )
		{
			MEM_COPY( mem, buffer, len );
		}
	}
	else
	{
		len = 0;
	}

	if ( fn )
	{
		SEMAPHORE_RELEASE( & usbbase->usb_Fkt_Semaphore );
	}

	return( len );
}

// --

static struct RealFunctionNode *Find_Function( struct USBBase *usbbase, USB2_ID id )
{
struct RealFunctionNode *fn;
struct USB2_Node *n;

	SEMAPHORE_OBTAIN( & usbbase->usb_Fkt_Semaphore );

	fn = NULL;

	n = usbbase->usb_Fkt_Header.uh_Tail;

	while( n )
	{
		fn = n->un_Data;

		if (( fn ) && ( fn->fkt_NotifyID == id ))
		{
			break;
		}

		n = n->un_Prev;
	}

	if ( ! n )
	{
		fn = NULL;
	}

	if ( ! fn )
	{
		SEMAPHORE_RELEASE( & usbbase->usb_Fkt_Semaphore );
	}

	return(	fn );
}

// --

#if 0


static struct USB2_HCDNode *			Find_HCD(		struct USBBase *usbbase, uint64 id );
static struct USB2_FunctionNode *	Find_Function(	struct USBBase *usbbase, uint64 id );
static struct USB2_InterfaceNode *	Find_Interface( struct USBBase *usbbase, uint64 id );


/* -- Stack Attriubute -- */

/// Stack Get Attribute

U32 _main_Stack_GetAttribute( struct USB2IFace *Self, uint64 id, U32 tag, APTR buffer, U32 buffersize )
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

	if ( ! buffer )
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

U32 _main_Stack_SetAttribute( struct USB2IFace *Self, uint64 id UNUSED, U32 tag, APTR buffer, U32 buffersize )
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

	if ( ! buffer )
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

	if ( ! retval )
	{
		SEMAPHORE_RELEASE( &usbbase->usb_USBFunctionSemaphore );
	}

	return(	retval );
}

// --



#endif

// --
