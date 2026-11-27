#ifndef USB2_VIDEO_TEST_PREVIEW_H
#define USB2_VIDEO_TEST_PREVIEW_H

struct TimerIFace;
struct USB2IFace;

struct VideoTestBuffers
{
	struct IORequest Device;
	struct USB2IFace *Allocator;
	struct VideoIsoBuffers Iso;
};

S32 VideoTest_PrepareIsoBuffers( struct IOStdReq *ioreq, struct VideoTestBuffers *buffers );
S32 VideoTest_StopCapture( struct IOStdReq *ioreq, struct VideoTestBuffers *buffers );

struct PreviewCameras
{
	U32 Units[VIDEO_MAX_UNITS];
	STR Labels[VIDEO_MAX_UNITS + 1];
	char Names[VIDEO_MAX_UNITS][32];
	U32 Count;
	U32 Selected;
};

void VideoTest_ReportCaptureStats( struct IOStdReq *ioreq );
U64 VideoTest_UptimeUS( struct TimerIFace *clock );
S32 VideoTest_QueryCamera( struct IOStdReq *ioreq, struct VideoFormat *format );
S32 VideoTest_OpenCamera( struct IOStdReq *ioreq, struct VideoFormat *format, U32 unit );
S32 VideoTest_Preview( struct IOStdReq **ioreq, struct TimeRequest *timer,
	const struct VideoFormat *format, struct PreviewCameras *cameras, S32 trace );
S32 VideoTest_YUY2ToRGB( const U8 *frame, U32 size, U32 width, U32 height,
	U8 *rgb, U32 outWidth, U32 outHeight );

#endif
