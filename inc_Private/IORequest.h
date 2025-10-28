
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef INC_PRIVATE_REQUEST_H
#define INC_PRIVATE_REQUEST_H

/***************************************************************************/

#if 0

struct __ohci
{
	struct ohci_soft_ed *	OHCI_EDHeader;

	struct ohci_soft_ed *	OHCI_StartHeader;
	U32						OHCI_StartSlot;
};

struct __uhci
{
	struct uhci_soft_qh *	UHCI_QHHeader;
};

#endif

struct __ehci
{
	struct EHCI_QH *		QH_Header;
	struct EHCI_QH *		Start_Header;
	U32						Start_Slot;
};

#if 0

struct __xhci
{
	U32 					XHCI_dummy;
};

#endif

/*[ Real Private Struct ]***************************************************/

enum IORStat
{
	IORS_Unset,			// 0 : hmm Error?
	IORS_User,			// 1 : User controlled
	IORS_HCD,			// 2 : Gived to HCD, processing it
	IORS_HCD_Queued,	// 3 : HCD have queued it
	IORS_HCD_Active,	// 4 : HCD have give it to Hardware
};

struct RealRequest
{
	struct USB2_IORequest			req_Public;					// At the moment must be first, todo: do same as EPResource or ?

	// -- 
	U32								req_StructID;
	S32								req_Locks;
	U16								req_Detach;
	U16								req_FreeMe;
//	struct USB2_TaskNode *			req_Task;
//	struct USB2_ASync *				req_ASync;
	// --

	struct RealFunctionNode *		req_Function;
	struct USB2_EndPointNode *		req_EndPoint;
	struct USB2_MsgPort				req_MsgPort;
	struct TimeRequest				req_TimerIOReq;
	enum IORStat					req_PublicStat;

	U8								req_TimerAdded;
	U8								req_DoingDestall;

	#if 0
//	U8								rr_HCDControlled;			// HCD has control, need to Reply it
//	U8								rr_Pad[3];

	U32								rr_DMASize;
	U32								rr_DMAFlags;

	U8								rr_MsgPortUsed;
	U8								rr_HCDActive;				// The HCD has made it Active
	U8								rr_Pad[3];

	#ifdef DO_DEBUG
	STR								rr_TaskName;
	#endif

	#endif

	union
	{
//		struct __ohci				OHCI;
//		struct __uhci				UHCI;
		struct __ehci				EHCI;
//		struct __xhci				XHCI;
	}								req_HCD;
};

/***************************************************************************/

#endif // INC_PRIVATE_REQUEST_H
