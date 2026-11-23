
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

#define NO(cls)			NewObject(cls,NULL
#define NO2(cls,id)		Main_Objects[id]=NewObject(cls,NULL

enum 
{
	GADMAINID_Dummy = 0,
	GADMAINID_Root,
	GADMAINID_ClickTab,
	GADMAINID_Page,
	GADMAINID_DeviceTree,
	GADMAINID_PropertiesList,
	GADMAINID_CaptureList,
	GADMAINID_IORequestList,
	GADMAINID_Status,
	GADMAINID_Time,
	GADMAINID_LAST
};

// --

enum
{
	MENU_SAVE_TREE = 1,
	MENU_ABOUT,
	MENU_QUIT,
	MENU_PREFERENCES
};

// --

struct Window *		Main_WindowAdr		= NULL;
static Object *		Main_MenuStrip		= NULL;
Object *			Main_WinObject		= NULL;
S32					Main_WinXPos		= 0;
S32					Main_WinYPos		= 0;
S32					Main_WinWidth		= 0;
S32					Main_WinHeight		= 0;
U32					Main_WindowBits		= 0;
Object *			Main_Objects[GADMAINID_LAST];

struct List			DeviceTreeUSBList;
struct List			DeviceTreeGUIList;
struct ColumnInfo *	DeviceTreeColInfo	= NULL;

struct List			PropertiesGUIList;
struct ColumnInfo *	PropertiesColInfo	= NULL;
static USB2_ID		SelectedDeviceID	= 0;

struct List			CaptureGUIList;
struct ColumnInfo *	CaptureColInfo		= NULL;

struct List			IORequestGUIList;
struct ColumnInfo *	IORequestColInfo	= NULL;

// --

static S32		Build_Device_TreeNode( struct List *l, U32 g, struct USB2_FktNode *n, STR namebuf );

// --

static S32
Main_CreateMenu( void )
{
	static struct TagItem items[] =
	{
		{ NM_Menu, (Tag) "Project" },
		{ NM_Item, (Tag) "Save tree" },
		{ MA_ID, MENU_SAVE_TREE },
		{ NM_Item, (Tag) ML_SEPARATOR },
		{ NM_Item, (Tag) "About" },
		{ MA_ID, MENU_ABOUT },
		{ MA_Key, (Tag) "?" },
		{ NM_Item, (Tag) ML_SEPARATOR },
		{ NM_Item, (Tag) "Quit" },
		{ MA_ID, MENU_QUIT },
		{ MA_Key, (Tag) "Q" },
		{ NM_Menu, (Tag) "Edit" },
		{ NM_Item, (Tag) "Preferences..." },
		{ MA_ID, MENU_PREFERENCES },
		{ MA_Key, (Tag) "P" },
		{ TAG_END, 0 }
	};

	Main_MenuStrip = NewObject( NULL, "menuclass", MA_Type, T_ROOT, TAG_END );

	if ( ! Main_MenuStrip )
	{
		printf( "USB2Analyzer: could not open menuclass (requires Intuition 54.6 or later).\n" );

		return ( FALSE );
	}

	if ( ! IDoMethod( Main_MenuStrip, MM_NEWMENU, 0, items ) )
	{
		printf( "USB2Analyzer: could not build the menu.\n" );

		return ( FALSE );
	}

	SetAttrs( Main_WinObject, WINDOW_MenuStrip, Main_MenuStrip, TAG_END );

	return ( TRUE );
}

// --

