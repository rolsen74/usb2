
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static struct USB2_InterfaceHeader *Find_InterfaceHeader(
	struct USBBase *usbbase,
	struct USB2_ConfigNode *cn,
	struct USB2_Interface_Desc *ifcdsc )
{
struct USB2_InterfaceHeader *ih;
struct USB2_InterfaceGroup *ig;
U32 nr;

	// --

	ih = NULL;
	ig = cn->cfg_InterfaceGroups.uh_Head;
	nr = ifcdsc->InterfaceNumber;

	while( INTERFACE_VALIDGROUP(ig) == VSTAT_Okay )
	{
		// We are using the fact that each IG only have one IH at this point
		ih = ig->ig_Group.uh_Head;

		if ( ih->ih_Number == nr )
		{
			break;
		}
		else
		{
			ig = Node_Next( ig );
			ih = NULL;
		}
	}

	// --

	if ( INTERFACE_VALIDGROUP(ig) != VSTAT_Okay )
	{
		ig = INTERFACE_ALLOCGROUP(cn);
		ih = ( ig ) ? INTERFACE_ALLOCHEADER( ig, nr ) : NULL;

		if ((ig) && (ih))
		{
			ig->ig_Class	= ifcdsc->InterfaceClass;
			ig->ig_SubClass	= ifcdsc->InterfaceSubClass;
			ig->ig_Protocol	= ifcdsc->InterfaceProtocol;

			NODE_ADDTAIL( & cn->cfg_InterfaceGroups, ig );
			NODE_ADDTAIL( & ig->ig_Group, ih );
		}
		else
		{
			INTERFACE_FREEHEADER(ih);
			INTERFACE_FREEGROUP(ig);
			ih = NULL;
		}
	}

	// --

	return( ih );
}

// --

// --
// Parse Config Descriptor and build internal Structions

SEC_CODE static S32
ValidEndPointSize( U32 speed, const struct USB2_EndPoint_Desc *ep )
{
	U32 raw;
	U32 size;
	U32 type;
	U32 limit;

	raw = LE_SWAP16( ep->MaxPacketSize );
	size = raw & 0x7ff;
	type = ep->Attributes & EPATT_Type_Mask;
	limit = 1024;

	if ( speed == USBSPEED_Low )
	{
		if ( ( type != EPATT_Type_Control ) && ( type != EPATT_Type_Interrupt ) )
		{
			return ( FALSE );
		}

		limit = 8;
	}
	else if ( speed == USBSPEED_Full )
	{
		limit = ( type == EPATT_Type_Isochronous ) ? 1023 : 64;
	}
	else if ( speed == USBSPEED_High )
	{
		if ( type == EPATT_Type_Control )
		{
			limit = 64;
		}
		else if ( type == EPATT_Type_Bulk )
		{
			limit = 512;
		}
	}

	if ( ( speed == USBSPEED_High ) &&
		( ( type == EPATT_Type_Isochronous ) || ( type == EPATT_Type_Interrupt ) ) )
	{
		if ( ( raw & 0xe000 ) || ( ( raw & 0x1800 ) == 0x1800 ) )
		{
			return ( FALSE );
		}
	}
	else if ( raw != size )
	{
		return ( FALSE );
	}

	return ( ( size <= limit ) && ( ( size ) || ( type == EPATT_Type_Isochronous ) ) );
}

// --

