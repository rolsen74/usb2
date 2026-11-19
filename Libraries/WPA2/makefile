
###########################################################################

TARGET		:= usb2wpa2.library
OBJDIR		:= obj
BINDIR		:= bin

ifeq ($(shell uname),AmigaOS)
CC		:= gcc
STRIP		:= strip
else
CC		:= ppc-amigaos-gcc
STRIP		:= ppc-amigaos-strip
endif

CFLAGS		:= -O2 -Wall -Wextra -ffreestanding -MMD -MP -mcrt=newlib -D__USE_INLINE__ -I../../inc_Public
LDFLAGS		:= -nostartfiles -mcrt=newlib

SRCS		:= $(sort $(wildcard src/*.c))
OBJS		:= $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean strip

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: src/%.c makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)/$(TARGET)

strip: $(BINDIR)/$(TARGET)
	$(STRIP) $(BINDIR)/$(TARGET)

-include $(OBJS:.o=.d)
