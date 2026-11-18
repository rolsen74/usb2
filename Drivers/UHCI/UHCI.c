
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "UHCI.h"

// --

SEC_RODATA const struct HCDFunctions UHCIFunctions = {

	// -- Chip

	.Chip_Preinit = UHCI_Chip_Preinit,
	.Chip_Reset = UHCI_Chip_Reset,
	.Chip_Alloc = UHCI_Chip_Alloc,
	.Chip_Init	= UHCI_Chip_Init,
	.Chip_Start = UHCI_Chip_Start,
	.Chip_Stop = UHCI_Chip_Stop,
	.Chip_Deinit = UHCI_Chip_Deinit,
	.Chip_Dealloc = UHCI_Chip_Dealloc,

	// -- Handlers

	.Handler_HCD = UHCI_Handler_HCD,
	.Handler_Reset = UHCI_Handler_Reset,
	.Handler_Interrupt = UHCI_Handler_Interrupt,

	// -- IORequest Transfers

	.Transfer_Check = UHCI_Transfer_Check,
	.Transfer_Free = UHCI_Transfer_Free,

	// -- Port Functions

	.Port_Clr_Enable	= UHCI_Port_Clr_Enable,
	.Port_Clr_Suspend	= UHCI_Port_Clr_Suspend,
	.Port_Clr_Power		= UHCI_Port_Clr_Power,
	.Port_Clr_Indicator = UHCI_Port_Clr_Indicator,

	.Port_Clr_Enable_Chg	  = UHCI_Port_Clr_Enable_Chg,
	.Port_Clr_Suspend_Chg	  = UHCI_Port_Clr_Suspend_Chg,
	.Port_Clr_OverCurrent_Chg = UHCI_Port_Clr_OverCurrent_Chg,
	.Port_Clr_Connection_Chg  = UHCI_Port_Clr_Connect_Chg,
	.Port_Clr_Reset_Chg		  = UHCI_Port_Clr_Reset_Chg,

	.Port_Get_Status = UHCI_Port_Get_Status,

	.Port_Set_Enable	= UHCI_Port_Set_Enable,
	.Port_Set_Suspend	= UHCI_Port_Set_Suspend,
	.Port_Set_Reset		= UHCI_Port_Set_Reset,
	.Port_Set_Power		= UHCI_Port_Set_Power,
	.Port_Set_Indicator = UHCI_Port_Set_Indicator,

	// -- Control / Interrupt / Bulk / Isochronous

	.Control_Add = UHCI_Control_Add,
	.Control_Build = UHCI_Control_Build,
	.Control_Length = UHCI_Control_Length,
	.Control_Remove = UHCI_Control_Remove,

	.Interrupt_Add = UHCI_Interrupt_Add,
	.Interrupt_Build = UHCI_Interrupt_Build,
	.Interrupt_Length = UHCI_Interrupt_Length,
	.Interrupt_Remove = UHCI_Interrupt_Remove,

	.Bulk_Add = UHCI_Bulk_Add,
	.Bulk_Build = UHCI_Bulk_Build,
	.Bulk_Length = UHCI_Bulk_Length,
	.Bulk_Remove = UHCI_Bulk_Remove,

	.Isochronous_Build = UHCI_Isochronous_Build,
	.Isochronous_Add = UHCI_Isochronous_Add,
	.Isochronous_Remove = UHCI_Isochronous_Remove,
	.Isochronous_Length = UHCI_Isochronous_Length,
};

// --
