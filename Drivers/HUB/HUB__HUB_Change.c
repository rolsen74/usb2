
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

SEC_CODE void HUB__HUB_Chg( struct USBBase *usbbase, struct intern *in )
{
	struct USB2_HUBStatus stat;

	S32 err;

	err = HUB__HUB_Status_Get( usbbase, in, &stat );

	if ( err != USB2Err_NoError )
	{
		USBERROR( "HUB__HUB_Chg : Error reading HUB status (%ld)", err );

		return;
	}

	if ( stat.wHubChange & HUBCHGF_Local_Power_Chg )
	{
		err = HUB__HUB_Feature_Clr( usbbase, in, HUB_CLR_LOCAL_POWER );

		if ( err != USB2Err_NoError )
		{
			USBERROR( "HUB__HUB_Chg : Error clearing power change (%ld)", err );
		}
	}

	if ( stat.wHubChange & HUBCHGF_Over_Current_Chg )
	{
		err = HUB__HUB_Feature_Clr( usbbase, in, HUB_CLR_OVER_CURRENT );

		if ( err != USB2Err_NoError )
		{
			USBERROR( "HUB__HUB_Chg : Error clearing over-current change (%ld)", err );
		}
	}
}

// --
