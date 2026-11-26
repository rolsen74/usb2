
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include <intuition/menuclass.h>

// --

#define NO( cls )			NewObject(cls,NULL
#define NO2( cls, id )		Main_Objects[id]=NewObject(cls,NULL

enum
{
	GADMAINID_Dummy = 0,
	GADMAINID_Root,
	GADMAINID_ClickTab,
	GADMAINID_Page,
	GADMAINID_DeviceTree,
	GADMAINID_PropertiesList,
	GADMAINID_CaptureList,
	GADMAINID_ActiveIOList,
	GADMAINID_IORequestList,
	GADMAINID_CaptureStart,
	GADMAINID_CaptureStop,
	GADMAINID_CaptureClear,
	GADMAINID_UHCICount,
	GADMAINID_OHCICount,
	GADMAINID_EHCICount,
	GADMAINID_XHCICount,
	GADMAINID_DeviceCount,
	GADMAINID_IORequestCount,
	GADMAINID_LAST
};

// --

enum
{
	MENU_SAVE_TREE = 1,
	MENU_ABOUT,
	MENU_QUIT,
	MENU_PREFERENCES,
	MENU_CONNECTED_DEVICES
};

enum
{
	REQUEST_PAGE_ACTIVE,
	REQUEST_PAGE_CAPTURE
};

// --

struct Window * Main_WindowAdr	= NULL;
static Object * Main_MenuStrip	= NULL;
Object *		Main_WinObject	= NULL;
S32				Main_WinXPos	= 0;
S32				Main_WinYPos	= 0;
S32				Main_WinWidth	= 0;
S32				Main_WinHeight	= 0;
U32				Main_WindowBits = 0;
Object *		Main_Objects[GADMAINID_LAST];

struct List				   DeviceTreeUSBList;
struct List				   DeviceTreeGUIList;
struct ColumnInfo *		   DeviceTreeColInfo = NULL;
static struct ColumnInfo * ConnectedDevicesColInfo;

struct List	   PropertiesGUIList;
static USB2_ID SelectedDeviceID = 0;
static S32	   ConnectedDevices = TRUE;

struct List			CaptureGUIList;
struct ColumnInfo * CaptureColInfo = NULL;
static U32 CaptureWidths[IO_CAPTURE_COLUMNS];
static struct List ActiveIOList;
static struct ColumnInfo *ActiveIOColInfo;
static S32 ActiveIOView;
static U32 ActiveIOColumn;
static S32 ActiveIOReverse;

struct List			IORequestGUIList;
struct ColumnInfo * IORequestColInfo = NULL;

// --

static S32 Build_Device_TreeNode ( struct List * l, U32 g, struct USB2_FktNode * n, STR namebuf );
static void Set_Active_IO_View ( S32 active );

// --

struct FooterText
{
	char Text[2][24];
	U32 Current;
};

static struct FooterText UHCICount = { .Text = { "UHCI: 0" } };
static struct FooterText OHCICount = { .Text = { "OHCI: 0" } };
static struct FooterText EHCICount = { .Text = { "EHCI: 0" } };
static struct FooterText XHCICount = { .Text = { "xHCI: 0" } };
static struct FooterText DeviceCount = { .Text = { "Devices: 0" } };
static struct FooterText IORequestCount = { .Text = { "IOReq: --" } };

// --

static void
Update_Footer_Count ( U32 gadget, struct FooterText * field, const char * label, U32 count )
{
	U32 next;

	next = field->Current ^ 1;

	snprintf ( field->Text[next], sizeof ( field->Text[0] ), "%s: %lu", label, (unsigned long)count );

	if ( ! strcmp ( field->Text[field->Current], field->Text[next] ) )
	{
		return;
	}

	field->Current = next;

	RefreshSetGadgetAttrs ( (PTR)Main_Objects[gadget], Main_WindowAdr, NULL,
							GA_Text, field->Text[next], TAG_END );
}

// --

static U32
Count_Connected_Devices ( const struct USB2_FktNode * hub )
{
	const struct USB2_FktNode * device;

	U32 count;
	U32 port;

	count = 0;

	if ( hub->Functions )
	{
		for ( port = 0; port < hub->Ports; port++ )
		{
			device = hub->Functions[port];

			if ( device )
			{
				count += 1 + Count_Connected_Devices ( device );
			}
		}
	}

	return ( count );
}

// --

static void
Update_Topology_Counts ( void )
{
	struct USB2_FktNode * node;
	struct USB2_FktNode * root;

	U32 counts[HCDTYPE_Last];
	U32 devices;

	devices = 0;

	memset ( counts, 0, sizeof ( counts ) );

	for ( node = (PTR)GetHead ( &DeviceTreeUSBList ); node; node = (PTR)GetSucc ( (PTR)node ) )
	{
		if ( node->HCDType < HCDTYPE_Last )
		{
			counts[node->HCDType]++;
		}

		for ( root = (PTR)GetHead ( &node->Header ); root; root = (PTR)GetSucc ( &root->Node ) )
		{
			devices += Count_Connected_Devices ( root );
		}
	}

	Update_Footer_Count ( GADMAINID_UHCICount, &UHCICount, "UHCI", counts[HCDTYPE_UHCI] );

	Update_Footer_Count ( GADMAINID_OHCICount, &OHCICount, "OHCI",
						  counts[HCDTYPE_OHCI] + counts[HCDTYPE_SAM440] + counts[HCDTYPE_SAM460] );

	Update_Footer_Count ( GADMAINID_EHCICount, &EHCICount, "EHCI",
						  counts[HCDTYPE_EHCI] + counts[HCDTYPE_P5020] + counts[HCDTYPE_P5040] + counts[HCDTYPE_TABOR] );

	Update_Footer_Count ( GADMAINID_XHCICount, &XHCICount, "xHCI", counts[HCDTYPE_XHCI] );

	Update_Footer_Count ( GADMAINID_DeviceCount, &DeviceCount, "Devices", devices );
}

// --

