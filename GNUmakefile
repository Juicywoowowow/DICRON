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
ifndef CONFIG_EXT2_CHECK
  CORE_SRCS := $(filter-out kernel/src/drivers/ext2/ext2_check.c,$(CORE_SRCS))
endif
ifndef CONFIG_EXT2_SYNC
  CORE_SRCS := $(filter-out kernel/src/drivers/ext2/ext2_sync.c,$(CORE_SRCS))
endif
endif
# ── New Drivers ──
ifdef CONFIG_VIRTIO_BLK
  CORE_SRCS += $(wildcard kernel/src/drivers/new/virtio/*.c)
endif
ifdef CONFIG_HPET
  CORE_SRCS += $(wildcard kernel/src/drivers/hpet/*.c)
endif
ifdef CONFIG_ATA
  CORE_SRCS += $(wildcard kernel/src/drivers/new/ata/*.c)
endif
ifdef CONFIG_PCI
  CORE_SRCS += $(wildcard kernel/src/drivers/pci/*.c)
endif
ifdef CONFIG_AHCI
  CORE_SRCS += $(wildcard kernel/src/drivers/pci/ahci/*.c)
endif
ifdef CONFIG_SATA
  CORE_SRCS += $(wildcard kernel/src/drivers/new/sata/*.c)
endif

# ── Swap subsystem (conditionally exclude from wildcard mm/ pick-up) ──
ifndef CONFIG_SWAP
  CORE_SRCS := $(filter-out kernel/src/mm/lz4.c,$(CORE_SRCS))
  CORE_SRCS := $(filter-out kernel/src/mm/zram.c,$(CORE_SRCS))
  CORE_SRCS := $(filter-out kernel/src/mm/swap.c,$(CORE_SRCS))
  CORE_SRCS := $(filter-out kernel/src/mm/swap_reclaim.c,$(CORE_SRCS))
endif
ifdef CONFIG_SWAP
ifndef CONFIG_ZRAM
  CORE_SRCS := $(filter-out kernel/src/mm/zram.c,$(CORE_SRCS))
endif
endif

# ── Demand paging (conditionally exclude dpage.c) ──
ifndef CONFIG_DEMAND_PAGING
  CORE_SRCS := $(filter-out kernel/src/mm/dpage.c,$(CORE_SRCS))
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
  ifdef CONFIG_TESTS_HPET
    TEST_SRCS += $(wildcard kernel/src/tests/test_hpet_*.c)
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
  ifdef CONFIG_TESTS_PCI
    TEST_SRCS += $(wildcard kernel/src/tests/test_pci_*.c)
  endif
  ifdef CONFIG_TESTS_RTC
    TEST_SRCS += $(wildcard kernel/src/tests/test_rtc_*.c)
  endif
  ifdef CONFIG_TESTS_SWAP
    TEST_SRCS += $(wildcard kernel/src/tests/test_swap_*.c)
    TEST_SRCS += $(wildcard kernel/src/tests/test_zram_*.c)
  endif
  ifdef CONFIG_TESTS_ATA
    TEST_SRCS += $(wildcard kernel/src/tests/test_ata_*.c)
  endif
  ifdef CONFIG_TESTS_PARTITION
    TEST_SRCS += $(wildcard kernel/src/tests/test_partition_*.c)
  endif
  ifdef CONFIG_TESTS_PCSPEAKER
    TEST_SRCS += $(wildcard kernel/src/tests/test_pcspeaker*.c)
  endif
  ifdef CONFIG_TESTS_VIRTIO_BLK
    TEST_SRCS += $(wildcard kernel/src/tests/test_virtio_blk*.c)
  endif
  ifdef CONFIG_TESTS_AHCI
    TEST_SRCS += $(wildcard kernel/src/tests/test_ahci_*.c)
  endif
  ifdef CONFIG_TESTS_SATA
    TEST_SRCS += $(wildcard kernel/src/tests/test_sata_*.c)
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

# ── User program (Lua) ──
USER_ELF := $(BUILD_DIR)/user/lua.elf
INITRD   ?= $(BUILD_DIR)/initrd.cpio

C_OBJS  := $(patsubst %.c, $(BUILD_DIR)/obj/%.o, $(C_SRCS))
A_OBJS  := $(patsubst %.asm, $(BUILD_DIR)/obj/%.o, $(A_SRCS))
DEPS    := $(patsubst %.c, $(BUILD_DIR)/d/%.d, $(C_SRCS))
OBJS    := $(C_OBJS) $(A_OBJS)

KERNEL  := $(BIN_DIR)/dicron
ISO     := $(BIN_DIR)/dicron.iso

RAM ?= 128
TEST_ATA_IMG    := $(BUILD_DIR)/test-ata.img
TEST_VIRTIO_IMG := $(BUILD_DIR)/test-virtio.img
TEST_SATA_IMG   := $(BUILD_DIR)/test-sata.img

# If CONFIG_TEST_ATA_DRIVE is enabled, create a temp ATA drive and attach it
ifdef CONFIG_TEST_ATA_DRIVE
  QEMU_ATA_FLAGS := -boot d \
    -device piix3-ide,id=testide \
    -drive if=none,id=testata,file=$(TEST_ATA_IMG),format=raw \
    -device ide-hd,drive=testata,bus=testide.0
else
  QEMU_ATA_FLAGS :=
endif

# If CONFIG_TEST_VIRTIO_DRIVE is enabled, attach an 8 MB virtio-blk disk
ifdef CONFIG_TEST_VIRTIO_DRIVE
  QEMU_VIRTIO_FLAGS := \
    -drive if=none,id=testvirtio,file=$(TEST_VIRTIO_IMG),format=raw \
    -device virtio-blk-pci,drive=testvirtio
else
  QEMU_VIRTIO_FLAGS :=
endif

# If CONFIG_TEST_SATA_DRIVE is enabled, attach an 8 MB SATA disk on ICH9-AHCI
ifdef CONFIG_TEST_SATA_DRIVE
  QEMU_SATA_FLAGS := \
    -drive if=none,id=testsata,file=$(TEST_SATA_IMG),format=raw \
    -device ide-hd,drive=testsata,bus=ide.4,unit=0
else
  QEMU_SATA_FLAGS :=
endif

.PHONY: all iso run ci-run ranmem setram clean defconfig menuconfig oldconfig test-ata-img test-virtio-img test-sata-img

all: $(KERNEL)

# ── Kconfig targets ──
defconfig:
	@python3 tools/genconfig.py defconfig

menuconfig:
	@python3 tools/genconfig.py menuconfig

oldconfig:
	@python3 tools/genconfig.py oldconfig

# ── Build user program (Lua) ──
$(USER_ELF):
	@mkdir -p $(dir $@)
	@if [ -d "user/lua" ]; then \
		echo "  MUSL-CC $@ (Lua)"; \
		cd user/lua && make clean && make all CC="../../musl-install/bin/musl-gcc -O2 -static -fno-pie -no-pie -Wl,-T,../linker.lds" && cp lua ../../$@; \
	else \
		echo "  SKIP    user/lua not found. Bring your own INITRD or clone Lua."; \
	fi

# Build initrd cpio archive with /init
$(BUILD_DIR)/initrd.cpio: $(USER_ELF)
	@mkdir -p $(dir $@)
	@rm -rf $(BUILD_DIR)/initrd_root
	@mkdir -p $(BUILD_DIR)/initrd_root
	@if [ -f "$(USER_ELF)" ]; then cp $(USER_ELF) $(BUILD_DIR)/initrd_root/init; fi
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

# ── Test ATA drive image ──
test-ata-img:
ifdef CONFIG_TEST_ATA_DRIVE
	@mkdir -p $(BUILD_DIR)
	@bash tools/mk-test-ata.sh $(TEST_ATA_IMG) 32
endif

# ── Test virtio-blk drive image ──
test-virtio-img:
ifdef CONFIG_TEST_VIRTIO_DRIVE
	@mkdir -p $(BUILD_DIR)
	@bash tools/mk-test-virtio.sh $(TEST_VIRTIO_IMG) 8
endif

# ── Test SATA drive image ──
test-sata-img:
ifdef CONFIG_TEST_SATA_DRIVE
	@mkdir -p $(BUILD_DIR)
	@bash tools/mk-test-sata.sh $(TEST_SATA_IMG) 8
endif

QEMU_AUDIO_FLAGS := -audiodev wav,id=snd0,path=/dev/null -machine pcspk-audiodev=snd0
ifdef CI
  QEMU_DISPLAY_FLAGS := -display none
else
  QEMU_DISPLAY_FLAGS :=
endif

# ── CI exit device ──
# Attached only by the ci-run target; invisible to normal 'make run'.
QEMU_CI_FLAGS := -device isa-debug-exit,iobase=0xf4,iosize=0x04

run: iso test-ata-img test-virtio-img test-sata-img
	qemu-system-x86_64 \
		-M q35 \
		-cdrom $(ISO) \
		$(QEMU_ATA_FLAGS) \
		$(QEMU_VIRTIO_FLAGS) \
		$(QEMU_SATA_FLAGS) \
		$(QEMU_AUDIO_FLAGS) \
		-serial stdio \
    $(QEMU_DISPLAY_FLAGS) \
		-no-reboot \
		-d int,cpu_reset -D qemu.log
ifdef CONFIG_TEST_ATA_DRIVE
	@rm -f $(TEST_ATA_IMG)
	@echo "  ATA-IMG $(TEST_ATA_IMG) cleaned up"
endif
ifdef CONFIG_TEST_VIRTIO_DRIVE
	@rm -f $(TEST_VIRTIO_IMG)
	@echo "  VIO-IMG $(TEST_VIRTIO_IMG) cleaned up"
endif
ifdef CONFIG_TEST_SATA_DRIVE
	@rm -f $(TEST_SATA_IMG)
	@echo "  SATA-IMG $(TEST_SATA_IMG) cleaned up"
endif

# ── CI run: inject isa-debug-exit + -DCONFIG_QEMU_CI_EXIT, then check exit 33 ──
# QEMU writes (value<<1)|1 on port 0xf4.  We write 0x10 → exit 33 = success.
ci-run: CFLAGS += -DCONFIG_QEMU_CI_EXIT
ci-run: iso test-ata-img test-virtio-img test-sata-img
	qemu-system-x86_64 \
		-M q35 \
		-cdrom $(ISO) \
		$(QEMU_ATA_FLAGS) \
		$(QEMU_VIRTIO_FLAGS) \
		$(QEMU_SATA_FLAGS) \
		$(QEMU_AUDIO_FLAGS) \
		$(QEMU_CI_FLAGS) \
		-serial stdio \
		-display none \
		-no-reboot \
		-d int,cpu_reset -D qemu.log; \
		exit_code=$$?; \
		if [ $$exit_code -eq 33 ]; then \
			echo "  CI      PASS (QEMU exit $$exit_code)"; \
		else \
			echo "  CI      FAIL (QEMU exit $$exit_code, expected 33)"; \
			exit 1; \
		fi
ifdef CONFIG_TEST_ATA_DRIVE
	@rm -f $(TEST_ATA_IMG)
endif
ifdef CONFIG_TEST_VIRTIO_DRIVE
	@rm -f $(TEST_VIRTIO_IMG)
endif
ifdef CONFIG_TEST_SATA_DRIVE
	@rm -f $(TEST_SATA_IMG)
endif

# Run with a specific amount of RAM (in MB): make setram RAM=64
setram: iso test-ata-img test-virtio-img test-sata-img
	@echo "=== Running with $(RAM) MB RAM ==="
	qemu-system-x86_64 \
		-M q35 \
		-m $(RAM) \
		-cdrom $(ISO) \
		$(QEMU_ATA_FLAGS) \
		$(QEMU_VIRTIO_FLAGS) \
		$(QEMU_SATA_FLAGS) \
		-serial stdio \
    $(QEMU_DISPLAY_FLAGS) \
		-no-reboot \
		-d int,cpu_reset -D qemu.log
ifdef CONFIG_TEST_ATA_DRIVE
	@rm -f $(TEST_ATA_IMG)
	@echo "  ATA-IMG $(TEST_ATA_IMG) cleaned up"
endif
ifdef CONFIG_TEST_VIRTIO_DRIVE
	@rm -f $(TEST_VIRTIO_IMG)
	@echo "  VIO-IMG $(TEST_VIRTIO_IMG) cleaned up"
endif
ifdef CONFIG_TEST_SATA_DRIVE
	@rm -f $(TEST_SATA_IMG)
	@echo "  SATA-IMG $(TEST_SATA_IMG) cleaned up"
endif

# Run with random RAM between 1 MB and 5120 MB
ranmem: iso test-ata-img test-virtio-img test-sata-img
	$(eval RAND_RAM := $(shell shuf -i 1-5120 -n 1))
	@echo "=== Running with $(RAND_RAM) MB RAM (random) ==="
	qemu-system-x86_64 \
		-M q35 \
		-m $(RAND_RAM) \
		-cdrom $(ISO) \
		$(QEMU_ATA_FLAGS) \
		$(QEMU_VIRTIO_FLAGS) \
		$(QEMU_SATA_FLAGS) \
		-serial stdio \
    $(QEMU_DISPLAY_FLAGS) \
		-no-reboot \
		-d int,cpu_reset -D qemu.log
ifdef CONFIG_TEST_ATA_DRIVE
	@rm -f $(TEST_ATA_IMG)
	@echo "  ATA-IMG $(TEST_ATA_IMG) cleaned up"
endif
ifdef CONFIG_TEST_VIRTIO_DRIVE
	@rm -f $(TEST_VIRTIO_IMG)
	@echo "  VIO-IMG $(TEST_VIRTIO_IMG) cleaned up"
endif
ifdef CONFIG_TEST_SATA_DRIVE
	@rm -f $(TEST_SATA_IMG)
	@echo "  SATA-IMG $(TEST_SATA_IMG) cleaned up"
endif

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) iso_root qemu.log $(TEST_ATA_IMG) $(TEST_VIRTIO_IMG) $(TEST_SATA_IMG)

-include $(DEPS)
