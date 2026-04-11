
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef DRIVERS_HID_HID_H
#define DRIVERS_HID_HID_H

/***************************************************************************/

#define HID_DEBUG(x)

#define HID_IOReqCount				3
#define HID_LED_NumLock				1
#define HID_LED_CapsLock			2
#define HID_LED_ScrollLock			4
#define HID_LED_Compose				8
#define HID_LED_Kana				16

// --

enum HID_Driver_Type
{
	HID_DType_Unset,
	HID_DType_Keyboard,
	HID_DType_Mouse,
	HID_DType_Other
};

enum HID_Driver_Mode
{
	HID_DMode_Unset,
	HID_DMode_Boot,
	HID_DMode_Report
};


// --

#pragma pack(1)

struct USB2_BootMouse		// 3 bytes
{
	U8		Buttons;
	S8		X;
	S8		Y;
//	S8		Wheel;			// non standard, can really be anything
};

#define BOOTMOUSE_BUTTONS_LEFT			0x01U
#define BOOTMOUSE_BUTTONS_RIGHT			0x02U
#define BOOTMOUSE_BUTTONS_MIDDLE		0x04U
#define BOOTMOUSE_BUTTONS_4TH			0x08U
#define BOOTMOUSE_BUTTONS_5TH			0x10U
#define BOOTMOUSE_BUTTONS_MASK			0x1FU

struct USB2_BootKeyboard	// 8 bytes
{
	U8		Modifier;
	U8		Reserved;
	U8		Keycode[6];
};

#pragma pack(0)

// --

struct HID_GlobalNode
{
	struct HID_GlobalNode *		Parent;
	U32							UsagePage;
	S32							MinLogical;
	S32							MaxLogical;
	S32							MinPhysical;
	S32							MaxPhysical;
	U32							Unit;
	U32							UnitExponent;
	U32							ReportID;
	U32							ReportSize;
	U32							ReportCount;
};

struct HID_CollectionNode
{
	U32							Type;
	struct CollectionNode *		Parent;
//	struct USB2_Header			Items;
};

struct HID_ReportNode
{
	struct USB2_Node				Node;
	struct HIDData *				HIDData;

	// ---

	struct USB2_MsgPort				IOReq_MsgPort;

	struct USB2_MsgPort				Input_MsgPort;
	struct IOStdReq					Input_IOReq;
	struct InputEvent				Input_Event;

	U32								Timer_Added;
	struct TimeRequest				Timer_IOReq;
	struct USB2_MsgPort				Timer_MsgPort;
	struct TimeVal					Time_Repeat;
	struct TimeVal					Time_Thresh;

	struct Preferences				Prefs_Buffer;

	STR								TaskName;
	struct Task *					TaskAdr;

	// ---

	U32								UsageID;
	U32								ReportID;
//	struct HID_CollectionNode *		Collection;

	struct USB2_Header				BitHeader;			// Temp
	struct USB2_Header				UsageHeader;		// Input

	U32								Offset_Temp;
	U32								Offset_In;
	U32								Offset_Out;
	U32								Offset_Feat;

	U8								Pad[1];
	U8								Boot_Mouse_Buttons;
	U16								Boot_Keyboard_LEDs;
	S16								Boot_Keyboard_Last;			// We want this key to become active
	S16								Boot_Keyboard_Repeat;		// Active key
	struct USB2_BootKeyboard		Boot_Keyboard_Data;

	U16								Keyboard_LEDs;
	S16								Keyboard_Last;				// We want this key to become active
	S16								Keyboard_Repeat;			// Active key
	U8								Keyboard_Data[8];

	U32								Running;
	U8								Buttons[8];
	U8								Controls[8];
};

struct HID_BitNode
{
	struct USB2_Node			Node;

	U32							BitOffset;			// Start bit offset
	U32							BitCount;			// Number of Entries (Values) in Node
	U32							BitSize;			// Number of bits (Value size in bits)

	U32							Flags;
	U32							ReportID;

	struct USB2_Header			UsageHeader;
};

struct HID_UsageNode
{
	struct USB2_Node			Node;
	U32							Min;
	U32							Max;
};

struct pintern
{
	struct HID_GlobalNode *gn;
	struct HID_ReportNode *rn;
	struct HID_UsageNode *un;
	struct HID_BitNode *bn;
	struct USB2_Header reports;
	struct USB2_Header usages;			// For outside Start/End Collection

	//U32 mindesign;
	//U32 maxdesign;
	//U32 minstring;
	//U32 maxstring;

	U32 minusage;
	U32 maxusage;
	U32 bSize;
	U32 bType;
	U32 level;
	U32 bTag;
	U32 size;
	U32 len;
	U32 uval;
	U32 mask;
	S32 sval;
	U8 *data;
	U8 *end;
	U8 usageflag;
	U8 stringflag;
	U8 designflag;
};

/***************************************************************************/

struct KeyboardMask
{
	U8		Mask1;
	U8		Mask2;
	U8		KeyCode;
	U8		Pad;
};

struct HIDData
{
	#ifdef DO_DEBUG
	U32								StructID;
	#endif

	struct USB2_Semaphore			Semaphore;

	enum HID_Driver_Type			Driver_Type;
	enum HID_Driver_Mode			Driver_Mode;

	struct TimeRequest				Timer_IOReq;
	struct USB2_MsgPort				Timer_MsgPort;

	struct USB2_EPResource *		Res_Interrupt;
	struct USB2_EPResource *		Res_Control;
	struct USB2_DriverMessage *		StartMsg;
	struct RealRegister *			Register;

