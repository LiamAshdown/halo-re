# `cseries` — Blam Win32 support library

Retail Halo PC `halo.exe` 1.0.10, `0x4491e0 .. 0x44971e` (9 functions), plain C / MSVC 7.1
(cl 13.10.3077, LTCG) / x86. Every file in this directory is one function, rewritten from its
Ghidra decompilation against `types/cseries.h`. The original decompile, and for most files the
objdump listing, is kept at the bottom of each file inside `#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py cseries` → **9 ok, 0 failed**, with no `-Wall` warnings.
`out/phase4/cseries_smoke.c` also passes.

## What the module contains

These are small, leaf-level wrappers around the Win32 API and the CRT. No function here touches
tags, game state or datum handles.

| Family | Range | What it is |
|---|---|---|
| strings | `0x4491e0` | in-place `tolower` over a C string |
| time | `0x449210` | millisecond clock: `QueryPerformanceCounter * 1000 / frequency` |
| filesystem | `0x449250`, `0x449390` | recursive `CreateDirectoryA`; profile directory setup (`-path`, else `<My Documents>\My Games\Halo`) |
| memory | `0x449370`, `0x449380` | `GlobalAlloc(GMEM_FIXED)` / `GlobalFree` wrappers |
| logging | `0x449450` | `debug.txt` appender with a one-time banner block and an optional timestamp |
| sort | `0x449590`, `0x4496d0` | non-recursive CRT-style quicksort over 4-byte elements with a byte-returning comparator, plus its selection-sort fallback for partitions of 8 or fewer elements |

The two sort functions were first grouped into the cutscene batch. They belong here, and
`src/cutscene/README.md` already marks them as misattributed. No function in this range is linked
CRT or library code: the real CRT `_qsort` is elsewhere in the image.

## Register conventions (LTCG, confirmed at the prologue and the call sites)

| Function | Arguments | Return |
|---|---|---|
| `string_to_lowercase` | EDI = `char *string` | EAX = `string` (callers ignore it) |
| `time_query_performance_counter_ms` | none | EDX:EAX from `__alldiv`; all 52 callers read only EAX |
| `directory_create_recursive` | `__cdecl (char *path)` | AL = 0/1 |
| `memory_global_alloc` | EAX = size | EAX = handle |
| `memory_global_free` | EAX = handle | EAX = `GlobalFree` result |
| `profile_path_initialize` | none | void |
| `write_to_error_file` | `__cdecl (char *message, uint8_t with_timestamp)` | void |
| `qsort_dword_array` | EAX = count, ECX = `int32_t *elements`, stack = comparator (caller pops 4) | void |
| `qsort_dword_array_shortsort` | EAX = `int32_t *last` (inclusive), stack = `first`, comparator (caller pops 8) | void |

## Struct layouts

The module owns no structs. No function reaches a field at a constant offset through a pointer.
The one exception is the CRT `struct tm` returned by `_localtime` in `write_to_error_file`. It uses
the stock MSVC layout and is not redeclared:

### `struct tm` (CRT, read-only here)

| Off | Field | Use |
|---|---|---|
| `0x00` | `tm_sec` | seconds |
| `0x04` | `tm_min` | minutes |
| `0x08` | `tm_hour` | hours |
| `0x0c` | `tm_mday` | day |
| `0x10` | `tm_mon` | printed as `+ 1` |
| `0x14` | `tm_year` | printed as `% 100` |

### `qsort_dword_compare_proc`

| Item | Value |
|---|---|
| signature | `uint8_t (*)(int32_t element, int32_t other)`, `__cdecl`, caller pops 8 |
| meaning | nonzero when `element` sorts after, or level with, `other` |
| known instances | `0x552c00` (signed `a >= b`); `0x4127b0` (indices into the 0x3c-stride table at `0x006f0c94`, ordered by byte `+0x30`) |

### Globals (one row per global)