static S32
Main_CreateMenu ( void )
{
	static struct TagItem items[] = { { NM_Menu, ( Tag ) "Project" },
									  { NM_Item, ( Tag ) "Save tree" },
									  { MA_ID, MENU_SAVE_TREE },
									  { NM_Item, (Tag)ML_SEPARATOR },
									  { NM_Item, ( Tag ) "About" },
									  { MA_ID, MENU_ABOUT },
									  { MA_Key, ( Tag ) "?" },
									  { NM_Item, (Tag)ML_SEPARATOR },
									  { NM_Item, ( Tag ) "Quit" },
									  { MA_ID, MENU_QUIT },
									  { MA_Key, ( Tag ) "Q" },
									  { NM_Menu, ( Tag ) "Edit" },
									  { NM_Item, ( Tag ) "Preferences..." },
									  { MA_ID, MENU_PREFERENCES },
									  { MA_Key, ( Tag ) "P" },
									  { NM_Menu, ( Tag ) "View" },
									  { NM_Item, ( Tag ) "Connected devices" },
									  { MA_ID, MENU_CONNECTED_DEVICES },
									  { MA_Toggle, TRUE },
									  { TAG_END, 0 } };

	Main_MenuStrip = NewObject ( NULL, "menuclass", MA_Type, T_ROOT, TAG_END );

	if ( ! Main_MenuStrip )
	{
		printf ( "USB2Analyzer: could not open menuclass (requires Intuition 54.6 or later).\n" );

		return ( FALSE );
	}

	if ( ! IDoMethod ( Main_MenuStrip, MM_NEWMENU, 0, items ) )
	{
		printf ( "USB2Analyzer: could not build the menu.\n" );

		return ( FALSE );
	}

	if ( ConnectedDevices )
	{
		IDoMethod ( Main_MenuStrip, MM_SETSTATE, 0, MENU_CONNECTED_DEVICES, MS_CHECKED, MS_CHECKED );
	}

	SetAttrs ( Main_WinObject, WINDOW_MenuStrip, Main_MenuStrip, TAG_END );

	return ( TRUE );
}

// --

static void
Main_HandleMenu ( void )
{
	static const struct EasyStruct about = { .es_StructSize	  = sizeof ( struct EasyStruct ),
											 .es_Title		  = "About USB2Analyzer",
											 .es_TextFormat	  = VERS " (" DATE ")\n"
																	 "Copyright (c) 2012-2026 by Rene W. Olsen\n"
																	 "GPL-3.0-or-later",
											 .es_GadgetFormat = "OK" };

	U32 id;

	id = NO_MENU_ID;

	while ( ( id = IDoMethod ( Main_MenuStrip, MM_NEXTSELECT, 0, id ) ) != NO_MENU_ID )
	{
		switch ( id )
		{
			case MENU_SAVE_TREE:
			{
				Request_Device_Tree_Save ( Main_WindowAdr );

				break;
			}

			case MENU_ABOUT:
			{
				EasyRequestArgs ( Main_WindowAdr, &about, NULL, NULL );

				break;
			}

			case MENU_PREFERENCES:
			{
				Prefs_OpenWindow ( Main_WindowAdr );

				break;
			}

			case MENU_CONNECTED_DEVICES:
			{
				ConnectedDevices = ( IDoMethod ( Main_MenuStrip, MM_GETSTATE, 0, id ) & MS_CHECKED ) != 0;

				Build_Device_Tree ();

				break;
			}

			case MENU_QUIT:
			{
				Running = FALSE;

				return;
			}
		}
	}
}

// --

static S32
Apply_Capture_Widths ( const U32 widths[IO_CAPTURE_COLUMNS] )
{
	U32 column;
	S32 changed;

	changed = FALSE;

	for ( column = 0; column < IO_CAPTURE_COLUMNS; column++ )
	{
		if ( CaptureWidths[column] == widths[column] )
		{
			continue;
		}

		SetLBColumnInfoAttrs ( CaptureColInfo, LBCIA_Column, column, LBCIA_Width, widths[column], TAG_END );

		CaptureWidths[column] = widths[column];
		changed = TRUE;
	}

	return ( changed );
}

// --

S32
Main_OpenWindow ( void )
{
	U32 widths[IO_CAPTURE_COLUMNS];
	S32 retval;

	retval = FALSE;

	if ( ! Main_WindowAdr )
	{
		if ( ! Main_CreateMenu () )
		{
			goto bailout;
		}

		Main_WindowAdr = (PTR)IDoMethod ( Main_WinObject, WM_OPEN );

		if ( ! Main_WindowAdr )
		{
			printf ( "Main_WindowAdr() : %p : failed\n", Main_WinObject );
			goto bailout;
		}

		GetAttrs ( Main_WinObject, WINDOW_SigMask, &Main_WindowBits, TAG_END );

		Measure_IO_Capture ( &CaptureGUIList, Main_WindowAdr->RPort, IOCaptureTitles, widths );

		if ( Apply_Capture_Widths ( widths ) )
		{
			RefreshSetGadgetAttrs ( (PTR)Main_Objects[GADMAINID_CaptureList], Main_WindowAdr, NULL,
									LISTBROWSER_ColumnInfo, CaptureColInfo, TAG_END );
		}

		Build_Device_Tree ();

		if ( USB_Has_Active_IO () )
		{
			Start_Window_Timer ();
		}

		if ( ActiveIOView )
		{
			Set_Active_IO_View ( TRUE );
		}
		else
		{
			Update_Window_Timer ();
		}
	}

	retval = TRUE;

bailout:

	return ( retval );
}

// --

void
Main_CloseWindow ( void )
{
	Stop_Window_Timer ();

	if ( Main_WindowAdr )
	{
		if ( Main_ScreenType == SCRTYPE_Window )
		{
			Main_WinXPos   = Main_WindowAdr->LeftEdge;
			Main_WinYPos   = Main_WindowAdr->TopEdge;
			Main_WinWidth  = Main_WindowAdr->Width;
			Main_WinHeight = Main_WindowAdr->Height;
		}

		// Descriptor images borrow the current screen's font.
		SetAttrs ( Main_Objects[GADMAINID_PropertiesList], LISTBROWSER_Labels, ~0UL, TAG_END );

		Free_Device_Properties ( &PropertiesGUIList );

		IDoMethod ( Main_WinObject, WM_CLOSE );
		Main_WindowBits = 0;
		Main_WindowAdr	= NULL;
	}

	if ( Main_MenuStrip )
	{
		SetAttrs ( Main_WinObject, WINDOW_MenuStrip, NULL, TAG_END );

		DisposeObject ( Main_MenuStrip );

		Main_MenuStrip = NULL;
	}
}

// --

