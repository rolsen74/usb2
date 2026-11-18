
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

SEC_CODE enum FSTAT __EndPointRes_Release( struct USBBase *usbbase, struct USB2_EPResource *epr, STR file UNUSED )

#else

SEC_CODE enum FSTAT __EndPointRes_Release( struct USBBase *usbbase, struct USB2_EPResource *epr )

#endif

{
struct RealEndPointResource *real;
struct RealRegister *reg;
enum FSTAT stat;
U32 control;

	USBDEBUG( "__EndPointRes_Release : EPR   %p : (%s)", epr, (file)?file:"<NULL>" );

	if ( ! epr )
	{
		stat = FSTAT_Null;
	}
	else
	{
		real = (PTR) ( (char *) epr - offsetof( struct RealEndPointResource, epr_Public ));

		if ( ENDPOINTRES_VALID(real) == VSTAT_Okay )
		{
			reg = real->epr_Register;
			control = FALSE;

			if ( reg )
			{
				if ( reg->reg_Public.Res_Control == & real->epr_Public )
				{
					reg->reg_Public.Res_Control = NULL;
					control = TRUE;
				}

				USBDEBUG( "--> : REM 2 : EPR   %p to REG %p : Title '%s' :", epr, reg, reg->reg_Title );
				NODE_REMNODE( & reg->reg_EPRHeader, & real->epr_Node );
			}

			stat = ENDPOINTRES_FREE( real );

			if ( ( reg ) && ( stat != FSTAT_Okay ) )
			{
				if ( control )
				{
					reg->reg_Public.Res_Control = & real->epr_Public;
				}

				USBDEBUG( "--> : ADD 3 :EPR %p to REG %p : Title '%s' :", epr, reg, reg->reg_Title );
				NODE_ADDHEAD( & reg->reg_EPRHeader, & real->epr_Node );
			}
		}
		else
		{
			#ifdef DO_DEBUG
			USBPANIC( "__EndPointRes_Release : Invalid EPR : %p -> %p", epr, real );
			#endif
			stat = FSTAT_Null;
		}
	}

	return( stat );
}

// --
