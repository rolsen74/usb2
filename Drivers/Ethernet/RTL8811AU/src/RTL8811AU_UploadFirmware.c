
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define FW_ENABLE  0x01
#define FW_READY   0x02
#define FW_CHECK   0x04
#define FW_RUNNING 0x40
#define FW_RAM     0x80
#define FW_WINDOW  0x1000
#define FW_PAGE    4096U

// --

static S32
WritePages( struct USB2_EPResource *control, U8 *image, U32 length )
{
	U32 page;
	U32 offset;
	U32 remaining;
	U16 chunk;

	for( page = 0; length; page++ )
	{
		if ( ! RTL8811AU_Update8( control, RTL8811AU_FW_CTRL + 2, 0x07, page ) )
		{
			return ( FALSE );
		}

		remaining = length < FW_PAGE ? length : FW_PAGE;

		for( offset = 0; remaining; offset += chunk )
		{
			chunk = remaining >= 196 ? 196 : ( remaining >= 4 ? 4 : 1 );

			if ( ! RTL8811AU_RegisterIO( control, CMD_WRITE, FW_WINDOW + offset, image, chunk ) )
			{
				return ( FALSE );
			}

			remaining -= chunk;
			length -= chunk;
			image += chunk;
		}
	}

	return ( TRUE );
}

// --

S32
RTL8811AU_UploadFirmware( struct USB2_EPResource *control, U8 *image, U32 size )
{
	U32 status;
	S32 uploaded;
	S32 disabled;

	if ( ( ! size ) || ( size > 8 * FW_PAGE ) )
	{
		return ( FALSE );
	}

	if ( ! RTL8811AU_ReadReg( control, RTL8811AU_FW_CTRL, 1, &status ) )
	{
		return ( FALSE );
	}

	if ( ( status & FW_RAM ) &&
		( ( ! RTL8811AU_WriteReg( control, RTL8811AU_FW_CTRL, 1, 0 ) ) ||
		  ( ! RTL8811AU_ResetMCU( control ) ) ) )
	{
		return ( FALSE );
	}

	uploaded = ( RTL8811AU_Update8( control, RTL8811AU_FW_CTRL, 0, FW_ENABLE ) ) &&
		( RTL8811AU_Update8( control, RTL8811AU_FW_CTRL + 2, 0x08, 0 ) ) &&
		( RTL8811AU_Update8( control, RTL8811AU_FW_CTRL, 0, FW_CHECK ) ) &&
		( WritePages( control, image, size ) ) &&
		( RTL8811AU_PollReg( control, RTL8811AU_FW_CTRL, 1, FW_CHECK, FW_CHECK, 50, NULL ) );

	disabled = RTL8811AU_Update8( control, RTL8811AU_FW_CTRL, FW_ENABLE, 0 );

	return ( ( uploaded ) && ( disabled ) &&
		( RTL8811AU_Update8( control, RTL8811AU_FW_CTRL, FW_RUNNING, FW_READY ) ) &&
		( RTL8811AU_ResetMCU( control ) ) &&
		( RTL8811AU_PollReg( control, RTL8811AU_FW_CTRL, 1, FW_RUNNING, FW_RUNNING, 50, NULL ) ) );
}

// --
