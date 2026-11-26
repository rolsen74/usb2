
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static S32 CaptureRunning;

// --

S32
Capture_Is_Checked( const struct USB2_FktNode *node )
{
	const struct USB2_FktNode *parent;

	if ( ( node->Type == TYPE_HCD ) || ( node->Capture != CAPTURE_INHERIT ) )
	{
		return ( node->Capture == CAPTURE_ON );
	}

	if ( node->Type == TYPE_INTERFACE )
	{
		parent = FktNode_Find( &DeviceTreeUSBList, node->FunctionID );

		return ( ( parent ) && ( Capture_Is_Checked( parent ) ) );
	}

	parent = FktNode_Find( &DeviceTreeUSBList, node->HCD_ID );

	return ( ( parent ) && ( parent->Capture == CAPTURE_ON ) );
}

// --

static U32
Collect_Capture_Targets( struct List *nodes, struct USB2_IOCaptureTarget *targets, U32 index )
{
	struct USB2_FktNode *node;

	for( node = (PTR) GetHead( nodes ); node; node = (PTR) GetSucc( &node->Node ) )
	{
		if ( ( ( node->Type == TYPE_HCD ) && ( node->Capture == CAPTURE_ON ) ) ||
			( ( node->Type != TYPE_HCD ) && ( node->Capture != CAPTURE_INHERIT ) ) )
		{
			if ( targets )
			{
				targets[index].ID = node->ID;
				targets[index].Enabled = ( node->Capture == CAPTURE_ON );
			}

			index++;
		}

		index = Collect_Capture_Targets( &node->Header, targets, index );
	}

	return ( index );
}

// --

static S32
Apply_Capture_Targets( void )
{
	struct USB2_IOCaptureTarget *targets;

	U32 count;
	S32 result;

	targets = NULL;

	count = Collect_Capture_Targets( &DeviceTreeUSBList, NULL, 0 );

	if ( count )
	{
		targets = AllocVecTags( count * sizeof( *targets ), TAG_END );

		if ( ! targets )
		{
			return ( FALSE );
		}

		Collect_Capture_Targets( &DeviceTreeUSBList, targets, 0 );
	}

	result = USB_Set_Capture_Filter( targets, count );

	if ( targets )
	{
		FreeVec( targets );
	}

	return ( result );
}

// --

S32
Capture_Set_Checked( struct USB2_FktNode *node, S32 checked )
{
	enum CaptureSelection previous;

	previous = node->Capture;
	node->Capture = checked ? CAPTURE_ON : CAPTURE_OFF;

	if ( ( CaptureRunning ) && ( ! Apply_Capture_Targets() ) )
	{
		node->Capture = previous;

		return ( FALSE );
	}

	return ( TRUE );
}

// --

S32
Capture_Is_Running( void )
{
	return ( CaptureRunning );
}

// --

S32
Capture_Set_Running( S32 running )
{
	S32 applied;

	if ( CaptureRunning == running )
	{
		return ( TRUE );
	}

	applied = running ? Apply_Capture_Targets() : USB_Set_Capture_Filter( NULL, 0 );

	if ( applied )
	{
		CaptureRunning = running;
	}

	return ( applied );
}

// --