S32
Main_BuildWindow ( void )
{
	static const char *const requestPages[] = { "Active", "Capture", NULL };
	static char stackVersion[32];
	struct LayoutIFace * ILayout;

	Class * pageClass;
	U16 version;
	U16 revision;
	U32 requestPage;
	S32 retval;

	retval = FALSE;

	DeviceTreeColInfo = AllocLBColumnInfo ( 3, LBCIA_Column, 0, LBCIA_Title, "Device Tree", LBCIA_Weight, 47, LBCIA_Column, 1,
											LBCIA_Title, "VID:PID", LBCIA_Weight, 16, LBCIA_Column, 2, LBCIA_Title, "Class Name",
											LBCIA_Weight, 37, TAG_END );

	ConnectedDevicesColInfo
		= AllocLBColumnInfo ( 3, LBCIA_Column, 0, LBCIA_Title, "Device", LBCIA_Weight, 47, LBCIA_Column, 1, LBCIA_Title,
							  "VID:PID", LBCIA_Weight, 16, LBCIA_Column, 2, LBCIA_Title, "Drivers", LBCIA_Weight, 37, TAG_END );

	if ( ( ! DeviceTreeColInfo ) || ( ! ConnectedDevicesColInfo ) )
	{
		printf ( "Error allocating ListBrowser Column Info\n" );
		goto bailout;
	}

	if ( ! Open_Descriptor_Image_Class () )
	{
		goto bailout;
	}

	CaptureColInfo = AllocLBColumnInfo (
		IO_CAPTURE_COLUMNS,
		LBCIA_Column, 0, LBCIA_Title, IOCaptureTitles[0],
		LBCIA_Column, 1, LBCIA_Title, IOCaptureTitles[1],
		LBCIA_Column, 2, LBCIA_Title, IOCaptureTitles[2],
		LBCIA_Column, 3, LBCIA_Title, IOCaptureTitles[3],
		LBCIA_Column, 4, LBCIA_Title, IOCaptureTitles[4],
		LBCIA_Column, 5, LBCIA_Title, IOCaptureTitles[5],
		TAG_END );

	if ( ! CaptureColInfo )
	{
		goto bailout;
	}

	NewList ( &ActiveIOList );

	ActiveIOColInfo = AllocLBColumnInfo ( IO_CAPTURE_COLUMNS,
		LBCIA_Column, 0, LBCIA_Title, ActiveIOTitles[0], LBCIA_Sortable, TRUE,
		LBCIA_Column, 1, LBCIA_Title, ActiveIOTitles[1], LBCIA_Sortable, TRUE,
		LBCIA_Column, 2, LBCIA_Title, ActiveIOTitles[2], LBCIA_Sortable, TRUE,
		LBCIA_Column, 3, LBCIA_Title, ActiveIOTitles[3], LBCIA_Sortable, TRUE,
		LBCIA_Column, 4, LBCIA_Title, ActiveIOTitles[4], LBCIA_Sortable, TRUE,
		LBCIA_Column, 5, LBCIA_Title, ActiveIOTitles[5], LBCIA_Sortable, TRUE, TAG_END );

	if ( ! ActiveIOColInfo )
	{
		goto bailout;
	}

	IORequestColInfo = AllocLBColumnInfo ( 2,
		LBCIA_Column, 0, LBCIA_Title, "Field", LBCIA_Weight, 40,
		LBCIA_Column, 1, LBCIA_Title, "Value", LBCIA_Weight, 60,
		TAG_END );

	if ( ! IORequestColInfo )
	{
		goto bailout;
	}

	ILayout = (PTR)GetInterface ( LayoutBase, "main", 1, NULL );

	if ( ! ILayout )
	{
		goto bailout;
	}

	pageClass = PAGE_GetClass ();

	DropInterface ( (PTR)ILayout );

	if ( ! pageClass )
	{
		goto bailout;
	}

	ActiveIOView = USB_Has_Active_IO ();
	requestPage = ActiveIOView ? REQUEST_PAGE_ACTIVE : REQUEST_PAGE_CAPTURE;

	USB_Get_Stack_Version ( &version, &revision );

	snprintf ( stackVersion, sizeof ( stackVersion ), "Stack: v%u.%u", (unsigned)version, (unsigned)revision );

	Main_WinObject = NO ( WindowClass ), WA_Activate, TRUE,
	//		WA_Top,										MainScreen->WBorTop + MainScreen->Font->ta_YSize,
	//		WA_Left,									0,
	//		WA_Width,									MainScreen->Width,
	//		WA_Height,									MainScreen->Height - ( MainScreen->WBorTop +
	// MainScreen->Font->ta_YSize ), 		WA_Backdrop,								TRUE, 		WA_Borderless,
	// TRUE, 		WA_CustomScreen,							MainScreen,
		WINDOW_ParentGroup, NO2 ( LayoutClass, GADMAINID_Root ), LAYOUT_Orientation, LAYOUT_ORIENT_VERT,

	// Toolbar
		LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_BevelStyle, BVS_THIN,

	LAYOUT_AddChild, NO ( LayoutClass ), End, End, CHILD_WeightedHeight, 0,

	// Listviews
		LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,

	LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_Orientation, LAYOUT_ORIENT_VERT,

	LAYOUT_AddChild, NO2 ( ListBrowserClass, GADMAINID_DeviceTree ), GA_ID, GADMAINID_DeviceTree, GA_RelVerify, TRUE,
	LISTBROWSER_Hierarchical, TRUE, LISTBROWSER_ShowSelected, TRUE, LISTBROWSER_ColumnTitles, TRUE, LISTBROWSER_ColumnInfo,
	ConnectedDevices ? ConnectedDevicesColInfo : DeviceTreeColInfo, LISTBROWSER_HorizontalProp, TRUE, LISTBROWSER_AutoFit,
	ConnectedDevices, LISTBROWSER_Labels, &DeviceTreeGUIList, End,

	LAYOUT_WeightBar, TRUE,

	// Properties List
		LAYOUT_AddChild, NO2 ( ListBrowserClass, GADMAINID_PropertiesList ), GA_ID, GADMAINID_PropertiesList,
	LISTBROWSER_Hierarchical, TRUE, LISTBROWSER_AutoFit, TRUE, LISTBROWSER_VertSeparators, FALSE, LISTBROWSER_Labels,
	&PropertiesGUIList, LISTBROWSER_HorizontalProp, TRUE, End, End,

	LAYOUT_WeightBar, TRUE,

	LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_Orientation, LAYOUT_ORIENT_VERT,

	// Request tabs
	LAYOUT_AddChild, NO2 ( ClickTabClass, GADMAINID_ClickTab ),
		GA_ID, GADMAINID_ClickTab, GA_RelVerify, TRUE, GA_Text, requestPages, CLICKTAB_Current, requestPage,
		CLICKTAB_PageGroup, NO2 ( pageClass, GADMAINID_Page ),
			PAGE_Current, requestPage,
			PAGE_Add, NO2 ( ListBrowserClass, GADMAINID_ActiveIOList ),
				GA_ID, GADMAINID_ActiveIOList, GA_RelVerify, TRUE, LISTBROWSER_VertSeparators, TRUE,
				LISTBROWSER_TitleClickable, TRUE,
				LISTBROWSER_ShowSelected, TRUE, LISTBROWSER_ColumnTitles, TRUE, LISTBROWSER_ColumnInfo, ActiveIOColInfo,
				LISTBROWSER_Labels, &ActiveIOList, LISTBROWSER_HorizontalProp, TRUE, End,
			PAGE_Add, NO ( LayoutClass ), LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
				LAYOUT_AddChild, NO2 ( ListBrowserClass, GADMAINID_CaptureList ),
					GA_ID, GADMAINID_CaptureList, GA_RelVerify, TRUE, LISTBROWSER_VertSeparators, TRUE,
					LISTBROWSER_ShowSelected, TRUE, LISTBROWSER_ColumnTitles, TRUE, LISTBROWSER_ColumnInfo, CaptureColInfo,
					LISTBROWSER_Labels, &CaptureGUIList, LISTBROWSER_HorizontalProp, TRUE, End,
				LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_SpaceInner, TRUE,
					LAYOUT_AddChild, NO2 ( ButtonClass, GADMAINID_CaptureStart ),
						GA_ID, GADMAINID_CaptureStart, GA_Text, "Start", GA_RelVerify, TRUE,
						GA_Disabled, ! USB_Has_Capture (), End,
					LAYOUT_AddChild, NO2 ( ButtonClass, GADMAINID_CaptureStop ),
						GA_ID, GADMAINID_CaptureStop, GA_Text, "Stop", GA_RelVerify, TRUE, GA_Disabled, TRUE, End,
					LAYOUT_AddChild, NO2 ( ButtonClass, GADMAINID_CaptureClear ),
						GA_ID, GADMAINID_CaptureClear, GA_Text, "Clear", GA_RelVerify, TRUE, End,
				End, CHILD_WeightedHeight, 0,
			End,
		End,
	End,

	LAYOUT_WeightBar, TRUE,

	// IORequest List
		LAYOUT_AddChild, NO2 ( ListBrowserClass, GADMAINID_IORequestList ), GA_ID, GADMAINID_IORequestList,
	LISTBROWSER_AutoFit, TRUE, LISTBROWSER_VertSeparators, TRUE, LISTBROWSER_ColumnTitles, TRUE,
	LISTBROWSER_ColumnInfo, IORequestColInfo, LISTBROWSER_HorizontalProp, TRUE,
		LISTBROWSER_Labels, &IORequestGUIList, End, End, End,

	// Stack footer
		LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_BevelStyle, BVS_THIN,
			LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_BevelStyle, BVS_THIN,
				LAYOUT_AddChild, NO ( ButtonClass ), GA_Text, stackVersion, GA_ReadOnly, TRUE,
					BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, End,
			End,
			LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_BevelStyle, BVS_THIN,
				LAYOUT_AddChild, NO2 ( ButtonClass, GADMAINID_UHCICount ),
					GA_Text, UHCICount.Text[UHCICount.Current], GA_ReadOnly, TRUE,
					BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, End,
			End,
			LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_BevelStyle, BVS_THIN,
				LAYOUT_AddChild, NO2 ( ButtonClass, GADMAINID_OHCICount ),
					GA_Text, OHCICount.Text[OHCICount.Current], GA_ReadOnly, TRUE,
					BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, End,
			End,
			LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_BevelStyle, BVS_THIN,
				LAYOUT_AddChild, NO2 ( ButtonClass, GADMAINID_EHCICount ),
					GA_Text, EHCICount.Text[EHCICount.Current], GA_ReadOnly, TRUE,
					BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, End,
			End,
			LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_BevelStyle, BVS_THIN,
				LAYOUT_AddChild, NO2 ( ButtonClass, GADMAINID_XHCICount ),
					GA_Text, XHCICount.Text[XHCICount.Current], GA_ReadOnly, TRUE,
					BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, End,
			End,
			LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_BevelStyle, BVS_THIN,
				LAYOUT_AddChild, NO2 ( ButtonClass, GADMAINID_DeviceCount ),
					GA_Text, DeviceCount.Text[DeviceCount.Current], GA_ReadOnly, TRUE,
					BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, End,
			End,
			LAYOUT_AddChild, NO ( LayoutClass ), LAYOUT_BevelStyle, BVS_THIN,
				LAYOUT_AddChild, NO2 ( ButtonClass, GADMAINID_IORequestCount ),
					GA_Text, IORequestCount.Text[IORequestCount.Current], GA_ReadOnly, TRUE,
					BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, End,
			End,
	End, CHILD_WeightedHeight, 0, End, End;

	if ( ! Main_WinObject )
	{
		goto bailout;
	}

	retval = TRUE;

bailout:

	return ( retval );
}

