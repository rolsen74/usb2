
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_CODE static S32
AddField( struct USBBase *usbbase, struct HIDData *hd, struct pintern *pi, enum HID_ReportType type )
{
	struct HID_ReportNode *rn;
	struct HID_ReportLayout *layout;
	struct HID_BitNode *bn;
	struct HID_UsageNode *un;

	U64 size;
	U32 id;
	U32 offset;

	rn = pi->rn;
	id = pi->gn->ReportID;
	size = (U64) pi->gn->ReportCount * pi->gn->ReportSize;

	if ( ( ! rn ) || ( ! size ) || ( size > HID_MAX_REPORT_BYTES * 8 ) )
	{
		return ( TRUE );
	}

	layout = hd->Layouts[id];

	if ( ! layout )
	{
		layout = MEM_ALLOCVEC( sizeof( *layout ), TRUE );

		if ( ! layout )
		{
			hd->AllocationFailed = TRUE;

			return ( TRUE );
		}

		hd->Layouts[id] = layout;
	}

	offset = layout->Bits[type];

	if ( ! offset )
	{
		offset = id ? 8 : 0;
		layout->Owner[type] = rn;
	}

	if ( ( layout->Owner[type] != rn ) || ( offset + size > HID_MAX_REPORT_BYTES * 8 ) )
	{
		return ( TRUE );
	}

	layout->Bits[type] = offset + size;

	// Retain decoded fields while counting the space used by padding.
	if ( ( type == HID_Report_Feature ) || ( pi->uval & 0x101 ) || ( ! pi->usages.uh_Head ) )
	{
		return ( FALSE );
	}

	if ( pi->gn->ReportSize > 32 )
	{
		return ( TRUE );
	}

	bn = Node_Bit_Alloc( usbbase );

	if ( ! bn )
	{
		hd->AllocationFailed = TRUE;

		return ( TRUE );
	}

	bn->BitOffset = offset;
	bn->BitCount = pi->gn->ReportCount;
	bn->BitSize = pi->gn->ReportSize;
	bn->ReportID = id;
	bn->Flags = pi->uval;
	bn->MinLogical = pi->gn->MinLogical;
	bn->MaxLogical = pi->gn->MaxLogical;

	while ( ( un = NODE_REMHEAD( &pi->usages ) ) )
	{
		NODE_ADDTAIL( &bn->UsageHeader, un );
	}

	NODE_ADDTAIL( ( type == HID_Report_Input ) ? &rn->BitHeader : &rn->OutputHeader, bn );

	return ( FALSE );
}

// --

SEC_CODE S32
Report_Parse_Main( struct USBBase *usbbase, struct HIDData *hd, struct pintern *pi )
{
	struct HID_UsageNode *un;

	S32 error;

	error = FALSE;

	switch ( pi->bTag )
	{
		case 8: // Input
			error = AddField( usbbase, hd, pi, HID_Report_Input );
			break;

		case 9: // Output
			error = AddField( usbbase, hd, pi, HID_Report_Output );
			break;

		case 11: // Feature
			error = AddField( usbbase, hd, pi, HID_Report_Feature );
			break;

		case 10: // Collection
			if ( ! pi->level )
			{
				un = Header_Head( &pi->usages );

				if ( ! un )
				{
					return ( TRUE );
				}

				pi->rn = Node_Report_Alloc( usbbase, hd );

				if ( ! pi->rn )
				{
					hd->AllocationFailed = TRUE;

					return ( TRUE );
				}

				pi->rn->UsageID = un->Min;

				NODE_ADDTAIL( &pi->reports, pi->rn );
			}

			pi->level++;
			break;

		case 12: // End Collection
			if ( ! pi->level )
			{
				return ( TRUE );
			}

			if ( ! --pi->level )
			{
				pi->rn = NULL;
			}
			break;
	}

	while ( ( un = NODE_REMHEAD( &pi->usages ) ) )
	{
		Node_Usage_Free( usbbase, un );
	}

	pi->usageflag = 0;

	return ( error );
}

// --
