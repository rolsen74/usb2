
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

#include "HCD__RW_LE.c"

// --

enum _FType1
{
	FT1_PCI,
	FT1_Onboard,
	FT1_Last,
};

enum _FType2
{
	#ifdef DRV_EHCI
	FT2_EHCI,
	#endif
	FT2_Last
};

enum _FEndian
{
//	FE_Big,
	FE_Little,
	FE_Last
};

struct _FindStruct
{
	enum _FType1		fs_Type1;		// Onboard / PCI / ??
	enum _FType2		fs_Type2;
	enum _FEndian		fs_Endian;
	enum USB2HCDType	fs_HCDType;
	U32					fs_Machine;		// 
	U32					fs_IOBase;		// For Onboard only
	U32					fs_IntNr;		// For Onboard only
};

static struct _FindStruct _FindList[] =
{
	#ifdef DRV_EHCI

{	FT1_Onboard, FT2_EHCI, FE_Little, HCDTYPE_P50XX, MACHINETYPE_X5000_20,	0xfe210100, 16 + 16 + 28 },
{	FT1_Onboard, FT2_EHCI, FE_Little, HCDTYPE_P50XX, MACHINETYPE_X5000_20,	0xfe210100, 16 + 16 + 29 },
{	FT1_Onboard, FT2_EHCI, FE_Little, HCDTYPE_P50XX, MACHINETYPE_X5000_40,	0xfe210100, 16 + 16 + 28 },
{	FT1_Onboard, FT2_EHCI, FE_Little, HCDTYPE_P50XX, MACHINETYPE_X5000_40,	0xfe210100, 16 + 16 + 29 },
{	FT1_Onboard, FT2_EHCI, FE_Little, HCDTYPE_TABOR, MACHINETYPE_A1222,		0xfe211100, 16 + 16 + 29 },
{	FT1_Onboard, FT2_EHCI, FE_Little, HCDTYPE_TABOR, MACHINETYPE_A1222,		0xffe23100, 16 + 16 + 30 },

	#endif

{	FT1_Last, 0, 0, 0, 0, 0, 0 },
};

// --