// --

void
Main_DestoryWindow ( void )
{
	if ( Main_WinObject )
	{
		DisposeObject ( Main_WinObject );
		Main_WinObject = NULL;
	}

	FreeListBrowserList ( &DeviceTreeGUIList );

	Free_IO_Capture ( &CaptureGUIList );

	if ( ActiveIOColInfo )
	{
		Free_IO_Request_Rows ( &ActiveIOList );

		FreeLBColumnInfo ( ActiveIOColInfo );
		ActiveIOColInfo = NULL;
	}

	FreeListBrowserList ( &IORequestGUIList );

	Free_Device_Properties ( &PropertiesGUIList );

	Close_Descriptor_Image_Class ();

	if ( DeviceTreeColInfo )
	{
		FreeLBColumnInfo ( DeviceTreeColInfo );
		DeviceTreeColInfo = NULL;
	}

	if ( ConnectedDevicesColInfo )
	{
		FreeLBColumnInfo ( ConnectedDevicesColInfo );
		ConnectedDevicesColInfo = NULL;
	}

	if ( CaptureColInfo )
	{
		FreeLBColumnInfo ( CaptureColInfo );
		CaptureColInfo = NULL;
	}

	if ( IORequestColInfo )
	{
		FreeLBColumnInfo ( IORequestColInfo );
		IORequestColInfo = NULL;
	}
}

// --

