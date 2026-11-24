/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "XHCI.h"

// --

SEC_RODATA const struct HCDFunctions XHCIFunctions = {
	// -- Chip

	.Chip_Preinit = XHCI_Chip_Preinit,
	.Chip_Reset	  = XHCI_Chip_Reset,
	.Chip_Alloc	  = XHCI_Chip_Alloc,
	.Chip_Init	  = XHCI_Chip_Init,
	.Chip_Start	  = XHCI_Chip_Start,
	.Chip_Stop	  = XHCI_Chip_Stop,
	.Chip_Deinit  = XHCI_Chip_Deinit,
	.Chip_Dealloc = XHCI_Chip_Dealloc,

	// -- Handlers

	.Handler_HCD	   = XHCI_Handler_HCD,
	.Handler_Reset	   = XHCI_Handler_Reset,
	.Handler_Interrupt = XHCI_Handler_Interrupt,
	.Retire_Detached   = XHCI_Slots_Retire,
	.Function_Retired  = XHCI_Slot_Is_Retired,

	// -- Transfers

	.Transfer_Check	 = XHCI_Transfer_Check,
	.Transfer_Free	 = XHCI_Transfer_Free,
	.Recover_Request = XHCI_Clear_TT,
	.Reset_Interface = XHCI_Endpoint_Drop,

	// -- Root HUB

	.Port_Clr_Enable		  = XHCI_Port_Clr_Enable,
	.Port_Clr_Power			  = XHCI_Port_Clr_Power,
	.Port_Clr_Connection_Chg  = XHCI_Port_Clr_Connect_Chg,
	.Port_Clr_Enable_Chg	  = XHCI_Port_Clr_Enable_Chg,
	.Port_Clr_OverCurrent_Chg = XHCI_Port_Clr_OverCurrent_Chg,
	.Port_Clr_Reset_Chg		  = XHCI_Port_Clr_Reset_Chg,
	.Port_Clr_Super_Change    = XHCI_Port_Clr_Super_Change,
	.Port_Get_Status		  = XHCI_Port_Get_Status,
	.Port_Set_Reset			  = XHCI_Port_Set_Reset,
	.Port_Set_Power			  = XHCI_Port_Set_Power,

	// -- Control

	.Control_Build	= XHCI_Control_Build,
	.Control_Add	= XHCI_Control_Add,
	.Control_Remove = XHCI_Control_Remove,
	.Control_Length = XHCI_Control_Length,

	// -- Bulk

	.Bulk_Build	 = XHCI_Bulk_Build,
	.Bulk_Add	 = XHCI_Bulk_Add,
	.Bulk_Remove = XHCI_Bulk_Remove,
	.Bulk_Length = XHCI_Bulk_Length,

	// -- Interrupt

	.Interrupt_Build  = XHCI_Interrupt_Build,
	.Interrupt_Add	  = XHCI_Interrupt_Add,
	.Interrupt_Remove = XHCI_Interrupt_Remove,
	.Interrupt_Length = XHCI_Interrupt_Length,

	// -- Isochronous

	.Isochronous_Build	= XHCI_Isochronous_Build,
	.Isochronous_Add	= XHCI_Isochronous_Add,
	.Isochronous_Remove = XHCI_Isochronous_Remove,
	.Isochronous_Length = XHCI_Isochronous_Length,
};

// --
