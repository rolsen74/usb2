
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
TARGET		:= eth_RTL8188EU.Driver
OBJDIR		:= obj
BINDIR		:= bin
INSTALLDIR	:= libs:usb2/
MAPFILE		:= $(TARGET).map

# Compiler flags
CFLAGS		:= -O2
CFLAGS		+= -I.
CFLAGS		+= -I../../../inc_Public
CFLAGS		+= -Iinc
CFLAGS		+= -Isrc
CFLAGS		+= -I../Common -I../Common/Wireless
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

ETHCLASS_SRCS	:= $(wildcard ../Common/EthClass_*.c)
WIRELESS_SRCS	:= $(wildcard ../Common/Wireless/EthWireless_*.c ../Common/Wireless/Wireless_*.c)
SANA2_SRCS	:= $(wildcard ../Common/Sana2_*.c)

OBJS		+= $(patsubst ../Common/%.c,$(OBJDIR)/common/%.o,$(ETHCLASS_SRCS))
OBJS		+= $(patsubst ../Common/Wireless/%.c,$(OBJDIR)/wireless/%.o,$(WIRELESS_SRCS))
OBJS		+= $(patsubst ../Common/%.c,$(OBJDIR)/common/%.o,$(SANA2_SRCS))
DEPS		:= $(OBJS:.o=.d)

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

CLEANFILES	:= $(wildcard $(OBJDIR)/src/*.o $(OBJDIR)/src/*.d $(OBJDIR)/common/*.o $(OBJDIR)/common/*.d $(OBJDIR)/wireless/*.o $(OBJDIR)/wireless/*.d $(BINDIR)/$(TARGET) $(MAPFILE) usb2.map)

clean:
	$(if $(CLEANFILES),$(RM) $(CLEANFILES))

###########################################################################

install:
ifeq ($(HOSTOS),AmigaOS)
	$(COPY) $(BINDIR)/$(TARGET) eth_RTL8188EU.Class $(INSTALLDIR)
else
	@echo "Install is AmigaOS-oriented. On Linux, copy manually to target media:"
	@echo "  $(COPY) $(BINDIR)/$(TARGET) eth_RTL8188EU.Class <your-amiga-volume>/Libs/USB2/"
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


$(OBJDIR)/wireless/%.o: ../Common/Wireless/%.c makefile
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