| Address | Declaration | Owner | Notes |
|---|---|---|---|
| `0x006ac8f8` | `int64_t` / `large_integer performance_frequency` | cseries | written once at `0x540eec`; about 40 other functions inline a read of it |
| `0x006ac900` | `char profile_directory[0x105]` | cseries | shell zeroes 0x105 bytes at `0x540ef9`; all writes are bounded to 0x104 |
| `0x00686b48` | `uint8_t error_file_needs_header` | cseries | image value 1; cleared on the first logged message |
| `0x0087ac06` | `uint8_t debug_log_level` | shell | every access is byte wide; logging needs 2 or more |
| `0x0087ac01` | `uint8_t error_file_enabled` | shell | set to 1 at `0x540fb2` |
| `0x0074626c` | `void *sh_get_folder_path` | shell | `SHGetFolderPathA`, `__stdcall` |

## Functions

| Address | Name | Size | Name conf | Rewrite conf | Notes |
|---|---|---|---|---|---|
| `0x4491e0` | `string_to_lowercase` | 36 | 0.6 | 0.85 | returns `char *`; the two foreign externs declare `void` |
| `0x449210` | `time_query_performance_counter_ms` | 59 | 0.8 | 0.85 | renamed from `FUN_00449210` (`symbols/agent_phase4_cseries.txt`) |
| `0x449250` | `directory_create_recursive` | 278 | 0.8 | 0.85 | skips `SetErrorMode` restore when the path already exists |
| `0x449370` | `memory_global_alloc` | 10 | 0.5 | 0.9 | |
| `0x449380` | `memory_global_free` | 8 | 0.5 | 0.9 | |
| `0x449390` | `profile_path_initialize` | 185 | 0.6 | 0.8 | prints `profile_directory` before it is filled (always empty) |
| `0x449450` | `write_to_error_file` | 314 | 0.85 | 0.8 | self-recursive banner; `fopen(network_log_path_resolve(ESI="debug.txt"), "a+b")` |
| `0x449590` | `qsort_dword_array` | 302 | 0.85 | 0.9 | explicit 30-deep partition stack |
| `0x4496d0` | `qsort_dword_array_shortsort` | 78 | 0.85 | 0.9 | max-selection sort |

## Known gaps and cross-module inconsistencies

These are not fixed here because the files belong to other modules:

- **`0x449210` naming.** 30 foreign files still declare `FUN_00449210`, as `int32_t`, `uint32_t` or
  even `void`. Five foreign files already use `time_query_performance_counter_ms` (`uint32_t`). A mechanical rename
  pass should apply the new name.
- **`0x0087ac06` type.** `types/interface.h` has `int32_t console_verbosity` and `types/networking.h` has
  `int16_t network_statistics_level`. Every access in the image is a byte, so both should become
  `uint8_t` under one name.
- **`profile_directory` size.** `types/cache.h`, `src/cache/cache_file_slot_read_header.c` and
  `src/main/main_loop.c` declare `[0x104]`. The storage the shell zeroes is 0x105 bytes. This only
  changes the extern array bound, not the layout.
- **Foreign prototypes.** `src/ai/actor_find_best_firing_position.c` declares
  `qsort_dword_array(void *comparator)`, which is the Ghidra view without EAX/ECX.
  `src/main/screenshot_render.c` declares `directory_create_recursive` as `void`.
  `hs_tokenize_primitive.c` and `console_command_bool_get_set.c` declare `string_to_lowercase` as `void`.
  All of these are harmless because the callers discard the result or pass the registers themselves.
- **`FUN_00624186`.** The CRT `fopen` is still unnamed in `symbols/functions.txt`. Every module
  declares it `void *(const char *path, [const] char *mode)`; the constness differs between files, and the ABI is the same.
- `0x8b` / `0x8c` are passed to `shell_display_fatal_error_dialog` as (resource id, help-text resource
  id). The constant names `k_profile_path_error_title` / `_message` are only a guess at which string is which.
