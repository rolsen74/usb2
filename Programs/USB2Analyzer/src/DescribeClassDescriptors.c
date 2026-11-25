
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include "inc/DescriptorFields.h"

// --

static const struct ClassDescriptor CDCDescriptors[] =
{
	[0x00] = { "CDC header", DESC_FIELDS( DESC_VERSION(bcdCDC) ), NULL },
	[0x01] = { "CDC call management",
		DESC_FIELDS( DESC_BITS(bmCapabilities, 1), DESC_BYTE(bDataInterface) ), NULL },
	[0x02] = { "CDC abstract control management", DESC_FIELDS( DESC_BITS(bmCapabilities, 1) ), NULL },
	[0x03] = { "CDC direct line management", DESC_FIELDS( DESC_BITS(bmCapabilities, 1) ), NULL },
	[0x0f] = { "CDC Ethernet networking",
		DESC_FIELDS( DESC_BYTE(iMACAddress), DESC_BITS(bmEthernetStatistics, 4),
			DESC_WORD(wMaxSegmentSize), DESC_BITS(wNumberMCFilters, 2), DESC_BYTE(bNumberPowerFilters) ), NULL },
	[0x1a] = { "CDC network control model",
		DESC_FIELDS( DESC_VERSION(bcdNcmVersion), DESC_BITS(bmNetworkCapabilities, 1) ), NULL },
	[0x1b] = { "CDC mobile broadband interface model",
		DESC_FIELDS( DESC_VERSION(bcdMBIMVersion), DESC_WORD(wMaxControlMessage), DESC_BYTE(bNumberFilters),
			DESC_BYTE(bMaxFilterSize), DESC_WORD(wMaxSegmentSize), DESC_BITS(bmNetworkCapabilities, 1) ), NULL },
	[0x1c] = { "CDC MBIM extended",
		DESC_FIELDS( DESC_VERSION(bcdMBIMExtendedVersion), DESC_BYTE(bMaxOutstandingCommandMessages),
			DESC_WORD(wMTU) ), NULL }
};

static const struct ClassDescriptor CCIDDescriptor =
{
	"CCID class descriptor",
	DESC_FIELDS( DESC_VERSION(bcdCCID), DESC_BYTE(bMaxSlotIndex), DESC_BITS(bVoltageSupport, 1),
		DESC_BITS(dwProtocols, 4), DESC_LONG(dwDefaultClock), DESC_LONG(dwMaximumClock),
		DESC_BYTE(bNumClockSupported), DESC_LONG(dwDataRate), DESC_LONG(dwMaxDataRate),
		DESC_BYTE(bNumDataRatesSupported), DESC_LONG(dwMaxIFSD), DESC_BITS(dwSynchProtocols, 4),
		DESC_BITS(dwMechanical, 4), DESC_BITS(dwFeatures, 4), DESC_LONG(dwMaxCCIDMessageLength),
		DESC_BITS(bClassGetResponse, 1), DESC_BITS(bClassEnvelope, 1), DESC_WORD(wLcdLayout),
		DESC_BITS(bPINSupport, 1), DESC_BYTE(bMaxCCIDBusySlots) ),
	NULL
};

static const struct ClassDescriptor DFUDescriptor =
{
	"DFU functional descriptor",
	DESC_FIELDS( DESC_BITS(bmAttributes, 1), DESC_WORD(wDetachTimeOut), DESC_WORD(wTransferSize),
		DESC_VERSION(bcdDFUVersion) ),
	NULL
};

// --

static U32
CDC_Union_Interfaces( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Descriptor_List( out, data, length, offset, DESC_FIELDS( DESC_BYTE(bSubordinateInterface) ),
		length - offset ) );
}

static const struct ClassDescriptor CDCUnion =
{
	"CDC union", DESC_FIELDS( DESC_BYTE(bControlInterface) ), CDC_Union_Interfaces
};

// --

const struct ClassDescriptor *
Descriptor_Lookup( const struct ClassDescriptor *table, U32 count, U32 index )
{
	if ( ( index < count ) && ( table[index].Name ) )
	{
		return ( & table[index] );
	}

	return ( NULL );
}

// --

const struct ClassDescriptor *
Class_Descriptor( const U8 *interface, const U8 *data, U32 length )
{
	if ( ! interface )
	{
		return ( NULL );
	}

	switch ( interface[5] )
	{
		case USBCLASS_Audio:
			return ( Audio_Descriptor( interface, data, length ) );

		case USBCLASS_Video:
			return ( Video_Descriptor( interface, data, length ) );

		case USBCLASS_CDC_Control:
		case USBCLASS_CDC_Data:
			if ( ( data[1] == DESC_CLASS_INTERFACE ) && ( length >= 3 ) )
			{
				if ( data[2] == 0x06 )
				{
					return ( & CDCUnion );
				}

				return ( DESC_LOOKUP( CDCDescriptors, data[2] ) );
			}
			break;

		case USBCLASS_Smart_Card:
			if ( ( data[1] == 0x21 ) && ( interface[6] == 0 ) && ( interface[7] == 0 ) )
			{
				return ( & CCIDDescriptor );
			}
			break;

		case USBCLASS_Application_Specific:
			if ( ( data[1] == 0x21 ) && ( interface[6] == 1 ) &&
				( ( interface[7] == 1 ) || ( interface[7] == 2 ) ) )
			{
				return ( & DFUDescriptor );
			}
			break;
	}

	return ( NULL );
}