SEC_CODE U32 __HCD_Controllers_Find( struct USBBase *usbbase )
{
struct USB2_HCDNode *node;
struct ExecIFace *IExec;
U32 retval;
U32 uhci;
U32 ohci;
U32 ehci;
U32 xhci;
U32 pos;

	retval = FALSE;
	USBDEBUG( "__HCD_Controllers_Find" );
	TASK_NAME_ENTER( "HCD : __HCD_Controllers_Find" );

	uhci = 0;
	ohci = 0;
	ehci = 0;
	xhci = 0;

	pos = -1;

	while( TRUE )
	{
		pos++;

		USBDEBUG( "Find pos %ld", pos );
	
		// End of List?
		if ( _FindList[pos].fs_Type1 == FT1_Last )
		{
			USBDEBUG( "Find Done %ld", pos );
			break;
		}

		// Machine type check
		if ( _FindList[pos].fs_Machine )
		{
			USBDEBUG( "Find Machine %ld", _FindList[pos].fs_Machine );

			if ( _FindList[pos].fs_Machine == usbbase->usb_MachineType )
			{
				USBDEBUG( "Find Machine Found" );
			}
			else
			{
				USBDEBUG( "Find Machine Skip" );
				continue;
			}
		}

		// -- Create Node

		node = HCD_ALLOC( _FindList[pos].fs_HCDType );

		USBDEBUG( "Find Node %p", node );

		if ( ! node )
		{
			USBERROR( "Error allocating EHCI HCD Node" );
			goto bailout;
		}

		NODE_ADDTAIL( & usbbase->usb_HCDHeader, node );

		// --

		if ( _FindList[pos].fs_Type1 == FT1_Onboard )
		{
			node->hn_PCIDevIOBase 	= _FindList[pos].fs_IOBase;
			node->hn_HCDInterruptNr = _FindList[pos].fs_IntNr;
		}

		// --

		/**/ if ( _FindList[pos].fs_Endian == FE_Little )
		{
			USBDEBUG( "Find FE_Little" );

			node->HCD_ReadByte		= LE_ReadByte;
			node->HCD_ReadWord		= LE_ReadWord;
			node->HCD_ReadLong		= LE_ReadLong;
			node->HCD_WriteByte 	= LE_WriteByte;
			node->HCD_WriteWord		= LE_WriteWord;
			node->HCD_WriteLong		= LE_WriteLong;
		}
		#if 0
		else if ( _FindList[pos].fs_Endian == FE_Big )
		{
			node->HCD_ReadByte		= BE_ReadByte;
			node->HCD_ReadWord		= BE_ReadWord;
			node->HCD_ReadLong		= BE_ReadLong;
			node->HCD_WriteByte 	= BE_WriteByte;
			node->HCD_WriteWord		= BE_WriteWord;
			node->HCD_WriteLong		= BE_WriteLong;
		}
		#endif
		else
		{
			USBPANIC( "HCD: Unknown fs_Endian %ld", _FindList[pos].fs_Endian );
		}

		// --

		/**/ if ( 0 )
		{
			// Well not used
		}
		#ifdef DRV_EHCI
		else if ( _FindList[pos].fs_Type2 == FT2_EHCI )
		{
			USBDEBUG( "Find FT2_EHCI" );

			node->hn_HCDIndex		= ehci++;
			node->hn_MaxSpeed		= USBSPEED_High;

			MEM_COPY( (PTR) & EHCIFunctions, & node->HCD_Functions, sizeof( struct HCDFunctions ));
		}
		#endif
		else
		{
			USBPANIC( "HCD: Unknown fs_FType %ld", _FindList[pos].fs_Type2 );
		}
	}






	#if 0
	if (( usbbase->usb_MachineType == MACHINETYPE_X5000_20 )
	|| (  usbbase->usb_MachineType == MACHINETYPE_X5000_40 ))
	{
		node = HCD_ALLOC( HCDTYPE_P50XX );

		if ( node )
		{
			node->hn_PCIDevIOBase 	= 0xfe210100;
			node->hn_HCDInterruptNr = 16 + 16 + 28;
			node->hn_HCDIndex		= ehci++;
			node->hn_MaxSpeed		= USBSPEED_High;

			node->HCD_ReadByte		= LE_ReadByte;
			node->HCD_ReadWord		= LE_ReadWord;
			node->HCD_ReadLong		= LE_ReadLong;
			node->HCD_WriteByte 	= LE_WriteByte;
			node->HCD_WriteWord		= LE_WriteWord;
			node->HCD_WriteLong		= LE_WriteLong;

			#if 0
			node->hn_HCDName		= "Enhanced";  // EHCI
			node->hn_HCDName2		= "EHCI";
			#endif

			MEM_COPY( (PTR) & EHCIFunctions, & node->HCD_Functions, sizeof( struct HCDFunctions ));

			NODE_ADDTAIL( & usbbase->usb_HCDHeader, node );
		}
		else
		{
			USBERROR( "Error allocating EHCI HCD Node" );
			goto bailout;
		}

		#if 1
		node = HCD_ALLOC( HCDTYPE_P50XX );

		if ( node )
		{
			node->hn_PCIDevIOBase 	= 0xfe211100;
			node->hn_HCDInterruptNr = 16 + 16 + 29;
			node->hn_HCDIndex		= ehci++;
			node->hn_MaxSpeed		= USBSPEED_High;

			node->HCD_ReadByte		= LE_ReadByte;
			node->HCD_ReadWord		= LE_ReadWord;
			node->HCD_ReadLong		= LE_ReadLong;
			node->HCD_WriteByte		= LE_WriteByte;
			node->HCD_WriteWord		= LE_WriteWord;
			node->HCD_WriteLong		= LE_WriteLong;

			#if 0
			node->hn_HCDName		= "Enhanced";  // EHCI
			node->hn_HCDName2		= "EHCI";
			#endif

			MEM_COPY( (PTR) & EHCIFunctions, & node->HCD_Functions, sizeof( struct HCDFunctions ));

			NODE_ADDTAIL( & usbbase->usb_HCDHeader, node );
		}
		else
		{
			USBERROR( "Error allocating EHCI HCD Node" );
			goto bailout;
		}
		#endif
	}

	#if 1

	if ( usbbase->usb_MachineType == MACHINETYPE_A1222 )
	{
		node = HCD_ALLOC( HCDTYPE_TABOR );
//		node = HCD_AllocNode( devBase, NULL, HCDTYPE_TABOR );

		if ( node )
		{
//			#ifdef DO_IOTRACE
//			node->hn_HCDName2 = "EHCI";
//			#endif

//			node->hn_HCDName		= "Enhanced";  // EHCI
			node->hn_PCIDevIOBase 	= 0xffe22100;
			node->hn_HCDInterruptNr = 16 + 16 + 12;
			node->hn_HCDIndex		= ehci++;
			node->hn_MaxSpeed		= USBSPEED_High;

			node->HCD_ReadByte		= LE_ReadByte;
			node->HCD_ReadWord		= LE_ReadWord;
			node->HCD_ReadLong		= LE_ReadLong;
			node->HCD_WriteByte 	= LE_WriteByte;
			node->HCD_WriteWord		= LE_WriteWord;
			node->HCD_WriteLong		= LE_WriteLong;

//			NODE_ADDTAIL( &devBase->dev_HCDHeader, node );
//			MEM_COPY( &EHCIFunctions, &node->HCD_Functions, sizeof( struct HCDFunctions ));

			MEM_COPY( (PTR) & EHCIFunctions, & node->HCD_Functions, sizeof( struct HCDFunctions ));

			NODE_ADDTAIL( & usbbase->usb_HCDHeader, node );
		}
		else
		{
			USBERROR( "Error allocating EHCI HCD Node" );
			goto bailout;
		}

		#if 1
		// The chip has 2 controllers but the second is not connected
		node = HCD_ALLOC( HCDTYPE_TABOR );
//		node = HCD_AllocNode( devBase, NULL, HCDTYPE_TABOR );

		if ( node )
		{
//			#ifdef DO_IOTRACE
//			node->hn_HCDName2 = "EHCI";
//			#endif

//			node->hn_HCDName		= "Enhanced";  // EHCI
			node->hn_PCIDevIOBase 	= 0xffe23100;
			node->hn_HCDInterruptNr = 16 + 16 + 30;
			node->hn_HCDIndex		= ehci++;
			node->hn_MaxSpeed		= USBSPEED_High;

			node->HCD_ReadByte		= LE_ReadByte;
			node->HCD_ReadWord		= LE_ReadWord;
			node->HCD_ReadLong		= LE_ReadLong;
			node->HCD_WriteByte 	= LE_WriteByte;
			node->HCD_WriteWord		= LE_WriteWord;
			node->HCD_WriteLong		= LE_WriteLong;

//			NODE_ADDTAIL( &devBase->dev_HCDHeader, node );
//			MEM_COPY( &EHCIFunctions, &node->HCD_Functions, sizeof( struct HCDFunctions ));

			MEM_COPY( (PTR) & EHCIFunctions, & node->HCD_Functions, sizeof( struct HCDFunctions ));

			NODE_ADDTAIL( & usbbase->usb_HCDHeader, node );
		}
		else
		{
			USBERROR( "Error allocating EHCI HCD Node" );
			goto bailout;
		}
		#endif
	}
	#endif

	#endif

	IExec = usbbase->usb_IExec;
	IExec->Disable();
	IExec->DebugPrintF( "Found %ld UHCI Controller%s\n", uhci, ( uhci == 1 ) ? "" : "s" );
	IExec->DebugPrintF( "Found %ld OHCI Controller%s\n", ohci, ( ohci == 1 ) ? "" : "s" );
	IExec->DebugPrintF( "Found %ld EHCI Controller%s\n", ehci, ( ehci == 1 ) ? "" : "s" );
	IExec->DebugPrintF( "Found %ld xHCI Controller%s\n", xhci, ( xhci == 1 ) ? "" : "s" );
	IExec->Enable();

	// There is no point in starting if no controller is found
	retval = (( uhci ) || ( ohci ) || ( ehci ) || ( xhci )) ? TRUE : FALSE;

bailout:

	TASK_NAME_LEAVE();

	return( retval );
}

// --
