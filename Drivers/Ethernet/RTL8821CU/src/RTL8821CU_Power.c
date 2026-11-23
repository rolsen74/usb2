
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Registers.h"

// --

enum PowerAction
{
	POWER_UPDATE,
	POWER_WAIT,
	POWER_DELAY
};

struct PowerStep
{
	U16 Register;
	U8 Mask;
	U8 Value;
	enum PowerAction Action;
};

// --

/*
 * USB power transition tables adapted from Linux rtw88/rtw8821c.c
 * under its BSD-3-Clause terms.
 *
 * Copyright (c) 2018-2019 Realtek Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
static const struct PowerStep PowerOn[] =
{
	{ 0x004a, 0x01, 0x00, POWER_UPDATE },
	{ 0x0005, 0x98, 0x00, POWER_UPDATE },
	{ 0x0020, 0x01, 0x01, POWER_UPDATE },
	{ 0x0000, 0x00, 0x00, POWER_DELAY },
	{ 0x0000, 0x20, 0x00, POWER_UPDATE },
	{ 0x0005, 0x1c, 0x00, POWER_UPDATE },
	{ 0x0006, 0x02, 0x02, POWER_WAIT },
	{ 0x0006, 0x01, 0x01, POWER_UPDATE },
	{ 0x0005, 0x80, 0x00, POWER_UPDATE },
	{ 0x0005, 0x18, 0x00, POWER_UPDATE },
	{ 0x10c3, 0x01, 0x01, POWER_UPDATE },
	{ 0x0005, 0x01, 0x01, POWER_UPDATE },
	{ 0x0005, 0x01, 0x00, POWER_WAIT },
	{ 0x0020, 0x08, 0x08, POWER_UPDATE },
	{ 0x007c, 0x02, 0x00, POWER_UPDATE }
};

static const struct PowerStep PowerOff[] =
{
	{ 0x0093, 0x08, 0x00, POWER_UPDATE },
	{ 0x001f, 0xff, 0x00, POWER_UPDATE },
	{ 0x0049, 0x02, 0x00, POWER_UPDATE },
	{ 0x0006, 0x01, 0x01, POWER_UPDATE },
	{ 0x0002, 0x02, 0x00, POWER_UPDATE },
	{ 0x10c3, 0x01, 0x00, POWER_UPDATE },
	{ 0x0005, 0x02, 0x02, POWER_UPDATE },
	{ 0x0005, 0x02, 0x00, POWER_WAIT },
	{ 0x0020, 0x08, 0x00, POWER_UPDATE },
	{ 0x0000, 0x20, 0x20, POWER_UPDATE },
	{ 0x0007, 0xff, 0x20, POWER_UPDATE },
	{ 0x0067, 0x20, 0x00, POWER_UPDATE },
	{ 0x004a, 0x01, 0x00, POWER_UPDATE },
	{ 0x0081, 0xc0, 0x00, POWER_UPDATE },
	{ 0x0005, 0x18, 0x08, POWER_UPDATE },
	{ 0x0090, 0x02, 0x00, POWER_UPDATE }
};

// --

static S32
RunPowerSequence( struct USB2_EPResource *control, const struct PowerStep *step, U32 count )
{
	for( ; count; step++, count-- )
	{
		switch( step->Action )
		{
			case POWER_UPDATE:
				if ( ! RTL8821CU_UpdateReg( control, step->Register, 1, step->Mask, step->Value ) )
				{
					return ( FALSE );
				}
				break;

			case POWER_WAIT:
				if ( ! RTL8821CU_PollReg( control, step->Register, 1, step->Mask, step->Value ) )
				{
					return ( FALSE );
				}
				break;

			case POWER_DELAY:
				Delay( 1 );

				break;
		}
	}

	return ( TRUE );
}

// --

static S32
PrepareMAC( struct USB2_EPResource *control )
{
	return ( ( RTL8821CU_WriteReg( control, REG_RSV_CTRL, 1, 0 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_PAD_CTRL, 4, 0x30000000, 0x30000000 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_LED_CFG, 4, 0x06000000, 0 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_GPIO_MUX, 4, 4, 4 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_SYS_FUNC_EN, 1, 3, 0 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_RF_CTRL, 1, 7, 0 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_WLRF1, 4, 0x07000000, 0 ) ) );
}

// --

S32
RTL8821CU_PowerOn( struct USB2_EPResource *control )
{
	U32 mac;
	U32 status;
	U32 firmware;
	U32 rpwm;

	if ( ( ! PrepareMAC( control ) ) ||
		( ! RTL8821CU_ReadReg( control, REG_MCUFW_CTRL, 2, & firmware ) ) )
	{
		return ( FALSE );
	}

	if ( firmware == FW_READY )
	{
		if ( ( ! RTL8821CU_ReadReg( control, 0xfe58, 1, & rpwm ) ) ||
			( ! RTL8821CU_WriteReg( control, 0xfe58, 1, ( rpwm ^ 0x80 ) & 0x80 ) ) )
		{
			return ( FALSE );
		}
	}

	if ( ( ! RTL8821CU_ReadReg( control, REG_CR, 1, & mac ) ) ||
		( ! RTL8821CU_ReadReg( control, REG_SYS_STATUS1 + 1, 1, & status ) ) )
	{
		return ( FALSE );
	}

	if ( ( mac != 0xea ) && ( ! ( status & 1 ) ) )
	{
		if ( ( ! RTL8821CU_PowerOff( control ) ) || ( ! PrepareMAC( control ) ) )
		{
			return ( FALSE );
		}
	}

	if ( ( ! RunPowerSequence( control, PowerOn, sizeof( PowerOn ) / sizeof( PowerOn[0] ) ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_SYS_STATUS1 + 1, 1, 1, 0 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_CPU_DMEM_CON, 4, 0x00010100, 0x00010100 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_SYS_FUNC_EN + 1, 1, 0xd8, 0xd8 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_CR_EXT + 3, 1, 0x0f, 0x0c ) ) ||
		( ! RTL8821CU_ReadReg( control, REG_MCUFW_CTRL, 4, & firmware ) ) )
	{
		return ( FALSE );
	}

	if ( firmware & 0x00100000 )
	{
		return ( ( RTL8821CU_WriteReg( control, REG_MCUFW_CTRL, 4, firmware & ~0x00100000U ) ) &&
			( RTL8821CU_UpdateReg( control, REG_GPIO_MUX, 4, 0x00080000, 0 ) ) );
	}

	return ( TRUE );
}

// --

S32
RTL8821CU_PowerOff( struct USB2_EPResource *control )
{
	return ( RunPowerSequence( control, PowerOff, sizeof( PowerOff ) / sizeof( PowerOff[0] ) ) );
}

// --
