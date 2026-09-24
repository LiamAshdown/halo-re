// Blam cseries module (halo.exe 1.0.10 retail, 0x4491e0..0x44971e, 9 Ghidra functions).
// The Win32 half of the engine support library: an in-place string lowercaser, the
// millisecond clock built on QueryPerformanceCounter, a recursive CreateDirectory,
// GlobalAlloc/GlobalFree wrappers, the profile directory setup, the debug.txt error
// log writer, and a 4-byte-element quicksort with its short-partition selection sort.
//
// None of the nine functions dereferences a pointer at a constant offset other than the
// CRT struct tm returned by localtime (write_to_error_file reads +0x00..+0x14, which is
// the stock CRT layout and is not repeated here). The module therefore owns no struct
// layouts; what it does own is the constant set below, the qsort comparator type, and
// the globals listed at the end. Everything is taken from objdump -d -M intel of
// bin/halo.exe over 0x4491e0..0x449720 plus the call sites named per function.
//
// Register conventions (LTCG, confirmed at the callee prologue and the call sites):
//   0x4491e0 string_to_lowercase        EDI = char *string; returns EAX = string
//                                       (callers 0x486283, 0x4e29f2 load EDI first)
//   0x449210 time_query_performance_counter_ms  no arguments; returns EDX:EAX from __alldiv of
//                                       counter * 1000 / frequency, every one of the 52
//                                       callers uses EAX only (a uint32_t millisecond clock)
//   0x449250 directory_create_recursive __cdecl (char *path), returns char 0/1 in AL
//   0x449370 memory_global_alloc        EAX = byte count; returns EAX = GlobalAlloc(0, n)
//                                       (caller 0x541271 sets EAX = strlen + 1)
//   0x449380 memory_global_free         EAX = handle (callers 0x54194c, 0x541956)
//   0x449390 profile_path_initialize    no arguments, void
//   0x449450 write_to_error_file        __cdecl (char *message, uint8_t with_timestamp)
//   0x449590 qsort_dword_array          EAX = element count, ECX = int32_t *elements,
//                                       stack = qsort_dword_compare_proc (caller pops 4)
//                                       (callers 0x413d0f, 0x552d0b)
//   0x4496d0 qsort_dword_array_shortsort  EAX = int32_t *last (inclusive), stack =
//                                       int32_t *first, qsort_dword_compare_proc
//                                       (caller pops 8)
//
// Types this module uses that are defined elsewhere and NOT repeated here:
//   types/math.h   large_integer (the performance frequency global)
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum cseries_constants {
    k_cseries_path_length = 0x104,           // MAX_PATH: every strncpy/_snprintf bound into
                                             // the profile directory (0x4493b4, 0x449401,
                                             // 0x44941a); also the SHGetFolderPathA output
                                             // buffer in the 0x10c byte frame of 0x449390
    k_directory_create_buffer_size = 0x100,  // directory_create_recursive copies the path
                                             // into frame+0x08..frame+0x107 (0x108 byte frame,
                                             // saved error mode at +0x04, result byte at +0x03)
                                             // with an unbounded strcpy loop (0x4492a0)
    k_profile_directory_storage_size = 0x105, // bytes the shell startup zeroes at 0x006ac900
                                             // (rep stosd x 0x41 + stosb, 0x540ef9..0x540f05)
    k_milliseconds_per_second = 1000,        // 0x449226 push 0x3e8 into __allmul
    k_qsort_dword_shortsort_cutoff = 8,      // 0x4495b9: partitions of <= 8 go to the shortsort
    k_qsort_dword_stack_depth = 30,          // two int32_t[30] pending-partition stacks at
                                             // esp+0x14 and esp+0x8c in the 0xf4 byte frame
    k_error_file_minimum_level = 2,          // 0x44945b: nothing is logged while the byte at
                                             // 0x0087ac06 is below 2
    k_csidl_personal = 5,                    // 0x4493e9: the folder SHGetFolderPathA resolves
    k_sem_noopenfileerrorbox = 0x8000,       // 0x449259: SetErrorMode around the directory walk
    k_profile_path_error_title = 0x8b,       // 0x449435: shell_display_fatal_error_dialog
    k_profile_path_error_message = 0x8c      // 0x449430 (third argument is 1)
} cseries_constants;

