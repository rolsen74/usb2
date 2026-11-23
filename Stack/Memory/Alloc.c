
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

SEC_CODE static inline void __Add_non_Aligned( struct USBBase *usbbase, struct Mem_HeaderNode *h )
{
struct Mem_FreeNode *mem;

	mem = MEM_ALLOCVEC( h->mhn_Mem_Size, FALSE );

	if ( mem )
	{
		// Init Mem Node
		mem->mfn_Next = h->mhn_Free_Nodes;
		mem->mfn_Addr = 0;

		// Update Stats
		h->mhn_Free_Nodes = mem;
		h->Mem.Normal.Free_Count++;
		h->Mem.Normal.Total_Count++;
	}
}

// --

SEC_CODE static void
__Add_Aligned( struct USBBase *usbbase, struct Mem_HeaderNode *h )
{
	struct Mem_PageNode *page;
	struct Mem_FreeNode *chunk;
	struct ExecIFace *IExec;
	struct MMUIFace *IMMU;

	PTR stack;
	PTR mem;
	U32 alignment;
	U32 size;
	U32 stride;
	U32 count;
	U32 index;
	U32 offset;
	U64 physical;

	size = h->Mem.Align.Real_Size;
	stride = h->Mem.Align.Stride;
	count = size / stride;
	alignment = MAX( usbbase->usb_Exec_PageSize, h->mhn_Align );
	IExec = usbbase->usb_IExec;
	IMMU = usbbase->usb_IMMU;

	page = MEM_ALLOCVEC( sizeof( *page ) + ( ( count + 31 ) / 32 ) * sizeof( U32 ), TRUE );

	if ( ! page )
	{
		return;
	}

	mem = IExec->AllocVecTags( size,
		AVT_PhysicalAlignment, alignment,
		AVT_Alignment, alignment,
		AVT_Type, MEMF_SHARED,
		AVT_Contiguous, TRUE,
		AVT_Lock, TRUE,
		TAG_END );

	page->mpn_Memory = mem;

	if ( ! mem )
	{
		goto bailout;
	}

	stack = IExec->SuperState();

	physical = (uintptr_t)IMMU->GetPhysicalAddress( mem );

	if ( ( ! physical ) || ( physical & ( alignment - 1 ) ) ||
		( (uintptr_t)mem & ( alignment - 1 ) ) || ( physical > 0xffffffffULL - ( size - 1 ) ) )
	{
		if ( stack )
		{
			IExec->UserState( stack );
		}

		USBERROR( "__Add_Aligned : Invalid DMA allocation at %p", mem );

		goto bailout;
	}

	IExec->CacheClearE( mem, size, CACRF_ClearD | CACRF_ClearI );

	IMMU->SetMemoryAttrs( mem, size, MEMATTRF_READ_WRITE | MEMATTRF_CACHEINHIBIT );

	IExec->CacheClearE( mem, size, CACRF_InvalidateD );

	if ( stack )
	{
		IExec->UserState( stack );
	}

	page->mpn_Physical = (U32)physical;
	page->mpn_Total = count;
	page->mpn_Free = count;
	h->Mem.Align.Idle_Chunks += count;

	NODE_ADDTAIL( &h->Mem.Align.Pages, page );

	for ( index = 0; index < count; index++ )
	{
		offset = index * stride;
		chunk = (PTR)( (U8 *)mem + offset );
		chunk->mfn_Addr = page->mpn_Physical + offset;
		chunk->mfn_Next = h->mhn_Free_Nodes;
		h->mhn_Free_Nodes = chunk;
	}

	return;

bailout:

	Memory_ReleasePage( usbbase, page, size );
}

// --

SEC_CODE static S32
__Take_Aligned( struct Mem_HeaderNode *h, struct Mem_FreeNode *mem )
{
	struct Mem_PageNode *page;

	U32 chunk;
	U32 bit;

	page = Memory_FindPage( h, mem, &chunk );

	if ( ! page )
	{
		return ( FALSE );
	}

	bit = 1UL << ( chunk % 32 );

	if ( ( page->mpn_InUse[chunk / 32] & bit ) || ( ! page->mpn_Free ) ||
		( mem->mfn_Addr != page->mpn_Physical + chunk * h->Mem.Align.Stride ) )
	{
		return ( FALSE );
	}

	if ( page->mpn_Free == page->mpn_Total )
	{
		h->Mem.Align.Idle_Chunks -= page->mpn_Total;
	}

	page->mpn_InUse[chunk / 32] |= bit;
	page->mpn_Free--;

	return ( TRUE );
}

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE PTR __Memory_Alloc( struct USBBase *usbbase, U32 memtype, S32 clear, STR file UNUSED )

#else

SEC_CODE PTR __Memory_Alloc( struct USBBase *usbbase, U32 memtype, S32 clear )

#endif
{
	struct Mem_HeaderNode *h;
	struct Mem_FreeNode *mem;

	if ( ( memtype >= MEMID_Last ) || ( ! usbbase->usb_Memory_Headers ) )
	{
		return ( NULL );
	}

	h = &usbbase->usb_Memory_Headers[memtype];

	SEMAPHORE_OBTAIN( &h->mhn_Semaphore );

	if ( ! h->mhn_Free_Nodes )
	{
		if ( h->mhn_Align > 1 )
		{
			__Add_Aligned( usbbase, h );
		}
		else
		{
			__Add_non_Aligned( usbbase, h );
		}
	}

	mem = h->mhn_Free_Nodes;

	if ( mem )
	{
		if ( h->mhn_Align <= 1 )
		{
			h->Mem.Normal.Free_Count--;

#ifdef DO_DEBUG
			if ( h->Mem.Normal.Free_Count < 0 )
			{
				USBDEBUG( "__Memory_Alloc : Negative free count in pool %lu", memtype );
			}
#endif
		}
		else if ( ! __Take_Aligned( h, mem ) )
		{
			USBERROR( "__Memory_Alloc : Invalid free chunk in pool %lu", memtype );

			mem = NULL;
		}

		if ( mem )
		{
			h->mhn_Free_Nodes = mem->mfn_Next;
		}
	}

	SEMAPHORE_RELEASE( &h->mhn_Semaphore );

	if ( ( mem ) && ( clear ) )
	{
		MEM_SET( mem, 0, h->mhn_Mem_Size );
	}

	return ( mem );
}

// --
