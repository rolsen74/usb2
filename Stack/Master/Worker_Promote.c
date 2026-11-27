
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

struct WorkerStart
{
	struct CmdHeader				  Command;
	struct WorkerState *			  Worker;
	const struct USB2_WorkerHandoff * Handoff;
	struct Task *					  Replacement;
	U32								  Signals;
	U32								  ResumeMask;
};

// --

SEC_CODE static void
Worker_Process_Entry ( void )
{
	struct WorkerStart * start;
	struct WorkerState	 worker;
	struct USBBase *	 usbbase;
	struct ExecIFace *	 IExec;
	struct Task *		 self;

	void ( *entry ) ( struct USBBase *, PTR, PTR );
	PTR data;
	U32 signals;
	U32 bit;
	S32 signal;
	S32 result;

	IExec = USB2_GetExecIFace ();

	self = IExec->FindTask ( NULL );

	start			  = self->tc_UserData;
	self->tc_UserData = NULL;
	usbbase			  = start->Worker->Base;

	MEM_COPY ( start->Worker, &worker, sizeof ( worker ) );

	worker.OriginalName = self->tc_Node.ln_Name;
	entry				= start->Handoff->Entry;
	data				= start->Handoff->Data;
	signals				= start->Signals;
	result				= TRUE;

#ifdef DO_DEBUG
	worker.OriginalSignals = IExec->SetSignal ( 0, 0 );
#endif

	// Keep the numbers used by published ports and controller IRQ handlers.
	for ( bit = 0; ( signals ) && ( bit < 32 ); bit++ )
	{
		if ( signals & ( 1UL << bit ) )
		{
			if ( IExec->AllocSignal ( bit ) < 0 )
			{
				result = FALSE;
				break;
			}

			signals &= ~( 1UL << bit );
		}
	}

	signal			  = ( result ) ? IExec->AllocSignal ( -1 ) : -1;
	result			  = signal >= 0;
	worker.ResumeMask = ( result ) ? 1UL << signal : 0;

	if ( result )
	{
		start->Replacement		 = self;
		start->ResumeMask		 = worker.ResumeMask;
		start->Command.cm_Result = TASK_Return_Stack_Okay;
	}

	IExec->Signal ( start->Command.cm_Task, start->Command.cm_Mask );

	if ( result )
	{
		IExec->Wait ( worker.ResumeMask );

		IExec->FreeSignal ( signal );

		self->tc_Node.ln_Name = worker.Node->tn_Name;

		IExec->DebugPrintF ( "USB : '%s' promoted to process\n", self->tc_Node.ln_Name );

		entry ( worker.Base, data, worker.Intern );

		CmdStart_Finish ( &worker );
	}
}

// --

SEC_CODE static S32
Cmd_Promote_Worker ( struct USBBase * usbbase, struct WorkerStart * start )
{
	struct ExecIFace * IExec;
	struct DOSIFace *  IDOS;

	IExec = usbbase->usb_IExec;
	IDOS  = usbbase->usb_IDOS;

	if ( ( ! IDOS ) || ( IExec->FindTask ( NULL )->tc_Node.ln_Type != NT_PROCESS ) )
	{
		return ( FALSE );
	}

	// DOS assigns the calling process as the parent of NP_Child workers.
	// Avoid opening files or locking directories while a USB controller waits.
	return ( IDOS->CreateNewProcTags ( NP_Name, start->Worker->Node->tn_Name, NP_Child, TRUE, NP_Priority,
									   start->Command.cm_Task->tc_Node.ln_Pri, NP_StackSize, start->Worker->StackSize, NP_Input,
									   0, NP_Output, 0, NP_CurrentDir, 0, NP_ProgramDir, 0, NP_CopyVars, FALSE, NP_Entry,
									   Worker_Process_Entry, NP_UserData, start, TAG_END )
			 != NULL );
}

// --

