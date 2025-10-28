
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "CompPro.h"

// --

extern PTR INewlib;
extern PTR NewlibBase;

void Resources_Cleanup( struct libBase *libBase )
{
//	  if ( libBase->lib_Mod_Lock )
//	  {
//		  IExec->FreeSysObject( ASOT_SEMAPHORE, libBase->lib_Mod_Lock );
//		  libBase->lib_Mod_Lock = NULL;
//	  }
//
//	  if ( libBase->lib_Mod_List )
//	  {
//		  IExec->FreeSysObject( ASOT_LIST, libBase->lib_Mod_List );
//		  libBase->lib_Mod_List = NULL;
//	  }
//
//	  if ( libBase->lib_Drivers_Lock )
//	  {
//		  IExec->FreeSysObject( ASOT_SEMAPHORE, libBase->lib_Drivers_Lock );
//		  libBase->lib_Drivers_Lock = NULL;
//	  }
//
//	  if ( libBase->lib_Drivers_List )
//	  {
//		  IExec->FreeSysObject( ASOT_LIST, libBase->lib_Drivers_List );
//		  libBase->lib_Drivers_List = NULL;
//	  }
//
//	  if ( libBase->lib_Context_Lock )
//	  {
//		  IExec->FreeSysObject( ASOT_SEMAPHORE, libBase->lib_Context_Lock );
//		  libBase->lib_Context_Lock = NULL;
//	  }
//
//	  if ( libBase->lib_Context_List )
//	  {
//		  IExec->FreeSysObject( ASOT_LIST, libBase->lib_Context_List );
//		  libBase->lib_Context_List = NULL;
//	  }
//
//	  if ( libBase->lib_Init_LockSemaphore )
//	  {
//		  IExec->FreeSysObject( ASOT_SEMAPHORE, libBase->lib_Init_LockSemaphore );
//		  libBase->lib_Init_LockSemaphore = NULL;
//	  }
//
//	  if ( libBase->lib_IUtility )
//	  {
//		  IExec->DropInterface( (struct Interface *)libBase->lib_IUtility );
//		  libBase->lib_IUtility = NULL;
//	  }
//
//	  if ( libBase->lib_UtilityBase )
//	  {
//		  IExec->CloseLibrary( libBase->lib_UtilityBase );
//		  libBase->lib_UtilityBase = NULL;
//	  }


	// --

	if ( libBase->lib_fdkey )
	{
		USBResUnregisterFD( libBase->lib_fdkey );
		libBase->lib_fdkey = NULL;
	}

	if ( IUSBResource )
	{
		DropInterface( (PTR) IUSBResource );
		IUSBResource = NULL;
	}

	if ( USBResourceBase )
	{
		CloseLibrary( USBResourceBase );
		USBResourceBase = NULL;
	}

	// --

	// if ( libBase->lib_IAIN )
    // {
	// 	IExec->DropInterface( (struct Interface *)libBase->lib_IAIN );
	// 	libBase->lib_IAIN = NULL;
    // }

	// if ( libBase->lib_AIN_Base )
    // {
	// 	IExec->CloseLibrary( libBase->lib_AIN_Base );
	// 	libBase->lib_AIN_Base = NULL;
    // }

	// if ( libBase->lib_IUtility )
    // {
	// 	IExec->DropInterface( (struct Interface *)libBase->lib_IUtility );
	// 	libBase->lib_IUtility = NULL;
    // }

	// if ( libBase->lib_UtilityBase )
    // {
	// 	IExec->CloseLibrary( libBase->lib_UtilityBase );
	// 	libBase->lib_UtilityBase = NULL;
    // }

	// --

	if ( INewlib )
	{	
		DropInterface( INewlib );
	}

	if ( NewlibBase )
	{
		CloseLibrary( NewlibBase );
	}

	// --
}

// --
