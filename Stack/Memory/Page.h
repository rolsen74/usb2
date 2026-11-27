
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef _STACK_MEMORY_PAGE_H_
#define _STACK_MEMORY_PAGE_H_

SEC_CODE struct Mem_PageNode * Memory_FindPage( struct Mem_HeaderNode *h, PTR mem, U32 *chunk );
SEC_CODE void Memory_ReleasePage( struct USBBase *usbbase, struct Mem_PageNode *page, U32 size );
SEC_CODE PTR Memory_AllocDMAPages( struct USBBase *usbbase, U32 size, U32 alignment, U32 *physical );
SEC_CODE void Memory_FreeDMAPages( struct USBBase *usbbase, PTR mem, U32 size );
SEC_CODE struct Mem_IOBuffer *Memory_FindIOBuffer( struct USBBase *usbbase, PTR mem );

#endif
