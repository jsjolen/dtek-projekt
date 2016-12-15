# Copyright (c) 2015, Axel Isaksson

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:

# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# PIC32 device number
DEVICE		= 32MX320F128H

# UART settings for programmer
TTYDEV		?=/dev/ttyUSB0
TTYBAUD		?=115200

# Name of the project
PROGNAME	= outfile

# Linkscript
LINKSCRIPT	:= p$(shell echo "$(DEVICE)" | tr '[:upper:]' '[:lower:]').ld

# Compiler and linker flags
CFLAGS		+= -ffreestanding -march=mips32r2 -msoft-float -Wa,-msoft-float
ASFLAGS		+= -msoft-float
LDFLAGS		+= -T $(LINKSCRIPT)

# Filenames
ELFFILE		= $(PROGNAME).elf
HEXFILE		= $(PROGNAME).hex

# Find all source files automatically
CFILES          = $(wildcard *.c)
ASFILES         = $(wildcard *.S)
SYMSFILES	= $(wildcard *.syms)

# Object file names
OBJFILES        = $(CFILES:.c=.c.o)
OBJFILES        +=$(ASFILES:.S=.S.o)
OBJFILES	+=$(SYMSFILES:.syms=.syms.o)

# Hidden directory for dependency files
DEPDIR = .deps
df = $(DEPDIR)/$(*F)

.PHONY: all clean install envcheck
.SUFFIXES:

all: $(HEXFILE)

clean:
	$(RM) $(HEXFILE) $(ELFFILE) $(OBJFILES)
	$(RM) -R $(DEPDIR)

envcheck:
	@echo "$(TARGET)" | grep mcb32 > /dev/null || (\
		echo ""; \
		echo " **************************************************************"; \
		echo " * Make sure you have sourced the cross compiling environment *"; \
		echo " * Do this by issuing:                                        *"; \
		echo " * . /path/to/crosscompiler/environment                       *"; \
		echo " **************************************************************"; \
		echo ""; \
		exit 1)

install: envcheck
	$(TARGET)avrdude -v -p $(shell echo "$(DEVICE)" | tr '[:lower:]' '[:upper:]') -c stk500v2 -P "$(TTYDEV)" -b $(TTYBAUD) -U "flash:w:$(HEXFILE)"

$(ELFFILE): $(OBJFILES) envcheck
	$(CC) $(CFLAGS) -o $@ $(OBJFILES) $(LDFLAGS)

$(HEXFILE): $(ELFFILE) envcheck
	$(TARGET)bin2hex -a $(ELFFILE)

$(DEPDIR):
	@mkdir -p $@

# Compile C files
%.c.o: %.c envcheck | $(DEPDIR)
	$(CC) $(CFLAGS) -c -MD -o $@ $<
	@cp $*.c.d $(df).c.P; sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' < $*.c.d >> $(df).c.P; $(RM) $*.c.d

# Compile ASM files with C pre-processor directives
%.S.o: %.S envcheck | $(DEPDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) -c -MD -o $@ $<
	@cp $*.S.d $(df).S.P; sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' -e '/^$$/ d' -e 's/$$/ :/' < $*.S.d >> $(df).S.P; $(RM) $*.S.d

# Link symbol lists to object files
%.syms.o: %.syms
	$(LD) -o $@ -r --just-symbols=$<

# Check dependencies
-include $(CFILES:%.c=$(DEPDIR)/%.c.P)
-include $(ASFILES:%.S=$(DEPDIR)/%.S.P)
