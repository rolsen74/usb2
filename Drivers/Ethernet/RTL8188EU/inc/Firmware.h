
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8188EU_FIRMWARE_H
#define RTL8188EU_FIRMWARE_H

// --

#define RTL8188EU_FIRMWARE_PATH "DEVS:Firmware/rtl8188eufw.bin"
#define RTL8188EU_FW_HEADER_SIZE 32
#define RTL8188EU_FW_PAGE_SIZE 4096
#define RTL8188EU_FW_PAGE_MASK 7
#define RTL8188EU_FW_MAX_SIZE ( ( RTL8188EU_FW_PAGE_MASK + 1 ) * RTL8188EU_FW_PAGE_SIZE )
#define RTL8188EU_FW_RAM 0x1000
#define RTL8188EU_FW_CTL 0x0080
#define RTL8188EU_FW_ENABLED 0x01
#define RTL8188EU_FW_READY 0x02
#define RTL8188EU_FW_CHECKSUM 0x04
#define RTL8188EU_FW_RUNNING 0x40
#define RTL8188EU_FW_RAM_SELECTED 0x80
#define RTL8188EU_SYS_FUNC_EN 0x0002
#define RTL8188EU_CPU_ENABLE 0x04
#define RTL8188EU_EFUSE_CTRL 0x0030
#define RTL8188EU_EFUSE_ADDRESS_LIMIT 1024

// --

S32 RTL8188EU_PowerOn( struct USB2_EPResource *control );
S32 RTL8188EU_UploadFirmware( struct USB2_EPResource *control, U8 *image, U32 length );
S32 RTL8188EU_Write8( struct USB2_EPResource *control, U16 address, U8 value );
S32 RTL8188EU_Update8( struct USB2_EPResource *control, U16 address, U8 clear, U8 set );
S32 RTL8188EU_Wait8( struct USB2_EPResource *control, U16 address, U8 mask, U8 value );
S32 RTL8188EU_ResetMCU( struct USB2_EPResource *control );
S32 RTL8188EU_ReadEFuseByte( struct USB2_EPResource *control, U16 offset, U8 *value );
S32 RTL8188EU_ReadEFuse( struct USB2_EPResource *control, U8 *rom, U32 size );

#endif
