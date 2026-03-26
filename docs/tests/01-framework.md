Test Framework
==============

1. Overview
-----------

DICRON includes a comprehensive test framework for kernel components.
Tests verify correctness of subsystems before and after the scheduler
is operational.

2. Test Configuration
---------------------

Tests are enabled via CONFIG_TESTS in the kernel configuration.
When disabled, test code is not compiled.

3. Test Types
------------

Boot Tests:
  - Run before scheduler initialization
  - Test basic kernel functionality
  - Use simple assertion-based framework

Post Tests:
  - Run after scheduler starts
  - Test concurrency and scheduling
  - Test syscalls and process management

4. Test Framework API
---------------------

  void ktest_init(void)
      - Initialize test harness

  int ktest_run_all(void)
      - Run all boot tests
      - Returns number of failures

  int ktest_run_post(void)
      - Run all post tests
      - Returns number of failures

  #define KTEST_ASSERT(expr)
      - Assert expression is true
      - Prints failure message with location

5. Test Files
------------

Boot tests in src/tests/:
  - test_pmm_*.c - Physical memory manager tests
  - test_vmm_*.c - Virtual memory manager tests
  - test_slab_*.c - Slab allocator tests
  - test_kheap_*.c - Kernel heap tests
  - test_fs_*.c - Filesystem tests
  - test_ata_*.c - ATA driver tests
  - test_ext2_*.c - EXT2 filesystem tests

Post tests in src/tests/post/:
  - test_post_*.c - Scheduler-dependent tests
  - test_syscall_*.c - Syscall tests
  - test_elf_*.c - ELF loader tests

6. Running Tests
----------------

Tests run automatically during boot if CONFIG_TESTS is enabled:
  - Boot tests run before scheduler
  - Post tests run after scheduler init
  - Any test failure triggers kernel panic

7. Test Utilities
----------------

Helper functions:
  - test_assert() - Basic assertion
  - test_strcmp() - String comparison for tests
  - test_memcmp() - Memory comparison for tests
