
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

SEC_CODE PTR __Mem_AllocIOBuffer( struct USBBase *usbbase, U32 size, S32 clr )
{
struct ExecIFace *IExec;
U32 real;
PTR mem;
U32 cs;

	#ifdef DO_PANIC

	if ( ! usbbase )
	{
		USBPANIC( "Mem_AllocIOBuffer : 1 : NULL Pointer" );
	}

	#endif

	if ( size )
	{
		cs = usbbase->usb_CPU_CacheSize;

		real = (( size + cs - 1 ) & ~ ( cs - 1 ));

		IExec = usbbase->usb_IExec;

		mem = IExec->AllocVecTags( real,
			( clr ) ? AVT_Clear : TAG_IGNORE, 0,
			AVT_PhysicalAlignment,  cs,
			AVT_Contiguous, TRUE,
			AVT_Alignment, cs,
			AVT_Type, MEMF_SHARED,
			TAG_END
		);

//		USBDEBUG( "AllocIO Mem              : Mem %p : Cache %ld : Size %ld : Real %ld : Clear %ld :", mem, cs, size, real, clr );
	}
	else
	{
		USBDEBUG( "Alloc IO Mem : NULL Pointer" );

		mem = NULL;
	}

	return( mem );
}

// --
