Kernel Logging (klog)
=====================

1. Overview
-----------

The kernel logging facility provides structured logging with different
severity levels for kernel debugging and diagnostics.

2. Log Levels
------------

  KLOG_DEBUG - Debug messages (verbose)
  KLOG_INFO  - Informational messages
  KLOG_WARN  - Warning messages (non-fatal issues)
  KLOG_ERR   - Error messages (significant problems)

3. API
-----

  void klog(int level, const char *fmt, ...)
      - Log a message with given level
      - Uses printf-style format string
      - Only logs if level >= current log level threshold

4. Implementation
-----------------

Messages are written to:
  - Serial port (for early boot)
  - Framebuffer console (if available)
  - Both simultaneously when possible

5. Usage
--------

klog is used throughout the kernel for:
  - Boot progress messages
  - Error reporting
  - Debugging information
  - Hardware detection status
