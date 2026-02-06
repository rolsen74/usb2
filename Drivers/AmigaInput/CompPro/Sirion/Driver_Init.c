
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

S32 Driver_Init( struct USB_Driver *in )
{
struct USBBusSetupData sd;
struct USBBusDscHead *dsc;
struct USBBusIntDsc *dsc_ifc;
struct USBBusCfgDsc *dsc_cfg;
struct USBBusEPDsc *dsc_ep;
struct USBIOReq *ioreq;
S32 retval;
S32 stat;
S32 run;

	retval = FALSE;

	// --
	// Get USBSys Infterface

	in->IUSBSys = (PTR) GetInterface( in->USBSysBase, "main", 1, NULL );

	if ( ! in->IUSBSys )
	{
		#ifdef DEBUG
		DebugPrintF( "Error Getting USBSys Interface\n" );
		#endif
		goto bailout;
	}

	IUSBSys = in->IUSBSys;

	// --

	/* Setup and Claim Interface */

	in->USB_Function_MsgPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! in->USB_Function_MsgPort )
	{
		#ifdef DEBUG
		DebugPrintF( "Error creating MsgPort\n" );
		#endif
//		IUSBSys->USBLogPuts( -1, (STR) "XBox GamePad Controller", (STR) "Error creating MsgPort" );
		goto bailout;
	}

	in->USB_Function_Bit = 1UL << in->USB_Function_MsgPort->mp_SigBit;

	// --
	// Claim Function

	in->USB_Function = USBClaimFunction( in->USB_RawFunction, in, in->USB_Function_MsgPort );

	if ( ! in->USB_Function )
	{
		#ifdef DEBUG
		DebugPrintF( "Error claiming USB Function\n" );
		#endif
//		IUSBSys->USBLogPuts( -1, (STRPTR)"XBox GamePad Controller", (STRPTR)"Error claiming USB Function" );
		goto bailout;
	}

	// --
	// Get and Set Configureation

	in->USB_Config_Descs = (PTR) USBFktGetCfgDescriptorsA( in->USB_IOReq, in->USB_Function, 0, NULL );

	if ( ! in->USB_Config_Descs )
	{
		#ifdef DEBUG
		DebugPrintF( "Could not get configuration descriptor from USB function\n" );
		#endif
//		IUSBSys->USBLogPuts( -1, (STRPTR)"XBox GamePad Controller", (STRPTR)"Could not get configuration descriptor from USB function" );
		goto bailout;
	}

	if ( USBFktSetConfigurationA( in->USB_IOReq, in->USB_Function, (PTR) in->USB_Config_Descs, NULL ) != USBSETCONFIG_OK )
	{
		#ifdef DEBUG
		DebugPrintF( "Could not set configuration descriptor from USB function\n" );
		#endif
//		IUSBSys->USBLogPuts( -1, (STRPTR)"XBox GamePad Controller", (STRPTR)"Could not set configuration descriptor from USB function" );
		goto bailout;
	}

	// -- 
	// -- Find Correct Config

	run = TRUE;
	dsc = (PTR) in->USB_Config_Descs;

	while(( dsc ) && ( run ))
	{
//		DebugPrintF( "Find cfg : current %p\n", dsc );

		switch( dsc->dh_Type )
		{
			case USBDESC_CONFIGURATION:
			{
				dsc_cfg = (PTR) dsc;

				// Find own Config
//				if ( dsc_cfg->cd_ConfigID == cfgnr )
				{
					DebugPrintF( "Found cfg %p\n", dsc );
					run = FALSE;
				}

				in->USB_Config_Dsc = dsc_cfg;
				break;
			}
		}

		dsc = USBNextDescriptor( dsc );
	}

	// --
	// Find Interfaces and Bulk EndPoints

	run = TRUE;
	dsc_ifc = NULL;

	while(( dsc ) && ( run ))
	{
//		DebugPrintF( "Find : current %p\n", dsc );

		switch( dsc->dh_Type )
		{
			case USBDESC_CONFIGURATION:
			{
				dsc_cfg = (PTR) dsc;

				DebugPrintF( "Config Dsc %p\n", dsc );

				// Skip own, but stop with the next one
				if ( dsc_cfg->cd_ConfigID != in->USB_Config_Dsc->cd_ConfigID )
				{
					run = FALSE;
				}
				break;
			}

			case USBDESC_INTERFACE:
			{
				dsc_ifc = (PTR) dsc;

				DebugPrintF( "Interface Dsc %p\n", dsc );

				if ( dsc_ifc->id_Class != USBCLASS_HID )
				{
					run = FALSE;
					#ifdef DEBUG
					DebugPrintF( "Interface Not a HID Class\n" );
					#endif
					break;
				}

				if ( in->USB_Interface_Dsc )
				{
					#ifdef DEBUG
					DebugPrintF( "Interface Allready Set Stopping\n" );
					#endif
					break;
				}

				in->USB_Interface_Dsc = (PTR) dsc;
				in->USB_Interface_Nr = in->USB_Interface_Dsc->id_InterfaceID;
				break;
			}

			case USBDESC_ENDPOINT:
			{
				dsc_ep = (PTR) dsc;

//				DebugPrintF( "EndPoint Dsc %p\n", dsc );

				if ( ! in->USB_HID_Dsc )
				{
					run = FALSE;
					#ifdef DEBUG
					DebugPrintF( "Error Endpoint found before HID Desc\n" );
					#endif
					break;
				}

				if ( ! in->USB_Interface_Dsc )
				{
					run = FALSE;
					#ifdef DEBUG
					DebugPrintF( "Error Endpoint found before Interface\n" );
					#endif
					break;
				}

				switch( dsc_ep->ed_Attributes )
				{
					case USBEPTT_INTERRUPT:
					{
						if ( dsc_ep->ed_Address & 0x80 )
						{
							#ifdef DEBUG
							DebugPrintF( "Found Interrupt In EndPoint Descriptor (EP: %02lx)\n", dsc_ep->ed_Address );
							#endif

							if ( in->USB_IntIn_EP_Dsc )
							{
								run = FALSE;
								#ifdef DEBUG
								DebugPrintF( "Error we already have a Interrupt In EndPoint Descriptor\n" );
								#endif
								break;
							}

							in->USB_IntIn_EP_Dsc = (PTR) dsc;
						}
						else
						{
							#ifdef DEBUG
							DebugPrintF( "Found Interrupt Out EndPoint Descriptor (EP: %02lx)\n", dsc_ep->ed_Address );
							#endif

							if ( in->USB_IntOut_EP_Dsc )
							{
								run = FALSE;
								#ifdef DEBUG
								DebugPrintF( "Error we already have a Interrupt Out EndPoint Descriptor\n" );
								#endif
								break;
							}

							in->USB_IntOut_EP_Dsc = (PTR) dsc;
						}
						break;
					}

					default:
					{
						DebugPrintF( "Unkown EndPoint Descriptor (EP: %02lx)\n", dsc_ep->ed_Address );
						break;
					}
				}
				break;
			}

			case 0x21: // HID Report Mode
			{
				#ifdef DEBUG
				DebugPrintF( "Found HID Descriptor : %p\n", dsc );
				#endif

				if ( in->USB_HID_Dsc )
				{
					run = FALSE;
					#ifdef DEBUG
					DebugPrintF( "Error we already have a HID Descriptor\n" );
					#endif
					break;
				}

				if ( ! in->USB_Interface_Dsc )
				{
					run = FALSE;
					#ifdef DEBUG
					DebugPrintF( "Error HID Desc found before Interface\n" );
					#endif
					break;
				}

				in->USB_HID_Dsc = (PTR) dsc;
				break;
			}

			default:
			{
				DebugPrintF( "Unknown Dsc %p, Type $%02lx\n", dsc, (U32) dsc->dh_Type );
				break;
			}
		}

		dsc = USBNextDescriptor( dsc );
	}

	if (( ! in->USB_HID_Dsc )
	||	( ! in->USB_IntIn_EP_Dsc ) 
	||	( ! in->USB_IntOut_EP_Dsc )
	||	( ! in->USB_Interface_Dsc ))
	{
		#ifdef DEBUG
		DebugPrintF( "Error one or more Descriptors is missing\n" );
		#endif
		goto bailout;
	}


	// -- 
	// Get Interface from Claimed Function

	in->USB_Interface = USBGetInterface( in->USB_Function, in->USB_Interface_Nr );

	if ( ! in->USB_Interface )
	{
		#ifdef DEBUG
		DebugPrintF( "Error getting USB Interface %lu\n", in->USB_Interface_Nr );
		#endif
		goto bailout;
	}

	// --
	// Get Control Info
	in->USB_Ctrl_EP_Ref = USBGetEndPoint( NULL, in->USB_Interface, (U32) 0 );

	if ( ! in->USB_Ctrl_EP_Ref )
	{
		#ifdef DEBUG
		DebugPrintF( "Error getting USB Control EndPoint\n" );
		#endif
		goto bailout;
	}

	in->USB_Ctrl_MP = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! in->USB_Ctrl_MP )
	{
		#ifdef DEBUG
		DebugPrintF( "Error creating USB Control MsgPort\n" );
		#endif
		goto bailout;
	}

	in->USB_Ctrl_MP_Bit = 1UL << in->USB_Ctrl_MP->mp_SigBit;

	in->USB_Ctrl_IOReq = USBAllocRequestA( in->USB_IOReq, NULL );

	if ( ! in->USB_Ctrl_IOReq )
	{
		#ifdef DEBUG
		DebugPrintF( "Error creating USB Control In IORequest\n" );
		#endif
		goto bailout;
	}

	in->USB_Ctrl_IOReq->io_Message.mn_ReplyPort = in->USB_Ctrl_MP;



	// --
	// Get Interrupt In Info
	in->USB_IntIn_EP_Ref = USBGetEndPoint( NULL, in->USB_Interface, (U32) in->USB_IntIn_EP_Dsc->ed_Address );

	if ( ! in->USB_IntIn_EP_Ref )
	{
		#ifdef DEBUG
		DebugPrintF( "Error getting USB Interrupt In EndPoint\n" );
		#endif
		goto bailout;
	}

	in->USB_IntIn_MP = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! in->USB_IntIn_MP )
	{
		#ifdef DEBUG
		DebugPrintF( "Error creating USB Interrupt In MsgPort\n" );
		#endif
		goto bailout;
	}

	in->USB_IntIn_MP_Bit = 1UL << in->USB_IntIn_MP->mp_SigBit;

	in->USB_IntIn_IOReq = USBAllocRequestA( in->USB_IOReq, NULL );

	if ( ! in->USB_IntIn_IOReq )
	{
		#ifdef DEBUG
		DebugPrintF( "Error creating USB Interrupt In IORequest\n" );
		#endif
		goto bailout;
	}

	in->USB_IntIn_IOReq->io_Message.mn_ReplyPort = in->USB_IntIn_MP;

	in->USB_IntIn_BufSize	= LE_SWAP16( in->USB_IntIn_EP_Dsc->ed_MaxPacketSize );
	in->USB_IntIn_Buffer	= AllocVecTags( in->USB_IntIn_BufSize,
		AVT_Clear, 0,
		TAG_END
	);

	if ( ! in->USB_IntIn_Buffer )
	{
		#ifdef DEBUG
		DebugPrintF( "Error Allocating USB Interrupt In Buffer\n" );
		#endif
		goto bailout;
	}



	// --
	// Get Interrupt Out Info
	in->USB_IntOut_EP_Ref = USBGetEndPoint( NULL, in->USB_Interface, (U32) in->USB_IntOut_EP_Dsc->ed_Address );

	if ( ! in->USB_IntOut_EP_Ref )
	{
		#ifdef DEBUG
		DebugPrintF( "Error getting USB Interrupt Out EndPoint\n" );
		#endif
		goto bailout;
	}

	in->USB_IntOut_MP = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! in->USB_IntOut_MP )
	{
		#ifdef DEBUG
		DebugPrintF( "Error creating USB Interrupt Out MsgPort\n" );
		#endif
		goto bailout;
	}

	in->USB_IntOut_MP_Bit = 1UL << in->USB_IntOut_MP->mp_SigBit;

	in->USB_IntOut_IOReq = USBAllocRequestA( in->USB_IOReq, NULL );

	if ( ! in->USB_IntOut_IOReq )
	{
		#ifdef DEBUG
		DebugPrintF( "Error creating USB Interrupt Out IORequest\n" );
		#endif
		goto bailout;
	}

	in->USB_IntOut_IOReq->io_Message.mn_ReplyPort = in->USB_IntOut_MP;

	in->USB_IntOut_BufSize	= LE_SWAP16( in->USB_IntOut_EP_Dsc->ed_MaxPacketSize );
	in->USB_IntOut_Buffer	= AllocVecTags( in->USB_IntOut_BufSize,
		AVT_Clear, 0,
		TAG_END
	);

	if ( ! in->USB_IntOut_Buffer )
	{
		#ifdef DEBUG
		DebugPrintF( "Error Allocating USB Interrupt Out Buffer\n" );
		#endif
		goto bailout;
	}

	// --
	// Change to Report Mode
	// Do own function? Set Protocol Mode?

	sd.sd_RequestType = USBSDT_DIR_HOSTTODEV | USBSDT_TYP_CLASS | USBSDT_REC_INTERFACE;
	sd.sd_Request	= 0x0b;
	sd.sd_Value		= LE_SWAP16( 1 );
	sd.sd_Index		= LE_SWAP16( in->USB_Interface_Nr );
	sd.sd_Length	= LE_SWAP16( 0 );

	ioreq = in->USB_Ctrl_IOReq;
	ioreq->io_SetupLength = sizeof( struct USBBusSetupData );
	ioreq->io_SetupData = & sd;
	ioreq->io_EndPoint = in->USB_Ctrl_EP_Ref;
	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Length = 0;
	ioreq->io_Data = NULL;

	stat = DoIO( (PTR) ioreq );

	if ( stat )
	{
		#ifdef DEBUG
		DebugPrintF( "Error Setting HID Report mode (%ld)\n", stat );
		#endif
		goto bailout;
	}




	// --
	// Retrive Report

	if ( ! Driver_Report_Get( in ))
	{
		#ifdef DEBUG
		DebugPrintF( "Error Getting HID Report\n" );
		#endif
		goto bailout;
	}

	// --
	// Parse Report

	if ( ! Driver_Report_Parse( in ))
	{
		#ifdef DEBUG
		DebugPrintF( "Error Parsing HID Report\n" );
		#endif
		goto bailout;
	}

	// ---




		DebugPrintF( "yay so far\n" );




	retval = TRUE;

bailout:

	return( retval );
}

// --
