# cseries module: type recovery notes

Target: retail Halo PC `halo.exe` 1.0.10, module `cseries` (9 Ghidra functions, 0x4491e0..0x44971e).
Header: `types/cseries.h`. Smoke test: `out/phase4/cseries_smoke.c`:

```
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -Wall -Wextra -I types out/phase4/cseries_smoke.c
```

It passes on the host compiler and with `-m32`. It also compiles after `math.h`. I built a translation unit that includes every
header in `types/`, and none of its diagnostics mention `cseries.h`. The 24 errors it reports come from forward references
that were already in other headers. The smoke test checks the constant values against the immediates in the code, the
qsort frame arithmetic (`4 + 2 * 30 * 4 == 0xf4`), the end of the profile directory block (`0x006ac900 + 0x105`), and it
assigns a real comparator to `qsort_dword_compare_proc`.

The disassembly comes from `objdump -d -M intel` of `bin/halo.exe` over 0x4491e0..0x449720. I also read the call sites
listed below and the shell startup code at 0x540ee0..0x540fd3.

## Structs

None. None of the nine functions reaches a field through a pointer at a constant offset, with one exception: the CRT
`struct tm` from `_localtime`. write_to_error_file reads +0x00 sec, +0x04 min, +0x08 hour, +0x0c mday, +0x10 mon (+1) and
+0x14 year (% 100) at 0x449526..0x449542. That is the stock MSVC layout, which Ghidra already has, so the header does not
repeat it. There is no struct in this module that needs every byte accounted for.

## What the header defines

| item | evidence |
|---|---|
| `cseries_constants` enum | Immediates in the code: `0x104` strncpy/_snprintf bound (0x4493b4, 0x449401, 0x44941a). `0x100` directory scratch buffer: frame 0x108, saved SetErrorMode result at frame+0x04, result byte at frame+0x03, buffer frame+0x08..+0x107, filled by an unbounded copy loop at 0x4492a0. `0x105` bytes zeroed at 0x006ac900 by the shell (`mov ecx,0x41; rep stosd; stosb`). `1000` (0x449226). Shortsort cutoff `8` (0x4495b9). Stack depth `30`: the two `int32_t` stacks at esp+0x14 / esp+0x8c are 0x78 bytes each, and with the counter at esp+0x10 they fill the 0xf4 frame. Log level `2` (0x44945b `cmp al,2; jb`). CSIDL `5` (0x4493e9). SetErrorMode `0x8000` (0x449259). Dialog ids `0x8b` / `0x8c` (0x449430..0x44943a). |
| `qsort_dword_compare_proc` | Called `__cdecl` with two dword elements; the caller pops 8 (0x449614, 0x449634, 0x4496fa). It returns a byte that only gets tested with `test al,al`. The concrete comparators are 0x552c00 (`a >= b` on signed ints) and 0x4127b0 (both arguments are indices into a 0x3c-stride table at 0x006f0c94, ordered by the byte at +0x30). |

## Globals

Owned:

- `0x006ac8f8 large_integer performance_frequency`. The only writer is `push 0x6ac8f8; call [QueryPerformanceFrequency]`
  at 0x540eec, which the shell startup runs once. There are 84 readers: the cseries clock 0x449210 and about 40 copies
  inlined by LTCG, some through `fild qword`. The header lists it as cseries-owned because cseries holds the only
  non-inlined consumer. main.h, shell.h and input.h tag it "(math)", but math.h has no `// global` line for it, so
  nothing is claimed twice. The type is math.h's `large_integer`, and it is not redefined here.
- `0x006ac900 char profile_directory[0x105]`. profile_path_initialize writes it. types/cache.h lists it as
  `char profile_directory[0x104]` and says it is written "by the shell startup code at 0x00544000-ish". Both
  details are wrong. The actual writer is 0x449390, which 0x540f06 calls. The shell zeroes the block
  just before that call, and it zeroes 0x105 bytes (one byte past MAX_PATH). All writes are bounded by 0x104. The name
  is kept for consistency with cache.h.
- `0x00686b48 uint8_t error_file_needs_header`. Its initial image value is 1, and its only accessors are 0x449463 and
  0x449473, both in write_to_error_file.
- The .rdata literals 0x006600b0..0x006601e8 are listed in the header. Two of them, `"debug.txt"` (0x660138) and
  `"a+b"` (0x660144), show that the file open is `fopen(0x4e40a0(ESI="debug.txt"), "a+b")`. Ghidra folded the pushed
  `"a+b"` into the argument of FUN_004e40a0, which is wrong: that call takes its argument in ESI. 0x624186 is `fopen`
  (two stack args, and one `add esp,8` pops both calls).

