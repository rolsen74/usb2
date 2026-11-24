
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

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef DRIVERS_HUB_HUB_H
#define DRIVERS_HUB_HUB_H

/***************************************************************************/

struct HUB_PortWork
{
	U64 Due;
	U64 DebounceUntil;
	U64 StableSince;
	U8 Attempts;
	U8 InitialSample;
	U8 EmptyPolls;
};

struct intern
{
	#ifdef DO_DEBUG
	U32						StructID;
	#endif

	struct TimeRequest				Delay_IOReq;
	struct USB2_MsgPort				Delay_MsgPort;
	struct TimeRequest				Tick_TimeRequest;
	struct USB2_MsgPort				Tick_MsgPort;
	struct USB2_EPResource *		Res_Interrupt;
	struct USB2_EPResource *		Res_Control;
	struct RealFunctionNode **		PortFunctions;
	struct HUB_PortWork *			PortWork;
	struct USB2_DriverMessage *		StartMsg;
	struct RealFunctionNode *		Function;
	struct USB2_HCDNode *			HCDNode;
	struct RealRegister *			Register;
	struct USB2_ASync				ASync_Drivers;
	struct USB2_ASync				ASync_Functions;
	struct USB2_Header				FN_Header;
	U32		Tick_TimerAdded;
	U32		ErrorCnt;
	U32		Running;
	U32		Ports;
	U32		HUB_ID;
	U32		EnumEpoch;
	U32		NextPort;
	U64		ControlDeadline;
	U64		TickDue;
	U64		HousekeepingDue;
};

#define HUB_DELAY_MS(x)				HUB_Wait_ms(usbbase,in,x)

static inline U32
HUB_Port_Is_Super( struct intern *in, const struct USB2_PortStatus *status )
{
	if ( in->Function->fkt_Tier )
	{
		return ( in->Function->fkt_Speed >= USBSPEED_Super );
	}

	return ( ( in->HCDNode->hn_HCDType == HCDTYPE_XHCI ) &&
		( status->wPortStatus & PORTSTATF_Super_Speed ) );
}

enum HUB_AttachResult
{
	HUB_Attach_Complete,
	HUB_Attach_Retry,
	HUB_Attach_Deferred,
	HUB_Attach_Reconnect,
	HUB_Attach_Gone
};

SEC_CODE U64 HUB_Uptime( struct USBBase *usbbase );
SEC_CODE U32 HUB_Can_Enumerate( struct USBBase *usbbase, struct intern *in );
SEC_CODE S32 HUB_Control_IO( struct USBBase *usbbase, struct intern *in, struct USB2_IORequest *request );
SEC_CODE void HUB_Port_Work( struct USBBase *usbbase, struct intern *in );
SEC_CODE void HUB_Queue_Port( struct USBBase *usbbase, struct intern *in, U32 port, U32 changed );
SEC_CODE void HUB_Arm_Tick( struct USBBase *usbbase, struct intern *in );
SEC_CODE S32 HUB__Port_Reset( struct USBBase *usbbase, struct intern *in, U32 port, struct USB2_PortStatus *status );
SEC_CODE void HUB__Port_Hold( struct USBBase *usbbase, struct intern *in, U32 port, U32 address );
SEC_CODE U32 HUB__Port_Isolate( struct USBBase *usbbase, struct intern *in, U32 port );
SEC_CODE U32 HUB__Port_Probe_Holds( struct USBBase *usbbase, struct intern *in, U32 port );
SEC_CODE struct RealFunctionNode *HUB__Port_Function_Address( struct USBBase *usbbase, struct intern *in, U32 speed, U32 port, U32 *address );
SEC_CODE U32 HUB__Port_Function_Configure( struct USBBase *usbbase, struct intern *in, struct RealFunctionNode *fn, U32 port, struct USB2_PortStatus *status );
SEC_CODE void HUB__Port_Function_Discard( struct USBBase *usbbase, struct intern *in, struct RealFunctionNode *fn );
SEC_CODE enum HUB_AttachResult HUB__Port_Attach_Super( struct USBBase *usbbase, struct intern *in, U32 port );
SEC_CODE enum HUB_AttachResult HUB__Port_Attach( struct USBBase *usbbase, struct intern *in, U32 port );

#define USB_HUB_MAX_DEPTH			5
#define USB_EXTRA_POWER_UP_TIME		20		/* ms */
#define USB_SET_ADDRESS_SETTLE		10		/* ms */
#define USB_PORT_RESET_DELAY		50		/* ms */

/***************************************************************************/

SEC_CODE struct RealFunctionNode *HUB__Port_Function_Add3( struct RealFunctionNode *parent, struct USBBase *usbbase, struct intern *in, U32 speed, U32 port, struct USB2_PortStatus *status );

SEC_CODE void	HUB__HUB_Chg( struct USBBase *usbbase, struct intern *in );
SEC_CODE S32	HUB__HUB_Feature_Clr( struct USBBase *usbbase, struct intern *in, U32 sel );
SEC_CODE S32	HUB__HUB_Status_Get( struct USBBase *usbbase, struct intern *in, struct USB2_HUBStatus *stat );

SEC_CODE void	HUB__Port_Chg( struct USBBase *usbbase, struct intern *in, U32 port );
SEC_CODE S32	HUB__Port_Feature_Clr( struct USBBase *usbbase, struct intern *in, U32 port, U32 sel );
SEC_CODE S32	HUB__Port_Feature_Set( struct USBBase *usbbase, struct intern *in, U32 port, U32 sel );
SEC_CODE S32	HUB__Port_Function_Add( struct USBBase *usbbase, struct intern *in, U32 port );
SEC_CODE U32	HUB__Port_Function_SetAddr( struct USBBase *usbbase, struct intern *in, struct RealFunctionNode *fn );
SEC_CODE void	HUB__Port_FAdd_Retry( struct RealFunctionNode *parent, struct USB2_PortStatus *stat, struct USBBase *usbbase,  struct intern *in, U32 retry, U32 port );
SEC_CODE void	HUB__Port_Function_Rem( struct USBBase *usbbase, struct intern *in, U32 port );
SEC_CODE S32	HUB__Port_Status_Get( struct USBBase *usbbase, struct intern *in, U32 port, struct USB2_PortStatus *stat );

SEC_CODE void	HUB_Main__Free_FNs( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Main__Normal( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Main__Signal( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Main__Stopping( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Main__Shutdown( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Main( struct USBBase *usbbase, struct intern *in );

SEC_CODE S32	HUB_Init( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Free( struct USBBase *usbbase, struct intern *in );

SEC_CODE U32	HUB_GetDesc( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Handle_Interrupt( struct USBBase *usbbase, struct intern *in );
SEC_CODE S32	HUB_Power_Up( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Initial_Scan( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Schedule( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Handle_Stack( struct USBBase *usbbase, struct intern *in );
SEC_CODE void	HUB_Wait_ms( struct USBBase *usbbase, struct intern *in, U32 ms );
SEC_CODE U32	HUB_Entry( struct USBBase *usbbase, struct USB2_DriverMessage *msg );

/***************************************************************************/

#endif // DRIVERS_HUB_HUB_H