void
Show_IO_Capture ( struct List * events )
{
	struct USB2_NotifyMessage * msg;
	const struct USB2_NotifyMessage * entry;
	struct Node * row;

	Object * browser;
	U32		 count;
	U32		 widths[IO_CAPTURE_COLUMNS];
	U32		 sequence;
	S32		 keepSelection;
	S32		 selected;
	S32		 index;
	S32		 changed;

	if ( ( ! Main_WindowAdr ) || ( ! Capture_Is_Running () ) )
	{
		return;
	}

	browser = Main_Objects[GADMAINID_CaptureList];
	count	= 0;
	row		= NULL;

	memcpy ( widths, CaptureWidths, sizeof ( widths ) );

	if ( ActiveIOView )
	{
		SetAttrs ( browser, LISTBROWSER_Labels, ~0UL, TAG_END );

		for ( msg = (PTR)GetHead ( events ); msg; msg = (PTR)GetSucc ( &msg->nm_Message.mn_Node ) )
		{
			Append_IO_Capture ( &CaptureGUIList, msg, Main_WindowAdr->RPort, widths );
		}

		Apply_Capture_Widths ( widths );

		SetAttrs ( browser, LISTBROWSER_ColumnInfo, CaptureColInfo, LISTBROWSER_Labels, &CaptureGUIList, TAG_END );

		return;
	}

	GetAttrs ( browser, LISTBROWSER_SelectedNode, &row, TAG_END );

	entry = Get_IO_Capture ( row );
	keepSelection = entry != NULL;
	sequence = entry ? entry->nm_Data.nm_IORequest.nm_Sequence : 0;
	selected = -1;

	SetAttrs ( browser, LISTBROWSER_Labels, ~0UL, TAG_END );

	for ( msg = (PTR)GetHead ( events ); msg; msg = (PTR)GetSucc ( &msg->nm_Message.mn_Node ) )
	{
		count = Append_IO_Capture ( &CaptureGUIList, msg, Main_WindowAdr->RPort, widths );
	}

	changed = Apply_Capture_Widths ( widths );

	if ( keepSelection )
	{
		index = 0;

		for ( row = GetHead ( &CaptureGUIList ); row; row = GetSucc ( row ), index++ )
		{
			entry = Get_IO_Capture ( row );

			if ( ( entry ) && ( entry->nm_Data.nm_IORequest.nm_Sequence == sequence ) )
			{
				selected = index;
				break;
			}
		}
	}

	RefreshSetGadgetAttrs ( (PTR)browser, Main_WindowAdr, NULL, LISTBROWSER_Labels, &CaptureGUIList,
							changed ? LISTBROWSER_ColumnInfo : TAG_IGNORE, CaptureColInfo,
							LISTBROWSER_Selected, selected,
							count ? LISTBROWSER_MakeVisible : TAG_IGNORE, count ? count - 1 : 0, TAG_END );
}

// --

static void
Set_Capture_Running ( S32 running )
{
	Handle_Notify ();

	if ( ! Capture_Set_Running ( running ) )
	{
		DisplayBeep ( Main_WindowAdr->WScreen );

		return;
	}

	RefreshSetGadgetAttrs ( (PTR)Main_Objects[GADMAINID_CaptureStart], Main_WindowAdr, NULL,
							GA_Disabled, running, TAG_END );

	RefreshSetGadgetAttrs ( (PTR)Main_Objects[GADMAINID_CaptureStop], Main_WindowAdr, NULL,
							GA_Disabled, ! running, TAG_END );
}

// --

static void
Clear_Capture ( void )
{
	Object * log;
	Object * details;
	U32 widths[IO_CAPTURE_COLUMNS];

	Handle_Notify ();

	log = Main_Objects[GADMAINID_CaptureList];
	details = Main_Objects[GADMAINID_IORequestList];

	SetAttrs ( log, LISTBROWSER_Labels, ~0UL, TAG_END );

	SetAttrs ( details, LISTBROWSER_Labels, ~0UL, TAG_END );

	Free_IO_Capture ( &CaptureGUIList );

	FreeListBrowserList ( &IORequestGUIList );

	Measure_IO_Capture ( &CaptureGUIList, Main_WindowAdr->RPort, IOCaptureTitles, widths );

	Apply_Capture_Widths ( widths );

	RefreshSetGadgetAttrs ( (PTR)log, Main_WindowAdr, NULL, LISTBROWSER_ColumnInfo, CaptureColInfo,
							LISTBROWSER_Labels, &CaptureGUIList, LISTBROWSER_Selected, -1,
							LISTBROWSER_Top, 0, LISTBROWSER_Left, 0, TAG_END );

	RefreshSetGadgetAttrs ( (PTR)details, Main_WindowAdr, NULL, LISTBROWSER_Labels, &IORequestGUIList,
							LISTBROWSER_Top, 0, LISTBROWSER_Left, 0, TAG_END );
}

// --

static void
Show_IO_Request_Details ( const struct USB2_NotifyMessage * entry )
{
	Object * browser;

	browser = Main_Objects[GADMAINID_IORequestList];

	SetAttrs ( browser, LISTBROWSER_Labels, ~0UL, TAG_END );

	FreeListBrowserList ( &IORequestGUIList );

	if ( entry )
	{
		Build_IO_Request_Details ( &IORequestGUIList, entry );
	}

	RefreshSetGadgetAttrs ( (PTR)browser, Main_WindowAdr, NULL, LISTBROWSER_Labels, &IORequestGUIList,
							LISTBROWSER_AutoFit, TRUE, LISTBROWSER_Top, 0, LISTBROWSER_Left, 0, TAG_END );
}

// --

static void
Show_IO_Request ( U32 gadget )
{
	struct Node * row;

	row = NULL;

	GetAttrs ( Main_Objects[gadget], LISTBROWSER_SelectedNode, &row, TAG_END );

	Show_IO_Request_Details ( Get_IO_Capture ( row ) );
}

// --

static void
Refresh_Active_IO ( void )
{
	Object * browser;
	U32 widths[IO_CAPTURE_COLUMNS];
	U32 column;
	U32 count;

	browser = Main_Objects[GADMAINID_ActiveIOList];

	SetAttrs ( browser, LISTBROWSER_Labels, ~0UL, TAG_END );

	Free_IO_Request_Rows ( &ActiveIOList );

	Measure_IO_Capture ( &ActiveIOList, Main_WindowAdr->RPort, ActiveIOTitles, widths );

	count = Build_Active_IO_Requests ( &ActiveIOList, Main_WindowAdr->RPort, widths, ActiveIOColumn, ActiveIOReverse );

	Update_Footer_Count ( GADMAINID_IORequestCount, &IORequestCount, "IOReq", count );

	for ( column = 0; column < IO_CAPTURE_COLUMNS; column++ )
	{
		SetLBColumnInfoAttrs ( ActiveIOColInfo, LBCIA_Column, column, LBCIA_Width, widths[column],
								LBCIA_SortArrow, column == ActiveIOColumn,
								LBCIA_SortDirection, ActiveIOReverse ? LBMSORT_REVERSE : LBMSORT_FORWARD, TAG_END );
	}

	RefreshSetGadgetAttrs ( (PTR)browser, Main_WindowAdr, NULL, LISTBROWSER_ColumnInfo, ActiveIOColInfo,
							LISTBROWSER_Labels, &ActiveIOList, TAG_END );
}

// --

void
Update_Window_Timer ( void )
{
	if ( ActiveIOView )
	{
		Refresh_Active_IO ();
	}
	else if ( USB_Has_Active_IO () )
	{
		U32 bytes;

		bytes = USB2_Attr_Get ( 0, USB2Tag_Attr_ActiveIORequests, NULL, 0 );

		Update_Footer_Count ( GADMAINID_IORequestCount, &IORequestCount, "IOReq",
							  bytes / sizeof ( struct USB2_ActiveIORequest ) );
	}
}

