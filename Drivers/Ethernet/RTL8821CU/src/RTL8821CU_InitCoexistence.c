
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "Registers.h"

// --

#define REG_COEX_OWNER 0x0073
#define REG_QUEUE_CTRL 0x04c6
#define REG_BCN_CTRL 0x0550
#define REG_COEX_TABLE_BT 0x06c0
#define REG_COEX_TABLE_WLAN 0x06c4
#define REG_COEX_BREAK_TABLE 0x06c8
#define REG_COEX_BCN_PRIORITY 0x06cf
#define REG_COEX_POLARITY 0x0762
#define REG_BT_STAT_CTRL 0x0778
#define REG_BT_TDMA_TIME 0x0790
#define REG_RFE_CTRL8 0x0cb4
#define COEX_GRANT_MASK 0x0000ff00U

// --

S32
RTL8821CU_InitCoexistence( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	U32 rfe;
	U32 grant;
	U32 antenna;

	rfe = unit->unit_RFOption & 0x1f;

	// These variants route 2.4 GHz Wi-Fi through the shared Bluetooth path.
	if ( ( rfe != 2 ) && ( rfe != 4 ) )
	{
		return ( TRUE );
	}

	control = unit->unit_USB_Register->Res_Control;
	antenna = rfe == 2 ? 2 : 1;

	if ( ( ! RTL8821CU_UpdateReg( control, REG_BCN_CTRL, 1, 0x08, 0x08 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_BT_TDMA_TIME, 1, 0x3f, 5 ) ) ||
		( ! RTL8821CU_WriteReg( control, REG_BT_STAT_CTRL, 1, 1 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_GPIO_MUX, 4, 0x220, 0x220 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_QUEUE_CTRL, 1, 0x30, 0x10 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_COEX_POLARITY, 2, 0x1000, 0x1000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_COEX_BCN_PRIORITY, 1, 8, 8 ) ) ||
		( ! RTL8821CU_WriteReg( control, REG_COEX_TABLE_BT, 4, 0x66555555 ) ) ||
		( ! RTL8821CU_WriteReg( control, REG_COEX_TABLE_WLAN, 4, 0x5a5a5a5a ) ) ||
		( ! RTL8821CU_WriteReg( control, REG_COEX_BREAK_TABLE, 4, 0xf0ffffff ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_LED_CFG, 4, 0x01800000, 0x01000000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_RFE_CTRL8, 4, 0xf00000ff, ( antenna << 28 ) | 0x77 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_COEX_OWNER, 1, 4, 4 ) ) ||
		( ! RTL8821CU_PollReg( control, REG_COEX_ACCESS, 4, COEX_READY, COEX_READY ) ) ||
		( ! RTL8821CU_WriteReg( control, REG_COEX_ACCESS, 4, COEX_READ_CONTROL ) ) ||
		( ! RTL8821CU_PollReg( control, REG_COEX_ACCESS, 4, COEX_READY, COEX_READY ) ) ||
		( ! RTL8821CU_ReadReg( control, REG_COEX_READ, 4, & grant ) ) )
	{
		return ( FALSE );
	}

	// Hardware arbitration owns both grants during 2.4 GHz operation.
	if ( ( ! RTL8821CU_WriteReg( control, REG_COEX_WRITE, 4, grant & ~COEX_GRANT_MASK ) ) ||
		( ! RTL8821CU_WriteReg( control, REG_COEX_ACCESS, 4, COEX_WRITE_CONTROL ) ) ||
		( ! RTL8821CU_PollReg( control, REG_COEX_ACCESS, 4, COEX_READY, COEX_READY ) ) )
	{
		return ( FALSE );
	}

	DebugPrintF( "RTL8821CU: shared antenna arbitration enabled, prior grant control $%08lx\n", grant );

	return ( TRUE );
}

// --
