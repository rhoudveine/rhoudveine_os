# System Programs Makefile
# Builds graviton (PID 1), rash (shell), and all utility programs.
# Mirrors the runtime directory structure:
#   System/Rhoudveine/Booter/graviton/ → /System/Rhoudveine/Booter/graviton/
#   System/Rhoudveine/Programs/        → /System/Rhoudveine/Programs/<name>/<name>

CC  := /home/linuxbrew/.linuxbrew/Cellar/x86_64-elf-gcc/15.2.0/bin/x86_64-elf-gcc
AS  := nasm
LD  := /home/linuxbrew/.linuxbrew/Cellar/x86_64-elf-binutils/2.46.0/bin/x86_64-elf-ld

CFLAGS := -I userlib -I ../kernel/src/impl/kernel/include \
          -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
          -O0 -mno-red-zone -fPIE -mcmodel=large
ASFLAGS := -f elf64

# Where the kernel dist puts the FAT32 image build inputs
DIST    := ../kernel/dist/x86_64

# crt0 object (entry point stub for all userspace ELFs)
CRT0    := build/userlib/crt0.o

.PHONY: all clean

# ── List of all programs to build ──────────────────────────────────────────
# Format: <source_path>:<output_path_in_dist>
# Single-file utilities go to Programs/<name>/<name>
# Multi-file programs have their own subdirectory source

GRAVITON_SRC := Rhoudveine/Booter/graviton/graviton.c
GRAVITON_OUT := $(DIST)/System/Rhoudveine/Booter/graviton/graviton

RASH_SRC     := Rhoudveine/Programs/rash/rash.c
RASH_OUT     := $(DIST)/System/Rhoudveine/Programs/rash/rash

# Single-file utilities: each .c in Programs/ becomes Programs/<name>/<name>
UTIL_SRCS := \
    Rhoudveine/Programs/clear.c \
    Rhoudveine/Programs/reset.c \
    Rhoudveine/Programs/acpi.c \
    Rhoudveine/Programs/safemode.c \
    Rhoudveine/Programs/sys.c \
    Rhoudveine/Programs/gravitonctl.c \
    Rhoudveine/Programs/gravitonadm.c \
    Rhoudveine/Programs/cdir.c \
    Rhoudveine/Programs/mount.c \
    Rhoudveine/Programs/cdl.c \
    Rhoudveine/Programs/uptime.c

# Derive output paths: Programs/foo.c → $(DIST)/System/Rhoudveine/Programs/foo/foo
UTIL_OUTS := $(foreach s,$(UTIL_SRCS),\
    $(DIST)/System/Rhoudveine/Programs/$(notdir $(basename $(s)))/$(notdir $(basename $(s))))

all: $(GRAVITON_OUT) $(RASH_OUT) $(UTIL_OUTS) copy_initrc

# ── graviton ───────────────────────────────────────────────────────────────
$(GRAVITON_OUT): $(CRT0) build/graviton.o
	@mkdir -p $(dir $@)
	@echo "  LD      $@"
	@$(LD) -o $@ -Ttext 0x2000000 --static $(CRT0) build/graviton.o

build/graviton.o: $(GRAVITON_SRC)
	@mkdir -p build
	@echo "  CC      $<"
	@$(CC) -c $(CFLAGS) $< -o $@

# ── rash ───────────────────────────────────────────────────────────────────
$(RASH_OUT): $(CRT0) build/rash.o
	@mkdir -p $(dir $@)
	@echo "  LD      $@"
	@$(LD) -o $@ -Ttext 0x2000000 --static $(CRT0) build/rash.o

build/rash.o: $(RASH_SRC)
	@mkdir -p build
	@echo "  CC      $<"
	@$(CC) -c $(CFLAGS) $< -o $@

# ── Single-file utilities ──────────────────────────────────────────────────
# Generic rule: Rhoudveine/Programs/<name>.c → Programs/<name>/<name>
define UTIL_RULE
$(DIST)/System/Rhoudveine/Programs/$(notdir $(basename $(1)))/$(notdir $(basename $(1))): \
    $(CRT0) build/$(notdir $(basename $(1))).o
	@mkdir -p $$(dir $$@)
	@echo "  LD      $$@"
	@$$(LD) -o $$@ -Ttext 0x2000000 --static $$(CRT0) build/$(notdir $(basename $(1))).o

build/$(notdir $(basename $(1))).o: $(1)
	@mkdir -p build
	@echo "  CC      $$<"
	@$$(CC) -c $$(CFLAGS) $$< -o $$@
endef

$(foreach src,$(UTIL_SRCS),$(eval $(call UTIL_RULE,$(src))))

# ── Copy initrc to dist ────────────────────────────────────────────────────
copy_initrc:
	@mkdir -p $(DIST)/System/Rhoudveine/Booter/graviton
	@cp Rhoudveine/Booter/graviton/initrc \
	    $(DIST)/System/Rhoudveine/Booter/graviton/initrc
	@echo "  CP      initrc"

# ── crt0 ──────────────────────────────────────────────────────────────────
build/userlib/%.o: userlib/%.asm
	@mkdir -p $(dir $@)
	@echo "  NASM    $<"
	@$(AS) $(ASFLAGS) $< -o $@

clean:
	@echo "  CLEAN   System"
	@rm -rf build