Read, not owned:

- `0x0087ac06`, a byte log/debug level. The shell zeroes it (0x540fac `mov byte,bl`). **Every access in the image is
  one byte wide** (`mov al,ds:` / `cmp byte ptr`). Two headers disagree with that: types/interface.h declares it
  `int32_t console_verbosity` and types/networking.h declares it `int16_t network_statistics_level`. Both types should
  become `uint8_t`, and the two names should be merged into one.
- `0x0087ac01` is a byte flag set to 1 by the shell at 0x540fb2. Its only reader is write_to_error_file. It sits next to
  other shell-written flags: 0x87ac00, 0x87ac04 and 0x87ac05 are bytes and 0x87ac08 is a word. None of them is ever
  reached through a base register, so they are separate globals, not a struct.
- `0x0074626c` is the SHGetFolderPathA pointer. The shell resolves it from shfolder.dll at 0x540f99 and 0x541706.
  0x4493ed calls it stdcall as `(0, CSIDL_PERSONAL, 0, 0, buffer)`.
- `0x006b85b8` is a `char[0x104]` filled and returned by 0x4e40a0 (networking range).

## Register conventions (LTCG)

These are confirmed at the callee prologue and at the call sites.

- 0x4491e0 string_to_lowercase: **EDI** = string. It returns EAX = the same string (`mov eax,edi` at 0x449206). Ghidra
  drops the return value.
- 0x449210 (FUN_00449210, the millisecond clock): takes no arguments and returns EDX:EAX from `__alldiv(counter*1000, freq)`.
  52 callers call it and none of them reads EDX afterwards. The EDX uses right after those calls are all unrelated
  reloads. Treat it as a `uint32_t` millisecond clock.
- 0x449250 directory_create_recursive: `__cdecl(char *path)`, returns the byte in AL.
- 0x449370 memory_global_alloc: **EAX** = size, EAX = handle. 0x449380 memory_global_free: **EAX** = handle.
- 0x449390 profile_path_initialize: takes no arguments. It calls command_line_check_flag with the flag name on the stack
  and **EDI** = `char **value` (zeroed by the callee first).
- 0x449450 write_to_error_file: `__cdecl(char *message, uint8_t with_timestamp)`.
- 0x449590 qsort_dword_array: **EAX** = count, **ECX** = `int32_t *elements`, stack = comparator.
- 0x4496d0 qsort_dword_array_shortsort: **EAX** = `int32_t *last` (inclusive), stack = `int32_t *first`, comparator. The
  pack's Ghidra signature is right about the stack pair. The `last` pointer really travels in EAX.

## Unresolved offsets

- None inside the module.
- The size of the static at 0x006ac900 depends on what follows it. No instruction references 0x6aca04..0x6aca1f, so
  0x105 is the size the zeroing establishes, not a proven end of the object.

## Misattributed / library functions

- 0x449590 and 0x4496d0 are in `out/phase2/cutscene/00.md` because that pack's range starts at 0x449590. They are not
  cutscene code. They sort generic 4-byte elements and are called from ai (0x413d0f) and structures (0x552d0b). The
  algorithm is the MSVC CRT `qsort` (CUTOFF 8, STKSIZ 30, a middle-element pivot, a max-selection shortsort) specialised
  to 4-byte elements and a byte-returning comparator. It is Bungie's copy, not the linked CRT routine, so it stays in
  cseries.
- 0x449370 and 0x449380 are plain Win32 wrappers. They belong in cseries (the Blam system_malloc / system_free
  pattern), not in memory.
- No function in the range is linked library code. The CRT callees are outside the range: _tolower 0x624687,
  _strpbrk 0x6252b0, _strncpy 0x623a90, _snprintf 0x623a2d, _sprintf 0x623693, _fprintf 0x623de2, fopen 0x624186,
  _fclose 0x6241e5, _time32 0x6240f1 and _localtime 0x624886.

## Decompiler errors worth knowing for the rewrite

- profile_path_initialize: in the default branch the `printf("Using profile path %s.\n", profile_directory)` runs
  **before** SHGetFolderPathA, so it prints the still-empty buffer. That is the binary's behaviour, not a decompiler
  artefact. Ghidra lost the printf argument and the SHGetFolderPath argument list (0x4493ce..0x4493ed).
- write_to_error_file: Ghidra shows `FUN_004e40a0(&DAT_00660144)` followed by a one-argument `FUN_00624186`. The real
  sequence is `fopen(FUN_004e40a0(ESI="debug.txt"), "a+b")`.
