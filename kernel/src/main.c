#include "limine.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/idt.h"
#include "arch/x86_64/tss.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/kheap.h"
#include "console/console.h"
#include "drivers/serial/com.h"
#include "drivers/ps2/kbd.h"
#include "drivers/timer/pit.h"
#include "drivers/hpet/hpet.h"
#include "drivers/timer/rtc.h"
#ifdef CONFIG_PCI
#include "drivers/pci/pci.h"
#endif
#ifdef CONFIG_VIRTIO_BLK
#include "drivers/new/virtio/virtio_blk.h"
#endif
#ifdef CONFIG_PCSPEAKER
#include "drivers/timer/pcspeaker.h"
#endif
#ifdef CONFIG_ATA
#include "drivers/new/ata/ata.h"
#include <dicron/blkdev.h>
#ifdef CONFIG_EXT2
#include "drivers/ext2/ext2.h"
#endif
#endif
#ifdef CONFIG_SATA
#include "drivers/new/sata/sata.h"
#ifndef CONFIG_ATA
#include <dicron/blkdev.h>
#endif
#ifdef CONFIG_EXT2
#ifndef CONFIG_ATA
#include "drivers/ext2/ext2.h"
#endif
#endif
#endif
#ifdef CONFIG_SWAP
#include "mm/zram.h"
#include "mm/swap.h"
#endif
#include <dicron/io.h>
#include <dicron/log.h>
#include <dicron/panic.h>
#include <dicron/time.h>
#include <dicron/acpi.h>
#include <dicron/sched.h>
#include <dicron/syscall.h>
#include <dicron/process.h>
#include <dicron/fs.h>
#include <dicron/cpio.h>
#include <generated/autoconf.h>

#ifdef CONFIG_TESTS
#include "tests/ktest.h"
#endif
#ifdef CONFIG_QEMU_CI_EXIT
#include <dicron/qemu_exit.h>
#endif

