
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "Page.h"

extern char *idnames[MEMID_Last];

// --

SEC_CODE static inline void __Cleanup_Aligned( struct USBBase *usbbase, struct Mem_HeaderNode *h, U32 memid )
{
struct Mem_PageNode *next;
struct Mem_PageNode *node;

	struct ExecIFace *IExec = usbbase->usb_IExec;
	char *str = ( idnames[memid] ) ? idnames[memid] : "??" ;
	IExec->DebugPrintF( "Freeing MemID #%ld : %s\n", memid, str );

	node = h->Mem.Align.Pages.uh_Head;

	while( node )
	{
		next = Node_Next(node);
		// --
		NODE_REMNODE( & h->Mem.Align.Pages, node );

		if ( node->mpn_Free == node->mpn_Total )
		{
			IExec->DebugPrintF( " --> Buffer %p : Size %lu\n", node->mpn_Memory, h->Mem.Align.Real_Size );

			Memory_ReleasePage( usbbase, node, h->Mem.Align.Real_Size );
		}
		else
		{
			IExec->DebugPrintF( " --> Buffer %p still in use : Free chunks %lu/%lu\n",
				node->mpn_Memory, node->mpn_Free, node->mpn_Total );

			MEM_FREEVEC( node );
		}

		node = next;
	}
}

// --

SEC_CODE static inline void __Cleanup_non_Aligned( struct USBBase *usbbase, struct Mem_HeaderNode *h, U32 memid )
{
struct Mem_FreeNode *node;
struct Mem_FreeNode *next;

	struct ExecIFace *IExec = usbbase->usb_IExec;
	char *str = ( idnames[memid] ) ? idnames[memid] : "??" ;
	IExec->DebugPrintF( "Freeing MemID #%ld : Nodes %ld/%ld : %s\n", memid,
		h->Mem.Normal.Free_Count, h->Mem.Normal.Total_Count, str );

	node = h->mhn_Free_Nodes;

	while( node )
	{
		next = node->mfn_Next;
		// --

		IExec->DebugPrintF( " --> mem %p\n", node );

		MEM_FREEVEC( node );
		// --
		node = next;
	}
}

// --

SEC_CODE void __Memory_Cleanup( struct USBBase *usbbase )
{
struct Mem_HeaderNode *mhn;
struct Mem_HeaderNode *h;
U32 cnt;

	mhn = usbbase->usb_Memory_Headers;

	if ( mhn )
	{
		usbbase->usb_Memory_Headers = NULL;

		for( cnt=0 ; cnt<MEMID_Last ; cnt++ )
		{
			h = & mhn[cnt] ;

			if ( h->mhn_Align > 1 )
			{
				// do alignment
				__Cleanup_Aligned( usbbase, h, cnt );
			}
			else
			{
				// not aligned
				__Cleanup_non_Aligned( usbbase, h, cnt );
			}
		}

		MEM_FREEVEC( mhn );
	}
}

// --
