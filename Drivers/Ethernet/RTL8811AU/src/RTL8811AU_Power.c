
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

/*-
 * Copyright (c) 2016 Andriy Voskoboinyk <avos@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

// --

#define HOST_SUSPEND 0x08
#define PCIE_SUSPEND 0x10
#define HARD_POWER_DOWN 0x80
#define RESUME 0x04
#define MAC_ON 0x01
#define MAC_OFF 0x02
#define HOST_POWER_READY 0x20000U
#define MAC_READY 0x01

// --

S32
RTL8811AU_PowerOn( struct USB2_EPResource *control )
{
	U32 config;

	if ( ( ! RTL8811AU_Update8( control, RTL8811AU_POWER + 1, HOST_SUSPEND | HARD_POWER_DOWN, 0 ) ) ||
		( ! RTL8811AU_Update8( control, 0x004a, 1, 0 ) ) ||
		( ! RTL8811AU_Update8( control, RTL8811AU_POWER + 1, HOST_SUSPEND | PCIE_SUSPEND, 0 ) ) ||
		( ! RTL8811AU_Update8( control, RTL8811AU_LDO, 0, 1 ) ) ||
		( ! RTL8811AU_Update8( control, 0x0067, 0x10, 0 ) ) )
	{
		return ( FALSE );
	}

	Delay( 1 );

	if ( ( ! RTL8811AU_Update8( control, RTL8811AU_SYS_ISO, 0x20, 0 ) ) ||
		( ! RTL8811AU_Update8( control, RTL8811AU_POWER + 1, RESUME | HOST_SUSPEND | PCIE_SUSPEND, 0 ) ) ||
		( ! RTL8811AU_PollReg( control, RTL8811AU_POWER, 4, HOST_POWER_READY, HOST_POWER_READY, 25, NULL ) ) ||
		( ! RTL8811AU_Update8( control, RTL8811AU_POWER + 2, 0, MAC_READY ) ) ||
		( ! RTL8811AU_Update8( control, RTL8811AU_POWER + 1, HARD_POWER_DOWN | HOST_SUSPEND | PCIE_SUSPEND, 0 ) ) ||
		( ! RTL8811AU_Update8( control, RTL8811AU_POWER + 1, 0, MAC_ON ) ) ||
		( ! RTL8811AU_PollReg( control, RTL8811AU_POWER + 1, 1, MAC_ON, 0, 25, NULL ) ) )
	{
		return ( FALSE );
	}

	if ( ( ! RTL8811AU_Update8( control, 0x004f, 0, 1 ) ) ||
		( ! RTL8811AU_Update8( control, 0x0067, 0, 0x30 ) ) ||
		( ! RTL8811AU_Update8( control, 0x0025, 0x40, 0 ) ) ||
		( ! RTL8811AU_Update8( control, 0x0049, 0, 2 ) ) ||
		( ! RTL8811AU_Update8( control, 0x0063, 0, 2 ) ) ||
		( ! RTL8811AU_Update8( control, 0x0062, 2, 0 ) ) ||
		( ! RTL8811AU_Update8( control, 0x0058, 0, 1 ) ) ||
		( ! RTL8811AU_Update8( control, 0x005a, 0, 2 ) ) ||
		( ! RTL8811AU_WriteReg( control, 0x002e, 1, 0x82 ) ) ||
		( ! RTL8811AU_Update8( control, 0x0010, 0, 0x40 ) ) ||
		( ! RTL8811AU_WriteReg( control, RTL8811AU_MAC_CTRL, 2, 0 ) ) ||
		( ! RTL8811AU_WriteReg( control, RTL8811AU_RF_CTRL, 1, 0 ) ) ||
		( ! RTL8811AU_WriteReg( control, RTL8811AU_TX_PAUSE, 1, 0xff ) ) ||
		( ! RTL8811AU_WriteReg( control, RTL8811AU_MAC_CTRL, 2, 0x043f ) ) ||
		( ! RTL8811AU_ReadReg( control, RTL8811AU_SYS_CFG, 4, &config ) ) )
	{
		return ( FALSE );
	}

	if ( config & 0x01000000 )
	{
		return ( RTL8811AU_Update8( control, 0x007c, 0, 0x40 ) );
	}

	return ( TRUE );
}

// --

void
RTL8811AU_PowerOff( struct USB2_EPResource *control )
{
	if ( ! RTL8811AU_WriteReg( control, RTL8811AU_MAC_CTRL, 1, 0 ) )
	{
		return;
	}

	RTL8811AU_WriteReg( control, RTL8811AU_TX_PAUSE, 1, 0xff );

	RTL8811AU_Update8( control, RTL8811AU_SYS_FUNC + 1, 0x04, 0 );

	RTL8811AU_WriteReg( control, RTL8811AU_FW_CTRL, 1, 0 );

	RTL8811AU_WriteReg( control, RTL8811AU_RF_CTRL, 1, 0 );

	RTL8811AU_WriteReg( control, 0x0076, 1, 0 );

	RTL8811AU_Update8( control, RTL8811AU_SYS_FUNC, 0x03, 0 );

	RTL8811AU_Update8( control, 0x004f, 1, 0 );

	RTL8811AU_Update8( control, 0x0049, 2, 0 );

	RTL8811AU_Update8( control, RTL8811AU_POWER + 2, 0, MAC_READY );

	if ( ( ! RTL8811AU_Update8( control, RTL8811AU_POWER + 1, 0, MAC_OFF ) ) ||
		( ! RTL8811AU_PollReg( control, RTL8811AU_POWER + 1, 1, MAC_OFF, 0, 10, NULL ) ) )
	{
		return;
	}

	RTL8811AU_Update8( control, RTL8811AU_SYS_ISO, 0, 0x20 );

	RTL8811AU_Update8( control, RTL8811AU_LDO, 1, 0 );

	RTL8811AU_Update8( control, RTL8811AU_POWER + 1, PCIE_SUSPEND, HOST_SUSPEND );

	RTL8811AU_Update8( control, 0x004a, 0, 1 );
}

// --