// --

static void
Handle_Active_IO_Click ( void )
{
	ULONG event;
	ULONG column;

	event = 0;
	column = (ULONG) -1;

	GetAttrs ( Main_Objects[GADMAINID_ActiveIOList], LISTBROWSER_RelEvent, &event,
				LISTBROWSER_RelColumn, &column, TAG_END );

	if ( ( event & LBRE_TITLECLICK ) && ( column < IO_CAPTURE_COLUMNS ) )
	{
		ActiveIOReverse = ( column == ActiveIOColumn ) && ( ! ActiveIOReverse );
		ActiveIOColumn = column;

		Refresh_Active_IO ();

		return;
	}

	Show_IO_Request ( GADMAINID_ActiveIOList );
}

// --

static void
Set_Active_IO_View ( S32 active )
{
	static const struct EasyStruct unavailable =
	{
		.es_StructSize = sizeof ( struct EasyStruct ),
		.es_Title = "Active I/O",
		.es_TextFormat = "Active I/O requests need usb2.kmod 1.124 or later.",
		.es_GadgetFormat = "OK"
	};

	if ( active )
	{
		if ( ! USB_Has_Active_IO () )
		{
			EasyRequestArgs ( Main_WindowAdr, &unavailable, NULL, NULL );

			active = FALSE;
		}
	}

	ActiveIOView = active;

	Show_IO_Request_Details ( NULL );

	if ( active )
	{
		Refresh_Active_IO ();
	}
	else
	{
		RefreshSetGadgetAttrs ( (PTR)Main_Objects[GADMAINID_ClickTab], Main_WindowAdr, NULL,
								CLICKTAB_Current, REQUEST_PAGE_CAPTURE, TAG_END );

		RefreshSetGadgetAttrs ( (PTR)Main_Objects[GADMAINID_CaptureList], Main_WindowAdr, NULL,
								LISTBROWSER_Position, LBP_BOTTOM, TAG_END );
	}
}

// --

static void
Show_Device_Properties ( struct USB2_FktNode * node )
{
	SelectedDeviceID = node ? node->ID : 0;

	SetAttrs ( Main_Objects[GADMAINID_PropertiesList], LISTBROWSER_Labels, ~0UL, TAG_END );

	Free_Device_Properties ( &PropertiesGUIList );

	Build_Device_Properties ( &PropertiesGUIList, node, Main_WindowAdr );

	RefreshSetGadgetAttrs ( (PTR)Main_Objects[GADMAINID_PropertiesList], Main_WindowAdr, NULL, LISTBROWSER_Labels,
							&PropertiesGUIList, LISTBROWSER_AutoFit, TRUE, LISTBROWSER_Top, 0, LISTBROWSER_Left, 0, TAG_END );
}

// --

static void
Handle_Capture_Checkbox ( void )
{
	static const struct EasyStruct unavailable
		= { .es_StructSize	 = sizeof ( struct EasyStruct ),
			.es_Title		 = "USB capture",
			.es_TextFormat	 = "USB capture is unavailable. Check that usb2.kmod 1.123 or later is loaded.",
			.es_GadgetFormat = "OK" };
	struct USB2_FktNode * node;
	struct Node *		  row;

	ULONG checked;

	// A checkbox click need not change the selected row.
	for ( row = GetHead ( &DeviceTreeGUIList ); row; row = GetSucc ( row ) )
	{
		node	= NULL;
		checked = FALSE;

		GetListBrowserNodeAttrs ( row, LBNA_UserData, &node, LBNA_Checked, &checked, TAG_END );

		if ( ! node )
		{
			if ( checked )
			{
				Build_Device_Tree ();

				return;
			}

			continue;
		}

		if ( ( checked != 0 ) == Capture_Is_Checked ( node ) )
		{
			continue;
		}

		if ( ! USB_Has_Capture () )
		{
			EasyRequestArgs ( Main_WindowAdr, &unavailable, NULL, NULL );
		}
		else if ( ! Capture_Set_Checked ( node, checked != 0 ) )
		{
			DisplayBeep ( Main_WindowAdr->WScreen );
		}

		Build_Device_Tree ();

		return;
	}
}

// --

void
Main_HandleWindow ( void )
{
	struct USB2_FktNode * node;
	struct Node *		  selected;

	U32	  result;
	U16	  code;
	ULONG event;

	while ( ( result = IDoMethod ( Main_WinObject, WM_HANDLEINPUT, &code ) ) != WMHI_LASTMSG )
	{
		switch ( result & WMHI_CLASSMASK )
		{
			case WMHI_GADGETUP:
			{
				switch ( result & WMHI_GADGETMASK )
				{
					case GADMAINID_DeviceTree:
					{
						selected = NULL;
						node	 = NULL;
						event	 = 0;

						GetAttrs ( Main_Objects[GADMAINID_DeviceTree], LISTBROWSER_SelectedNode, &selected, LISTBROWSER_RelEvent,
								   &event, TAG_END );

						if ( selected )
						{
							GetListBrowserNodeAttrs ( selected, LBNA_UserData, &node, TAG_END );
						}

						if ( event & ( LBRE_CHECKED | LBRE_UNCHECKED ) )
						{
							Handle_Capture_Checkbox ();

							break;
						}

						Show_Device_Properties ( node );

						break;
					}

					case GADMAINID_CaptureList:
					{
						Show_IO_Request ( GADMAINID_CaptureList );

						break;
					}

					case GADMAINID_ActiveIOList:
					{
						Handle_Active_IO_Click ();

						break;
					}

					case GADMAINID_ClickTab:
					{
						ULONG page;

						page = REQUEST_PAGE_CAPTURE;

						GetAttr ( CLICKTAB_Current, Main_Objects[GADMAINID_ClickTab], &page );

						Set_Active_IO_View ( page == REQUEST_PAGE_ACTIVE );

						break;
					}

					case GADMAINID_CaptureStart:
					case GADMAINID_CaptureStop:
					{
						Set_Capture_Running ( ( result & WMHI_GADGETMASK ) == GADMAINID_CaptureStart );

						break;
					}

					case GADMAINID_CaptureClear:
					{
						Clear_Capture ();

						break;
					}

					default:
					{
						break;
					}
				}
				break;
			}

				//			case WMHI_RAWKEY:
				//			{
				//				Main_HandleKeys( code );
				//				break;
				//			}
				//
			case WMHI_MENUPICK:
			{
				Main_HandleMenu ();

				if ( ! Running )
				{
					return;
				}

				break;
			}

			case WMHI_CLOSEWINDOW:
			{
				Running = FALSE;
				break;
			}

			default:
			{
				break;
			}
		}
	}

}

