CC      := gcc
AS      := nasm
LD      := ld

BUILD_DIR := build
BIN_DIR   := bin

# ── Kconfig integration ──
# Include .config if it exists (CONFIG_xxx=y variables)
-include .config

CFLAGS  := -std=gnu11 -Wall -Wextra -Werror \
           -Wconversion -Wsign-conversion \
           -ffreestanding -fno-stack-protector -fno-pic -fno-pie \
           -mcmodel=kernel -mno-red-zone -mno-sse -mno-mmx -mno-sse2 \
           -I kernel/include -I kernel/include/generated -I kernel/src

LDFLAGS := -nostdlib -static -T kernel/linker.lds
ASFLAGS := -f elf64

# ── Core sources (always built) ──
CORE_SRCS := $(shell find kernel/src -maxdepth 1 -name '*.c')
CORE_SRCS += $(shell find kernel/src/arch -name '*.c')
ifdef CONFIG_FRAMEBUFFER
  CORE_SRCS += $(shell find kernel/src/console -name '*.c')
endif
CORE_SRCS += $(shell find kernel/src/fs -name '*.c')
CORE_SRCS += $(shell find kernel/src/io -name '*.c')
CORE_SRCS += $(shell find kernel/src/lib -name '*.c')
CORE_SRCS += $(shell find kernel/src/mm -name '*.c')
CORE_SRCS += $(shell find kernel/src/proc -name '*.c')
CORE_SRCS += $(shell find kernel/src/sched -name '*.c')
CORE_SRCS += $(shell find kernel/src/syscall -name '*.c')

A_SRCS  := $(shell find kernel/src -name '*.asm')

# ── Drivers (conditional) ──
# Driver registry is always built
CORE_SRCS += kernel/src/drivers/registry.c

