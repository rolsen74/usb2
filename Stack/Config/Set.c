
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static S32
__Update( struct USBBase *usbbase, struct RealFunctionNode *fn, struct USB2_ConfigNode *cfgnode )
{
	struct USB2_InterfaceHeader *ih;
	struct USB2_InterfaceGroup *ig;
	struct USB2_InterfaceNode *in;
	struct USB2_EndPointNode **array;
	struct USB2_EndPointNode *ep;

	U32 offset;

	SEMAPHORE_OBTAIN( & fn->fkt_Semaphore );

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	if ( ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) || ( fn->fkt_Config_Active ) )
	{
		SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

		SEMAPHORE_RELEASE( & fn->fkt_Semaphore );

		return ( FALSE );
	}

	// --

	#ifdef DO_DEBUG

	if ( fn->fkt_Address > 127 )
	{
		USBPANIC( "__Update : Invalid Funktion Address #%lu", fn->fkt_Address );
	}

	#endif

	array	= fn->fkt_HCD->hn_EndPoint_Array;
	offset	= fn->fkt_Address * 32;

	MEM_SET( & array[offset], 0, 32 * sizeof( *array ) );

	array[offset+ 0] = fn->fkt_ControlEndPoint;
	array[offset+16] = fn->fkt_ControlEndPoint;

	// --

	fn->fkt_Config_Active = cfgnode;

	ig = cfgnode->cfg_InterfaceGroups.uh_Head;

	while( ig )
	{
		ih = ig->ig_Group.uh_Head;

		while( ih )
		{
			in = ih->ih_Active;
			ih->ih_AltNumber	= in->in_AltNumber;
			ih->ih_Owner		= NULL;

			ep = in->in_EndPoints.uh_Head;

			while( ep )
			{
				ep->ep_Active = TRUE;

				if ( ep->ep_Direction == EPDIR_Out )
				{
					array[ offset + ep->ep_Number + 16 ] = ep;
				}
				else
				{
					array[ offset + ep->ep_Number +  0 ] = ep;
				}

				ep = Node_Next( ep );
			}

			ih = Node_Next( ih );
		}

		ig = Node_Next( ig );
	}

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE( & fn->fkt_Semaphore );

	return ( TRUE );
}

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE S32 __Config_Set(
	struct USBBase *usbbase,
	struct RealFunctionNode *fn,
	struct RealRequest *ioreq,
	U32 CfgNr,
	STR file UNUSED )

#else

SEC_CODE S32 __Config_Set(
	struct USBBase *usbbase,
	struct RealFunctionNode *fn,
	struct RealRequest *ioreq,
	U32 CfgNr )

#endif