// --

static S32
Has_Connected_Ports ( const struct USB2_FktNode * hub )
{
	U32 port;

	if ( ! hub->Functions )
	{
		return ( FALSE );
	}

	for ( port = 0; port < hub->Ports; port++ )
	{
		if ( hub->Functions[port] )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

static void
Format_Device_ID ( const struct USB2_FktNode * node, char * id, U32 size )
{
	snprintf ( id, size, "%04X:%04X", (unsigned)LE_SWAP16 ( node->DevDsc.VendorID ),
			   (unsigned)LE_SWAP16 ( node->DevDsc.DeviceID ) );
}

// --

static S32
Add_Connected_Row ( struct USB2_FktNode * node, U32 level, U32 flags, const char * label, const char * id )
{
	struct Node * row;

	const char * drivers;
	const char * driverText;
	char		 matches[256];
	STR			 extra;
	U32			 tag;

	extra	   = NULL;
	driverText = "";

	if ( ( ! ( flags & LBFLG_HASCHILDREN ) ) && ( ( node->Type == TYPE_INTERFACE ) || ( node->Port ) ) )
	{
		tag = ( node->Type == TYPE_INTERFACE ) ? USB2Tag_Attr_DriverTitles : USB2Tag_Attr_DriverTitleSummary;

		drivers = Read_Driver_Titles ( node->ID, tag, matches, sizeof ( matches ), &extra );

		driverText = drivers ? ( *drivers ? drivers : "No match" ) : "Unavailable";
	}

	row = AllocListBrowserNode ( 3, LBNA_UserData, node, LBNA_Generation, level, LBNA_Flags, flags, LBNA_Column, 0,
								 LBNCA_CopyText, TRUE, LBNCA_Text, label, LBNA_CheckBox, TRUE, LBNA_Checked,
								 Capture_Is_Checked ( node ), LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, id, LBNA_Column,
								 2, LBNCA_CopyText, TRUE, LBNCA_Text, driverText, TAG_END );

	if ( extra )
	{
		FreeVec ( extra );
	}

	if ( ! row )
	{
		return ( FALSE );
	}

	AddTail ( &DeviceTreeGUIList, row );

	return ( TRUE );
}

// --

static S32
Has_Multiple_Interfaces ( struct USB2_FktNode * device )
{
	struct USB2_FktNode * child;

	U32 count;

	count = 0;

	for ( child = (PTR)GetHead ( &device->Header ); child; child = (PTR)GetSucc ( &child->Node ) )
	{
		if ( ( child->Type == TYPE_INTERFACE ) && ( ++count == 2 ) )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

static S32
Build_Connected_Ports ( const struct USB2_FktNode * hub )
{
	struct USB2_FktNode * node;

	const char * product;
	char		 id[10];
	U32			 port;
	S32			 composite;

	if ( ! hub->Functions )
	{
		return ( TRUE );
	}

	for ( port = 0; port < hub->Ports; port++ )
	{
		node = hub->Functions[port];

		if ( ! node )
		{
			continue;
		}

		product
			= node->ProductStr[0] ? node->ProductStr : ( node->DevDsc.DeviceClass == USBCLASS_HUB ? "USB hub" : "USB device" );

		Format_Device_ID ( node, id, sizeof ( id ) );

		composite = Has_Multiple_Interfaces ( node );

		if ( ! Add_Connected_Row ( node, 2, composite ? LBFLG_HASCHILDREN : 0, product, id ) )
		{
			return ( FALSE );
		}

		if ( composite )
		{
			struct USB2_FktNode * child;

			char label[128];

			for ( child = (PTR)GetHead ( &node->Header ); child; child = (PTR)GetSucc ( &child->Node ) )
			{
				if ( child->Type != TYPE_INTERFACE )
				{
					continue;
				}

				FktNode_Format_Interface ( child, label, sizeof ( label ) );

				if ( ! Add_Connected_Row ( child, 3, 0, label, "" ) )
				{
					return ( FALSE );
				}
			}
		}

		if ( ! Build_Connected_Ports ( node ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

void
Build_Device_Tree ( void )
{
	struct USB2_FktNode * node;
	struct USB2_FktNode * n;
	struct USB2_FktNode * selected;
	struct Node *		  guinode;

	S32	 index;
	S32	 selectedindex;
	char namebuf[256];

	selected	  = NULL;
	selectedindex = -1;

	Update_Topology_Counts ();

	SetAttrs ( Main_Objects[GADMAINID_DeviceTree], LISTBROWSER_Labels, ~0UL, TAG_END );

	FreeListBrowserList ( &DeviceTreeGUIList );

	node = (PTR)GetHead ( &DeviceTreeUSBList );

	while ( node )
	{
		// --

		n = (PTR)GetHead ( &node->Header );

		if ( ( ConnectedDevices ) && ( ( ! n ) || ( ! Has_Connected_Ports ( n ) ) ) )
		{
			node = (PTR)GetSucc ( (PTR)node );
			continue;
		}

		guinode = AllocListBrowserNode ( 3, LBNA_UserData, node, LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text,
										 ( node->Name ) ? node->Name : "", LBNA_CheckBox, TRUE, LBNA_Checked,
										 Capture_Is_Checked ( node ), LBNA_Generation, 1, LBNA_Flags, n ? LBFLG_HASCHILDREN : 0,
										 LBNA_Column, 2, LBNCA_Text, ConnectedDevices ? "" : "Host Controller", TAG_END );

		if ( ! guinode )
		{
			break;
		}

		AddTail ( &DeviceTreeGUIList, guinode );

		// --

		if ( ( ConnectedDevices ) && ( n ) )
		{
			if ( ( ! Add_Connected_Row ( n, 2, 0, "Root HUB", "" ) ) || ( ! Build_Connected_Ports ( n ) ) )
			{
				break;
			}
		}
		else if ( n )
		{
			namebuf[0] = 0;

			if ( ! Build_Device_TreeNode ( &DeviceTreeGUIList, 2, n, namebuf ) )
			{
				// Break, out of mem no point in contiueing
				break;
			}
		}

		// --

		node = (PTR)GetSucc ( (PTR)node );
	}

	ShowAllListBrowserChildren ( &DeviceTreeGUIList );

	if ( SelectedDeviceID )
	{
		index = 0;

		for ( guinode = GetHead ( &DeviceTreeGUIList ); guinode; guinode = GetSucc ( guinode ), index++ )
		{
			n = NULL;

			GetListBrowserNodeAttrs ( guinode, LBNA_UserData, &n, TAG_END );

			if ( ( n ) && ( n->ID == SelectedDeviceID ) )
			{
				selected	  = n;
				selectedindex = index;
				break;
			}
		}
	}

	RefreshSetGadgetAttrs ( (PTR)Main_Objects[GADMAINID_DeviceTree], Main_WindowAdr, NULL, LISTBROWSER_ColumnInfo,
							ConnectedDevices ? ConnectedDevicesColInfo : DeviceTreeColInfo, LISTBROWSER_Labels,
							&DeviceTreeGUIList, LISTBROWSER_AutoFit, ConnectedDevices, LISTBROWSER_Selected, selectedindex,
							TAG_END );

	if ( SelectedDeviceID )
	{
		Show_Device_Properties ( selected );
	}
}

// --

static S32
Build_Device_TreeNode ( struct List * l, U32 g, struct USB2_FktNode * n, STR namebuf )
{
	struct USB2_FktNode * node;
	struct Node *		  guinode;
	S32					  retval;
	U32					  pos;
	U32					  cnt;
	char				  id[10];

	retval = FALSE;
	id[0]  = 0;

	if ( ( n->Type == TYPE_FUNCTION ) && ( n->Port ) )
	{
		Format_Device_ID ( n, id, sizeof ( id ) );
	}

	/**/ if ( ( n->Type == TYPE_FUNCTION ) && ( n->DevDsc.DeviceClass == USBCLASS_HUB ) )
	{
		pos = strlen ( namebuf );

		if ( g == 2 )
		{
			sprintf ( &namebuf[pos], "Root HUB" );
		}
		else
		{
			sprintf ( &namebuf[pos], "Generic HUB" );
		}

		guinode = AllocListBrowserNode ( 3, LBNA_UserData, n, LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, namebuf,
										 LBNA_CheckBox, TRUE, LBNA_Checked, Capture_Is_Checked ( n ),
										 LBNA_Generation, g, LBNA_Flags, LBFLG_HASCHILDREN, LBNA_Column, 1, LBNCA_CopyText, TRUE,
										 LBNCA_Text, id, LBNA_Column, 2, LBNCA_Text, "HUB Class",
										 //				LBNA_CheckBox,		TRUE,
										 TAG_END );

		if ( ! guinode )
		{
			goto bailout;
		}

		AddTail ( l, guinode );

		// --

		for ( cnt = 0; cnt < n->Ports; cnt++ )
		{
			sprintf ( namebuf, "Port #%d: ", cnt + 1 );

			node = n->Functions[cnt];

			if ( node )
			{
				if ( ! Build_Device_TreeNode ( l, g + 1, node, namebuf ) )
				{
					// out of mem
					goto bailout;
				}
			}
			else
			{
				pos = strlen ( namebuf );

				sprintf ( &namebuf[pos], "Empty" );

				guinode = AllocListBrowserNode ( 3, LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, namebuf, LBNA_Generation,
												 g + 1, LBNA_Flags, LBFLG_READONLY, LBNA_CheckBox, TRUE, LBNA_Checked, FALSE,
												 LBNA_Column, 2, LBNCA_Text, "Function", TAG_END );

				if ( ! guinode )
				{
					// out of mem
					goto bailout;
				}

				AddTail ( l, guinode );
			}
		}

		for ( node = (PTR)GetHead ( &n->Header ); node; node = (PTR)GetSucc ( &node->Node ) )
		{
			if ( ( n->Functions ) && ( node->Type == TYPE_FUNCTION ) )
			{
				continue;
			}

			// The hub row already displays its interface descriptor.
			if ( ( node->Type == TYPE_INTERFACE ) && ( node->IfcDesc ) && ( node->IfcDesc->InterfaceClass == USBCLASS_HUB ) )
			{
				continue;
			}

			namebuf[0] = 0;

			if ( ! Build_Device_TreeNode ( l, g + 1, node, namebuf ) )
			{
				goto bailout;
			}
		}
	}
	else if ( n->Type == TYPE_FUNCTION )
	{
		pos = strlen ( namebuf );

		snprintf ( &namebuf[pos], 256 - pos, "%s", n->ProductStr[0] ? n->ProductStr : "USB Device" );

		guinode = AllocListBrowserNode ( 3, LBNA_UserData, n, LBNA_Column, 0, LBNCA_CopyText, TRUE, LBNCA_Text, namebuf,
										 LBNA_CheckBox, TRUE, LBNA_Checked, Capture_Is_Checked ( n ),
										 LBNA_Generation, g, LBNA_Flags, IsListEmpty ( &n->Header ) ? 0 : LBFLG_HASCHILDREN,
										 LBNA_Column, 1, LBNCA_CopyText, TRUE, LBNCA_Text, id, LBNA_Column, 2, LBNCA_Text,
										 "Function",
										 //				LBNA_CheckBox,		TRUE,
										 TAG_END );

		if ( ! guinode )
		{
			goto bailout;
		}

		AddTail ( l, guinode );

		// ---

		node = (PTR)GetHead ( &n->Header );

		while ( node )
		{
			namebuf[0] = 0;

			if ( ! Build_Device_TreeNode ( &DeviceTreeGUIList, g + 1, node, namebuf ) )
			{
				// Break, out of mem no point in contiueing
				break;
			}

			node = (PTR)GetSucc ( (PTR)node );
		}
	}
	else if ( n->Type == TYPE_INTERFACE )
	{
		STR clsstring;

		if ( n->IfcDesc )
		{
			clsstring
				= GetClassName ( n->IfcDesc->InterfaceClass, n->IfcDesc->InterfaceSubClass, n->IfcDesc->InterfaceProtocol );
		}
		else
		{
			clsstring = GetClassName ( 0, 0, 0 );
		}

		guinode = AllocListBrowserNode ( 3, LBNA_UserData, n, LBNA_Column, 0, LBNCA_CopyText, FALSE, LBNCA_Text,
										 ( clsstring ) ? clsstring : "", LBNA_CheckBox, TRUE, LBNA_Checked,
										 Capture_Is_Checked ( n ), LBNA_Generation, g, LBNA_Column, 2, LBNCA_Text, "Interface",
										 //				LBNA_CheckBox,		TRUE,
										 TAG_END );

		if ( ! guinode )
		{
			goto bailout;
		}

		AddTail ( l, guinode );

		// ---
	}
	//	  #ifdef DEBUG
	//	  else if ( n->Type == TYPE_HCD )
	//	  {
	//		  SHOWMSG( "Warning HCD Node" );
	//		  goto bailout;
	//	  }
	//	  else
	//	  {
	//		  SHOWMSG( "Unknonw Node Type" );
	//		  goto bailout;
	//	  }
	//	  #endif

	// --

	retval = TRUE;

bailout:

	return ( retval );
}

// --
