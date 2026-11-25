
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

#define ACM_NO_INTERFACE	 256U
#define ACM_CS_INTERFACE	 0x24
#define ACM_CALL_MANAGEMENT	 0x01
#define ACM_ABSTRACT_CONTROL 0x02
#define ACM_UNION			 0x06

static U32
FindDataAlt ( const U8 * config, const U8 * end, U32 data_nr )
{
	const U8 * desc;

	U32 alt;
	U32 endpoints;
	U32 best;

	alt		  = ACM_NO_INTERFACE;
	endpoints = 0;
	best	  = ACM_NO_INTERFACE;

	for ( desc = config; desc < end; desc += desc[0] )
	{
		if ( desc[1] == DSCTYPE_Interface )
		{
			alt		  = ( ( desc[0] >= 9 ) && ( desc[2] == data_nr ) && ( desc[5] == USBCLASS_CDC_Data ) ) ? desc[3]
																										   : ACM_NO_INTERFACE;
			endpoints = 0;
		}
		else if ( ( alt < best ) && ( desc[1] == DSCTYPE_EndPoint ) && ( desc[0] >= 7 ) && ( desc[2] & 0x0f )
				  && ( ( desc[3] & EPATT_Type_Mask ) == EPATT_Type_Bulk ) && ( ( desc[4] ) || ( desc[5] ) ) )
		{
			endpoints |= ( desc[2] & EPADR_Dir_Mask ) ? 1 : 2;

			if ( endpoints == 3 )
			{
				best = alt;
			}
		}
	}

	return ( best );
}

// --

SEC_CODE S32
Serial_ACM_Parse ( struct SerialUnit * unit, U32 * interface, U32 * alt )
{
	struct USB2_DriverMessage * msg;
	const U8 *					config;
	const U8 *					end;
	const U8 *					desc;

	U32 size;
	U32 control;
	U32 data_nr;
	U32 call_nr;
	U32 iad_nr;
	U32 data_alt;
	S32 in_control;

	msg		= unit->StartMsg;
	config	= (PTR)msg->ConfigDescriptors;
	control = msg->InterfaceDescriptor->InterfaceNumber;

	if ( ( ! config ) || ( ! msg->Function )
		 || ( USB2_Attr_Get ( msg->Function->FKT_ID, USB2Tag_Attr_ConfigSize, &size, sizeof ( size ) ) != sizeof ( size ) )
		 || ( size < 9 ) || ( config[0] < 9 ) || ( config[1] != DSCTYPE_Configuration )
		 || ( size != ( (U32)config[2] | ( (U32)config[3] << 8 ) ) ) )
	{
		return ( FALSE );
	}

	end		   = config + size;
	data_nr	   = ACM_NO_INTERFACE;
	call_nr	   = ACM_NO_INTERFACE;
	iad_nr	   = ACM_NO_INTERFACE;
	in_control = FALSE;

	for ( desc = config; desc < end; desc += desc[0] )
	{
		if ( ( desc[0] < 2 ) || ( desc[0] > end - desc ) )
		{
			return ( FALSE );
		}

		if ( desc[1] == DSCTYPE_Interface )
		{
			in_control = ( desc[0] >= 9 ) && ( desc[2] == control ) && ( desc[3] == msg->InterfaceDescriptor->AlternateSetting )
						 && ( desc[5] == USBCLASS_CDC_Control ) && ( desc[6] == SERIAL_ACM_SUBCLASS );
		}
		else if ( ( desc[1] == DSCTYPE_Interface_Association ) && ( desc[0] >= 8 ) && ( desc[2] == control ) && ( desc[3] == 2 )
				  && ( desc[4] == USBCLASS_CDC_Control ) && ( desc[5] == SERIAL_ACM_SUBCLASS ) )
		{
			iad_nr = control + 1;
		}
		else if ( ( in_control ) && ( desc[1] == ACM_CS_INTERFACE ) && ( desc[0] >= 4 ) )
		{
			switch ( desc[2] )
			{
				case ACM_ABSTRACT_CONTROL:
					unit->ACMCapabilities = desc[3];
					break;

				case ACM_UNION:
					if ( ( desc[0] >= 5 ) && ( desc[3] == control ) )
					{
						data_nr = desc[4];
					}

					break;

				case ACM_CALL_MANAGEMENT:
					if ( desc[0] >= 5 )
					{
						call_nr = desc[4];
					}

					break;
			}
		}
	}

	if ( data_nr == ACM_NO_INTERFACE )
	{
		data_nr = ( iad_nr != ACM_NO_INTERFACE ) ? iad_nr : call_nr;
	}

	if ( ( data_nr == ACM_NO_INTERFACE ) || ( data_nr == control ) )
	{
		return ( FALSE );
	}

	data_alt = FindDataAlt ( config, end, data_nr );

	if ( data_alt == ACM_NO_INTERFACE )
	{
		return ( FALSE );
	}

	*interface = data_nr;
	*alt	   = data_alt;

	return ( TRUE );
}

// --
