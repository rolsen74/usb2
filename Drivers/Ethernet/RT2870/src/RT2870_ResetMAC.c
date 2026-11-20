
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RT2870_DEVICE_MODE		0x01
#define RT2870_MODE_RESET		0x01
#define RT2870_USB_DMA_CONFIG		0x02a0
#define RT2870_PBF_SYS_CTRL		0x0400
#define RT2870_PBF_PME_OUTPUT		0x00002000
#define RT2870_MAC_SYS_CTRL		0x1004
#define RT2870_MAC_RESET_CSR		0x00000001
#define RT2870_MAC_RESET_BBP		0x00000002

// --

S32
RT2870_ResetMAC( struct USB2_EPResource *control )
{
	U32 value;
	S32 error;
	S32 releaseError;

	error = RT2870_ReadRegister( control, RT2870_PBF_SYS_CTRL, & value );

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_PBF_SYS_CTRL, value & ~RT2870_PBF_PME_OUTPUT );
	}

	if ( error )
	{
		return ( error );
	}

	error = RT2870_WriteRegister( control, RT2870_MAC_SYS_CTRL, RT2870_MAC_RESET_CSR | RT2870_MAC_RESET_BBP );

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_USB_DMA_CONFIG, 0 );
	}

	if ( ! error )
	{
		error = RT2870_Control( control, REQTYPE_Write, RT2870_DEVICE_MODE, RT2870_MODE_RESET, 0, NULL, 0 );
	}

	// Release reset even if a later request failed after asserting it.
	releaseError = RT2870_WriteRegister( control, RT2870_MAC_SYS_CTRL, 0 );

	return ( error ? error : releaseError );
}

// --
