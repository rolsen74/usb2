
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef INTERFACES_USB2UNITIDENTITY_H
#define INTERFACES_USB2UNITIDENTITY_H

#include <interfaces/usb2.h>

struct USB2UnitIdentity
{
	struct USB2_PortPath Port;
	uint32_t Instance;
	uint16_t VendorID;
	uint16_t DeviceID;
	uint8_t Interface;
	uint8_t SerialLength;
	uint8_t Serial[255];
};

static inline void
USB2_ReadUnitIdentity( struct USB2UnitIdentity *identity, const struct USB2_DriverMessage *msg, uint32_t instance )
{
	uint32_t length;
	uint32_t index;
	uint16_t code;
	int usable;

	identity->SerialLength = 0;
	identity->Port.Depth = 0;

	if ( ( ! msg ) || ( ! msg->Function ) || ( ! msg->InterfaceDescriptor ) || ( ! msg->IUSB2 ) )
	{
		return;
	}

	identity->VendorID = msg->VendorID;
	identity->DeviceID = msg->DeviceID;
	identity->Interface = msg->InterfaceDescriptor->InterfaceNumber;
	identity->Instance = instance;

	length = ( msg->IUSB2->USB2_Attr_Get )( msg->Function->FKT_ID, USB2Tag_Attr_SerialStr,
		identity->Serial, sizeof( identity->Serial ) );

	if ( ( length >= 4 ) && ( length <= sizeof( identity->Serial ) ) &&
		( identity->Serial[0] == length ) && ( ! ( length & 1 ) ) && ( identity->Serial[1] == DSCTYPE_String ) )
	{
		usable = 0;

		for ( index = 2; index < length; index += 2 )
		{
			code = identity->Serial[index] | ( (uint16_t)identity->Serial[index + 1] << 8 );

			if ( ! code )
			{
				usable = 0;
				break;
			}

			usable |= code != ' ';
		}

		if ( usable )
		{
			identity->SerialLength = length;
		}
	}

	length = ( msg->IUSB2->USB2_Attr_Get )( msg->Function->FKT_ID, USB2Tag_Attr_PortPath,
		&identity->Port, sizeof( identity->Port ) );

	if ( ( length != sizeof( identity->Port ) ) || ( identity->Port.Depth > USB2_PORT_PATH_DEPTH ) )
	{
		identity->Port.Depth = 0;
	}
}

static inline int
USB2_HasUnitIdentity( const struct USB2UnitIdentity *identity )
{
	return ( identity->SerialLength || identity->Port.Depth );
}

static inline int
USB2_UnitIdentityBytesMatch( const uint8_t *known, const uint8_t *found, uint32_t length )
{
	uint32_t index;

	for ( index = 0; index < length; index++ )
	{
		if ( known[index] != found[index] )
		{
			return ( 0 );
		}
	}

	return ( 1 );
}

// A matching serial can move ports. The same port breaks a tie.
static inline int
USB2_MatchUnitIdentity( const struct USB2UnitIdentity *known, const struct USB2UnitIdentity *found )
{
	int samePort;

	if ( ( ! USB2_HasUnitIdentity( known ) ) || ( ! USB2_HasUnitIdentity( found ) ) ||
		( known->VendorID != found->VendorID ) || ( known->DeviceID != found->DeviceID ) ||
		( known->Interface != found->Interface ) || ( known->Instance != found->Instance ) )
	{
		return ( 0 );
	}

	samePort = ( known->Port.Depth ) && ( known->Port.Depth == found->Port.Depth ) &&
		( known->Port.Controller == found->Port.Controller ) && ( known->Port.IsPCI == found->Port.IsPCI ) &&
		( USB2_UnitIdentityBytesMatch( known->Port.Ports, found->Port.Ports, known->Port.Depth ) );

	if ( ( known->SerialLength ) || ( found->SerialLength ) )
	{
		if ( ( known->SerialLength != found->SerialLength ) ||
			( ! USB2_UnitIdentityBytesMatch( known->Serial, found->Serial, known->SerialLength ) ) )
		{
			return ( 0 );
		}

		return ( samePort ? 2 : 1 );
	}

	return ( samePort ? 2 : 0 );
}

struct USB2UnitSearch
{
	uint32_t Matching;
	uint32_t Unused;
	uint32_t Reusable;
	uint32_t Limit;
	uint32_t BestMatch;
};

static inline void
USB2_InitUnitSearch( struct USB2UnitSearch *search, uint32_t limit )
{
	search->Matching = limit;
	search->Unused = limit;
	search->Reusable = limit;
	search->Limit = limit;
	search->BestMatch = 0;
}

// Consider one disconnected slot. A reservation excludes unrelated devices.
static inline void
USB2_ConsiderUnit( struct USB2UnitSearch *search, const struct USB2UnitIdentity *known,
	const struct USB2UnitIdentity *found, uint32_t index, int reserved )
{
	uint32_t match;

	match = USB2_MatchUnitIdentity( known, found );

	if ( match )
	{
		match = 2 * match + !! reserved;

		if ( match > search->BestMatch )
		{
			search->BestMatch = match;
			search->Matching = index;
		}
	}

	if ( ! reserved )
	{
		if ( search->Reusable == search->Limit )
		{
			search->Reusable = index;
		}

		if ( ( search->Unused == search->Limit ) && ( ! USB2_HasUnitIdentity( known ) ) )
		{
			search->Unused = index;
		}
	}
}

static inline uint32_t
USB2_SelectUnit( const struct USB2UnitSearch *search )
{
	if ( search->Matching < search->Limit )
	{
		return ( search->Matching );
	}

	return ( ( search->Unused < search->Limit ) ? search->Unused : search->Reusable );
}

#endif
