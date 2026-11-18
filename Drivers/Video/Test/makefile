.PHONY: all clean

TARGET := VideoTest
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

$(TARGET): VideoTest.c ../inc/VideoDevice.h
	$(CC) $(CFLAGS) -o $@ VideoTest.c

clean:
	$(RM) $(TARGET)
