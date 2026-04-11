
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE PTR __Node_RemTail( struct USBBase *usbbase UNUSED, struct USB2_Header *header )
{
struct USB2_Node *prev;
struct USB2_Node *node;

	node = header->uh_Tail;

	if ( ! node )
	{
		// No Nodes in Header

		header->uh_Head = NULL;
//		header->uh_Tail = NULL;
		header->uh_Nodes = 0;
	}
	else if ( header->uh_Head == node )
	{
		// Head = Tail, so there is one Node

		#if 0
//		#ifdef DO_PANIC
		node->un_Next = (PTR) 0xAC111111;
		node->un_Prev = (PTR) 0xAC222222;
		#else
		node->un_Next = NULL;
		node->un_Prev = NULL;
		#endif

		header->uh_Head = NULL;
		header->uh_Tail = NULL;
		header->uh_Nodes = 0;
	}
	else
	{
		// More than one Node

		prev = node->un_Prev;

		prev->un_Next = NULL;

		#if 0
//		#ifdef DO_PANIC
		node->un_Next = (PTR) 0xAC111111;
		node->un_Prev = (PTR) 0xAC222222;
		#else
		node->un_Next = NULL;
		node->un_Prev = NULL;
		#endif

		header->uh_Tail = prev;
		header->uh_Nodes--;
	}

	return(	node );
}