	// --
	struct USB2_Header				Reports;
	struct USB2_ASync				ASync_Drivers;
	U32								InputBufferSize;
	U32								ReportSize;
	U8 *							ReportBuffer;
	struct Task *					TaskAdr;
	// --

	U32								ErrorCnt;
	U32								Running;
};

extern const struct KeyboardMask KeyboardData[];
extern const S16 USBKeyMap1[256];
extern const S16 USBKeyMap2[256];


SEC_CODE S32	HID_Init(				struct USBBase *usbbase, struct HIDData *hd );
SEC_CODE void	HID_Free(				struct USBBase *usbbase, struct HIDData *hd );
SEC_CODE void	HID_Main_Boot(			struct USBBase *usbbase, struct HIDData *hd );
SEC_CODE void	HID_Main_Report(		struct USBBase *usbbase, struct HIDData *hd );

SEC_CODE void	USB_Send_LED(			struct USBBase *usbbase, struct HIDData *hd, U32 ReportID, U32 LEDStat );

SEC_CODE S32	Report_Add_Usage(		struct USBBase *usbbase, struct HIDData *hd, struct USB2_Header *list, U32 minusage, U32 maxusage );
SEC_CODE S32	Report_Read_Item(		struct USBBase *usbbase, struct HIDData *hd, struct pintern *pi );
SEC_CODE S32	Report_Read_Report(		struct USBBase *usbbase, struct HIDData *hd, U32 ifcnr );
SEC_CODE S32	Report_Bind_Drivers(	struct USBBase *usbbase, struct HIDData *hd );
SEC_CODE S32	Report_Parse(			struct USBBase *usbbase, struct HIDData *hd );
SEC_CODE S32	Report_Parse_Global(	struct USBBase *usbbase, struct HIDData *hd, struct pintern *pi );
SEC_CODE S32	Report_Parse_Local(		struct USBBase *usbbase, struct HIDData *hd, struct pintern *pi );
SEC_CODE S32	Report_Parse_Main(		struct USBBase *usbbase, struct HIDData *hd, struct pintern *pi );
SEC_CODE S32	Report_Read_Value(		struct USBBase *usbbase, struct RealRequest *ioreq, struct HID_BitNode *bn, U32 index, U32 Signed );


SEC_CODE S32	__Boot_Keyboard_Init(	struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Boot_Keyboard_Main(	struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Boot_Keyboard_Free(	struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Boot_Keyboard_Data(	struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq );
SEC_CODE void	__Boot_Keyboard_Timer(	struct USBBase *usbbase, struct HID_ReportNode *rn );


SEC_CODE S32	__Boot_Mouse_Init(		struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Boot_Mouse_Main(		struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Boot_Mouse_Free(		struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Boot_Mouse_Data(		struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq );


SEC_CODE S32	__Mouse_Init(			struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Mouse_Main(			struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Mouse_Free(			struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Mouse_Data(			struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq );
SEC_CODE void	__Mouse_Move(			struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq, struct HID_BitNode *bn, U32 idx, U32 id );
SEC_CODE void	__Mouse_Scroll(			struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq, struct HID_BitNode *bn, U32 idx, U32 id );
SEC_CODE void	__Mouse_Button( 		struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq, struct HID_BitNode *bn, U32 idx, U32 id );


SEC_CODE S32	__Keyboard_Init(		struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Keyboard_Main(		struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Keyboard_Free(		struct USBBase *usbbase, PTR userdata, PTR in_ptr );
SEC_CODE void	__Keyboard_Data(		struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq );
SEC_CODE void	__Keyboard_Qual(		struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq, struct HID_BitNode *bn, U32 idx, U32 id );
SEC_CODE void	__Keyboard_Keys(		struct USBBase *usbbase, struct HID_ReportNode *rn, struct RealRequest *ioreq, struct HID_BitNode *bn, U32 idx, U32 id );
SEC_CODE void	__Keyboard_Timer(		struct USBBase *usbbase, struct HID_ReportNode *rn );


SEC_CODE void	Input_Mouse_Button(		struct USBBase *usbbase, struct HID_ReportNode *rn, U16 code );
SEC_CODE void	Input_Mouse_Move(		struct USBBase *usbbase, struct HID_ReportNode *rn, S32 x, S32 y );
SEC_CODE void	Input_Mouse_Scroll(		struct USBBase *usbbase, struct HID_ReportNode *rn, S32 v );
SEC_CODE void	Input_Raw_Key(			struct USBBase *usbbase, struct HID_ReportNode *rn, U16 rawkey );


struct HID_BitNode *		Node_Bit_Alloc(			struct USBBase *usbbase );
void						Node_Bit_Free(			struct USBBase *usbbase, struct HID_BitNode *node );
struct HID_CollectionNode *	Node_Collection_Alloc(	struct USBBase *usbbase );
void						Node_Collection_Free(	struct USBBase *usbbase, struct HID_CollectionNode *node );
struct HID_GlobalNode *		Node_Global_Alloc(		struct USBBase *usbbase );
void						Node_Global_Free(		struct USBBase *usbbase, struct HID_GlobalNode *node );
struct HID_ReportNode *		Node_Report_Alloc(		struct USBBase *usbbase, struct HIDData *hd );
void						Node_Report_Free(		struct USBBase *usbbase, struct HID_ReportNode *node );
struct HID_UsageNode *		Node_Usage_Alloc(		struct USBBase *usbbase );
void						Node_Usage_Free(		struct USBBase *usbbase, struct HID_UsageNode *node );

/***************************************************************************/

#endif // DRIVERS_HUB_HUB_H
