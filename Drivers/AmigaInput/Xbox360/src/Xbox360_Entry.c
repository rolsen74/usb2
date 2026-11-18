
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Xbox360.h"

// --

/*
 * FreeBSD xb360gp interface match, copyright 2020 Vladimir Kondratyev and Val Packett.
 * See LICENSE.FreeBSD for the BSD-2-Clause terms.
 */

// --

U32
Xbox360_Entry( struct USB2DriverIFace *Self UNUSED, struct USB2_DriverMessage *msg )
{
	struct USB2_Interface_Desc *desc;
	struct USB2IFace *usb;
	struct USB2_Register *reg;
	struct USB2_EPResource *input;

	S32 ready;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) ||
		( ! msg->InterfaceDescriptor ) )
	{
		return ( FALSE );
	}

	desc = msg->InterfaceDescriptor;
	usb = msg->IUSB2;
	ready = FALSE;

	// FreeBSD matches the wired gamepad by interface class, subclass, and protocol.
	if ( ( desc->InterfaceClass != USBCLASS_Vendor_Specific ) ||
		( desc->InterfaceSubClass != XBOX360_SUBCLASS ) ||
		( desc->InterfaceProtocol != XBOX360_PROTOCOL ) || ( desc->AlternateSetting != 0 ) )
	{
		return ( FALSE );
	}

	reg = usb->USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, "Xbox 360 wired controller",
		TAG_END );

	if ( ! reg )
	{
		DebugPrintF( "Xbox360: could not register controller interface\n" );

		return ( FALSE );
	}

	input = usb->USB2_EPRes_ObtainTags( reg,
		USB2Tag_EPRes_InterfaceNr, desc->InterfaceNumber,
		USB2Tag_EPRes_EPType, EPATT_Type_Interrupt,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_NrOfIORequest, 1,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END );

	if ( input )
	{
		if ( input->BufferSize >= XBOX360_REPORT_SIZE )
		{
			ready = TRUE;

			Xbox360_Poll( usb, reg, input, desc->InterfaceNumber );
		}

		usb->USB2_EPRes_Release( input );
	}

	if ( ! ready )
	{
		DebugPrintF( "Xbox360: no interrupt IN endpoint for 20-byte reports\n" );
	}

	usb->USB2_Reg_Unregister( reg );

	return ( ready );
}

// --
