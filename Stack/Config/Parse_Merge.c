
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static S32
__Merge( struct USBBase *usbbase, struct RealFunctionNode *fn )
{
	struct USB2_InterfaceGroup *groups[256];
	struct USB2_Association_Desc *association;
	struct USB2_InterfaceHeader *ih;
	struct USB2_InterfaceGroup *member;
	struct USB2_InterfaceGroup *group;
	struct USB2_ConfigNode *config;
	struct USB2_Descriptor *desc;

	U32 first;
	U32 end;
	U32 index;
	S32 remaining;

	if ( ( fn->fkt_Class != 0xef ) || ( fn->fkt_SubClass != 0x02 ) || ( fn->fkt_Protocol != 0x01 ) )
	{
		return ( TRUE );
	}

	config = fn->fkt_Configs.uh_Head;
	desc = fn->fkt_Config_Desc_Buf;
	remaining = fn->fkt_Config_Desc_Size;

	MEM_SET( groups, 0, sizeof( groups ) );

	for( group = config->cfg_InterfaceGroups.uh_Head; group; group = Node_Next( group ) )
	{
		ih = group->ig_Group.uh_Head;
		groups[ih->ih_Number] = group;
	}

	// The first pass checked the length of every record.
	while( remaining )
	{
		if ( desc->DescriptorType == DSCTYPE_Interface_Association )
		{
			association = (PTR) desc;
			first = association->FirstInterface;
			end = first + association->InterfaceCount;

			if ( ( first == end ) || ( end > sizeof( groups ) / sizeof( *groups ) ) )
			{
				USBERROR( "__Merge: Invalid association range" );

				return ( FALSE );
			}

			for( index = first; index < end; index++ )
			{
				if ( ! groups[index] )
				{
					USBERROR( "__Merge: Missing or already associated interface %lu", index );

					return ( FALSE );
				}
			}

			group = groups[first];
			groups[first] = NULL;
			group->ig_Class = association->FunctionClass;
			group->ig_SubClass = association->FunctionSubClass;
			group->ig_Protocol = association->FunctionProtocol;

			for( index = first + 1; index < end; index++ )
			{
				member = groups[index];
				groups[index] = NULL;
				ih = member->ig_Group.uh_Head;

				NODE_REMNODE( & config->cfg_InterfaceGroups, member );

				NODE_REMNODE( & member->ig_Group, ih );

				INTERFACE_LOCKGROUP( group );

				INTERFACE_UNLOCKGROUP( member );

				ih->ih_Group = group;

				NODE_ADDTAIL( & group->ig_Group, ih );

				INTERFACE_FREEGROUP( member );
			}
		}

		remaining -= desc->Length;
		desc = (PTR) ( (U8 *) desc + desc->Length );
	}

	return ( TRUE );
}

// --
