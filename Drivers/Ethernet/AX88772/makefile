
#
#      -+- Universal serial bus -+-
# Copyright (c) 2012-2026 by Rene W. Olsen
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

###########################################################################

.PHONY: all clean install strip

###########################################################################

# Target binary and install path
TARGET		:= eth_AX88772.Driver
OBJDIR		:= obj
BINDIR		:= bin
INSTALLDIR	:= libs:usb2/
MAPFILE		:= $(TARGET).map

# Compiler flags
CFLAGS		:= -O2
CFLAGS		+= -I.
CFLAGS		+= -I../../../inc_Public -I../Common -I../Common/Wired
CFLAGS		+= -Iinc
CFLAGS		+= -Isrc
CFLAGS		+= -W
CFLAGS		+= -Wall
CFLAGS		+= -mcrt=newlib
CFLAGS		+= -MMD
CFLAGS		+= -MP
CFLAGS		+= -g

# Linker flags and libraries
LDFLAGS		:= -nostartfiles
LDFLAGS		+= -mcrt=newlib
LIBS		:=

# Host tools
HOSTOS		:= $(shell uname)

###########################################################################

# --

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
RM			:= rm
CC			:= ppc-amigaos-gcc
STRIP		:= ppc-amigaos-strip
MKDIR		:= mkdir -p
COPY		:= cp
SDK			:= /SDK/

endif

###########################################################################
#

SRCDIRS		:= src

SRCS		+= $(wildcard $(addsuffix /*.c, $(SRCDIRS)))
SRCS		:= $(sort $(SRCS))

OBJS		:= $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
DEPS		:= $(patsubst %.c,$(OBJDIR)/%.d,$(SRCS))

COMMON_SRCS	:= $(wildcard ../Common/EthClass_*.c ../Common/Sana2_*.c)
OBJS		+= $(patsubst ../Common/%.c,$(OBJDIR)/common/%.o,$(COMMON_SRCS))
DEPS		+= $(patsubst ../Common/%.c,$(OBJDIR)/common/%.d,$(COMMON_SRCS))

WIRED_SRCS	:= $(wildcard ../Common/Wired/*.c)
OBJS		+= $(patsubst ../Common/Wired/%.c,$(OBJDIR)/wired/%.o,$(WIRED_SRCS))
DEPS		+= $(patsubst ../Common/Wired/%.c,$(OBJDIR)/wired/%.d,$(WIRED_SRCS))

all: $(BINDIR)/$(TARGET)

###########################################################################

$(BINDIR)/$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(LDFLAGS) -o $(BINDIR)/$(TARGET) $(OBJS) $(LIBS) -Wl,--cref,-Map=$(MAPFILE)

$(BINDIR):
	$(MKDIR) $(BINDIR)

###########################################################################

# Include dependency files
-include $(DEPS)

###########################################################################

CLEANFILES	:= $(wildcard $(OBJDIR)/src/*.o $(OBJDIR)/src/*.d $(OBJDIR)/common/*.o $(OBJDIR)/common/*.d $(OBJDIR)/wired/*.o $(OBJDIR)/wired/*.d $(BINDIR)/$(TARGET) $(MAPFILE) usb2.map)

clean:
	$(if $(CLEANFILES),$(RM) $(CLEANFILES))

###########################################################################

install:
ifeq ($(HOSTOS),AmigaOS)
	$(COPY) $(BINDIR)/$(TARGET) eth_AX88772.Class $(INSTALLDIR)
else
	@echo "Install is AmigaOS-oriented. On Linux, copy manually to target media:"
	@echo "  $(COPY) $(BINDIR)/$(TARGET) eth_AX88772.Class <your-amiga-volume>/Libs/USB2/"
endif

###########################################################################

strip:
	@$(LS)		$(BINDIR)/$(TARGET)
	@$(STRIP)	$(BINDIR)/$(TARGET)
	@$(LS)		$(BINDIR)/$(TARGET)

###########################################################################

$(OBJDIR)/%.o: %.c makefile
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

###########################################################################

$(OBJDIR)/common/%.o: ../Common/%.c makefile
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


$(OBJDIR)/wired/%.o: ../Common/Wired/%.c makefile
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
