
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

/*
 * RTL8821C setup adapted from Linux rtw88 RTL8821C sources
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

#define TX_PAGE_SIZE 128U
#define TX_PAGES ( 65536U / TX_PAGE_SIZE )
#define FW_TX_PAGES 4U
#define H2C_QUEUE_PAGES 8U
#define TX_BOUNDARY ( TX_PAGES - FW_TX_PAGES - RTL8821CU_FW_TX_OFFSET_PAGES )
#define H2C_START ( ( TX_PAGES - FW_TX_PAGES - H2C_QUEUE_PAGES ) * TX_PAGE_SIZE )
#define H2C_END ( H2C_START + H2C_QUEUE_PAGES * TX_PAGE_SIZE )
#define QUEUE_PAGES 16U
#define PUBLIC_PAGES ( TX_BOUNDARY - 3 * QUEUE_PAGES - 1 )
#define RX_BOUNDARY ( 16384U - 256U - 1 )
#define RXDMA_MODE 0x0290
#define RXDMA_BURST_MODE 0x0e
#define RXDMA_BURST_1024 0x00
#define RXDMA_BURST_512 0x10
#define RXDMA_BURST_64 0x20

// --

S32
RTL8821CU_InitDMA( struct USB2_EPResource *control, U32 max_packet )
{
	U32 burst;

	burst = RXDMA_BURST_64;

	if ( max_packet >= 1024 )
	{
		burst = RXDMA_BURST_1024;
	}
	else if ( max_packet >= 512 )
	{
		burst = RXDMA_BURST_512;
	}

	// Three pipes: high for management, normal for voice/video, low for data.
	return ( ( RTL8821CU_WriteReg( control, 0x010c, 2, 0xf5a1 ) ) &&
		( RTL8821CU_WriteReg( control, REG_CR, 1, 0 ) ) &&
		( RTL8821CU_WriteReg( control, REG_CR, 1, 0xff ) ) &&
		( RTL8821CU_WriteReg( control, REG_H2CQ_CSR, 4, 0x80000000 ) ) &&
		( RTL8821CU_WriteReg( control, 0x0230, 2, QUEUE_PAGES ) ) &&
		( RTL8821CU_WriteReg( control, 0x0234, 2, QUEUE_PAGES ) ) &&
		( RTL8821CU_WriteReg( control, 0x0238, 2, QUEUE_PAGES ) ) &&
		( RTL8821CU_WriteReg( control, 0x023c, 2, 0 ) ) &&
		( RTL8821CU_WriteReg( control, 0x0240, 2, PUBLIC_PAGES ) ) &&
		( RTL8821CU_UpdateReg( control, REG_RQPN_CTRL_2, 4, 0, 0x80000000 ) ) &&
		( RTL8821CU_WriteReg( control, 0x0204, 2, TX_BOUNDARY ) ) &&
		( RTL8821CU_WriteReg( control, 0x0206, 2, TX_BOUNDARY ) ) &&
		( RTL8821CU_WriteReg( control, 0x0424, 2, TX_BOUNDARY ) ) &&
		( RTL8821CU_WriteReg( control, 0x0456, 2, TX_BOUNDARY ) ) &&
		( RTL8821CU_UpdateReg( control, 0x0422, 1, 0, 0x10 ) ) &&
		( RTL8821CU_WriteReg( control, 0x011c, 4, RX_BOUNDARY ) ) &&
		( RTL8821CU_UpdateReg( control, 0x0208, 1, 0xf0, 0x31 ) ) &&
		( RTL8821CU_WriteReg( control, 0x020b, 1, 3 ) ) &&
		( RTL8821CU_PollReg( control, 0x0208, 1, 1, 0 ) ) &&
		( RTL8821CU_WriteReg( control, REG_CR + 3, 1, 0 ) ) &&
		( RTL8821CU_UpdateReg( control, 0x0244, 4, 0x0003ffff, H2C_START ) ) &&
		( RTL8821CU_UpdateReg( control, 0x024c, 4, 0x0003ffff, H2C_START ) ) &&
		( RTL8821CU_UpdateReg( control, 0x0248, 4, 0x0003ffff, H2C_END ) ) &&
		( RTL8821CU_UpdateReg( control, 0x0254, 1, 3, 5 ) ) &&
		( RTL8821CU_UpdateReg( control, 0x020d, 1, 0, 0x82 ) ) &&
		( RTL8821CU_WriteReg( control, RXDMA_MODE, 1, RXDMA_BURST_MODE | burst ) ) );
}

// --
