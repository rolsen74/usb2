
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
TARGET		:= usb2sis163u.device
OBJDIR		:= obj
BINDIR		:= bin
INSTALLDIR	:= devs:networks
MAPFILE		:= $(TARGET).map

# Compiler flags
CFLAGS		:= -O2
CFLAGS		+= -I.
CFLAGS		+= -I../../../inc_Public
CFLAGS		+= -Iinc
CFLAGS		+= -Isrc
CFLAGS		+= -W
CFLAGS		+= -Wall
CFLAGS		+= -D__USE_INLINE__
CFLAGS		+= -mcrt=newlib
CFLAGS		+= -MMD
CFLAGS		+= -MP

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

CLEANFILES	:= $(wildcard $(OBJDIR)/src/*.o $(OBJDIR)/src/*.d $(BINDIR)/$(TARGET) $(MAPFILE) usb2.map)

clean:
	$(if $(CLEANFILES),$(RM) $(CLEANFILES))

###########################################################################

install:
ifeq ($(HOSTOS),AmigaOS)
	$(COPY) $(BINDIR)/$(TARGET) $(INSTALLDIR)
else
	@echo "Install is AmigaOS-oriented. On Linux, copy manually to target media:"
	@echo "  $(COPY) $(BINDIR)/$(TARGET) <your-amiga-volume>/Devs/Networks/"
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