static void
Main_HandleMenu( void )
{
	static const struct EasyStruct about =
	{
		.es_StructSize = sizeof( struct EasyStruct ),
		.es_Title = "About USB2Analyzer",
		.es_TextFormat = VERS " (" DATE ")\n"
			"Copyright (c) 2012-2026 by Rene W. Olsen\n"
			"GPL-3.0-or-later",
		.es_GadgetFormat = "OK"
	};

	U32 id;

	id = NO_MENU_ID;

	while( ( id = IDoMethod( Main_MenuStrip, MM_NEXTSELECT, 0, id ) ) != NO_MENU_ID )
	{
		switch( id )
		{
			case MENU_SAVE_TREE:
			{
				Request_Device_Tree_Save( Main_WindowAdr );

				break;
			}

			case MENU_ABOUT:
			{
				EasyRequestArgs( Main_WindowAdr, & about, NULL, NULL );

				break;
			}

			case MENU_PREFERENCES:
			{
				Prefs_OpenWindow( Main_WindowAdr );

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

S32 Main_OpenWindow( void )
{
S32 retval;

	retval = FALSE;

	if ( ! Main_WindowAdr )
	{
		if ( ! Main_CreateMenu() )
		{
			goto bailout;
		}

		Main_WindowAdr = (PTR) IDoMethod( Main_WinObject, WM_OPEN );

		if ( ! Main_WindowAdr )
		{
			printf( "Main_WindowAdr() : %p : failed\n", Main_WinObject );
			goto bailout;
		}

		GetAttrs( Main_WinObject,
			WINDOW_SigMask, & Main_WindowBits,
			TAG_END
		);
	}

	retval = TRUE;

bailout:

	return(	retval );
}

// --

void Main_CloseWindow( void )
{
	if ( Main_WindowAdr )
	{
		if ( Main_ScreenType == SCRTYPE_Window )
		{
			Main_WinXPos	= Main_WindowAdr->LeftEdge;
			Main_WinYPos	= Main_WindowAdr->TopEdge;
			Main_WinWidth	= Main_WindowAdr->Width;
			Main_WinHeight	= Main_WindowAdr->Height;
		}

		IDoMethod( Main_WinObject, WM_CLOSE );
		Main_WindowBits = 0;
		Main_WindowAdr = NULL;
	}

	if ( Main_MenuStrip )
	{
		SetAttrs( Main_WinObject, WINDOW_MenuStrip, NULL, TAG_END );

		DisposeObject( Main_MenuStrip );

		Main_MenuStrip = NULL;
	}
}

// --

S32 Main_BuildWindow( void )
{
S32 retval;

	retval = FALSE;

	DeviceTreeColInfo = AllocLBColumnInfo( 2,
		LBCIA_Column, 0,
		 LBCIA_Title, "Device Tree",
		 LBCIA_Weight, 60,
		LBCIA_Column, 1,
		 LBCIA_Title, "Class Name",
		 LBCIA_Weight, 40,
		TAG_END
	);

	if ( ! DeviceTreeColInfo )
	{
		printf( "Error allocating ListBrowser Column Info\n" );
		goto bailout;
	}

	PropertiesColInfo = AllocLBColumnInfo( 4,
		LBCIA_Column, 0,
		 LBCIA_Title, "Field",
		 LBCIA_Weight, 36,
		LBCIA_Column, 1,
		 LBCIA_Title, "Size",
		 LBCIA_Weight, 8,
		LBCIA_Column, 2,
		 LBCIA_Title, "Value",
		 LBCIA_Weight, 24,
		LBCIA_Column, 3,
		 LBCIA_Title, "Description",
		 LBCIA_Weight, 32,
		TAG_END
	);

	if ( ! PropertiesColInfo )
	{
		goto bailout;
	}

	Main_WinObject = NO(WindowClass),
		WA_Activate,								TRUE,
//		WA_Top,										MainScreen->WBorTop + MainScreen->Font->ta_YSize,
//		WA_Left,									0,
//		WA_Width,									MainScreen->Width,
//		WA_Height,									MainScreen->Height - ( MainScreen->WBorTop + MainScreen->Font->ta_YSize ),
//		WA_Backdrop,								TRUE,
//		WA_Borderless,								TRUE,
//		WA_CustomScreen,							MainScreen,
		WINDOW_ParentGroup,							NO2(LayoutClass,GADMAINID_Root),
			LAYOUT_Orientation,						LAYOUT_ORIENT_VERT,

			// Toolbar
			LAYOUT_AddChild,						NO(LayoutClass),
				LAYOUT_Orientation,					LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,					BVS_THIN,

				LAYOUT_AddChild,					NO(LayoutClass),
				End,
			End,
			CHILD_WeightedHeight,					0,

			// Listviews
			LAYOUT_AddChild,						NO(LayoutClass),
				LAYOUT_Orientation,					LAYOUT_ORIENT_HORIZ,

				LAYOUT_AddChild,					NO(LayoutClass),
					LAYOUT_Orientation,				LAYOUT_ORIENT_VERT,

					LAYOUT_AddChild,				NO2(ListBrowserClass,GADMAINID_DeviceTree),
						GA_ID,						GADMAINID_DeviceTree,
						GA_RelVerify,				TRUE,
						LISTBROWSER_Hierarchical,	TRUE,
						LISTBROWSER_ShowSelected,	TRUE,
						LISTBROWSER_ColumnTitles,	TRUE,
						LISTBROWSER_ColumnInfo,		DeviceTreeColInfo,
						LISTBROWSER_Labels,			& DeviceTreeGUIList,
					End,

					LAYOUT_WeightBar,				TRUE,

					// Properties List
					LAYOUT_AddChild,				NO2(ListBrowserClass,GADMAINID_PropertiesList),
						GA_ID,						GADMAINID_PropertiesList,
						LISTBROWSER_Hierarchical,	TRUE,
						LISTBROWSER_AutoFit,		TRUE,
						LISTBROWSER_VertSeparators,	FALSE,
						LISTBROWSER_ColumnTitles,	TRUE,
						LISTBROWSER_ColumnInfo,		PropertiesColInfo,
						LISTBROWSER_Labels,			& PropertiesGUIList,
						LISTBROWSER_HorizontalProp,	TRUE,
					End,
				End,

				LAYOUT_WeightBar,					TRUE,

				LAYOUT_AddChild,					NO(LayoutClass),
					LAYOUT_Orientation,				LAYOUT_ORIENT_VERT,

					// Capture List
					LAYOUT_AddChild,				NO2(ListBrowserClass,GADMAINID_CaptureList),
						GA_ID,						GADMAINID_CaptureList,
						GA_RelVerify,				TRUE,
						LISTBROWSER_AutoFit,		TRUE,
						LISTBROWSER_VertSeparators,	TRUE,
						LISTBROWSER_ShowSelected,	TRUE,
						LISTBROWSER_ColumnTitles,	TRUE,
//						LISTBROWSER_ColumnInfo,		CaptureColInfo,
						LISTBROWSER_Labels,			& CaptureGUIList,
						LISTBROWSER_HorizontalProp,	TRUE,
					End,

					LAYOUT_WeightBar,				TRUE,

					// IORequest List
					LAYOUT_AddChild,				NO2(ListBrowserClass,GADMAINID_IORequestList),
						GA_ID,						GADMAINID_IORequestList,
//						GA_ReadOnly,				TRUE,
						GA_RelVerify,				TRUE,
						LISTBROWSER_AutoFit,		TRUE,
						LISTBROWSER_Hierarchical,	TRUE,
						LISTBROWSER_VertSeparators,	TRUE,
						LISTBROWSER_ColumnTitles,	TRUE,
//						LISTBROWSER_ColumnInfo,		IORequestColInfo,
						LISTBROWSER_Labels,			& IORequestGUIList,
					End,
				End,
			End,

			// Status
			LAYOUT_AddChild,						NO(LayoutClass),
				LAYOUT_Orientation,					LAYOUT_ORIENT_HORIZ,
				LAYOUT_BevelStyle,					BVS_THIN,
				LAYOUT_EvenSize,					TRUE,

				LAYOUT_AddChild,					NO(LayoutClass),
					LAYOUT_BevelStyle,				BVS_THIN,
					LAYOUT_AddChild,				NO(LayoutClass),
						LAYOUT_AddChild,			NO(ButtonClass),
							GA_Text,				"Stack: ",
							GA_ReadOnly,			TRUE,
							BUTTON_Justification,	BCJ_RIGHT,
							BUTTON_Transparent,		TRUE,
							BUTTON_BevelStyle,		BVS_NONE,
						End,
					End,
					CHILD_WeightedWidth,			0,

					LAYOUT_AddChild,				NO(LayoutClass),
						LAYOUT_AddChild,			NO2(ButtonClass,GADMAINID_Status),
							GA_ReadOnly,			TRUE,
							BUTTON_Justification,	BCJ_LEFT,
							BUTTON_Transparent,		TRUE,
							BUTTON_BevelStyle,		BVS_NONE,
						End,
					End,
				End,

				LAYOUT_AddChild,					NO(LayoutClass),
					LAYOUT_BevelStyle,				BVS_THIN,
					LAYOUT_AddChild,				NO(LayoutClass),
						LAYOUT_AddChild,			NO(ButtonClass),
							GA_Text,				"Uptime: ",
							GA_ReadOnly,			TRUE,
							BUTTON_Justification,	BCJ_RIGHT,
							BUTTON_Transparent,		TRUE,
							BUTTON_BevelStyle,		BVS_NONE,
						End,
					End,
					CHILD_WeightedWidth,			0,

					LAYOUT_AddChild,				NO(LayoutClass),
						LAYOUT_AddChild,			NO2(ButtonClass,GADMAINID_Time),
							GA_ReadOnly,			TRUE,
							BUTTON_Justification,	BCJ_LEFT,
							BUTTON_Transparent,		TRUE,
							BUTTON_BevelStyle,		BVS_NONE,
						End,
					End,
				End,

			End,
			CHILD_WeightedHeight,					0,
		End,
	End;

	if ( ! Main_WinObject )
	{
		goto bailout;
	}

	retval = TRUE;

bailout:

	return( retval );
}

// --

void Main_DestoryWindow( void )
{
	if ( Main_WinObject )
	{
		DisposeObject( Main_WinObject );
		Main_WinObject = NULL;
	}

	FreeListBrowserList( & DeviceTreeGUIList );
	FreeListBrowserList( & PropertiesGUIList );

	if ( DeviceTreeColInfo )
	{
		FreeLBColumnInfo( DeviceTreeColInfo );
		DeviceTreeColInfo = NULL;
	}

	if ( PropertiesColInfo )
	{
		FreeLBColumnInfo( PropertiesColInfo );
		PropertiesColInfo = NULL;
	}
}

// --

static void
Show_Device_Properties( struct USB2_FktNode *node )
{
	SelectedDeviceID = node ? node->ID : 0;

	SetAttrs( Main_Objects[GADMAINID_PropertiesList],
		LISTBROWSER_Labels, NULL,
		TAG_END
	);

	FreeListBrowserList( & PropertiesGUIList );

	Build_Device_Properties( & PropertiesGUIList, node );

	RefreshSetGadgetAttrs( (PTR) Main_Objects[GADMAINID_PropertiesList], Main_WindowAdr, NULL,
		LISTBROWSER_Labels, & PropertiesGUIList,
		LISTBROWSER_AutoFit, TRUE,
		LISTBROWSER_Top, 0,
		LISTBROWSER_Left, 0,
		TAG_END
	);
}

// --

void Main_HandleWindow( void )
{
struct USB2_FktNode *node;
struct Node *selected;

U32 result;
U16 code;

	while(( result = IDoMethod( Main_WinObject, WM_HANDLEINPUT, & code )) != WMHI_LASTMSG )
	{
		switch( result & WMHI_CLASSMASK )
		{
			case WMHI_GADGETUP:
			{
				switch( result & WMHI_GADGETMASK )
				{
					case GADMAINID_DeviceTree:
					{
						selected = NULL;
						node = NULL;

						GetAttrs( Main_Objects[GADMAINID_DeviceTree],
							LISTBROWSER_SelectedNode, & selected,
							TAG_END
						);

						if ( selected )
						{
							GetListBrowserNodeAttrs( selected, LBNA_UserData, & node, TAG_END );
						}

						Show_Device_Properties( node );

						break;
					}

//					case GADMAINID_CaptureList:
//					{
//						GetAttrs( MainObjects[ GADMAINID_CaptureList ],
//							LISTBROWSER_SelectedNode, &node2,
//							TAG_END
//						);
//						break;
//					}

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
				Main_HandleMenu();

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

//	if (( node2 != NULL ) && ( node2->ln_Name != NULL ))
//	{
//		BuildIORequestList( (struct CaptureNode *)node2->ln_Name );
//	}
}

// --

void Build_Device_Tree( void )
{
struct USB2_FktNode *node;
struct USB2_FktNode *n;
struct USB2_FktNode *selected;
struct Node *guinode;

S32 index;
S32 selectedindex;
char namebuf[256];

	selected = NULL;
	selectedindex = -1;

	SetAttrs( Main_Objects[GADMAINID_DeviceTree],
		LISTBROWSER_Labels, NULL,
		TAG_END
	);

	FreeListBrowserList( & DeviceTreeGUIList );

	node = (PTR) GetHead( & DeviceTreeUSBList );

	while( node )
	{
		// --

		guinode = AllocListBrowserNode( 2,
			LBNA_UserData,		node,
			LBNA_Column,		0,
				LBNCA_CopyText,	TRUE,
				LBNCA_Text,		( node->Name ) ? node->Name : "",
			LBNA_Generation,	1,
			LBNA_Flags,			IsListEmpty( &node->Header ) ? 0 : LBFLG_HASCHILDREN,
			LBNA_Column,		1,
				LBNCA_Text,		"Host Controller",
//				LBNA_CheckBox,	TRUE,
			TAG_END
		);

		if ( ! guinode )
		{
			break;
		}

		AddTail( & DeviceTreeGUIList, guinode );

		// --

		n = (PTR) GetHead( & node->Header );
		
		if ( n )
		{
			namebuf[0] = 0;

			if ( ! Build_Device_TreeNode( & DeviceTreeGUIList, 2, n , namebuf ))
			{
				// Break, out of mem no point in contiueing
				break;
			}
		}

		// --

		node = (PTR) GetSucc( (PTR) node );
	}

	ShowAllListBrowserChildren( & DeviceTreeGUIList );

	if ( SelectedDeviceID )
	{
		index = 0;

		for( guinode = GetHead( & DeviceTreeGUIList ); guinode; guinode = GetSucc( guinode ), index++ )
		{
			n = NULL;

			GetListBrowserNodeAttrs( guinode, LBNA_UserData, & n, TAG_END );

			if ( ( n ) && ( n->ID == SelectedDeviceID ) )
			{
				selected = n;
				selectedindex = index;
				break;
			}
		}
	}

	RefreshSetGadgetAttrs( (PTR) Main_Objects[ GADMAINID_DeviceTree ], Main_WindowAdr, NULL,
		LISTBROWSER_Labels, & DeviceTreeGUIList,
		LISTBROWSER_Selected, selectedindex,
		TAG_END
	);

	if ( SelectedDeviceID )
	{
		Show_Device_Properties( selected );
	}
}

// --

static S32 Build_Device_TreeNode( struct List *l, U32 g, struct USB2_FktNode *n, STR namebuf )
{
struct USB2_FktNode *node;
struct Node *guinode;
S32 retval;
U32 pos;
U32 cnt;

	retval = FALSE;

	/**/ if (( n->Type == TYPE_FUNCTION ) && ( n->DevDsc.DeviceClass == USBCLASS_HUB ))
	{
		pos = strlen( namebuf );

		if ( g == 2 )
		{
			sprintf( & namebuf[pos], "Root HUB" );
		}
		else
		{
			sprintf( & namebuf[pos], "Generic HUB" );
		}

		guinode = AllocListBrowserNode( 2,
			LBNA_UserData,		n,
			LBNA_Column,			0,
				LBNCA_CopyText,		TRUE,
				LBNCA_Text,			namebuf,
			LBNA_Generation,		g,
			LBNA_Flags,				LBFLG_HASCHILDREN,
			LBNA_Column,			1,
				LBNCA_Text,			"HUB Class",
//				LBNA_CheckBox,		TRUE,
			TAG_END
		);

		if ( ! guinode )
		{
			goto bailout;
		}

		AddTail( l, guinode );

		// --

		for( cnt=0 ; cnt < n->Ports ; cnt++ )
		{
			sprintf( namebuf, "Port #%d: ", cnt+1 );

			node = n->Functions[cnt];

			if ( node )
			{
				if ( ! Build_Device_TreeNode( l, g + 1, node, namebuf ))
				{
					// out of mem
					goto bailout;
				}
			}
			else
			{
				pos = strlen( namebuf );

				sprintf( & namebuf[pos], "Empty" );

				guinode = AllocListBrowserNode( 2,
					LBNA_Column,			0,
						LBNCA_CopyText,		TRUE,
						LBNCA_Text,			namebuf,
					LBNA_Generation,		g + 1,
					LBNA_Flags,				0,
					LBNA_Column,			1,
						LBNCA_Text,			"Function",
//						LBNA_CheckBox,		TRUE,
					TAG_END
				);

				if ( ! guinode )
				{
					// out of mem
					goto bailout;
				}

				AddTail( l, guinode );
			}
		}

		for ( node = (PTR) GetHead( & n->Header ); node; node = (PTR) GetSucc( & node->Node ) )
		{
			if ( ( n->Functions ) && ( node->Type == TYPE_FUNCTION ) )
			{
				continue;
			}

			// The hub row already displays its interface descriptor.
			if ( ( node->Type == TYPE_INTERFACE ) && ( node->IfcDesc ) &&
				( node->IfcDesc->InterfaceClass == USBCLASS_HUB ) )
			{
				continue;
			}

			namebuf[0] = 0;

			if ( ! Build_Device_TreeNode( l, g + 1, node, namebuf ) )
			{
				goto bailout;
			}
		}
	}
	else if ( n->Type == TYPE_FUNCTION )
	{
		pos = strlen( namebuf );

		snprintf( & namebuf[pos], 256 - pos, "%s", n->ProductStr[0] ? n->ProductStr : "USB Device" );

		guinode = AllocListBrowserNode( 2,
			LBNA_UserData,		n,
			LBNA_Column,			0,
				LBNCA_CopyText,		TRUE,
				LBNCA_Text,			namebuf,
			LBNA_Generation,		g,
			LBNA_Flags,				IsListEmpty( & n->Header ) ? 0 : LBFLG_HASCHILDREN,
			LBNA_Column,			1,
				LBNCA_Text,			"Function",
//				LBNA_CheckBox,		TRUE,
			TAG_END
		);

		if ( ! guinode )
		{
			goto bailout;
		}

		AddTail( l, guinode );

		// ---

		node = (PTR) GetHead( & n->Header );

		while( node )
		{
			namebuf[0] = 0;

			if ( ! Build_Device_TreeNode( & DeviceTreeGUIList, g + 1, node , namebuf ))
			{
				// Break, out of mem no point in contiueing
				break;
			}

			node = (PTR) GetSucc( (PTR) node );
		}
	}
	else if ( n->Type == TYPE_INTERFACE )
	{
		STR clsstring;

		if ( n->IfcDesc )
		{
			clsstring = GetClassName(
				n->IfcDesc->InterfaceClass,
				n->IfcDesc->InterfaceSubClass,
				n->IfcDesc->InterfaceProtocol
			);
		}
		else
		{
			clsstring = GetClassName( 0, 0, 0 );
		}

		guinode = AllocListBrowserNode( 2,
			LBNA_UserData,		n,
			LBNA_Column,			0,
				LBNCA_CopyText,		FALSE,
				LBNCA_Text,			( clsstring ) ? clsstring : "",
			LBNA_Generation,		g,
			LBNA_Column,			1,
				LBNCA_Text,			"Interface",
//				LBNA_CheckBox,		TRUE,
			TAG_END
		);

		if ( ! guinode )
		{
			goto bailout;
		}

		AddTail( l, guinode );

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

	return( retval );
}

// --
