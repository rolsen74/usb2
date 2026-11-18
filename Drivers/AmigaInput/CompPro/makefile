
###########################################################################

.PHONY: all clean strip

###########################################################################

ifneq ($(filter-out USB2,$(USBTYPE)),)
$(error Unsupported USBTYPE: $(USBTYPE). Only USB2 is available)
endif

# Target binary file
TARGET		:= CompPro.Driver

# Object Dir
OBJDIR		:= obj

# Binary Dir
BINDIR		:= bin

# Compiler Flags
CFLAGS		:= -O2
CFLAGS		+= -Iinc
CFLAGS		+= -MMD
CFLAGS		+= -MP
CFLAGS		+= -W
CFLAGS		+= -Wall
CFLAGS		+= -mcrt=newlib

# Linker Flags
LDFLAGS		:= -nostartfiles
LDFLAGS		+= -mcrt=newlib

# Host SYS we compile on
HOSTOS		:= $(shell uname)

###########################################################################
# Set Compiler

ifeq ($(HOSTOS),AmigaOS)

LS		:= list
RM		:= delete
CC		:= gcc
STRIP		:= strip
MKDIR		:= makedir FORCE

else ifeq ($(HOSTOS),Linux)

LS		:= ls -lort
RM		:= rm -rf
CC		:= ppc-amigaos-gcc
STRIP		:= ppc-amigaos-strip
MKDIR		:= mkdir -p

else

$(error Unknown Host SYS type: $(HOSTOS).)

endif

###########################################################################

# Build the USB2 entry point while the AmigaInput callbacks await integration.
SRCS		:= src/CompPro.c
OBJS		:= $(patsubst src/%.c,$(OBJDIR)/%.o,$(SRCS))
DEPS		:= $(OBJS:.o=.d)

all: $(BINDIR)/$(TARGET)

###########################################################################

$(BINDIR)/$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -Wl,--cref,-Map=$(TARGET).map

$(BINDIR) $(OBJDIR):
	$(MKDIR) $@

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

###########################################################################

-include $(DEPS)

###########################################################################

clean:
	$(RM) $(OBJS) $(DEPS) $(BINDIR)/$(TARGET) $(TARGET).map

###########################################################################

strip: all
	@$(LS)		$(BINDIR)/$(TARGET)
	@$(STRIP)	$(BINDIR)/$(TARGET)
	@$(LS)		$(BINDIR)/$(TARGET)

###########################################################################