{
struct USB2_ConfigNode *cn;
struct USB2_SetupData sd;
S32 retval;
S32 locked;
//U8 data[1];

	retval = FALSE;
	locked = FALSE;

	#if 0
	{
		struct USB2_EndPointNode **array = fn->fkt_HCD->hn_EndPoint_Array;
		U32 adr = fn->fkt_Address;
		U32 cnt;

		usbbase->usb_IExec->Disable();
		usbbase->usb_IExec->DebugPrintF( "\n## Active Config EndPoints (Before)\nIn : " );
		for( cnt=0 ; cnt<16 ; cnt++ )
		{
			usbbase->usb_IExec->DebugPrintF( "%08lx ", array[ adr * 32 + cnt ] );
		}
		usbbase->usb_IExec->DebugPrintF( "\nOut: " );
		for( ; cnt<32 ; cnt++ )
		{
			usbbase->usb_IExec->DebugPrintF( "%08lx ", array[ adr * 32 + cnt ] );
		}
		usbbase->usb_IExec->DebugPrintF( "\n##\n\n" );
		usbbase->usb_IExec->Enable();
	}
	#endif

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	if ( ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) )
	{
		SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

		goto bailout;
	}

	// Enumeration fetches and validates one configuration.
	cn = fn->fkt_Configs.uh_Head;

	if ( ( ! cn ) || ( cn->cfg_Number != CfgNr ) )
	{
		SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

		goto bailout;
	}

	if ( fn->fkt_Config_Active )
	{
		retval = fn->fkt_Config_Active == cn;

		SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

		goto bailout;
	}

	locked = CONFIG_LOCK( cn ) == LSTAT_Okay;

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	if ( ! locked )
	{
		goto bailout;
	}

	USBDEBUG( "__Config_Set             : Switching to Config nr #%lu", CfgNr );

	sd.RequestType	= REQTYPE_Write | REQTYPE_Standard | REQTYPE_Device;
	sd.RequestCode	= REQCODE_Set_Configuration;
	sd.Value		= LE_SWAP16( CfgNr );
	sd.Index		= LE_SWAP16( 0 );
	sd.Length		= LE_SWAP16( 0 );

	ioreq->req_Public.io_Command		= CMD_WRITE;
	ioreq->req_Public.io_Data		= NULL;
	ioreq->req_Public.io_Length		= 0;
	ioreq->req_Public.io_SetupData	= & sd;
	ioreq->req_Public.io_SetupLength	= sizeof( struct USB2_SetupData );

	IO_DO( ioreq );

	if ( ioreq->req_Public.io_Error != USB2Err_NoError )
	{
		USBDEBUG( "__Config_Set             : Set Configuration return %ld", (S32) ioreq->req_Public.io_Error );
		goto bailout;
	}

	// -- Settle Time (yikes busy wait)
	MISC_MICRODELAY( 25 * 1000 );	// 25ms

	// --
	#if 0
	// My Competition Pro joystick do not return correct ConfigNr value
	// so disabling for now.. there are proberly many more that would fail

	USBDEBUG( "__Config_Set             : Try and Verifying Config nr" );

	sd.RequestType	= REQTYPE_Read | REQTYPE_Standard | REQTYPE_Device;
	sd.RequestCode	= REQCODE_Get_Configuration;
	sd.Value		= LE_SWAP16( 0 );
	sd.Index		= LE_SWAP16( 0 );
	sd.Length		= LE_SWAP16( 1 );

	ioreq->req_Public.io_Command		= CMD_READ;
	ioreq->req_Public.io_Data			= data;
	ioreq->req_Public.io_Length			= 1;
	ioreq->req_Public.io_SetupData		= & sd;
	ioreq->req_Public.io_SetupLength	= sizeof( struct USB2_SetupData );

	#ifdef DO_PANIC
	data[0] = 172;
	#endif

	IO_DO( ioreq );

	USBDEBUG( "io_Actual ... : %lu", ioreq->req_Public.io_Actual );
	USBDEBUG( "io_Error .... : %ld", (S32) ioreq->req_Public.io_Error );
	USBDEBUG( "data[0] ..... : %lu", (U32) data[0] );

	/**/ if ( ioreq->req_Public.io_Error == USB2Err_Host_Stall )
	{
		// Not all Devices support Get Config
		data[0] = CfgNr;
	}
	else if ( ioreq->req_Public.io_Error != USB2Err_NoError )
	{
		USBDEBUG( "__Config_Set             : Get Configuration return %ld", ioreq->req_Public.io_Error );
		goto bailout;
	}

	if ( data[0] != CfgNr )
	{
		USBDEBUG( "__Config_Set             : Switching Configuration failed ( %lu != %lu )", (U32) data[0], CfgNr );
		goto bailout;
	}
	#endif

	retval = __Update( usbbase, fn, cn );

bailout:

	if ( locked )
	{
		CONFIG_UNLOCK( cn );
	}

	#if 0
	{
		struct USB2_EndPointNode **array = fn->fkt_HCD->hn_EndPoint_Array;
		U32 adr = fn->fkt_Address;
		U32 cnt;

		usbbase->usb_IExec->Disable();
		usbbase->usb_IExec->DebugPrintF( "\n## Active Config EndPoints (After)\nIn : " );
		for( cnt=0 ; cnt<16 ; cnt++ )
		{
			usbbase->usb_IExec->DebugPrintF( "%08lx ", array[ adr * 32 + cnt ] );
		}
		usbbase->usb_IExec->DebugPrintF( "\nOut: " );
		for( ; cnt<32 ; cnt++ )
		{
			usbbase->usb_IExec->DebugPrintF( "%08lx ", array[ adr * 32 + cnt ] );
		}
		usbbase->usb_IExec->DebugPrintF( "\n##\n\n" );
		usbbase->usb_IExec->Enable();
	}
	#endif

	return( retval );
}

// --
