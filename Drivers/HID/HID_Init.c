
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_CODE S32 HID_Init( struct USBBase *usbbase, struct HIDData *hd )
{
struct USB2_Interface_Desc *ifcdsc;
struct USB2_IORequest *ioreq;
struct USB2_SetupData *sd;
struct HID_ReportNode *rn;
S32 retval;
S32 ifcnr;
STR name;
PTR init;
PTR main;
PTR free;
U32 cnt;

	retval = FALSE;

	TASK_NAME_ENTER( "HID_Init" );

	USBDEBUG( "HID : HID_Init" );

	// --

	hd->TaskAdr = TASK_FIND();
	ASYNC_INIT( & hd->ASync_Drivers, hd->TaskAdr );
	SEMAPHORE_INIT( & hd->Semaphore );

	// --

	if ( ! MSGPORT_INIT( & hd->Timer_MsgPort ))
	{
		USBDEBUG( "Timer MsgPort init failed" );
		goto bailout;
	}

	MEM_COPY( usbbase->usb_TimeRequest, & hd->Timer_IOReq, sizeof( struct TimeRequest ));

	hd->Timer_IOReq.Request.io_Message.mn_ReplyPort = & hd->Timer_MsgPort.ump_MsgPort;
	hd->Timer_IOReq.Time.Microseconds = 0;
	hd->Timer_IOReq.Time.Seconds = 1;

	// --

	hd->Register = REGISTER_REGISTERTAGS(
		USB2Tag_Reg_DriverMessage, hd->StartMsg,
		USB2Tag_Reg_Title, "HID",
		TAG_END
	);

	if ( ! hd->Register )
	{
		USBDEBUG( "HID_Init : Error Registering Device" );
		goto bailout;
	}

	hd->Res_Control = hd->Register->reg_Public.Res_Control;
	ifcdsc	= hd->StartMsg->InterfaceDescriptor;
	ioreq	= hd->Res_Control->IORequests[0];
	sd		= hd->Res_Control->SetupData;
	ifcnr	= ifcdsc->InterfaceNumber;

	// --

	hd->Res_Interrupt = ENDPOINTRES_OBTAINTAGS( hd->Register,
		USB2Tag_EPRes_EPType, EPATT_Type_Interrupt,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		USB2Tag_EPRes_NrOfIORequest, HID_IOReqCount,
		USB2Tag_EPRes_InterfaceNr, ifcnr,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		TAG_END
	);

	if ( ! hd->Res_Interrupt )
	{
		USBDEBUG( "HID_Init : Error obtaining EndPoint Resource" );
		goto bailout;
	}

	// --
	//  Switch to Report Protocol

	// Is it running in BOOT Protocol mode
	if ( ifcdsc->InterfaceSubClass != USBHID_SUBCLASS_REPORT )
	{
		USBDEBUG( "HID_Init : Trying to switch to Report Mode" );

		#if 1
		// ah it is lets try and change it to Report.
		sd->RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
		sd->RequestCode = REQCODE_Set_Protocol;
		sd->Value		= LE_SWAP16( 1 );		// Report Protocol
		sd->Index		= LE_SWAP16( ifcnr );
		sd->Length		= LE_SWAP16( 0 );

		ioreq->io_Command		= CMD_WRITE;
		ioreq->io_Data			= NULL;
		ioreq->io_Length		= 0;
		ioreq->io_SetupData		= sd;
		ioreq->io_SetupLength	= sizeof( struct USB2_SetupData );

		IO_DO( ioreq );

		if ( ioreq->io_Error == USB2Err_NoError )
		{
			hd->Driver_Mode = HID_DMode_Report;
		}
		else
		#endif
		{
			hd->Driver_Mode = HID_DMode_Boot;
		}
	}
	else
	{
			hd->Driver_Mode = HID_DMode_Report;
	}

	// -- Check Supported modes

	/**/ if ( hd->Driver_Mode == HID_DMode_Boot )
	{
		/**/ if ( ifcdsc->InterfaceProtocol == USBHID_PROTOCOL_KEYBOARD )
		{
			USBDEBUG( "HID_Init : Boot Keyboard" );
			hd->Driver_Type = HID_DType_Keyboard;

			name = "Boot Mouse";
			init = __Boot_Keyboard_Init;
			main = __Boot_Keyboard_Main;
			free = __Boot_Keyboard_Free;
		}
		else if ( ifcdsc->InterfaceProtocol == USBHID_PROTOCOL_MOUSE )
		{
			USBDEBUG( "HID_Init : Boot Mouse" );
			hd->Driver_Type = HID_DType_Mouse;

			name = "Boot Keyboard";
			init = __Boot_Mouse_Init;
			main = __Boot_Mouse_Main;
			free = __Boot_Mouse_Free;
		}
		else
		{
			USBDEBUG( "HID_Init : Unsupported HID Boot protocol (%ld)", ifcdsc->InterfaceProtocol );
			goto bailout;
		}

		// --

		rn = Node_Report_Alloc( usbbase, hd );

		if ( ! rn )
		{
			USBERROR( "Error allocating memory" );
			goto bailout;
		}

		NODE_ADDTAIL( & hd->Reports, rn );
		rn->TaskName = name;

		// --

		TASK_START(
			TASK_Tag_Func_Init, init,
			TASK_Tag_Func_Main, main,
			TASK_Tag_Func_Free, free,
			TASK_Tag_UserData, rn,
			TASK_Tag_Prioity, hd->TaskAdr->tc_Node.ln_Pri,
			TASK_Tag_ASync, & hd->ASync_Drivers,
			TASK_Tag_Type, TASK_Type_HID,
			TASK_Tag_Name, name,
			TAG_END
		);

		// --
	}
	else if ( hd->Driver_Mode == HID_DMode_Report )
	{
		for( cnt=0 ; cnt<5 ; cnt++ )
		{
			if ( Report_Read_Report( usbbase, hd, ifcnr ))
			{
				break;
			}
		}

		if ( cnt == 5 )
		{
			goto bailout;
		}

		if ( Report_Parse( usbbase, hd ))
		{
			goto bailout;
		}

		if ( Report_Bind_Drivers( usbbase, hd ))
		{
			goto bailout;
		}
	}
	else
	{
		USBDEBUG( "HID_Init : Error unknown HID Device type" );
		goto bailout;
	}

	// --

	retval = TRUE;

	// --

bailout:

	TASK_NAME_LEAVE();

	return( retval );
}

// --
