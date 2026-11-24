
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

SEC_RODATA static const struct TagItem memtags[] =
{
	{ AVT_Type, MEMF_SHARED },
	{ TAG_END, TAG_END }
};

SEC_CODE S32 MSD_Cmd_A004_NSCmd_GetGeometry64( struct USBBase *usbbase, struct MSDDevice *msddev UNUSED, struct IOStdReq *ioreq )
{
struct DriveGeometry64 tmp_dg;
struct MSDDisk *msddisk;
U32 actual;
S32 reply;
U32 flags;

	TASK_NAME_ENTER( "MSD : NSCmd_GetGeometry64" );
	USBERROR( "MSD : _cmd_A004_NSCmd_GetGeometry64" );

	reply = TRUE;

	if (( ! ioreq->io_Data ) || ( ! ioreq->io_Length ))
	{
		ioreq->io_Error = IOERR_BADLENGTH;
		USBERROR( "MSD : NULL Pointer" );
		goto bailout;
	}

	msddisk = (PTR) ioreq->io_Unit;

	if ( msddisk->msddisk_Removable )
	{
		MSD_Disk_MediaCheck( usbbase, msddisk );
	}

	if ( ( ! msddisk->msddisk_MediaPresent ) || ( ! msddisk->msddisk_Block_Size ) )
	{
		ioreq->io_Error = TDERR_DiskChanged;
		ioreq->io_Actual = 0;
		goto bailout;
	}

	MEM_SET( & tmp_dg, 0, sizeof( struct DriveGeometry64 ));

	flags = 0;

	if ( msddisk->msddisk_Removable )
	{
		flags |= DGF_REMOVABLE;
	}

	tmp_dg.dg_SectorSize	= msddisk->msddisk_Block_Size;
//	tmp_dg.dg_Reserved1		= 0;
	tmp_dg.dg_TotalSectors	= msddisk->msddisk_TotalBlocks;
	tmp_dg.dg_BufMemTags	= memtags;
	tmp_dg.dg_DeviceType	= msddisk->msddisk_DeviceType;
	tmp_dg.dg_Flags			= flags;
//	tmp_dg.dg_Reserved2		= 0;

	actual = MIN( ioreq->io_Length, sizeof( struct DriveGeometry64 ));

	// Update User Buffers
	MEM_COPY( & tmp_dg, ioreq->io_Data, actual );
	ioreq->io_Actual = actual;
	ioreq->io_Error = 0;

	// DebugPrintF consumes longwords for these numeric arguments.
	MSDDEBUG( "MSD: unit %lu geometry64, $%08lX%08lX sectors of %lu bytes\n",
		msddisk->msddisk_UnitNr, (U32) ( tmp_dg.dg_TotalSectors >> 32 ),
		(U32) tmp_dg.dg_TotalSectors, tmp_dg.dg_SectorSize );

bailout:

	TASK_NAME_LEAVE();
	return( reply );
}

#endif
// --
