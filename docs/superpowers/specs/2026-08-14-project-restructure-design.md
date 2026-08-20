# Project Restructure Design

## Problem

The repository is functional but structurally hard to maintain:

- `CM7/Core/example` contains approximately 2,029 vendor/reference files and more than 500 MB that are not part of either firmware build.
- The same dual-core shared-memory headers are maintained separately under both `CM4/Core/Inc` and `CM7/Core/Inc`.
- CM4 protocol capture code is mixed into generated CubeMX source directories even though it is application/module code.
- Verification depends on manually rebuilding both cores and remembering artifact paths.
- Existing build caches recorded `cube-cmake`, a command not available in a normal terminal PATH.

## Goals

- Keep the working dual-core firmware behavior unchanged.
- Keep CubeMX and TouchGFX generation compatible by retaining the `CM4` and `CM7` project roots.
- Make the Git-tracked tree contain only project sources, generated inputs, documentation, and build configuration.
- Make cross-core contracts single-source.
- Give each CM4 protocol module a clear location and public interface.
- Provide one command that verifies shared contracts, both core builds, and both ELF artifacts.
- Complete the work on an isolated remote branch for review.

## Non-Goals

- No migration to another build system.
- No reorganization of the `.ioc` file or CubeMX-generated startup/peripheral files.
- No TouchGFX redesign.
- No protocol behavior changes or new protocol features.
- No deletion of local vendor reference material; it is only removed from Git tracking.

## Chosen Approach

Use a conservative CubeMX-compatible structure:

```text
CM4/
  Core/                  CubeMX-generated core and startup code
  Modules/
    Protocols/
      UART/
      SPI/
      I2C/
      CAN/
      Timing/
CM7/
  Core/                  CM7 system code and hardware adapters
  TouchGFX/              TouchGFX project and UI code
  FATFS/                 FatFs integration
Common/
  Inc/                   authoritative cross-core contracts
  Src/                   shared startup support
docs/
tools/
```

This preserves paths expected by the `.ioc`, linkers, TouchGFX Designer, and STM32CubeMX while separating application modules and cross-core contracts from generated code.

## Alternatives Considered

### Fully Generic Layout

Move everything into `applications`, `shared`, and `protocols`. This reads cleanly as a generic project, but it would require broad changes to CubeMX, TouchGFX, linker, and CMake paths for little runtime benefit. It also increases the chance that regeneration would overwrite or bypass the new layout.

### Git Cleanup Only

Remove the vendor examples and generated artifacts but leave duplicated shared headers and protocol modules in place. This is lower risk, but it does not solve contract drift or make module ownership clear.

## Verification

Every slice must leave both cores buildable. Final verification must:

1. Prove Git has exactly one copy of each cross-core shared header.
2. Configure and build CM4 with the STM32Cube toolchain.
3. Configure and build CM7 with the STM32Cube toolchain.
4. Confirm both `qiansai_CM4.elf` and `qiansai_CM7.elf` exist.
5. Review the branch diff and commit history before pushing.

Hardware behavior still requires on-target smoke testing after the branch is reviewed.
