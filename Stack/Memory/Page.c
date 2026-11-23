
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

SEC_CODE void
Memory_ReleasePage( struct USBBase *usbbase, struct Mem_PageNode *page, U32 size )
{
	struct ExecIFace *IExec;

	IExec = usbbase->usb_IExec;

	if ( page->mpn_Memory )
	{
		IExec->UnlockMem( page->mpn_Memory, size );

		IExec->FreeVec( page->mpn_Memory );
	}

	MEM_FREEVEC( page );
}

// --
