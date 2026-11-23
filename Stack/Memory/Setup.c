
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

struct myMemList
{
	U32		Align;		// Min Mem Alignment
	U32		Size;		// Structure byte Size
	U32		Max;		// Max free Nodes
	U32		Pre;		// Prealloc on startup
};

static const struct myMemList TheMemList[MEMID_Last] =
{
	[MEMID_USBIORequest]		= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct RealRequest ) },
	[MEMID_USBRegister]			= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct RealRegister ) },
	[MEMID_USBEndPointResource]	= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct RealEndPointResource ) },
	[MEMID_USBInterfaceGroup]	= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_InterfaceGroup ) },
	[MEMID_USBInterfaceHeader]	= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_InterfaceHeader ) },
	[MEMID_USBInterfaceNode]	= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_InterfaceNode ) },
	[MEMID_USBFunction]			= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct RealFunctionNode ) },
	[MEMID_USBEndPoint]			= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_EndPointNode ) },
	[MEMID_USBConfig]			= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_ConfigNode ) },
	[MEMID_USBDriver]			= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_DriverNode ) },
	[MEMID_USBTaskMsg]			= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_TaskMsg ) },
	// Well HCDs are only allocaterd on startup, queuing is not needed
	[MEMID_USBHCD]				= { .Max =  0, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_HCDNode ) },
	[MEMID_USBFktDriver]		= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_FktDriverNode ) },
	[MEMID_USBTask]				= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_TaskNode ) },
	[MEMID_EHCI_QH]				= { .Max = 25, .Pre = 0, .Align = EHCI_QH_ALIGN, .Size = sizeof( struct EHCI_QH ) },
	[MEMID_EHCI_TD]				= { .Max = 25, .Pre = 0, .Align = EHCI_TD_ALIGN, .Size = sizeof( struct EHCI_TD ) },
	[MEMID_OHCI_ED]				= { .Max = 25, .Pre = 0, .Align = OHCI_ED_ALIGN, .Size = sizeof( struct OHCI_ED ) },
	[MEMID_OHCI_TD]				= { .Max = 25, .Pre = 0, .Align = OHCI_TD_ALIGN, .Size = sizeof( struct OHCI_TD ) },
	[MEMID_OHCI_HCCA]			= { .Max = 25, .Pre = 0, .Align = OHCI_HCCA_ALIGN, .Size = sizeof( struct OHCI_HCCA ) },
	[MEMID_UHCI_QH]				= { .Max = 25, .Pre = 0, .Align = 16, .Size = sizeof( struct UHCI_QH ) },
	[MEMID_UHCI_TD]				= { .Max = 25, .Pre = 0, .Align = 16, .Size = sizeof( struct UHCI_TD ) },
	[MEMID_HCD_20k]				= { .Max = 25, .Pre = 0, .Align = 1024 * 4, .Size = 1024 * 20 },
	[MEMID_HCD_8k]				= { .Max = 25, .Pre = 0, .Align = 1024 * 4, .Size = 1024 * 8 },
	[MEMID_HCD_4k]				= { .Max = 25, .Pre = 0, .Align = 1024 * 4, .Size = 1024 * 4 },
	// There are not really an alignment requirement, but we need to enable alignment with a x>1 value
	[MEMID_USBSetupData]		= { .Max = 25, .Pre = 0, .Align = 2, .Size = sizeof( struct RealSetupData ) },
	[MEMID_NotifyNode]			= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_NotifyNode ) },
	[MEMID_NotifyMessage]		= { .Max = 25, .Pre = 0, .Align = 1, .Size = sizeof( struct USB2_NotifyMessage ) },
};

// --

