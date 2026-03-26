# DQSF — Driver Quality SubFolder System

DICRON organizes drivers by maturity level using three folders. Every driver
starts in `staged/` or `new/` and graduates to the next tier through a formal
review process.

---

## Folder Hierarchy

```
kernel/src/drivers/
├── serial/          ← Stable drivers (production quality)
├── ps2/
├── timer/
├── blkdev/
├── ext2/
├── pci/
├── registry.c
├── new/             ← New drivers (functional, subject to change)
│   └── ata/
└── staged/          ← Staged drivers (experimental, stubbed, or buggy)
```

---

## Quality Tiers

### Stable (`kernel/src/drivers/`)

- **Production quality.** Well-tested, API-stable, reviewed.
- **Kconfig default:** Enabled (`default y`).
- Stable drivers must have comprehensive tests in `kernel/src/tests/`.
- Breaking changes require updating all callers and tests.

### New (`kernel/src/drivers/new/`)

- **Functional but young.** The driver works, but the API or internals may
  change as it matures.
- **Kconfig default:** User chooses (no default, or `default n`).
- New drivers should have at least basic tests covering core functionality.
- Code quality must still meet project standards (compiles clean with
  `-Wall -Wextra -Werror -Wconversion`).

### Staged (`kernel/src/drivers/staged/`)

- **Experimental.** Unstable, incomplete, stubbed out, or known-buggy.
- **Kconfig default:** Disabled (`default n`).
- Staged drivers may not compile cleanly — they are opt-in for developers
  who want to work on or test them.
- Can be individually enabled in `make menuconfig` for testing.

---

## Graduation Process

### Staged → New

Requirements:
1. Driver compiles cleanly with all warnings enabled.
2. Basic functionality works (can initialize, perform core operations).
3. At least minimal test coverage exists.
4. Has a Kconfig entry with a help description.

Steps:
1. Move the driver folder: `drivers/staged/<name>/` → `drivers/new/<name>/`
2. Update `Kconfig.drivers`: move the config entry to the "New Drivers" menu.
3. Update `GNUmakefile`: move the `ifdef` block to the new drivers section.
4. Update any `#include` paths that reference the old location.
5. Run `make oldconfig` to regenerate config files.
6. Verify all tests pass.

### New → Stable

Requirements:
1. Driver has been in `new/` for a reasonable period without breaking changes.
2. Comprehensive test coverage (unit tests + integration if applicable).
3. API is considered stable — no planned breaking changes.
4. Code has been reviewed for correctness, error handling, and style.
5. Documentation exists (at minimum, header comments on public functions).

Steps:
1. Move the driver folder: `drivers/new/<name>/` → `drivers/<name>/`
2. Update `Kconfig.drivers`: move the config entry to the main "Drivers" menu.
   Change default to `y` if appropriate.
3. Update `GNUmakefile`: move the `ifdef` block to the stable drivers section.
4. Update all `#include` paths (source files, tests, main.c).
5. Run `make oldconfig` to regenerate config files.
6. Verify all tests pass.

---

## Kconfig Organization

Driver configs in `kernel/Kconfig.drivers` are grouped by tier:

```kconfig
menu "Drivers"
    # Stable drivers here (serial, ps2, timer, ext2, ...)
endmenu

menu "New Drivers"
    # New drivers here (ata, ...)
endmenu

menu "Staged Drivers"
    # Staged/experimental drivers here
endmenu
```

Each driver gets its own `config` entry with:
- A `bool` type with descriptive prompt
- A `depends on` if it requires another driver
- A `help` section explaining what it does
- `default y` for stable, no default for new, `default n` for staged

---

## GNUmakefile Organization

Source inclusion is grouped by tier in the Makefile:

```makefile
# ── Stable Drivers ──
ifdef CONFIG_SERIAL
  CORE_SRCS += $(wildcard kernel/src/drivers/serial/*.c)
endif

# ── New Drivers ──
ifdef CONFIG_ATA
  CORE_SRCS += $(wildcard kernel/src/drivers/new/ata/*.c)
endif

# ── Staged Drivers ──
ifdef CONFIG_MY_STAGED_DRIVER
  CORE_SRCS += $(wildcard kernel/src/drivers/staged/mydriver/*.c)
endif
```

---

## Rules for New Driver Development

1. **Start in `staged/` or `new/`.** Never add untested code directly to
   `drivers/`.
2. **One driver per subfolder.** Each driver lives in its own directory
   (e.g., `drivers/new/ata/`).
3. **Include a header.** Every driver needs at least one `.h` file with
   its public API.
4. **Write tests.** Even staged drivers should have at least stub tests
   to verify compilation.
5. **Guard with Kconfig.** All driver code must be behind an
   `ifdef CONFIG_<NAME>` block.
6. **Use kernel APIs.** Use `kmalloc`/`kzalloc`/`kfree` for memory,
   `klog` for logging, `struct blkdev` for block devices.
7. **Return errno values.** Functions return `0` for success, negative
   errno (`-EINVAL`, `-ENOMEM`, `-EIO`) for failure.

---

## Demotion

If a stable or new driver becomes unmaintained, broken, or is superseded:

1. Move it down one tier (stable → new, or new → staged).
2. Update Kconfig defaults and Makefile paths.
3. Log the reason in the commit message.
4. If a staged driver is abandoned entirely, it can be removed.
