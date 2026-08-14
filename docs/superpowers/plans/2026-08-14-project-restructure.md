# Project Restructure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the dual-core firmware repository modular, reproducibly buildable, and independently verifiable without breaking CubeMX/TouchGFX generation.

**Architecture:** Keep the CubeMX-compatible `CM4` and `CM7` project roots. Move cross-core contracts into `Common`, organize CM4 protocol capture code into module directories, remove non-build reference code from Git tracking, and provide one PowerShell verification entry point that builds each core and checks firmware artifacts.

**Tech Stack:** STM32H747 dual-core firmware, CMake + Ninja, arm-none-eabi-gcc, TouchGFX 4.26.1, FatFs, FreeRTOS, Git.

**Spec:** `docs/superpowers/specs/2026-08-14-project-restructure-design.md`

## Global Constraints

- Preserve the current dual-core behavior; this is a structure and verification change only.
- Do not delete local reference code; remove only its Git tracking.
- Keep `.ioc`, CubeMX-generated files, linker scripts, and TouchGFX project roots in their expected locations.
- Every committed slice must build both `qiansai_CM4.elf` and `qiansai_CM7.elf`.
- Use the STM32Cube CMake/Ninja toolchain paths already present on this machine when regenerating caches.

---

### Task 1: Remove Reference Code From Git Tracking

**Files:**
- Modify: Git index only for `CM7/Core/example/**`
- Existing: `.gitignore`

**Interfaces:**
- Consumes: existing ignored path `CM7/Core/example/`
- Produces: a Git history that no longer carries the vendor example tree or its generated `.hex`/`.map` files

- [x] Run `git rm -r --cached -- CM7/Core/example`
- [x] Verify local files still exist and `git status` only reports deletions from Git tracking
- [x] Run the root aggregate build
- [x] Commit `chore: remove vendor examples from git tracking`

### Task 2: Consolidate Cross-Core Headers

**Files:**
- Create: `Common/Inc/shared_buf.h`, `Common/Inc/shared_config.h`
- Delete from Git: duplicate `CM4/Core/Inc/shared_buf.h`, `CM4/Core/Inc/shared_config.h`, `CM7/Core/Inc/shared_buf.h`, `CM7/Core/Inc/shared_config.h`
- Modify: `CM4/CMakeLists.txt`, `CM4/mx-generated.cmake`, `CM7/CMakeLists.txt`, `CM7/mx-generated.cmake` as needed

**Interfaces:**
- Consumes: the existing `shared_ring_t`, `shm_push`, `proto_config_t`, shared-memory addresses, and HSEM IDs
- Produces: one authoritative header set under `Common/Inc`

- [x] Compare both copies byte-for-byte before choosing the source
- [x] Move the headers with `git mv`
- [x] Add `${CMAKE_CURRENT_SOURCE_DIR}/../Common/Inc` to both core targets
- [x] Build CM4 and CM7 independently
- [x] Search the repository to prove no duplicate shared header remains in Git
- [ ] Commit `refactor: consolidate dual-core shared headers`

### Task 3: Organize CM4 Protocol Modules

**Files:**
- Create: `CM4/Modules/Protocols/UART`, `CM4/Modules/Protocols/SPI`, `CM4/Modules/Protocols/I2C`, `CM4/Modules/Protocols/CAN`
- Move: `CM4/Core/Src/my_uart_check.[ch]`, `my_spi_check.[ch]`, `my_i2c_check.[ch]`, `my_can_check.[ch]`, and `my_dwt_count.[ch]`
- Modify: `CM4/CMakeLists.txt`

**Interfaces:**
- Consumes: existing public functions used by `CM4/Core/Src/main.c`
- Produces: unchanged public APIs in protocol-specific directories

- [x] Move each protocol pair with `git mv`
- [x] Add the module sources and include directories to `CM4/CMakeLists.txt`
- [x] Build CM4
- [x] Inspect compile commands to prove each module source compiles from its new location
- [ ] Commit `refactor: organize cm4 protocol modules`

### Task 4: Add Module Verification

**Files:**
- Create: `tools/verify.ps1`
- Modify: `工程解析.md`

**Interfaces:**
- Produces: `tools/verify.ps1 [-Clean]`, which validates shared-header uniqueness, configures/builds CM4 and CM7, and checks both ELF artifacts

- [x] Implement tool discovery for STM32Cube CMake and Ninja
- [x] Implement CM4 and CM7 build commands
- [x] Implement artifact existence checks
- [x] Implement duplicate shared-header checks
- [x] Run `tools/verify.ps1 -Clean`
- [ ] Commit `build: add modular firmware verification`

### Task 5: Final Branch Delivery

**Files:**
- Modify: `HANDOFF.md`

**Interfaces:**
- Produces: a remote branch named `codex/restructure-project`

- [ ] Run the full verification script from a clean build
- [ ] Review `git status`, `git diff --stat`, and commit history
- [ ] Write the branch handoff
- [ ] Push `codex/restructure-project` to `origin`