SEC_RWDATA char *idnames[MEMID_Last] =
{
	// Stack buffers
	[MEMID_USBFunction]				= "MEMID_USBFunction",
	[MEMID_USBEndPoint]				= "MEMID_USBEndPoint",
	[MEMID_USBEndPointResource]		= "MEMID_USBEndPointResource",
	[MEMID_USBIORequest]			= "MEMID_USBIORequest",
	[MEMID_USBRegister]				= "MEMID_USBRegister",
	[MEMID_USBInterfaceGroup]		= "MEMID_USBInterfaceGroup",
	[MEMID_USBInterfaceHeader]		= "MEMID_USBInterfaceHeader",
	[MEMID_USBInterfaceNode]		= "MEMID_USBInterfaceNode",
	[MEMID_USBConfig]				= "MEMID_USBConfig",
	[MEMID_USBDriver]				= "MEMID_USBDriver",
	[MEMID_USBTaskMsg]				= "MEMID_USBTaskMsg",
	[MEMID_USBHCD]					= "MEMID_USBHCD",
	[MEMID_USBFktDriver]			= "MEMID_USBFktDriver",
	[MEMID_USBTask]					= "MEMID_USBTask",

	// HCDs buffers
	[MEMID_USBSetupData]			= "MEMID_USBSetupData",
	[MEMID_HCD_20k]					= "MEMID_HCD_20k",
	[MEMID_HCD_4k]					= "MEMID_HCD_4k",
	[MEMID_HCD_8k]					= "MEMID_HCD_8k",

	// EHCI
	[MEMID_EHCI_QH]					= "MEMID_EHCI_QH",
	[MEMID_EHCI_TD]					= "MEMID_EHCI_TD",
	
	// OHCI
	[MEMID_OHCI_ED]					= "MEMID_OHCI_ED",
	[MEMID_OHCI_TD]					= "MEMID_OHCI_TD",
	[MEMID_OHCI_HCCA]				= "MEMID_OHCI_HCCA",
	[MEMID_UHCI_QH]				= "MEMID_UHCI_QH",
	[MEMID_UHCI_TD]				= "MEMID_UHCI_TD",

	// Notify
	[MEMID_NotifyNode]				= "MEMID_NotifyNode",
	[MEMID_NotifyMessage]			= "MEMID_NotifyMessage",

};

SEC_CODE S32
__Memory_Setup( struct USBBase *usbbase )
{
	const struct myMemList *l;
	struct Mem_HeaderNode *mhn;
	struct Mem_HeaderNode *h;

	U64 stride;
	U64 real_size;
	U32 chunk_align;
	U32 page_size;
	U32 cnt;

	page_size = usbbase->usb_Exec_PageSize;

	if ( ( ! page_size ) || ( page_size & ( page_size - 1 ) ) )
	{
		USBERROR( "__Memory_Setup : Invalid Exec page size %lu", page_size );

		return ( FALSE );
	}

	mhn = MEM_ALLOCVEC( sizeof( struct Mem_HeaderNode ) * MEMID_Last, TRUE );

	if ( ! mhn )
	{
		USBERROR( "__Memory_Setup : Could not allocate pool headers" );

		return ( FALSE );
	}

	for ( cnt = 0; cnt < MEMID_Last; cnt++ )
	{
		l = &TheMemList[cnt];
		h = &mhn[cnt];

		SEMAPHORE_INIT( &h->mhn_Semaphore );

		h->mhn_Align = l->Align;
		h->mhn_Mem_Size = MAX( l->Size, sizeof( struct Mem_FreeNode ) );

		if ( h->mhn_Align > 1 )
		{
			chunk_align = MAX( l->Align, __alignof__( struct Mem_FreeNode ) );
			stride = ( (U64)h->mhn_Mem_Size + chunk_align - 1 ) & ~( (U64)chunk_align - 1 );
			real_size = ( stride + page_size - 1 ) & ~( (U64)page_size - 1 );

			if ( ( l->Align & ( l->Align - 1 ) ) || ( real_size > 0xffffffffULL ) )
			{
				USBERROR( "__Memory_Setup : Invalid geometry for pool %lu", cnt );

				MEM_FREEVEC( mhn );

				return ( FALSE );
			}

			h->Mem.Align.Stride = (U32)stride;
			h->Mem.Align.Real_Size = (U32)real_size;
			h->Mem.Align.Free_Max = l->Max;
		}
		else
		{
			h->Mem.Normal.Free_Max = l->Max;
		}
	}

	usbbase->usb_Memory_Headers = mhn;

	return ( TRUE );
}

// --
