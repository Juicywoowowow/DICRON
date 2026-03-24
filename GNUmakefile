CC      := gcc
AS      := nasm
LD      := ld

CFLAGS  := -std=gnu11 -Wall -Wextra -Werror \
           -Wconversion -Wsign-conversion \
           -ffreestanding -fno-stack-protector -fno-pic -fno-pie \
           -mcmodel=kernel -mno-red-zone -mno-sse -mno-mmx -mno-sse2 \
           -I kernel/include -I kernel/src

LDFLAGS := -nostdlib -static -T kernel/linker.lds
ASFLAGS := -f elf64

# --- sources ---
C_SRCS  := $(shell find kernel/src -name '*.c')
A_SRCS  := $(shell find kernel/src -name '*.asm')

BUILD_DIR := build
BIN_DIR   := bin

# --- user program ---
USER_ELF := $(BUILD_DIR)/user/hello_musl.elf
USER_OBJ := $(BUILD_DIR)/obj/user_hello.o

C_OBJS  := $(patsubst %.c, $(BUILD_DIR)/obj/%.o, $(C_SRCS))
A_OBJS  := $(patsubst %.asm, $(BUILD_DIR)/obj/%.o, $(A_SRCS))
DEPS    := $(patsubst %.c, $(BUILD_DIR)/d/%.d, $(C_SRCS))
OBJS    := $(C_OBJS) $(A_OBJS) $(USER_OBJ)

KERNEL  := $(BIN_DIR)/dicron
ISO     := $(BIN_DIR)/dicron.iso

RAM ?= 128

.PHONY: all iso run ranmem setram clean

all: $(KERNEL)

# Build user program using musl cross-compiler script
$(USER_ELF): user/hello_musl.c user/linker.lds
	@mkdir -p $(dir $@)
	@echo "  MUSL-CC $@"
	@musl-install/bin/musl-gcc -O2 -static -fno-pie -no-pie -Wl,-T,user/linker.lds -o $@ user/hello_musl.c

# Embed user ELF as a binary blob in a linkable .o
$(USER_OBJ): $(USER_ELF)
	@mkdir -p $(dir $@)
	@echo "  OBJCOPY $@"
	@objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
		--rename-section .data=.rodata,alloc,load,readonly,data,contents \
		$< $@

$(KERNEL): $(OBJS)
	@mkdir -p $(BIN_DIR)
	@echo "  LD      $@"
	@$(LD) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/obj/%.o: %.c
	@mkdir -p $(dir $@) $(dir $(BUILD_DIR)/d/$*.d)
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -MMD -MP -MF $(BUILD_DIR)/d/$*.d -c -o $@ $<

$(BUILD_DIR)/obj/%.o: %.asm
	@mkdir -p $(dir $@)
	@echo "  AS      $@"
	@$(AS) $(ASFLAGS) -o $@ $<

iso: $(KERNEL)
	@echo "  LIM     limine bootloader setup"
	@mkdir -p iso_root/boot/limine
	@cp $(KERNEL) iso_root/boot/dicron
	@cp limine.conf iso_root/boot/limine/
	@cp Limine/limine-bios.sys iso_root/boot/limine/
	@cp Limine/limine-bios-cd.bin iso_root/boot/limine/
	@echo "  ISO     $(ISO)"
	@xorriso -as mkisofs \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--protective-msdos-label \
		iso_root -o $(ISO) > /dev/null 2>&1
	@rm -rf iso_root

run: iso
	qemu-system-x86_64 \
		-M q35 \
		-cdrom $(ISO) \
		-serial stdio \
		-no-reboot \
		-d int,cpu_reset -D qemu.log
# Run with a specific amount of RAM (in MB): make setram RAM=64
setram: iso
	@echo "=== Running with $(RAM) MB RAM ==="
	qemu-system-x86_64 \
		-M q35 \
		-m $(RAM) \
		-cdrom $(ISO) \
		-serial stdio \
		-no-reboot \
		-d int,cpu_reset -D qemu.log

# Run with random RAM between 1 MB and 5120 MB
ranmem: iso
	$(eval RAND_RAM := $(shell shuf -i 1-5120 -n 1))
	@echo "=== Running with $(RAND_RAM) MB RAM (random) ==="
	qemu-system-x86_64 \
		-M q35 \
		-m $(RAND_RAM) \
		-cdrom $(ISO) \
		-serial stdio \
		-no-reboot \
		-d int,cpu_reset -D qemu.log

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) iso_root qemu.log

-include $(DEPS)
