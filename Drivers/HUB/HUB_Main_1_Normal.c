
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// --

#include "usb2_all.h"

#include "HUB.h"

// --

SEC_CODE static void
RecoverPorts( struct USBBase *usbbase, struct intern *in )
{
	struct RealFunctionNode *fn;
	struct USB2_HCDNode *hn;

	U32 port;
	U8 recovery;

	hn = in->Function->fkt_HCD;

	for( port = 1; port <= in->Ports; port++ )
	{
		recovery = PORT_RECOVERY_NONE;

		SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

		fn = in->PortFunctions[port];

		if ( ( fn ) && ( hn->hn_AcceptIO ) && ( ! in->Function->fkt_Detach )
		  && ( ! in->Function->fkt_FreeMe ) && ( fn->fkt_HCDGeneration == hn->hn_Generation ) )
		{
			recovery = fn->fkt_PortRecovery;

			if ( ( recovery == PORT_RECOVERY_REQUESTED ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe ) )
			{
				fn->fkt_PortRecovery = PORT_RECOVERY_DRAINING;
				hn->hn_PortRecoveryPending = TRUE;
			}
			else if ( recovery != PORT_RECOVERY_READY )
			{
				recovery = PORT_RECOVERY_NONE;
			}
		}

		SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

		if ( recovery == PORT_RECOVERY_REQUESTED )
		{
			FUNCTION_MARKDETACH( fn );

			TASK_STOP_FN( fn );
		}
		else if ( recovery == PORT_RECOVERY_READY )
		{
			HUB__Port_Chg( usbbase, in, port );
		}

		if ( in->PortWork )
		{
			HUB__Port_Probe_Holds( usbbase, in, port );
		}
	}
}

// --

SEC_CODE static void
HUB_Resume( struct USBBase *usbbase, PTR data, PTR unused UNUSED )
{
	struct intern *in;

	in = data;

	HUB_Main( usbbase, in );

	HUB_Free( usbbase, in );

	MEM_FREEVEC( in );
}

// --

SEC_CODE static S32
HUB_Promote( struct USBBase *usbbase, struct intern *in, U32 mask )
{
	struct USB2_WorkerHandoff handoff;

	MEM_SET( &handoff, 0, sizeof( handoff ) );

	handoff.Entry = HUB_Resume;
	handoff.Data = in;
	handoff.Ports[0] = &in->Register->reg_MsgPort.ump_MsgPort;
	handoff.Ports[1] = &in->Delay_MsgPort.ump_MsgPort;
	handoff.Ports[2] = &in->Tick_MsgPort.ump_MsgPort;
	handoff.Endpoints[0] = in->Res_Interrupt;
	handoff.Endpoints[1] = in->Res_Control;
	handoff.Owners[0] = &in->ASync_Drivers.ua_Parent;
	handoff.Owners[1] = &in->ASync_Functions.ua_Parent;
	handoff.Pending = mask;

	return ( USB2_PromoteWorker( usbbase, &handoff ) );
}

// --

SEC_CODE S32
HUB_Main__Normal( struct USBBase *usbbase, struct intern *in )
{
	U32 mask;
	U32 wait;
	U64 now;

	wait = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E | in->Res_Interrupt->MsgPortBit |
		in->Register->reg_Public.Stack_MsgPortBit | in->Tick_MsgPort.ump_Signal.sig_Signal_Mask;

	if ( ! in->Running )
	{
		in->Running = TRUE;

		in->HousekeepingDue = HUB_Uptime( usbbase ) + 2000000;

		HUB_Arm_Tick( usbbase, in );
	}

	while ( in->Running )
	{
		mask = TASK_WAIT( wait );

		if ( ( mask & SIGBREAKF_CTRL_E ) && ( ! ( mask & SIGBREAKF_CTRL_C ) ) && ( HUB_Promote( usbbase, in, mask ) ) )
		{
			return ( TRUE );
		}

		if ( mask & in->Register->reg_Public.Stack_MsgPortBit )
		{
			HUB_Handle_Stack( usbbase, in );
		}

		if ( ( mask & SIGBREAKF_CTRL_C ) || ( ! in->Running ) )
		{
			in->Running = FALSE;
			break;
		}

		if ( mask & in->Res_Interrupt->MsgPortBit )
		{
			HUB_Handle_Interrupt( usbbase, in );
		}

		if ( ! in->Running )
		{
			break;
		}

		if ( mask & SIGBREAKF_CTRL_D )
		{
			HUB_Main__Free_FNs( usbbase, in );
		}

		if ( ( mask & in->Tick_MsgPort.ump_Signal.sig_Signal_Mask ) &&
			( MSGPORT_GETMSG( &in->Tick_MsgPort ) ) )
		{
			in->Tick_TimerAdded = FALSE;

			now = HUB_Uptime( usbbase );

			if ( now >= in->HousekeepingDue )
			{
				RecoverPorts( usbbase, in );

				HUB_Main__Free_FNs( usbbase, in );

				in->HousekeepingDue = now + 2000000;
			}

			if ( in->PortWork )
			{
				HUB_Port_Work( usbbase, in );
			}
		}

		HUB_Arm_Tick( usbbase, in );
	}

	return ( FALSE );
}

// --
