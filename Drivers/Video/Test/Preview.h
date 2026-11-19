#ifndef USB2_VIDEO_TEST_PREVIEW_H
#define USB2_VIDEO_TEST_PREVIEW_H

S32 VideoTest_Preview( struct IOStdReq *ioreq, struct TimeRequest *timer,
	const struct VideoFormat *format );
S32 VideoTest_YUY2ToRGB( const U8 *frame, U32 size, U32 width, U32 height,
	U8 *rgb, U32 outWidth, U32 outHeight );

#endif
