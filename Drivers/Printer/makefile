
###########################################################################

.PHONY: all clean revision install makedirs

###########################################################################
#
# Compile info:
#
# Compile : make
#

ifneq ($(filter-out USB2,$(USBTYPE)),)
$(error Unsupported USBTYPE: $(USBTYPE). Only USB2 is available)
endif

VERSION		:= 1
REVISION	:= 1

# Object Dir
OBJDIR		:= obj

# Binary Dir
BINDIR		:= bin

# Install Dir
INSTALLDIR	:= libs:usb2/

# Compiler Flags
CFLAGS		:= -O2
CFLAGS		+= -I.
CFLAGS		+= -I../../inc_Public
CFLAGS		+= -Iinc
#CFLAGS		+= -gstabs
CFLAGS		+= -W
CFLAGS		+= -Wall
CFLAGS		+= -MMD
CFLAGS		+= -MP
CFLAGS		+= -mcrt=newlib

# Print Error Messages on serial
#CFLAGS		+= -DDO_ERROR

# Print Info Messages on serial
#CFLAGS		+= -DDO_INFO

# Print Debug Messages on serial
#CFLAGS		+= -DDO_DEBUG

# Linker Flags
LDFLAGS		:= -mcrt=newlib

# Link Libraries
LIBS		:=

# Empty Source File List (Will grow)
SRCS		:=

# Host SYS we compile on
HOSTOS		:= $(shell uname)

###########################################################################

# --

ifeq ($(HOSTOS),AmigaOS)

LS			:= list
RM			:= delete
RMDIR		:= rm -rf
MKDIR		:= makedir ALL

# --

else ifeq ($(HOSTOS),Linux)

LS			:= ls -lort
RM			:= rm
RMDIR		:= rm -rf
MKDIR		:= mkdir -p

# --

else

$(error Unknown Host SYS type: $(HOSTOS).)

endif

###########################################################################
# USB2 printer class driver

SRCS		+= src/Printer.c
SRCS		+= src/Resources_Init.c
SRCS		+= src/Resources_Free.c
SRCS		+= src/Process_Setup_Device.c
SRCS		+= src/Process_Main.c
SRCS		+= src/Printer_Abort.c
SRCS		+= src/FormFeed.c
SRCS		+= src/Buffer_Schedule.c
SRCS		+= src/Direct_Schedule.c
SRCS		+= src/Dot4.c

CFLAGS		+= -Isrc -D__USE_INLINE__
LDFLAGS	:= -nostartfiles -mcrt=newlib
TARGET		:= cls_Printer.Driver

###########################################################################
# Set Compiler

ifeq ($(HOSTOS),AmigaOS)

LS			:= list
RM			:= delete
CC			:= gcc
STRIP		:= strip
MKDIR		:= makedir FORCE
COPY		:= copy
SDK			:= sdk:

# --

else ifeq ($(HOSTOS),Linux)

LS			:= ls -lort
RM			:= rm -rf
CC			:= ppc-amigaos-gcc
STRIP		:= ppc-amigaos-strip
MKDIR		:= mkdir -p
COPY		:= cp
SDK			:= /SDK/

endif

###########################################################################
# 

SRCS		+= src/Debug.c
SRCS		+= src/Driver_Library.c

SRCS		+= src/Process.c


SRCS		+= src/Buffer_Flush.c
SRCS		+= src/Buffer_Write.c

SRCS		+= src/_cmd_0003_Write_Buffer.c
SRCS		+= src/_cmd_0003_Write_Direct.c

###########################################################################

OBJS		:= $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
DEPS		:= $(patsubst %.c,$(OBJDIR)/%.d,$(SRCS))

all: $(BINDIR)/$(TARGET)

###############################################################################

$(BINDIR)/$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(LDFLAGS) -o $(BINDIR)/$(TARGET) $(OBJS) $(LIBS) -Wl,--cref,-M,-Map=usb2.map

$(BINDIR):
	$(MKDIR) $(BINDIR)

###############################################################################

# Include dependency files
-include $(DEPS)

###############################################################################

clean:
	$(RM) $(OBJS) $(DEPS) $(BINDIR)/$(TARGET) usb2.map

###############################################################################

#revision:
#	bumprev -v $(VERSION) -r $(REVISION) -n $(TARGET) -b $(TARGET)

###############################################################################

install:
	$(COPY) $(BINDIR)/$(TARGET) cls_Printer.Class $(INSTALLDIR)

###############################################################################

#installsdk:
#	$(MKDIR) $(SDK)local/common/include/devices
#	$(MKDIR) $(SDK)local/common/include/inline4
#	$(MKDIR) $(SDK)local/common/include/interfaces
#	$(MKDIR) $(SDK)local/common/include/proto
#	$(COPY) include/devices/usb2.h		$(SDK)local/common/include/devices
#	$(COPY) include/inline4/usb2.h		$(SDK)local/common/include/inline4
#	$(COPY) include/interfaces/usb2.h	$(SDK)local/common/include/interfaces
#	$(COPY) include/proto/usb2.h		$(SDK)local/common/include/proto

###############################################################################

#includes:
#	idltool -i -n -c usb2.xml

###############################################################################

strip:
	@$(LS)		$(BINDIR)/$(TARGET)
	@$(STRIP)	$(BINDIR)/$(TARGET)
	@$(LS)		$(BINDIR)/$(TARGET)

###############################################################################

$(OBJDIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

###########################################################################
