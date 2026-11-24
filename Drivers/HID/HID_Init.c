
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

SEC_CODE static S32
SetProtocol( struct USBBase *usbbase, struct HIDData *hd, U16 protocol )
{
	struct USB2_IORequest *ioreq;
	struct USB2_SetupData *sd;

	ioreq = hd->Res_Control->IORequests[0];
	sd = hd->Res_Control->SetupData;
	sd->RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
	sd->RequestCode = REQCODE_Set_Protocol;
	sd->Value = LE_SWAP16( protocol );
	sd->Index = LE_SWAP16( hd->StartMsg->InterfaceDescriptor->InterfaceNumber );
	sd->Length = 0;
	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;
	ioreq->io_SetupData = sd;
	ioreq->io_SetupLength = sizeof( *sd );

	IO_DO( ioreq );

	return ( ioreq->io_Error == USB2Err_NoError );
}

// --

SEC_CODE static S32
StartBootWorker( struct USBBase *usbbase, struct HIDData *hd, U32 protocol )
{
	struct HID_ReportNode *rn;

	enum TaskReturn stat;

	rn = Node_Report_Alloc( usbbase, hd );

	if ( ! rn )
	{
		return ( FALSE );
	}

	NODE_ADDTAIL( &hd->Reports, rn );

	if ( protocol == USBHID_PROTOCOL_KEYBOARD )
	{
		rn->TaskName = "Boot Keyboard";

		stat = TASK_START(
			TASK_Tag_Func_Init, __Keyboard_Init,
			TASK_Tag_Func_Main, __Keyboard_Main,
			TASK_Tag_Func_Free, __Keyboard_Free,
			TASK_Tag_UserData, rn,
			TASK_Tag_Prioity, hd->TaskAdr->tc_Node.ln_Pri,
			TASK_Tag_ASync, &hd->ASync_Drivers,
			TASK_Tag_Type, TASK_Type_HID,
			TASK_Tag_Name, rn->TaskName,
			TAG_END
		);
	}
	else
	{
		rn->TaskName = "Boot Mouse";
		rn->Pointer_Data = __Boot_Mouse_Data;

		stat = TASK_START(
			TASK_Tag_Func_Init, HID_Pointer_Init,
			TASK_Tag_Func_Main, HID_Pointer_Main,
			TASK_Tag_Func_Free, HID_Pointer_Free,
			TASK_Tag_UserData, rn,
			TASK_Tag_Prioity, hd->TaskAdr->tc_Node.ln_Pri,
			TASK_Tag_ASync, &hd->ASync_Drivers,
			TASK_Tag_Type, TASK_Type_HID,
			TASK_Tag_Name, rn->TaskName,
			TAG_END
		);
	}

	return ( ( stat == TASK_Return_Stack_Okay ) || ( stat == TASK_Return_Okay ) );
}

// --

SEC_CODE static struct USB2_EPResource *
ObtainInterrupt( struct USBBase *usbbase, struct HIDData *hd, S32 size )
{
	return ( ENDPOINTRES_OBTAINTAGS( hd->Register,
		USB2Tag_EPRes_EPType, EPATT_Type_Interrupt,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		USB2Tag_EPRes_NrOfIORequest, HID_IOReqCount,
		USB2Tag_EPRes_InterfaceNr, hd->StartMsg->InterfaceDescriptor->InterfaceNumber,
		USB2Tag_EPRes_BufferSize, size,
		TAG_END
	) );
}

// --

SEC_CODE S32
HID_Init( struct USBBase *usbbase, struct HIDData *hd )
{
	struct USB2_Interface_Desc *ifcdsc;

	U32 boot;
	U32 parsed;
	U32 count;

	hd->TaskAdr = TASK_FIND();

	ASYNC_INIT( &hd->ASync_Drivers, hd->TaskAdr );

	SEMAPHORE_INIT( &hd->Semaphore );

	if ( ( ! MSGPORT_INIT( &hd->Timer_MsgPort ) ) || ( ! MSGPORT_INIT( &hd->Return_MsgPort ) ) )
	{
		return ( FALSE );
	}

	MEM_COPY( usbbase->usb_TimeRequest, &hd->Timer_IOReq, sizeof( struct TimeRequest ) );

	hd->Timer_IOReq.Request.io_Message.mn_ReplyPort = &hd->Timer_MsgPort.ump_MsgPort;

	hd->Register = REGISTER_REGISTERTAGS(
		USB2Tag_Reg_DriverMessage, hd->StartMsg,
		USB2Tag_Reg_Title, "HID",
		TAG_END
	);

	if ( ! hd->Register )
	{
		return ( FALSE );
	}

	hd->Res_Control = hd->Register->reg_Public.Res_Control;
	ifcdsc = hd->StartMsg->InterfaceDescriptor;
	boot = ( ifcdsc->InterfaceSubClass == USBHID_SUBCLASS_BOOT ) &&
		( ( ifcdsc->InterfaceProtocol == USBHID_PROTOCOL_KEYBOARD ) ||
		  ( ifcdsc->InterfaceProtocol == USBHID_PROTOCOL_MOUSE ) );
	parsed = FALSE;
	hd->Driver_Mode = HID_DMode_Report;

	if ( ( ifcdsc->InterfaceSubClass != USBHID_SUBCLASS_BOOT ) || ( SetProtocol( usbbase, hd, 1 ) ) )
	{
		for ( count = 0; ( count < 5 ) && ( ! hd->AllocationFailed ); count++ )
		{
			if ( Report_Read_Report( usbbase, hd, ifcdsc->InterfaceNumber ) )
			{
				parsed = ! Report_Parse( usbbase, hd );
				break;
			}
		}
	}

	if ( ( ! parsed ) || ( Report_Bind_Drivers( usbbase, hd ) ) )
	{
		if ( ( ! boot ) || ( hd->AllocationFailed ) || ( ! SetProtocol( usbbase, hd, 0 ) ) )
		{
			return ( FALSE );
		}

		Report_Free( usbbase, hd );

		hd->Driver_Mode = HID_DMode_Boot;
		hd->InputBufferSize = ( ifcdsc->InterfaceProtocol == USBHID_PROTOCOL_KEYBOARD ) ?
			sizeof( struct USB2_BootKeyboard ) : sizeof( struct USB2_BootMouse );
	}

	// Keep one full packet of room for devices that pad short reports.
	hd->Res_Interrupt = ObtainInterrupt( usbbase, hd, USB2Val_BufferSize_MaxPacketSize );

	if ( ( hd->Res_Interrupt ) && ( hd->InputBufferSize > hd->Res_Interrupt->BufferSize ) )
	{
		ENDPOINTRES_RELEASE( hd->Res_Interrupt );

		hd->Res_Interrupt = ObtainInterrupt( usbbase, hd, hd->InputBufferSize );
	}

	if ( ! hd->Res_Interrupt )
	{
		return ( FALSE );
	}

	if ( hd->Driver_Mode == HID_DMode_Boot )
	{
		return ( StartBootWorker( usbbase, hd, ifcdsc->InterfaceProtocol ) );
	}

	return ( TRUE );
}

// --
