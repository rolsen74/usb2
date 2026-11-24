#ifndef USB2_VIDEO_TEST_PREVIEW_AVI_H
#define USB2_VIDEO_TEST_PREVIEW_AVI_H

#include "PreviewRecording.h"

struct VideoFormat;
struct PreviewAviIndex;

struct PreviewAvi
{
	struct PreviewAviIndex *Index;

	BPTR File;
	U32 End;
	U32 Frames;
	U32 Capacity;
	U32 LargestFrame;
	S32 WriteFailed;
	U64 FirstTime;
	U64 LastTime;
};

S32 PreviewAvi_Write( struct PreviewAvi *avi, const char *path, const struct VideoFormat *format,
	const U8 *frame, U32 bytes, U64 time );
enum PreviewRecordingResult PreviewAvi_Close( struct PreviewAvi *avi, const char *path, const struct VideoFormat *format );

#endif
