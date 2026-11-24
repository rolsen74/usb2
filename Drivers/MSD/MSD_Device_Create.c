
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "MSD.h"

// --
#if 1

SEC_RODATA const PTR MSD_Manager_Vectors[] =
{
	(PTR) MSD_Manager_Obtain,
	(PTR) MSD_Manager_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) MSD_Manager_Open,
	(PTR) MSD_Manager_Close,
	(PTR) MSD_Manager_Expunge,
	(PTR) NULL,
	(PTR) MSD_Manager_BeginIO,
	(PTR) MSD_Manager_AbortIO,
	(PTR) -1
};

SEC_RODATA static const struct TagItem MSD_Manager_Tags[] =
{
	{ MIT_Name,			(Tag) "__device"		},
	{ MIT_VectorTable,	(Tag) MSD_Manager_Vectors	},
	{ MIT_Version,		(Tag) 1					},
	{ TAG_END,			(Tag) 0					}
};

SEC_RODATA static const PTR devInterfaces[] =
{
	(PTR) MSD_Manager_Tags,
	(PTR) NULL
};

SEC_RODATA static const struct TagItem devCreateTags[] =
{
	{ CLT_DataSize,		(Tag) sizeof( struct MSDBase ) },
	{ CLT_InitFunc,		(Tag) MSD_Manager_Init		},
	{ CLT_Interfaces,	(Tag) devInterfaces		},
	{ TAG_END,			(Tag) 0					}
};

#endif
// --

SEC_CODE S32
MSD_CreateDevice( struct USBBase *usbbase )
{
	struct MSDBase *msdbase;

	S32 ready;

	SEMAPHORE_OBTAIN( &usbbase->usb_MSDSemaphore );

	if ( ! usbbase->usb_MounterBase )
	{
		usbbase->usb_MounterBase = usbbase->usb_IExec->OpenLibrary( "mounter.library", 0 );
	}

	if ( ( usbbase->usb_MounterBase ) && ( ! usbbase->usb_IMounter ) )
	{
		usbbase->usb_IMounter = (PTR) usbbase->usb_IExec->GetInterface( usbbase->usb_MounterBase, "main", 1, NULL );
	}

	if ( ( usbbase->usb_IMounter ) && ( ! usbbase->usb_MSDDevice ) )
	{
		msdbase = MISC_CREATELIBRARY( (PTR) devCreateTags );

		if ( msdbase )
		{
			msdbase->msdbase_usbbase = usbbase;

			SEMAPHORE_INIT( &msdbase->msdbase_MSDDisk_Semaphore );

			MISC_ADDDEVICE( msdbase );

			usbbase->usb_MSDDevice = msdbase;
		}
	}

	ready = usbbase->usb_MSDDevice != NULL;

	SEMAPHORE_RELEASE( &usbbase->usb_MSDSemaphore );

	return ( ready );
}

// --
