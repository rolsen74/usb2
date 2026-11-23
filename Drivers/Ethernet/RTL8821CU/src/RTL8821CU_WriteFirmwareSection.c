
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

#define FW_TX_BUFFER_ADDRESS 0x18780000U
#define FW_BEACON_QUEUE 0x10
#define FW_LAST_SEGMENT 0x04
#define DDMA_OWN 0x80000000U
#define DDMA_CHECKSUM 0x20000000U
#define DDMA_CHECKSUM_ERROR 0x08000000U
#define DDMA_RESET_CHECKSUM 0x02000000U
#define DDMA_CONTINUE 0x01000000U

// --

static S32
WriteReservedPage( struct USB2_EPResource *bulk, const U8 *data, U32 length )
{
	static const U8 EmptyDescriptor[RTL8821CU_TX_DESC_SIZE] = { 0 };

	struct USB2_IORequest *ioreq;

	U8 *buffer;
	U32 packet;
	S32 error;

	if ( ( ! bulk->EndPointMax ) || ( bulk->BufferSize < RTL8821CU_FIRMWARE_BUFFER_SIZE ) )
	{
		return ( FALSE );
	}

	ioreq = bulk->IORequests[0];
	buffer = bulk->Buffers[0];
	packet = RTL8821CU_TX_DESC_SIZE + length;

	CopyMem( (PTR) EmptyDescriptor, buffer, RTL8821CU_TX_DESC_SIZE );

	CopyMem( (PTR) data, buffer + RTL8821CU_TX_DESC_SIZE, length );

	// End the USB transfer with a short packet. DDMA copies the unpadded length.
	if ( packet % bulk->EndPointMax == 0 )
	{
		buffer[packet++] = 0;
		length++;
	}

	buffer[0] = length;
	buffer[1] = length >> 8;
	buffer[2] = RTL8821CU_TX_DESC_SIZE;
	buffer[3] = FW_LAST_SEGMENT;
	buffer[5] = FW_BEACON_QUEUE;

	RTL8821CU_ChecksumTX( buffer );

	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Length = packet;
	ioreq->io_Data = buffer;
	ioreq->io_TimeOut = 500000;

	error = DoIO( (PTR) ioreq );

	return ( ( ! error ) && ( ioreq->io_Actual == packet ) );
}

// --

S32
RTL8821CU_WriteFirmwareSection( struct EthernetUnit *unit, const struct FirmwareSection *section )
{
	struct USB2_EPResource *control;
	struct USB2_EPResource *bulk;

	U32 offset;
	U32 length;
	U32 mode;
	U32 valid;

	control = unit->unit_USB_Register->Res_Control;
	bulk = unit->unit_Bulk_Out[0];
	mode = DDMA_OWN | DDMA_CHECKSUM;

	if ( ( ! RTL8821CU_PollReg( control, REG_DDMA_CONTROL, 4, DDMA_OWN, 0 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_DDMA_CONTROL, 4, DDMA_RESET_CHECKSUM, DDMA_RESET_CHECKSUM ) ) )
	{
		return ( FALSE );
	}

	for( offset = 0; offset < section->Length; offset += length )
	{
		length = section->Length - offset;

		if ( length > RTL8821CU_FIRMWARE_CHUNK_SIZE )
		{
			length = RTL8821CU_FIRMWARE_CHUNK_SIZE;
		}

		if ( ( ! RTL8821CU_WriteReg( control, REG_FIFOPAGE_CTRL_2, 2, BCN_PAGE_VALID ) ) ||
			( ! WriteReservedPage( bulk, section->Data + offset, length ) ) ||
			( ! RTL8821CU_PollReg( control, REG_FIFOPAGE_CTRL_2, 2, BCN_PAGE_VALID, BCN_PAGE_VALID ) ) ||
			( ! RTL8821CU_WriteReg( control, REG_DDMA_SOURCE, 4, FW_TX_BUFFER_ADDRESS + RTL8821CU_TX_DESC_SIZE ) ) ||
			( ! RTL8821CU_WriteReg( control, REG_DDMA_DEST, 4, section->Address + offset ) ) ||
			( ! RTL8821CU_WriteReg( control, REG_DDMA_CONTROL, 4, mode | length ) ) ||
			( ! RTL8821CU_PollReg( control, REG_DDMA_CONTROL, 4, DDMA_OWN, 0 ) ) )
		{
			return ( FALSE );
		}

		mode |= DDMA_CONTINUE;
	}

	if ( ! RTL8821CU_PollReg( control, REG_DDMA_CONTROL, 4, DDMA_CHECKSUM_ERROR, 0 ) )
	{
		return ( FALSE );
	}

	valid = ( section->Address < FW_DMEM_BASE ) ? FW_IMEM_VALID : FW_DMEM_VALID;

	return ( RTL8821CU_UpdateReg( control, REG_MCUFW_CTRL, 1, valid, valid ) );
}

// --
