
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Registers.h"

// --

#define COEX_READY 0x20000000U
#define COEX_READ_CONTROL 0x800f0038U
#define COEX_WRITE_CONTROL 0xc00f0038U
#define LOAD_QUEUE_PAGES 0x80000000U
#define H2C_QUEUE_FULL 0x80000000U

// --

// The page allocation command must be restored after the page count.
static const struct
{
	U16 Address;
	U16 Width;
	U32 SetOnRestore;
} SavedRegisters[] =
{
	{ REG_TXDMA_PRIORITY, 1, 0 },
	{ REG_CR, 1, 0 },
	{ REG_CR + 1, 1, 0 },
	{ REG_FIFOPAGE_INFO_1, 2, 0 },
	{ REG_RQPN_CTRL_2, 4, LOAD_QUEUE_PAGES },
	{ REG_BCN_CTRL, 1, 0 },
	{ REG_FIFOPAGE_CTRL_2, 2, BCN_PAGE_VALID }
};

#define SAVED_REGISTER_COUNT ( sizeof( SavedRegisters ) / sizeof( SavedRegisters[0] ) )

// --

static S32
SetCPU( struct USB2_EPResource *control, S32 enable )
{
	if ( enable )
	{
		return ( ( RTL8821CU_UpdateReg( control, REG_RSV_CTRL + 1, 1, 1, 1 ) ) &&
			( RTL8821CU_UpdateReg( control, REG_SYS_FUNC_EN + 1, 1, 4, 4 ) ) );
	}

	return ( ( RTL8821CU_UpdateReg( control, REG_SYS_FUNC_EN + 1, 1, 4, 0 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_RSV_CTRL + 1, 1, 1, 0 ) ) );
}

// --

static S32
PrepareDownload( struct USB2_EPResource *control )
{
	return ( ( RTL8821CU_WriteReg( control, REG_TXDMA_PRIORITY, 1, 0xc0 ) ) &&
		( RTL8821CU_WriteReg( control, REG_CR, 1, 0x05 ) ) &&
		( RTL8821CU_WriteReg( control, REG_H2CQ_CSR, 4, H2C_QUEUE_FULL ) ) &&
		( RTL8821CU_WriteReg( control, REG_FIFOPAGE_INFO_1, 2, 0x0200 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_RQPN_CTRL_2, 4, LOAD_QUEUE_PAGES, LOAD_QUEUE_PAGES ) ) &&
		( RTL8821CU_UpdateReg( control, REG_BCN_CTRL, 1, 0x18, 0x10 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_CR + 1, 1, 1, 1 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_CPU_DMEM_CON + 2, 1, 1, 0 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_SYS_CLK_CTRL + 1, 1, 0x40, 0 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_CPU_DMEM_CON + 2, 1, 1, 1 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_SYS_CLK_CTRL + 1, 1, 0x40, 0x40 ) ) &&
		( RTL8821CU_UpdateReg( control, REG_MCUFW_CTRL, 2, 0xc7ff, FW_DOWNLOAD_ENABLE ) ) );
}

// --

S32
RTL8821CU_DownloadFirmware( struct EthernetUnit *unit, const struct FirmwareSection *sections, U32 count )
{
	struct USB2_EPResource *control;

	U32 saved[SAVED_REGISTER_COUNT];
	U32 coex;
	U32 index;
	S32 ready;
	S32 coex_restored;

	control = unit->unit_USB_Register->Res_Control;

	if ( ( ! RTL8821CU_PollReg( control, REG_COEX_ACCESS, 4, COEX_READY, COEX_READY ) ) ||
		( ! RTL8821CU_WriteReg( control, REG_COEX_ACCESS, 4, COEX_READ_CONTROL ) ) ||
		( ! RTL8821CU_ReadReg( control, REG_COEX_READ, 4, & coex ) ) )
	{
		return ( FALSE );
	}

	for( index = 0; index < SAVED_REGISTER_COUNT; index++ )
	{
		if ( ! RTL8821CU_ReadReg( control, SavedRegisters[index].Address, SavedRegisters[index].Width, & saved[index] ) )
		{
			return ( FALSE );
		}
	}

	ready = FALSE;

	if ( ( SetCPU( control, FALSE ) ) && ( PrepareDownload( control ) ) )
	{
		ready = TRUE;

		for( index = 0; index < count; index++ )
		{
			if ( ! RTL8821CU_WriteFirmwareSection( unit, & sections[index] ) )
			{
				DebugPrintF( "RTL8821CU: firmware transfer to $%08lx failed\n", sections[index].Address );

				ready = FALSE;

				break;
			}
		}
	}

	for( index = 0; index < SAVED_REGISTER_COUNT; index++ )
	{
		if ( ! RTL8821CU_WriteReg( control, SavedRegisters[index].Address, SavedRegisters[index].Width,
			saved[index] | SavedRegisters[index].SetOnRestore ) )
		{
			ready = FALSE;
		}
	}

	if ( ! RTL8821CU_WriteReg( control, REG_H2CQ_CSR, 4, H2C_QUEUE_FULL ) )
	{
		ready = FALSE;
	}

	if ( ready )
	{
		ready = ( RTL8821CU_WriteReg( control, REG_TXDMA_STATUS, 4, 4 ) ) &&
			( RTL8821CU_UpdateReg( control, REG_MCUFW_CTRL, 2, FW_DOWNLOAD_ENABLE | FW_DOWNLOAD_READY, FW_DOWNLOAD_READY ) ) &&
			( SetCPU( control, TRUE ) );
	}

	coex_restored = ( RTL8821CU_PollReg( control, REG_COEX_ACCESS, 4, COEX_READY, COEX_READY ) ) &&
		( RTL8821CU_WriteReg( control, REG_COEX_WRITE, 4, coex ) ) &&
		( RTL8821CU_WriteReg( control, REG_COEX_ACCESS, 4, COEX_WRITE_CONTROL ) );

	if ( ( ready ) && ( coex_restored ) && ( RTL8821CU_PollReg( control, REG_MCUFW_CTRL, 2, 0xcfff, FW_READY ) ) )
	{
		return ( TRUE );
	}

	RTL8821CU_UpdateReg( control, REG_MCUFW_CTRL, 1, FW_DOWNLOAD_ENABLE, 0 );

	return ( FALSE );
}

// --