/* Limine base revision */
__attribute__((used, section(".limine_requests")))
static volatile uint64_t base_revision[] = LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request fb_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
	.id = LIMINE_MODULE_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t requests_start[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t requests_end[] = LIMINE_REQUESTS_END_MARKER;

static void enable_sse(void)
{
	uint64_t cr0, cr4;
	
	__asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 &= ~(1ULL << 2); /* Clear EM */
	cr0 |= (1ULL << 1);  /* Set MP */
	__asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

	__asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= (1ULL << 9);  /* Set OSFXSR */
	cr4 |= (1ULL << 10); /* Set OSXMMEXCPT */
	__asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
}

void kmain(void)
{
	enable_sse();
	if (!LIMINE_BASE_REVISION_SUPPORTED(base_revision))
		kpanic("unsupported Limine base revision\n");

	gdt_init();
	tss_init();
	idt_init();

	if (!memmap_request.response || !hhdm_request.response)
		kpanic("Limine memory map or HHDM not available\n");

	pmm_init((struct limine_memmap_response *)memmap_request.response,
		 hhdm_request.response->offset);
	
	/* Enable SSE right away so user programs don't hit #UD */
	enable_sse();

	vmm_init(hhdm_request.response->offset);
	kheap_init();

	if (!fb_request.response || fb_request.response->framebuffer_count == 0) {
		klog(KLOG_WARN, "No framebuffer available — running headless (serial only)\n");
	} else {
		console_init(fb_request.response->framebuffers[0]);
	}

	serial_init();
	kbd_init();
	acpi_init(rsdp_request.response ? rsdp_request.response->address : NULL,
		  hhdm_request.response->offset);
	hpet_init();
	pit_init();
#ifdef CONFIG_PCSPEAKER
	pcspeaker_init();
#endif
	rtc_init();
#ifdef CONFIG_PCI
	pci_init();
#endif
#ifdef CONFIG_VIRTIO_BLK
	virtio_blk_init();
#endif
#ifdef CONFIG_ATA
	ata_init();
#endif
#ifdef CONFIG_SATA
	sata_init();
#endif
	syscall_init();
	syscall_table_init();
	vfs_init();
	devfs_init();
	ramfs_init();

#ifdef CONFIG_ATA
	/* If an ATA drive is present, scan for ext2 partitions and mount */
	if (ata_drive_count() > 0) {
		struct ata_drive *drv = ata_get_drive(0);
		struct blkdev *disk = ata_blkdev_create(drv);
		if (disk) {
#ifdef CONFIG_PARTITION_MBR
			struct partition_info parts[4];
			int nparts = partition_scan(disk, parts, 4);
			for (int i = 0; i < nparts; i++) {
				if (parts[i].type == MBR_PART_TYPE_LINUX) {
					struct blkdev *pdev =
						partition_blkdev_create(
							disk,
							parts[i].start_lba,
							parts[i].sector_count);
					if (!pdev)
						continue;
#ifdef CONFIG_EXT2
					struct ext2_fs *fs = ext2_mount(pdev);
					if (fs) {
						ext2_vfs_mount(fs, "/disk");
						klog(KLOG_INFO,
						     "ata: mounted ext2 "
						     "partition %d at "
						     "/disk\n", i);
					}
#endif
					break;
				}
			}
#endif /* CONFIG_PARTITION_MBR */
		}
	}
#endif /* CONFIG_ATA */

#ifdef CONFIG_SWAP
	/* Initialize swap subsystem: ZRAM always, disk swap only if ATA present */
	zram_init();
	{
		struct blkdev *swap_blkdev = NULL;
#ifdef CONFIG_ATA
		if (ata_drive_count() > 0) {
			struct ata_drive *drv = ata_get_drive(0);
			swap_blkdev = ata_blkdev_create(drv);
		}
#endif
#ifdef CONFIG_SATA
		if (!swap_blkdev && sata_drive_count() > 0)
			swap_blkdev = sata_blkdev_create(sata_get_drive(0));
#endif
		swap_init(swap_blkdev);
	}
#endif /* CONFIG_SWAP */

	klog(KLOG_INFO, "PMM: %lu free / %lu total pages (%lu KiB free)\n",
	     pmm_free_pages_count(), pmm_total_pages_count(),
	     pmm_free_pages_count() * 4);

#ifdef CONFIG_TESTS
	/* boot test harness — run all tests before continuing */
	int failures = ktest_run_all();
	if (failures > 0) {
#ifdef CONFIG_QEMU_CI_EXIT
		klog(KLOG_ERR, "boot tests failed (%d failures) — signalling QEMU exit\n", failures);
		qemu_exit(QEMU_EXIT_FAILURE_VAL);
#endif
		kpanic("boot tests failed (%d failures) — halting\n", failures);
	}
#endif

	/* all tests passed — now start the scheduler */
	sched_init();

#ifdef CONFIG_TESTS
	/* post-boot tests — require live scheduler */
	int post_failures = ktest_run_post();
	if (post_failures > 0) {
#ifdef CONFIG_QEMU_CI_EXIT
		klog(KLOG_ERR, "post-boot tests failed (%d failures) — signalling QEMU exit\n", post_failures);
		qemu_exit(QEMU_EXIT_FAILURE_VAL);
#endif
		kpanic("post-boot tests failed (%d failures) — halting\n", post_failures);
	}
#ifdef CONFIG_QEMU_CI_EXIT
	qemu_exit(QEMU_EXIT_SUCCESS_VAL);
#endif
#endif

	/* Extract initrd (cpio archive) from Limine module */
	if (!module_request.response || module_request.response->module_count == 0)
		kpanic("No initrd module provided by bootloader\n");

	struct limine_file *initrd = module_request.response->modules[0];
	klog(KLOG_INFO, "initrd: %lu bytes at %p\n", initrd->size, initrd->address);

	if (cpio_extract_all(initrd->address, initrd->size) != 0)
		kpanic("Failed to extract initrd cpio archive\n");

	/* Load /init from ramfs */
	struct inode *init_inode = vfs_namei("/init");
	if (!init_inode)
		kpanic("No /init found in initrd\n");
	if (!init_inode->f_op || !init_inode->f_op->read)
		kpanic("/init is not a readable file\n");

	/* Read the ELF into a buffer */
	size_t user_elf_size = init_inode->size;
	uint8_t *elf_buf = kmalloc(user_elf_size);
	if (!elf_buf)
		kpanic("Failed to allocate %lu bytes for /init\n", user_elf_size);

	struct file init_file = {
		.f_inode = init_inode,
		.f_op = init_inode->f_op,
		.f_pos = 0,
	};
	long nread = init_inode->f_op->read(&init_file, (char *)elf_buf, user_elf_size);
	if (nread < 0 || (size_t)nread != user_elf_size)
		kpanic("Failed to read /init (%ld bytes read)\n", nread);

	kio_printf("\nLaunching /init (%lu bytes)...\n", user_elf_size);
	struct process *init_proc = process_create(elf_buf, user_elf_size);
	if (!init_proc)
		kpanic("Failed to create init process\n");

	/* Spawn the process into the scheduler and yield to it.
	 * We call schedule() which will context-switch to the user thread.
	 * When the user process exits, we return here and idle. */
	mlfq_enqueue(init_proc->main_thread);

	kio_printf("DICRON ready. Waiting for interrupts...\n");

	/* Mark the boot context as the idle loop — keep yielding forever.
	 * This ensures the scheduler always has somewhere to return to. */
	for (;;) {
		schedule();
		__asm__ volatile("sti; hlt");
	}
}