ifdef CONFIG_SERIAL
  CORE_SRCS += $(wildcard kernel/src/drivers/serial/*.c)
endif
ifdef CONFIG_PS2_KBD
  CORE_SRCS += $(wildcard kernel/src/drivers/ps2/*.c)
endif
ifdef CONFIG_PIT
  CORE_SRCS += $(wildcard kernel/src/drivers/timer/*.c)
endif
ifdef CONFIG_RAMDISK
  CORE_SRCS += $(wildcard kernel/src/drivers/blkdev/*.c)
endif
ifdef CONFIG_EXT2
  CORE_SRCS += $(wildcard kernel/src/drivers/ext2/*.c)
endif

# ── Tests (conditional) ──
TEST_SRCS :=

ifdef CONFIG_TESTS
  # Always need the test runner
  TEST_SRCS += kernel/src/tests/ktest.c

ifdef CONFIG_TESTS_BOOT
  ifdef CONFIG_TESTS_PMM
    TEST_SRCS += $(wildcard kernel/src/tests/test_pmm_*.c)
  endif
  ifdef CONFIG_TESTS_VMM
    TEST_SRCS += $(wildcard kernel/src/tests/test_vmm_*.c)
  endif
  ifdef CONFIG_TESTS_KHEAP
    TEST_SRCS += $(wildcard kernel/src/tests/test_kheap_*.c)
  endif
  ifdef CONFIG_TESTS_SLAB
    TEST_SRCS += $(wildcard kernel/src/tests/test_slab_*.c)
  endif
  ifdef CONFIG_TESTS_SCHED
    TEST_SRCS += $(wildcard kernel/src/tests/test_sched_*.c)
  endif
  ifdef CONFIG_TESTS_MLFQ
    TEST_SRCS += $(wildcard kernel/src/tests/test_mlfq_*.c)
  endif
  ifdef CONFIG_TESTS_FS
    TEST_SRCS += $(wildcard kernel/src/tests/test_fs_*.c)
    TEST_SRCS += $(wildcard kernel/src/tests/test_ramfs_*.c)
    TEST_SRCS += $(wildcard kernel/src/tests/test_vfs_*.c)
  endif
  ifdef CONFIG_TESTS_CPIO
    TEST_SRCS += $(wildcard kernel/src/tests/test_cpio_*.c)
  endif
  ifdef CONFIG_TESTS_BLKDEV
    TEST_SRCS += $(wildcard kernel/src/tests/test_blkdev_*.c)
  endif
  ifdef CONFIG_TESTS_EXT2
    TEST_SRCS += kernel/src/tests/test_ext2_structs.c
  endif
  ifdef CONFIG_TESTS_EXT2_WRITE
    TEST_SRCS += kernel/src/tests/ext2_test_helper.c
    TEST_SRCS += kernel/src/tests/test_ext2_bitmap_alloc.c
    TEST_SRCS += kernel/src/tests/test_ext2_bitmap_free.c
    TEST_SRCS += kernel/src/tests/test_ext2_write_inode.c
    TEST_SRCS += kernel/src/tests/test_ext2_create_file.c
    TEST_SRCS += kernel/src/tests/test_ext2_mkdir.c
    TEST_SRCS += kernel/src/tests/test_ext2_file_write.c
    TEST_SRCS += kernel/src/tests/test_ext2_unlink.c
  endif
  ifdef CONFIG_TESTS_STRING
    TEST_SRCS += $(wildcard kernel/src/tests/test_string.c)
    TEST_SRCS += $(wildcard kernel/src/tests/test_math.c)
  endif
  ifdef CONFIG_TESTS_SPINLOCK
    TEST_SRCS += $(wildcard kernel/src/tests/test_spinlock.c)
  endif
  ifdef CONFIG_TESTS_IDT
    TEST_SRCS += $(wildcard kernel/src/tests/test_idt_*.c)
  endif
  ifdef CONFIG_TESTS_TIMER
    TEST_SRCS += $(wildcard kernel/src/tests/test_timer_*.c)
  endif
  ifdef CONFIG_TESTS_KSTACK
    TEST_SRCS += $(wildcard kernel/src/tests/test_kstack*.c)
  endif
  ifdef CONFIG_TESTS_ERROR
    TEST_SRCS += $(wildcard kernel/src/tests/test_error_*.c)
  endif
  ifdef CONFIG_TESTS_FUZZ
    TEST_SRCS += $(wildcard kernel/src/tests/test_fuzz.c)
  endif
endif # CONFIG_TESTS_BOOT

ifdef CONFIG_TESTS_POST
  ifdef CONFIG_TESTS_POST_SCHED
    TEST_SRCS += $(wildcard kernel/src/tests/post/test_post_*.c)
  endif
  ifdef CONFIG_TESTS_POST_SYSCALL
    TEST_SRCS += $(wildcard kernel/src/tests/post/test_syscall_*.c)
  endif
  ifdef CONFIG_TESTS_POST_ELF
    TEST_SRCS += $(wildcard kernel/src/tests/post/test_elf_*.c)
  endif
endif # CONFIG_TESTS_POST

endif # CONFIG_TESTS

# ── Combine all sources ──
C_SRCS  := $(CORE_SRCS) $(TEST_SRCS)

# ── User program ──
USER_ELF := $(BUILD_DIR)/user/hello_musl.elf
INITRD   := $(BUILD_DIR)/initrd.cpio

C_OBJS  := $(patsubst %.c, $(BUILD_DIR)/obj/%.o, $(C_SRCS))
A_OBJS  := $(patsubst %.asm, $(BUILD_DIR)/obj/%.o, $(A_SRCS))
DEPS    := $(patsubst %.c, $(BUILD_DIR)/d/%.d, $(C_SRCS))
OBJS    := $(C_OBJS) $(A_OBJS)

KERNEL  := $(BIN_DIR)/dicron
ISO     := $(BIN_DIR)/dicron.iso

RAM ?= 128

.PHONY: all iso run ranmem setram clean defconfig menuconfig oldconfig

all: $(KERNEL)

# ── Kconfig targets ──
defconfig:
	@python3 tools/genconfig.py defconfig

menuconfig:
	@python3 tools/genconfig.py menuconfig

oldconfig:
	@python3 tools/genconfig.py oldconfig

# ── Build user program ──
$(USER_ELF): user/hello_musl.c user/linker.lds
	@mkdir -p $(dir $@)
	@echo "  MUSL-CC $@"
	@musl-install/bin/musl-gcc -O2 -static -fno-pie -no-pie -Wl,-T,user/linker.lds -o $@ user/hello_musl.c

# Build initrd cpio archive with /init
$(INITRD): $(USER_ELF)
	@mkdir -p $(dir $@)
	@rm -rf $(BUILD_DIR)/initrd_root
	@mkdir -p $(BUILD_DIR)/initrd_root
	@cp $(USER_ELF) $(BUILD_DIR)/initrd_root/init
	@echo "  CPIO    $@"
	@cd $(BUILD_DIR)/initrd_root && find . | cpio -o -H newc --quiet > ../initrd.cpio

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

iso: $(KERNEL) $(INITRD)
	@echo "  LIM     limine bootloader setup"
	@mkdir -p iso_root/boot/limine
	@cp $(KERNEL) iso_root/boot/dicron
	@cp $(INITRD) iso_root/boot/initrd.cpio
	@cp limine.conf iso_root/boot/limine/
	@cp Limine/limine-bios.sys iso_root/boot/limine/
	@cp Limine/limine-bios-cd.bin iso_root/boot/limine/
	@echo "  ISO     $(ISO)"
	@xorriso -as mkisofs -R \
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
