
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE struct USB2_EPResource *
__EndPointRes_ObtainList ( struct USBBase * usbbase, STR file UNUSED, struct RealRegister * reg, struct TagItem * taglist )

#else

SEC_CODE struct USB2_EPResource *
__EndPointRes_ObtainList ( struct USBBase * usbbase, struct RealRegister * reg, struct TagItem * taglist )

#endif

{
	struct RealEndPointResource * epr;
	struct USB2_EPResource *	  result;
	struct RealRequest *		  request;
	struct USB2_InterfaceHeader * ih;
	struct USB2_InterfaceGroup *  ig;
	struct USB2_InterfaceNode *	  in;
	struct USB2_EndPointNode *	  epn;
	struct RealFunctionNode *	  fn;
	struct USB2_ConfigNode *	  cn;
	struct TagItem *			  tag;
	struct MsgPort *			  mp;

	U32 shortp;
	U32 eptype;
	U32 addzp;
	U32 epdir;
	U32 ifcnr;
	U32 epnr;
	U32 cnt;
	U32 noi;
	U32 dir;
	S32 bs;
	U32 to;

	// --

	//	USBDEBUG( "__EndPointRes_ObtainList : REG   %p : (%s)", reg, (file)?file:"<NULL>" );
	//	USBDEBUG( "__EndPointRes_ObtainList" );

	// --

	result = NULL;
	eptype = -1U;
	epdir  = EPDIR_Both;
	epnr   = -1U;
	epn	   = NULL;
	ifcnr  = -1U;  // Interface Nr or -1 for unset
	noi	   = 1;	   // Number of IORequest
	bs	   = 0;	   // Buffer Size
	to	   = 0;	   // TimeOut in Seconds
	mp	   = NULL; // (user) MsgPort
	addzp  = -1U;  // Add Zero Packet (if needed on write)
	shortp = -1U;  // Allow short Packets (read)

	// --

	epr = ENDPOINTRES_ALLOC ();

	if ( ! epr )
	{
		USBERROR ( "__EndPointRes_ObtainList : Error allocating memory" );
		goto bailout;
	}

	// --

	if ( REGISTER_LOCK ( reg ) != LSTAT_Okay )
	{
		USBERROR ( "__EndPointRes_ObtainList : Invalid REG %p", reg );
		goto bailout;
	}

	epr->epr_Register = reg;

	// --

	//	USBDEBUG( "__EndPointRes_ObtainList : 2 :" );

	fn = reg->reg_Function;

	while ( ( tag = MISC_NEXTTAGITEM ( &taglist ) ) )
	{
		switch ( tag->ti_Tag )
		{
				// -- IORequest settings

			case USB2Tag_EPRes_MsgPort:
			{
				mp = (PTR)tag->ti_Data;

				//				USBERROR( "USB2Tag_EPRes_MsgPort .. : %p", mp );
				break;
			}

			case USB2Tag_EPRes_BufferSize:
			{
				bs = tag->ti_Data;

				//				USBERROR( "USB2Tag_EPRes_BufferSize  : %ld", bs );
				break;
			}

			case USB2Tag_EPRes_TimeOut:
			{
				to = tag->ti_Data;

				//				USBERROR( "USB2Tag_EPRes_TimeOut ... : %lu ms", to );
				break;
			}

			case USB2Tag_EPRes_NrOfIORequest:
			{
				// Zero is valid
				noi = tag->ti_Data;

				//				USBERROR( "USB2Tag_EPRes_NrOfIOReq . : %lu", noi );
				break;
			}

				// -- Interface Nr is required for anything but Control

			case USB2Tag_EPRes_InterfaceNr:
			{
				ifcnr = tag->ti_Data;

				if ( ifcnr > 255 )
				{
					goto bailout;
				}

				//				USBERROR( "USB2Tag_EPRes_InterfaceNr : %lu", ifcnr );
				break;
			}

				// -- Use Direct EndPoint Nr

			case USB2Tag_EPRes_EndPointNr:
			{
				epnr = tag->ti_Data;

				if ( epnr & ~( EPADR_Dir_Mask | EPADR_EndPoint_Mask ) )
				{
					goto bailout;
				}

				// Note:
				//  watch out .. ep_Number must be 0x0F
				//  but this tag also have 0x80 for Direction

				//				USBERROR( "USB2Tag_EPRes_EndPointNr : %lu", epnr );
				break;
			}

				// -- Find EndPoint from Type and Direction

			case USB2Tag_EPRes_EPType:
			{
				eptype = ( tag->ti_Data & EPATT_Type_Mask );

				//				USBERROR( "EndPoint Type ........... : %lu ($%02lx)", eptype, tag->ti_Data );
				break;
			}

			case USB2Tag_EPRes_EPDirection:
			{
				if ( ( tag->ti_Data & EPADR_Dir_Mask ) == EPADR_Dir_In )
				{
					epdir = EPDIR_In;
				}
				else
				{
					epdir = EPDIR_Out;
				}

				//				USBERROR( "EndPoint Direction ...... : %lu ($%02lx)", epdir, tag->ti_Data );
				break;
			}

			case USB2Tag_EPRes_AddZeroPacket:
			{
				addzp = ( tag->ti_Data ) ? TRUE : FALSE;

				//				USBERROR( "USB2Tag_EPRes_AddZeroPacket %ld", addzp );
				break;
			}

				// --

			case USB2Tag_EPRes_AllowShortPackets:
			{
				shortp = ( tag->ti_Data ) ? TRUE : FALSE;

				//				USBERROR( "USB2Tag_EPRes_ShortPackets %ld", shortp );
				break;
			}

			default:
			{
				USBERROR ( "__EndPointRes_ObtainList : Unknown tag [%08lx:%08lx]", tag->ti_Tag, tag->ti_Data );
				break;
			}
		}
	}

	//	USBDEBUG( "__EndPointRes_ObtainList : 3 :" );

	// ---

#ifdef DO_PANIC

	if ( FUNCTION_VALID ( fn ) != VSTAT_Okay )
	{
		USBERROR ( "__EndPointRes_ObtainList : NULL Pointer" );
		goto bailout;
	}

#endif

	// ---

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	if ( ( reg->reg_FreeMe ) || ( ! fn ) || ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) || ( fn->fkt_PortRecovery ) )
	{
		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		goto bailout;
	}

	dir = ( epnr & EPADR_Dir_Mask ) ? EPDIR_In : EPDIR_Out;

	if ( ( ( epnr == -1U ) && ( ( eptype == EPATT_Type_Control ) || ( ( eptype == -1U ) && ( epdir == EPDIR_Both ) ) ) )
		 || ( ( epnr != -1U ) && ( EPADR_Dir_Number ( epnr ) == 0 ) ) )
	{
		if ( ( ( eptype == -1U ) || ( eptype == EPATT_Type_Control ) )
			 && ( ( epnr == -1U ) || ( epdir == EPDIR_Both ) || ( epdir == dir ) ) )
		{
			epn = fn->fkt_ControlEndPoint;
		}
	}
	else
	{
		//		USBDEBUG( "__EndPointRes_ObtainList : 6 :" );

		cn = fn->fkt_Config_Active;
		ig = ( cn ) ? cn->cfg_InterfaceGroups.uh_Head : NULL;

		epn = NULL;

		while ( ig )
		{
			ih = ig->ig_Group.uh_Head;

			while ( ih )
			{
				if ( ( ! ih->ih_Changing ) && ( ( ih->ih_Owner == reg ) || ( fn->fkt_Owner == reg ) )
					 && ( ( ifcnr == -1U ) || ( ifcnr == ih->ih_Number ) ) )
				{
					in = ih->ih_Active;

					epn = ( in ) ? in->in_EndPoints.uh_Head : NULL;

					while ( epn )
					{
						if ( ( ( epnr == -1U )
							   || ( ( epn->ep_Number == EPADR_Dir_Number ( epnr ) ) && ( epn->ep_Direction == dir ) ) )
							 && ( ( eptype == -1U ) || ( epn->ep_Type == eptype ) )
							 && ( ( epdir == EPDIR_Both ) || ( epn->ep_Direction == epdir ) ) )
						{
							break;
						}

						epn = Node_Next ( epn );
					}
				}

				if ( epn )
				{
					break;
				}

				ih = Node_Next ( ih );
			}

			if ( epn )
			{
				break;
			}

			ig = Node_Next ( ig );
		}
	}

	//	USBDEBUG( "__EndPointRes_ObtainList : 7 :" );

	if ( ( ! epn ) || ( ENDPOINT_LOCK ( epn ) != LSTAT_Okay ) )
	{
		SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

		USBERROR ( "__EndPointRes_ObtainList : EndPoint Node not found : EP    %p", epn );
		goto bailout;
	}

	epr->epr_EndPoint			= epn;
	epr->epr_Public.EndPoint	= (PTR)epn;
	epr->epr_Public.EndPointMax = epn->ep_MaxPacketSize;

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

	// ---

	if ( noi )
	{
		if ( noi >= (U32)-1 / sizeof ( PTR ) )
		{
			goto bailout;
		}

		epr->epr_RequestCount = noi;

		// Alloc Array -- Not the IORequests
		epr->epr_Public.IORequests = MEM_ALLOCVEC ( sizeof ( PTR ) * ( noi + 1 ), TRUE );

		if ( ! epr->epr_Public.IORequests )
		{
			USBERROR ( "__EndPointRes_ObtainList : Error allocaing memory" );
			goto bailout;
		}

		// Alloc Array -- Not the buffers
		epr->epr_Public.Buffers = MEM_ALLOCVEC ( sizeof ( PTR ) * ( noi + 1 ), TRUE );

		if ( ! epr->epr_Public.Buffers )
		{
			USBERROR ( "__EndPointRes_ObtainList : Error allocaing memory" );
			goto bailout;
		}

		if ( bs < 0 )
		{
			switch ( bs )
			{
				case USB2Val_BufferSize_MaxPacketSize:
					bs = epn->ep_MaxPacketSize * 1;
					break;
				case USB2Val_BufferSize_MaxPacketSize_2:
					bs = epn->ep_MaxPacketSize * 2;
					break;
				case USB2Val_BufferSize_MaxPacketSize_4:
					bs = epn->ep_MaxPacketSize * 4;
					break;
				case USB2Val_BufferSize_MaxPacketSize_8:
					bs = epn->ep_MaxPacketSize * 8;
					break;
				case USB2Val_BufferSize_MaxPacketSize_16:
					bs = epn->ep_MaxPacketSize * 16;
					break;
				case USB2Val_BufferSize_MaxPacketSize_32:
					bs = epn->ep_MaxPacketSize * 32;
					break;
				case USB2Val_BufferSize_MaxPacketSize_64:
					bs = epn->ep_MaxPacketSize * 64;
					break;
				case USB2Val_BufferSize_MaxPacketSize_128:
					bs = epn->ep_MaxPacketSize * 128;
					break;
				case USB2Val_BufferSize_MaxPacketSize_256:
					bs = epn->ep_MaxPacketSize * 256;
					break;

				default:
				{
					USBERROR ( "__EndPointRes_ObtainList : Error invalid Buffer size (%ld)", bs );
					goto bailout;
				}
			}
		}

		epr->epr_Public.BufferSize = bs;

		for ( cnt = 0; cnt < noi; cnt++ )
		{
			// if mp == NULL then alloc ioreq will create the msgport
			request = IOREQUEST_ALLOCTAGS (
				//				USB2Tag_ShortPackets, sp,
				( shortp == -1U ) ? TAG_IGNORE : USB2Tag_IOReq_AllowShortPackets, shortp,
				( addzp == -1U ) ? TAG_IGNORE : USB2Tag_IOReq_AddZeroPacket, addzp, USB2Tag_IOReq_EndPoint, epn,
				USB2Tag_IOReq_MsgPort, mp, USB2Tag_IOReq_TimeOut, to, TAG_END );

			//			USBDEBUG( "__EndPointRes_ObtainList : EPR   %p : IOReq %p : Pos %ld", epr, epr->epr_Public.IORequests,
			// cnt );

			if ( ! request )
			{
				USBERROR ( "__EndPointRes_ObtainList : Error creating IORequest" );
				goto bailout;
			}

			if ( ! mp )
			{
				epr->epr_ReplyOwner = request->req_MsgPort.ump_MsgPort.mp_SigTask;
			}

			request->req_Resource			= epr;
			epr->epr_Public.IORequests[cnt] = &request->req_Public;

			// Now we use only one MsgPort
			mp = epr->epr_Public.IORequests[0]->io_Message.mn_ReplyPort;

			if ( bs )
			{
				//				USBDEBUG( "__EndPointRes_ObtainList : BufferSize %ld", bs );

				// Clear is proberly not needed but why not to make sure
				epr->epr_Public.Buffers[cnt] = MEM_ALLOCIOBUFFER ( bs, TRUE );

				if ( ! epr->epr_Public.Buffers[cnt] )
				{
					USBERROR ( "__EndPointRes_ObtainList : Error allocating memory" );
					goto bailout;
				}

				epr->epr_Public.IORequests[cnt]->io_Data   = epr->epr_Public.Buffers[cnt];
				epr->epr_Public.IORequests[cnt]->io_Length = bs;
			}
		}
	}

	// --

	//	USBDEBUG( "__EndPointRes_ObtainList : 8 :" );

