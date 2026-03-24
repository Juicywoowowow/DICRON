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
#include "tests/ktest.h"
#include <dicron/io.h>
#include <dicron/log.h>
#include <dicron/panic.h>
#include <dicron/time.h>
#include <dicron/sched.h>
#include <dicron/syscall.h>
#include <dicron/process.h>
#include <dicron/fs.h>

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
	pit_init();
	syscall_init();
	syscall_table_init();
	vfs_init();
	devfs_init();
	ramfs_init();

	klog(KLOG_INFO, "PMM: %lu free / %lu total pages (%lu KiB free)\n",
	     pmm_free_pages_count(), pmm_total_pages_count(),
	     pmm_free_pages_count() * 4);

	/* boot test harness — run all tests before continuing */
	int failures = ktest_run_all();
	if (failures > 0)
		kpanic("boot tests failed (%d failures) — halting\n", failures);

	/* all tests passed — now start the scheduler */
	sched_init();

	/* post-boot tests — require live scheduler */
	int post_failures = ktest_run_post();
	if (post_failures > 0)
		kpanic("post-boot tests failed (%d failures) — halting\n", post_failures);

	/* Launch first user process */
	extern const uint8_t _binary_build_user_hello_musl_elf_start[];
	extern const uint8_t _binary_build_user_hello_musl_elf_end[];
	size_t user_elf_size = (size_t)(_binary_build_user_hello_musl_elf_end -
					_binary_build_user_hello_musl_elf_start);

	kio_printf("\nLaunching user process (hello.elf, %lu bytes)...\n", user_elf_size);
	struct process *init_proc = process_create(
		_binary_build_user_hello_musl_elf_start, user_elf_size);
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
