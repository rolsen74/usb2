/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HUB.h"

// --

SEC_CODE void
HUB__Port_Hold( struct USBBase *usbbase, struct intern *in, U32 port, U32 address )
{
	struct RealFunctionNode *hub;
	struct USB2_AddressHold *hold;

	U32 level;
	U32 leaf;

	leaf = port;
	hold = &in->HCDNode->hn_AddressHold[address];
	hub = in->Function;

	SEMAPHORE_OBTAIN( &usbbase->usb_Addr_Semaphore );

	MEM_SET( hold, 0, sizeof( *hold ) );

	hold->Serial = ++in->HCDNode->hn_AddressHoldSerial;

	for ( level = 0; ( hub ) && ( level < USB2_ENUM_ROUTE_DEPTH ); level++ )
	{
		hold->Hub[level] = hub->fkt_NotifyID;
		hold->Port[level] = port;
		port = hub->fkt_PortNr;
		hub = hub->fkt_Parent;
	}

	SEMAPHORE_RELEASE( &usbbase->usb_Addr_Semaphore );

	usbbase->usb_IExec->DebugPrintF( "USB HUB: HCD %lu hub %lu port %lu epoch %lu retaining address %lu after failed isolation\n",
		in->HCDNode->hn_HCDIndex, in->Function->fkt_Address, leaf, in->EnumEpoch, address );
}

// --

SEC_CODE U32
HUB__Port_Isolate( struct USBBase *usbbase, struct intern *in, U32 port )
{
	struct USB2_PortStatus status;

	S32 err;

	if ( ( ! in->Function->fkt_Tier ) && ( ! in->HCDNode->HCD_Functions.Port_Get_Status ) )
	{
		return ( FALSE );
	}

	err = HUB__Port_Status_Get( usbbase, in, port, &status );

	if ( ( err ) || ( status.wPortStatus & PORTSTATF_Reset ) )
	{
		return ( FALSE );
	}

	if ( ! ( status.wPortStatus & PORTSTATF_Enable ) )
	{
		return ( HUB_Can_Enumerate( usbbase, in ) );
	}

	err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_ENABLE );

	if ( ( err ) && ( err != USB2Err_Device_OwnershipChange ) )
	{
		return ( FALSE );
	}

	err = HUB__Port_Status_Get( usbbase, in, port, &status );

	return ( ( ! err ) && ( ! ( status.wPortStatus & ( PORTSTATF_Enable | PORTSTATF_Reset ) ) ) &&
		( HUB_Can_Enumerate( usbbase, in ) ) );
}

// --

SEC_CODE U32
HUB__Port_Probe_Holds( struct USBBase *usbbase, struct intern *in, U32 port )
{
	struct USB2_AddressHold *hold;
	struct USB2_PortStatus status;

	USB2_ID id;
	U32 address;
	U32 level;
	U32 direct;
	U32 contained;
	U32 disconnected;
	U32 entirePath;
	U32 serial;
	U32 pending;

	id = in->Function->fkt_NotifyID;
	direct = FALSE;
	contained = FALSE;

	SEMAPHORE_OBTAIN( &usbbase->usb_Addr_Semaphore );

	serial = in->HCDNode->hn_AddressHoldSerial;

	for ( address = 0; address < 128; address++ )
	{
		hold = &in->HCDNode->hn_AddressHold[address];

		for ( level = 0; ( level < USB2_ENUM_ROUTE_DEPTH ) && ( hold->Hub[level] ); level++ )
		{
			if ( ( hold->Hub[level] == id ) && ( hold->Port[level] == port ) )
			{
				direct |= ( level == 0 );
				contained |= ( level != 0 );
				break;
			}
		}
	}

	SEMAPHORE_RELEASE( &usbbase->usb_Addr_Semaphore );

	if ( ( ! direct ) && ( ! contained ) )
	{
		return ( FALSE );
	}

	disconnected = FALSE;
	entirePath = FALSE;
	pending = TRUE;

	if ( ( HUB_Can_Enumerate( usbbase, in ) ) &&
		( ( in->Function->fkt_Tier ) || ( in->HCDNode->HCD_Functions.Port_Get_Status ) ) &&
		( SEMAPHORE_ATTEMPT( &in->HCDNode->hn_AddressZero ) ) )
	{
		SEMAPHORE_OBTAIN( &usbbase->usb_LockSemaphore );

		in->EnumEpoch = in->HCDNode->hn_IOEpoch;

		SEMAPHORE_RELEASE( &usbbase->usb_LockSemaphore );

		in->ControlDeadline = HUB_Uptime( usbbase ) + 500000;

		if ( ( ! HUB__Port_Status_Get( usbbase, in, port, &status ) ) &&
			( ! ( status.wPortStatus & ( PORTSTATF_Connection | PORTSTATF_Enable | PORTSTATF_Reset ) ) ) )
		{
			disconnected = TRUE;
		}

		entirePath = ( disconnected ) && ( HUB_Can_Enumerate( usbbase, in ) );

		if ( ( entirePath ) ||
			( ( ( direct ) || ( ! in->PortFunctions[port] ) ) && ( HUB__Port_Isolate( usbbase, in, port ) ) ) )
		{
			entirePath |= ! in->PortFunctions[port];
			SEMAPHORE_OBTAIN( &usbbase->usb_Addr_Semaphore );

			for ( address = 0; address < 128; address++ )
			{
				hold = &in->HCDNode->hn_AddressHold[address];

				for ( level = 0; ( level < USB2_ENUM_ROUTE_DEPTH ) && ( hold->Hub[level] ); level++ )
				{
					if ( ( (S32) ( serial - hold->Serial ) >= 0 ) &&
						( ( ! level ) || ( entirePath ) ) && ( hold->Hub[level] == id ) &&
						( hold->Port[level] == port ) )
					{
						MEM_SET( hold, 0, sizeof( *hold ) );
						break;
					}
				}
			}

			SEMAPHORE_RELEASE( &usbbase->usb_Addr_Semaphore );

			usbbase->usb_IExec->DebugPrintF( "USB HUB: HCD %lu hub %lu port %lu cleared address holds by %s\n",
				in->HCDNode->hn_HCDIndex, in->Function->fkt_Address, port, disconnected ? "disconnect" : "disable" );

			pending = ( contained ) && ( ! entirePath );

			if ( ( ! pending ) && ( ! in->PortFunctions[port] ) && ( ! in->PortWork[port].Due ) )
			{
				HUB_Queue_Port( usbbase, in, port, TRUE );
			}
		}

		in->ControlDeadline = 0;

		SEMAPHORE_RELEASE( &in->HCDNode->hn_AddressZero );
	}

	return ( pending );
}

// --