#ifdef DO_PANIC

	if ( ( noi ) && ( ! epr->epr_Public.IORequests ) )
	{
		USBPANIC ( "__EndPointRes_ObtainList : NULL Pointer 1" );
	}

	if ( ( noi ) && ( ! epr->epr_Public.IORequests[0] ) )
	{
		USBPANIC ( "__EndPointRes_ObtainList : NULL Pointer 2" );
	}

#endif

	if ( noi )
	{
		epr->epr_Public.MsgPort = epr->epr_Public.IORequests[0]->io_Message.mn_ReplyPort;
	}
	else
	{
		epr->epr_Public.MsgPort = mp;
	}

	if ( epr->epr_Public.MsgPort )
	{
		epr->epr_Public.MsgPortBit = 1UL << epr->epr_Public.MsgPort->mp_SigBit;
	}

	if ( epn->ep_Type == EPATT_Type_Control )
	{
		//		epr->epr_Public.SetupData	= MEM_ALLOCIOBUFFER( sizeof( struct USB2_SetupData ), FALSE );
		epr->epr_Public.SetupData = SETUPDATA_ALLOC ();

		if ( ! epr->epr_Public.SetupData )
		{
			USBERROR ( "__EndPointRes_ObtainList : Error allocating memory" );
			goto bailout;
		}
	}

	// --

	SEMAPHORE_OBTAIN ( &usbbase->usb_LockSemaphore );

	in = epn->ep_Interface;

	if ( ( ! reg->reg_FreeMe ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe )
		 && ( ! fn->fkt_PortRecovery ) && ( epn->ep_Active )
		 && ( ( ! in )
			  || ( ( ! in->in_Parent->ih_Changing ) && ( in->in_Parent->ih_Active == in )
				   && ( ( in->in_Parent->ih_Owner == reg ) || ( fn->fkt_Owner == reg ) ) ) ) )
	{
		NODE_ADDTAIL ( &reg->reg_EPRHeader, &epr->epr_Node );

		result = &epr->epr_Public;
	}

	SEMAPHORE_RELEASE ( &usbbase->usb_LockSemaphore );

bailout:

	if ( ( ! result ) && ( epr ) )
	{
		ENDPOINTRES_FREE ( epr );
	}

	return ( result );
}

// --

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )

SEC_CODE struct USB2_EPResource * VARARGS68K
__EndPointRes_ObtainTags ( struct USBBase * usbbase, STR file, struct RealRegister * reg, ... )

#else

SEC_CODE struct USB2_EPResource * VARARGS68K
__EndPointRes_ObtainTags ( struct USBBase * usbbase, struct RealRegister * reg, ... )

#endif

{
	struct USB2_EPResource * epr;
	va_list					 ap;

	TASK_NAME_ENTER ( "__EndPointRes_ObtainTags" );

	//	USBDEBUG( "__EndPointRes_ObtainTags : REG   %p : (%s)", reg, (file)?file:"<NULL>" );

	va_start ( ap, reg );

	epr = ENDPOINTRES_OBTAINLIST ( reg, va_getlinearva ( ap, struct TagItem * ) );

	va_end ( ap );

	//	USBDEBUG( "__EndPointRes_ObtainTags : REG   %p : EPR   %p (new)", reg, epr );

	TASK_NAME_LEAVE ();

	return ( epr );
}

// --
