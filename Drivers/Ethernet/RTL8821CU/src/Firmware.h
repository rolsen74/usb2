
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RTL8821CU_FIRMWARE_H
#define RTL8821CU_FIRMWARE_H

#define FW_HEADER_SIZE 64
#define FW_CHECKSUM_SIZE 8
#define FW_SECTION_COUNT 3
#define FW_DMEM_BASE 0x00200000

struct FirmwareSection
{
	const U8 *Data;
	U32 Address;
	U32 Length;
};

S32 RTL8821CU_DownloadFirmware( struct EthernetUnit *unit, const struct FirmwareSection *sections, U32 count );
S32 RTL8821CU_WriteFirmwareSection( struct EthernetUnit *unit, const struct FirmwareSection *section );

#endif
