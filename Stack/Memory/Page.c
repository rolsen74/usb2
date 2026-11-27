
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "Page.h"
#include <stdint.h>

// --

SEC_CODE struct Mem_PageNode *
Memory_FindPage( struct Mem_HeaderNode *h, PTR mem, U32 *chunk )
{
	struct Mem_PageNode *page;

	uintptr_t offset;
	U32 stride;

	stride = h->Mem.Align.Stride;
	page = h->Mem.Align.Pages.uh_Head;

	while ( page )
	{
		offset = (uintptr_t)mem - (uintptr_t)page->mpn_Memory;

		if ( offset < page->mpn_Total * stride )
		{
			if ( offset % stride )
			{
				return ( NULL );
			}

			*chunk = (U32)offset / stride;

			return ( page );
		}

		page = Node_Next( page );
	}

	return ( NULL );
}

// --

SEC_CODE PTR
Memory_AllocDMAPages( struct USBBase *usbbase, U32 size, U32 alignment, U32 *physical )
{
	struct ExecIFace *IExec;
	struct MMUIFace *IMMU;

	PTR mem;
	PTR stack;
	U64 address;

	IExec = usbbase->usb_IExec;
	IMMU = usbbase->usb_IMMU;

	mem = IExec->AllocVecTags( size,
		AVT_PhysicalAlignment, alignment,
		AVT_Alignment, alignment,
		AVT_Type, MEMF_SHARED,
		AVT_Contiguous, TRUE,
		AVT_Lock, TRUE,
		TAG_END );

	if ( ! mem )
	{
		return ( NULL );
	}

	stack = IExec->SuperState();

	address = (uintptr_t)IMMU->GetPhysicalAddress( mem );

	if ( ( ! address ) || ( address & ( alignment - 1 ) ) ||
		( (uintptr_t)mem & ( alignment - 1 ) ) || ( address > 0xffffffffULL - ( size - 1 ) ) )
	{
		if ( stack )
		{
			IExec->UserState( stack );
		}

		USBERROR( "Memory_AllocDMAPages : Invalid DMA allocation at %p", mem );

		Memory_FreeDMAPages( usbbase, mem, size );

		return ( NULL );
	}

	IExec->CacheClearE( mem, size, CACRF_ClearD | CACRF_ClearI );

	IMMU->SetMemoryAttrs( mem, size, MEMATTRF_READ_WRITE | MEMATTRF_CACHEINHIBIT );

	IExec->CacheClearE( mem, size, CACRF_InvalidateD );

	if ( stack )
	{
		IExec->UserState( stack );
	}

	*physical = (U32)address;

	return ( mem );
}

// --

SEC_CODE void
Memory_FreeDMAPages( struct USBBase *usbbase, PTR mem, U32 size )
{
	struct ExecIFace *IExec;

	IExec = usbbase->usb_IExec;

	IExec->UnlockMem( mem, size );

	IExec->FreeVec( mem );
}

// --

SEC_CODE void
Memory_ReleasePage( struct USBBase *usbbase, struct Mem_PageNode *page, U32 size )
{
	if ( page->mpn_Memory )
	{
		Memory_FreeDMAPages( usbbase, page->mpn_Memory, size );
	}

	MEM_FREEVEC( page );
}

// --

// The caller holds usb_IOBufferSemaphore.
SEC_CODE struct Mem_IOBuffer *
Memory_FindIOBuffer( struct USBBase *usbbase, PTR mem )
{
	struct Mem_IOBuffer *buffer;

	buffer = usbbase->usb_IOBuffers.uh_Head;

	while ( buffer )
	{
		if ( (uintptr_t)mem - (uintptr_t)buffer->Memory < buffer->Length )
		{
			return ( buffer );
		}

		buffer = Node_Next( buffer );
	}

	return ( NULL );
}

// --

SEC_CODE U32
Memory_IOBufferPhysical( struct USBBase *usbbase, PTR mem, U32 length )
{
	struct Mem_IOBuffer *buffer;

	U32 offset;
	U32 physical;

	physical = 0;

	if ( ( ! mem ) || ( ! length ) )
	{
		return ( 0 );
	}

	SEMAPHORE_OBTAIN( &usbbase->usb_IOBufferSemaphore );

	buffer = Memory_FindIOBuffer( usbbase, mem );

	if ( buffer )
	{
		offset = (uintptr_t)mem - (uintptr_t)buffer->Memory;

		if ( length <= buffer->Length - offset )
		{
			physical = buffer->Physical + offset;
		}
	}

	SEMAPHORE_RELEASE( &usbbase->usb_IOBufferSemaphore );

	return ( physical );
}

// --