SEC_CODE static S32 __Parse( struct USBBase *usbbase, struct RealFunctionNode *fn )
{
struct USB2_InterfaceHeader *ih;
struct USB2_InterfaceGroup *ig;
struct USB2_Interface_Desc *ifcdsc;
struct USB2_EndPoint_Desc *epdsc;
struct USB2_InterfaceNode *in;
struct USB2_InterfaceNode *alt;
struct USB2_EndPointNode *ep;
struct USB2_Config_Desc *cfgdsc;
struct USB2_Descriptor *dsc;
struct USB2_ConfigNode *cn;
S32 retval;
S32 len;
U32 slot;
U32 mask;
U32 endpoints;
U32 owners[32];

	retval	= FALSE;
	dsc		= fn->fkt_Config_Desc_Buf;
	len		= fn->fkt_Config_Desc_Size;
	cn		= NULL;
	in		= NULL;
	endpoints = 0;

	if ( ( ! dsc ) || ( len < (S32) sizeof( *cfgdsc ) ) ||
		( dsc->DescriptorType != DSCTYPE_Configuration ) )
	{
		return ( FALSE );
	}

	MEM_SET( owners, 0, sizeof( owners ) );

	while(( dsc ) && ( len > 1 ))
	{
		if ( ( dsc->Length < sizeof( struct USB2_Descriptor ) ) || ( dsc->Length > len ) )
		{
			USBERROR( "__Parse: Invalid descriptor length %lu with %ld bytes remaining", (U32) dsc->Length, len );
			goto bailout;
		}

		switch( dsc->DescriptorType )
		{
			case DSCTYPE_Configuration:
			{
				USBDEBUG( "Config Descriptor" );

				if ( dsc->Length < sizeof( struct USB2_Config_Desc ) )
				{
					USBERROR( "__Parse: Short configuration descriptor" );
					goto bailout;
				}

				cfgdsc = (PTR) dsc;

				if ( ( cn ) || ( LE_SWAP16( cfgdsc->TotalLength ) != len ) ||
					( ! cfgdsc->ConfigurationValue ) )
				{
					USBERROR( "__Parse: Invalid configuration header" );
					goto bailout;
				}

				cn = CONFIG_ALLOC(fn);

				if ( ! cn )
				{
					USBERROR( "__Parse: Error allocating memory" );
					goto bailout;
				}

				cn->cfg_Descriptor = cfgdsc;
				cn->cfg_Number = cfgdsc->ConfigurationValue;

				NODE_ADDTAIL( & fn->fkt_Configs, cn );
				break;
			}

			/*
			** FN : 0x6FE5DB50, Class: 0000, SubClass: 0000, Protocol: 0000, Addr: 3, Tier: 2
			** CN : 0x6FF0B850, ConfigNr 1
			**  IG : 0x6F9690A0, Class: 0003, SubClass: 0001, Protocol: 0001
			**   IH : 0x6FF0B8D0, InterfaceNr: 0
			**    IN : 0x6FF0AE70, AltNr: 0
			**     EP : 0x6F95D0C0, Type: 3, Num: 1
			**  IG : 0x6F9690D0, Class: 0003, SubClass: 0000, Protocol: 0000
			**   IH : 0x6FF0B890, InterfaceNr: 1
			**    IN : 0x6F95D160, AltNr: 0
			**     EP : 0x6F95D1B0, Type: 3, Num: 2
			*/

			case DSCTYPE_Interface:
			{
				USBDEBUG( "Interface Descriptor" );

				if ( dsc->Length < sizeof( struct USB2_Interface_Desc ) )
				{
					USBERROR( "__Parse: Short interface descriptor" );
					goto bailout;
				}

				if ( CONFIG_VALID(cn) != VSTAT_Okay )
				{
					USBERROR( "__Parse: Error Unexpected Interface" );
					goto bailout;
				}

				ifcdsc = (PTR) dsc;

				ih = Find_InterfaceHeader( usbbase, cn, ifcdsc );

				if ( INTERFACE_VALIDHEADER(ih) != VSTAT_Okay )
				{
					USBERROR( "__Parse: Error allocating memory" );
					goto bailout;
				}

				for( alt = ih->ih_AltSettings.uh_Head; alt; alt = Node_Next( alt ) )
				{
					if ( alt->in_AltNumber == ifcdsc->AlternateSetting )
					{
						USBERROR( "__Parse: Duplicate alternate setting" );
						goto bailout;
					}
				}

				in = INTERFACE_ALLOCNODE(ih);

				if ( ! in )
				{
					USBERROR( "__Parse: Error allocating memory" );
					goto bailout;
				}

				in->in_Descriptor	= ifcdsc;
				in->in_AltNumber	= ifcdsc->AlternateSetting;

				endpoints = 0;

				if ( ! in->in_AltNumber )
				{
					ih->ih_Active		= in;
					ih->ih_AltNumber	= in->in_AltNumber;
				}

				NODE_ADDTAIL( & ih->ih_AltSettings, in );
				break;
			}

			case DSCTYPE_EndPoint:
			{
				USBDEBUG( "EndPoint Descriptor" );

				if ( dsc->Length < sizeof( struct USB2_EndPoint_Desc ) )
				{
					USBERROR( "__Parse: Short endpoint descriptor" );
					goto bailout;
				}

				if ( INTERFACE_VALIDNODE(in) != VSTAT_Okay )
				{
					USBERROR( "__Parse: Error Unexpected EndPoint" );
					goto bailout;
				}

				epdsc = (PTR) dsc;
				slot = EPADR_Dir_Number( epdsc->EndPointAddress );

				if ( ( ! slot ) || ( epdsc->EndPointAddress & 0x70 ) ||
					( ! ValidEndPointSize( fn->fkt_Speed, epdsc ) ) )
				{
					USBERROR( "__Parse: Invalid endpoint descriptor" );
					goto bailout;
				}

				slot += ( epdsc->EndPointAddress & EPADR_Dir_Mask ) ? 0 : 16;
				mask = 1UL << slot;

				if ( ( endpoints & mask ) || ( ( owners[slot] ) &&
					( owners[slot] != (U32) in->in_Descriptor->InterfaceNumber + 1 ) ) )
				{
					USBERROR( "__Parse: Conflicting endpoint address" );
					goto bailout;
				}

				endpoints |= mask;
				owners[slot] = (U32) in->in_Descriptor->InterfaceNumber + 1;

				ep = ENDPOINT_ALLOC( fn, in, epdsc );

				if ( ! ep )
				{
					USBERROR( "__Parse: Error allocating memory" );
					goto bailout;
				}

				NODE_ADDTAIL( & in->in_EndPoints, ep );
				break;
			}

			case DSCTYPE_Interface_Association:
			{
				if ( dsc->Length < sizeof( struct USB2_Association_Desc ) )
				{
					USBERROR( "__Parse: Short interface association descriptor" );
					goto bailout;
				}

				break;
			}

			default:
			{
				break;
			}
		}

		len -= dsc->Length;
		dsc = ( len > 1 ) ? (PTR) ( (U8 *) dsc + dsc->Length ) : NULL;
	}

	if ( len )
	{
		USBERROR( "__Parse: Trailing descriptor byte" );
		goto bailout;
	}

	for( ig = cn->cfg_InterfaceGroups.uh_Head; ig; ig = Node_Next( ig ) )
	{
		ih = ig->ig_Group.uh_Head;

		if ( ! ih->ih_Active )
		{
			USBERROR( "__Parse: Interface has no alternate zero" );
			goto bailout;
		}
	}

	retval = TRUE;

bailout:

	return( retval );
}

// --
