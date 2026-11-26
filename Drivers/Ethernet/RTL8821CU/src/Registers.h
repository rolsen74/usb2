
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8821CU_REGISTERS_H
#define RTL8821CU_REGISTERS_H

#define REG_SYS_FUNC_EN 0x0002
#define REG_SYS_CLK_CTRL 0x0008
#define REG_RSV_CTRL 0x001c
#define REG_RF_CTRL 0x001f
#define REG_EFUSE_CTRL 0x0030
#define REG_LDO_EFUSE_CTRL 0x0034
#define REG_GPIO_MUX 0x0040
#define REG_LED_CFG 0x004c
#define REG_PAD_CTRL 0x0064
#define REG_MCUFW_CTRL 0x0080
#define REG_WLRF1 0x00ec
#define REG_SYS_STATUS1 0x00f4
#define REG_CR 0x0100
#define REG_TXDMA_PRIORITY 0x010d
#define REG_FIFOPAGE_CTRL_2 0x0204
#define REG_TXDMA_STATUS 0x0210
#define REG_RQPN_CTRL_2 0x022c
#define REG_FIFOPAGE_INFO_1 0x0230
#define REG_TXPKT_EMPTY 0x041a
#define REG_HWSEQ_CTRL 0x0423
#define REG_PRECNT_CTRL 0x04e5
#define REG_TXPAUSE 0x0522
#define REG_ACKTO_CCK 0x0639
#define REG_BCN_CTRL 0x0550
#define REG_CPU_DMEM_CON 0x1080
#define REG_CR_EXT 0x1100

#define REG_DDMA_SOURCE 0x1200
#define REG_DDMA_DEST 0x1204
#define REG_DDMA_CONTROL 0x1208
#define REG_H2CQ_CSR 0x1330
#define REG_COEX_ACCESS 0x1700
#define REG_COEX_WRITE 0x1704
#define REG_COEX_READ 0x1708
#define COEX_READY 0x20000000U
#define COEX_READ_CONTROL 0x800f0038U
#define COEX_WRITE_CONTROL 0xc00f0038U

#define BCN_PAGE_VALID 0x8000
#define FW_INIT_READY 0x8000
#define FW_DOWNLOAD_READY 0x4000
#define FW_DOWNLOAD_ENABLE 0x0001
#define FW_DMEM_VALID 0x0060
#define FW_IMEM_VALID 0x0018
#define FW_READY ( FW_INIT_READY | FW_DOWNLOAD_READY | FW_DMEM_VALID | FW_IMEM_VALID )

S32 RTL8821CU_ReadReg( struct USB2_EPResource *control, U16 address, U16 width, U32 *value );
S32 RTL8821CU_WriteReg( struct USB2_EPResource *control, U16 address, U16 width, U32 value );
S32 RTL8821CU_UpdateReg( struct USB2_EPResource *control, U16 address, U16 width, U32 mask, U32 value );
S32 RTL8821CU_PollReg( struct USB2_EPResource *control, U16 address, U16 width, U32 mask, U32 wanted );

#endif