SEC_CODE S32
USB2_PromoteWorker ( struct USBBase * usbbase, const struct USB2_WorkerHandoff * handoff )
{
	struct WorkerStart			  start;
	struct WorkerState *		  worker;
	struct ExecIFace *			  IExec;
	struct Task *				  self;
	struct Task *				  replacement;
	struct MsgPort *			  port;
	struct MsgPort *			  ports[sizeof ( handoff->Ports ) / sizeof ( handoff->Ports[0] )
										+ sizeof ( handoff->Endpoints ) / sizeof ( handoff->Endpoints[0] )];
	struct RealEndPointResource * resource;

	U32 count;
	U32 pending;
	U32 i;

	if ( ( ! usbbase->usb_WorkerPromotionReady ) || ( ! usbbase->usb_IDOS ) )
	{
		return ( FALSE );
	}

	IExec = usbbase->usb_IExec;

	self = IExec->FindTask ( NULL );

	if ( ( self->tc_Node.ln_Type != NT_TASK ) || ( ! self->tc_UserData ) )
	{
		return ( FALSE );
	}

	worker = self->tc_UserData;

	if ( ( worker->PromotionAttempted ) || ( worker->Node->tn_StopRequested ) )
	{
		return ( FALSE );
	}

	worker->PromotionAttempted = TRUE;
	start.Command.cm_Command   = CMD_PROMOTE_WORKER;
	start.Worker			   = worker;
	start.Handoff			   = handoff;
	start.Signals			   = handoff->Signals;

	count = 0;

	for ( i = 0; i < sizeof ( handoff->Ports ) / sizeof ( handoff->Ports[0] ); i++ )
	{
		port = handoff->Ports[i];

		if ( ( port ) && ( port->mp_SigTask == self ) )
		{
			ports[count++] = port;
			start.Signals |= 1UL << port->mp_SigBit;
		}
	}

	for ( i = 0; i < sizeof ( handoff->Endpoints ) / sizeof ( handoff->Endpoints[0] ); i++ )
	{
		port = ( handoff->Endpoints[i] ) ? handoff->Endpoints[i]->MsgPort : NULL;

		if ( ( port ) && ( port->mp_SigTask == self ) )
		{
			ports[count++] = port;
			start.Signals |= 1UL << port->mp_SigBit;
		}
	}

	if ( MISC_DOCOMMAND ( &start.Command ) != TASK_Return_Stack_Okay )
	{
		IExec->DebugPrintF ( "USB : could not promote '%s'\n", worker->Node->tn_Name );

		return ( FALSE );
	}

	replacement = start.Replacement;

	// Publish the destination and replay signals atomically with I/O replies and IRQs.
	IExec->Disable ();

	pending = IExec->SetSignal ( 0, 0 );
	pending |= handoff->Pending;
	pending &= start.Signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D;

	for ( i = 0; i < count; i++ )
	{
		port			 = ports[i];
		port->mp_SigTask = replacement;

		if ( port->mp_MsgList.lh_Head->ln_Succ )
		{
			pending |= 1UL << port->mp_SigBit;
		}
	}

	for ( i = 0; i < sizeof ( handoff->Endpoints ) / sizeof ( handoff->Endpoints[0] ); i++ )
	{
		if ( handoff->Endpoints[i] )
		{
			resource = (PTR)( (U8 *)handoff->Endpoints[i] - offsetof ( struct RealEndPointResource, epr_Public ) );

			if ( resource->epr_ReplyOwner == self )
			{
				resource->epr_ReplyOwner = replacement;
			}
		}
	}

	for ( i = 0; i < sizeof ( handoff->Owners ) / sizeof ( handoff->Owners[0] ); i++ )
	{
		if ( handoff->Owners[i] )
		{
			*handoff->Owners[i] = replacement;
		}
	}

	worker->Replacement		 = replacement;
	worker->ResumeMask		 = start.ResumeMask;
	worker->Node->tn_TaskAdr = replacement;

	IExec->Signal ( replacement, pending );

	IExec->Enable ();

	return ( TRUE );
}

// --
