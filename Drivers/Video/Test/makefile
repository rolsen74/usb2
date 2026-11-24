.PHONY: all clean

TARGET := USB2VideoTest
HOSTOS := $(shell uname)
CFLAGS := -O2 -I../../../inc_Public -W -Wall -D__USE_INLINE__ -mcrt=newlib

ifeq ($(HOSTOS),AmigaOS)
CC := gcc
RM := delete
else ifeq ($(HOSTOS),Linux)
CC := ppc-amigaos-gcc
RM := rm -f
else
$(error Unknown host: $(HOSTOS))
endif

all: $(TARGET)

SOURCES := VideoTest.c Preview.c PreviewAudio.c PreviewAvi.c PreviewRecording.c YUY2.c ../../Audio/Test/Wav.c
HEADERS := Preview.h PreviewAudio.h PreviewAvi.h PreviewRecording.h ../inc/VideoDevice.h ../../Audio/inc/AudioDevice.h ../../Audio/Test/Wav.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

clean:
	$(RM) $(TARGET)
