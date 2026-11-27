
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

SEC_CODE static inline void __Free_non_Aligned( struct USBBase *usbbase, struct Mem_HeaderNode *h, struct Mem_FreeNode *mem, U32 phyadr )
{
	if ( h->Mem.Normal.Free_Count >= h->Mem.Normal.Free_Max )
	{
		// Free Memory

		// Release memory to system
		MEM_FREEVEC( mem );

		// Update Stats
		h->Mem.Normal.Total_Count--;
	}
	else
	{
		// Queue Memory

		// Update Stats
		h->Mem.Normal.Free_Count++;

		// Init Mem Node
		mem->mfn_Next = h->mhn_Free_Nodes;
		mem->mfn_Addr = phyadr;

		// Link
		h->mhn_Free_Nodes = mem;
	}
}

// --

SEC_CODE static void
__Unlink_Free_Chunks( struct Mem_HeaderNode *h, struct Mem_PageNode *page )
{
	struct Mem_FreeNode **link;
	struct Mem_FreeNode *mem;

	U32 size;

	size = page->mpn_Total * h->Mem.Align.Stride;
	link = &h->mhn_Free_Nodes;

	while ( *link )
	{
		mem = *link;

		if ( (uintptr_t)mem - (uintptr_t)page->mpn_Memory < size )
		{
			*link = mem->mfn_Next;
		}
		else
		{
			link = &mem->mfn_Next;
		}
	}
}

// --

SEC_CODE static struct Mem_PageNode *
__Free_Aligned( struct USBBase *usbbase, struct Mem_HeaderNode *h, struct Mem_FreeNode *mem, U32 phyadr )
{
	struct Mem_PageNode *page;

	U32 chunk;
	U32 bit;

	page = Memory_FindPage( h, mem, &chunk );

	if ( ( ! page ) || ( phyadr != page->mpn_Physical + chunk * h->Mem.Align.Stride ) )
	{
		USBERROR( "__Free_Aligned : Invalid DMA chunk %p, address $%08lx", mem, phyadr );

		return ( NULL );
	}

	bit = 1UL << ( chunk % 32 );

	if ( ! ( page->mpn_InUse[chunk / 32] & bit ) )
	{
		USBERROR( "__Free_Aligned : DMA chunk %p is already free", mem );

		return ( NULL );
	}

	page->mpn_InUse[chunk / 32] &= ~bit;
	page->mpn_Free++;

	if ( page->mpn_Free == page->mpn_Total )
	{
		// Keep one idle backing allocation for small descriptors.
		if ( ( ! h->Mem.Align.Free_Max ) ||
			( ( h->Mem.Align.Idle_Chunks ) &&
			  ( h->Mem.Align.Idle_Chunks + page->mpn_Total > h->Mem.Align.Free_Max ) ) )
		{
			__Unlink_Free_Chunks( h, page );

			NODE_REMNODE( &h->Mem.Align.Pages, page );

			return ( page );
		}

		h->Mem.Align.Idle_Chunks += page->mpn_Total;
	}

#ifdef DO_PANIC
	MEM_SET( mem, 0x55, h->mhn_Mem_Size );
#endif

	mem->mfn_Addr = phyadr;
	mem->mfn_Next = h->mhn_Free_Nodes;
	h->mhn_Free_Nodes = mem;

	return ( NULL );
}

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE void __Memory_Free( struct USBBase *usbbase, U32 memtype, PTR mem, U32 phyadr, STR file UNUSED )

#else

SEC_CODE void __Memory_Free( struct USBBase *usbbase, U32 memtype, PTR mem, U32 phyadr )

#endif
{
	struct Mem_HeaderNode *h;
	struct Mem_PageNode *released;

	U32 size;

	if ( ( ! mem ) || ( memtype >= MEMID_Last ) || ( ! usbbase->usb_Memory_Headers ) )
	{
		return;
	}

	h = &usbbase->usb_Memory_Headers[memtype];

	SEMAPHORE_OBTAIN( &h->mhn_Semaphore );

	if ( h->mhn_Align > 1 )
	{
		size = h->Mem.Align.Real_Size;

		released = __Free_Aligned( usbbase, h, mem, phyadr );

		SEMAPHORE_RELEASE( &h->mhn_Semaphore );

		if ( released )
		{
			Memory_ReleasePage( usbbase, released, size );
		}

		return;
	}

#ifdef DO_PANIC
	MEM_SET( mem, 0x55, h->mhn_Mem_Size );
#endif

	__Free_non_Aligned( usbbase, h, mem, phyadr );

	SEMAPHORE_RELEASE( &h->mhn_Semaphore );
}

// --

SEC_CODE void
Memory_FreeIOBuffer( struct USBBase *usbbase, PTR mem )
{
	struct Mem_IOBuffer *buffer;

	if ( ! mem )
	{
		return;
	}

	SEMAPHORE_OBTAIN( &usbbase->usb_IOBufferSemaphore );

	buffer = Memory_FindIOBuffer( usbbase, mem );

	if ( ( buffer ) && ( buffer->Memory == mem ) )
	{
		NODE_REMNODE( &usbbase->usb_IOBuffers, buffer );
	}
	else
	{
		buffer = NULL;
	}

	SEMAPHORE_RELEASE( &usbbase->usb_IOBufferSemaphore );

	if ( buffer )
	{
		Memory_FreeDMAPages( usbbase, mem, buffer->Allocated );

		MEM_FREEVEC( buffer );
	}
}

// --