// ---------------------------------------------------------------------------
// qsort_dword_array comparator
// Called __cdecl with two array elements (the caller pops 8). It returns nonzero when
// the first argument belongs after (or level with) the second: the partition loop at
// 0x449607 advances while compare(element, pivot) is zero, and the shortsort at
// 0x4496f6 keeps the running maximum by taking the element whenever
// compare(element, maximum) is nonzero, then swaps it to the end.
// Known comparators: 0x552c00 (plain signed a >= b) and 0x4127b0 (indexes a 0x3c byte
// table at 0x006f0c94 and orders on its +0x30 byte).
// ---------------------------------------------------------------------------
typedef uint8_t (*qsort_dword_compare_proc)(int32_t element, int32_t other);

// ---------------------------------------------------------------------------
// module globals
// ---------------------------------------------------------------------------
// global 0x006ac8f8: large_integer performance_frequency     QueryPerformanceFrequency output;
//                    filled once by the shell startup (push 0x6ac8f8 at 0x540eec) and the
//                    divisor of the clock 0x449210 plus about 40 inlined copies. Every src file
//                    (this module included) declares it extern int64_t, the same 8 bytes.
// global 0x006ac900: char profile_directory[k_profile_directory_storage_size]
//                    written by profile_path_initialize 0x449390: the -path argument, else
//                    "<CSIDL_PERSONAL>\My Games\Halo", else "." plus the fatal dialog;
//                    directory_create_recursive(profile_directory) runs next (0x540fa4)
// global 0x00686b48: uint8_t error_file_needs_header         1 in the image; write_to_error_file
//                    clears it and writes the banner block on the first logged message
//
// .rdata owned by the module
// global 0x006600e0: char profile_path_flag[]                "-path"
// global 0x006600c8: char profile_path_message[]             "Using profile path %s.\n"
// global 0x006600b4: char profile_path_format[]              "%s\\My Games\\Halo"
// global 0x006600b0: char profile_path_fallback[]            "."
// global 0x006600e8: char path_separator_string[]            "\\" (the _strpbrk set)
// global 0x006601e8: char error_file_spacer[]                "\r\n\r\n"
// global 0x00660198: char error_file_banner[]                "halo pc 01.00.10.0621(CACHE) ----...\r\n"
// global 0x0066017c: char error_file_function_name[]         "_write_to_error_file"
// global 0x00660160: char error_file_function_format[]       "reference function: %s\r\n"
// global 0x00660148: char error_file_address_format[]        "reference address: %x\r\n"
// global 0x00660144: char error_file_open_mode[]             "a+b"
// global 0x00660138: char error_file_name[]                  "debug.txt"
// global 0x00660118: char error_file_timestamp_format[]      "%02d.%02d.%02d %02d:%02d:%02d  "
// global 0x00660100: char error_file_no_timestamp[]          "<TIME UNAVAILABLE>  "
//
// Globals this module reads but does not own:
//   0x0087ac06  uint8_t debug level (shell): zeroed by 0x540fac; every access in the
//               image is byte wide, write_to_error_file logs only at 2 or more
//   0x0087ac01  uint8_t error file enabled (shell): set to 1 by 0x540fb2, only reader is
//               write_to_error_file 0x4494ce
//   0x0074626c  SHGetFolderPathA pointer (shell): GetProcAddress results stored at
//               0x540f99 and 0x541706, called __stdcall(0, 5, 0, 0, buffer) at 0x4493ed
//   0x006b85b8  char[0x104] returned by 0x4e40a0 (ESI = file name), the path the error
//               log is opened with
//   0x0065efec  "%s" (shared .rdata literal)
// ---------------------------------------------------------------------------

#pragma pack(pop)
