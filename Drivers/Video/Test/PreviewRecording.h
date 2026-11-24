#ifndef USB2_VIDEO_TEST_PREVIEW_RECORDING_H
#define USB2_VIDEO_TEST_PREVIEW_RECORDING_H

#define PREVIEW_PATH_SIZE 512

enum PreviewRecordingResult
{
	PREVIEW_RECORD_EMPTY,
	PREVIEW_RECORD_SAVED,
	PREVIEW_RECORD_PARTIAL,
	PREVIEW_RECORD_FAILED
};

struct PreviewRecording
{
	char Drawer[PREVIEW_PATH_SIZE];
	char Path[PREVIEW_PATH_SIZE];
	char Destination[PREVIEW_PATH_SIZE];
	BPTR DrawerLock;
};

S32 PreviewRecording_Stage( struct PreviewRecording *recording, const char *destination, const char *filename );
void PreviewRecording_Finish( struct PreviewRecording *recording, struct Window *window, const char *kind,
	enum PreviewRecordingResult result );

#endif
