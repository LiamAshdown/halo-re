/**
 * @file include/halo/core/win32_constants.hpp
 * Named Win32 constants for the engine code that calls the file, wait and error APIs with raw numbers.
 */
#pragma once

#include <cstdint>

namespace halo::win32 {

/** dwDesiredAccess values. */
inline constexpr uint32_t k_generic_read = 0x80000000u;
inline constexpr uint32_t k_generic_write = 0x40000000u;
inline constexpr uint32_t k_generic_read_write = k_generic_read | k_generic_write;

/** dwShareMode values. */
inline constexpr uint32_t k_file_share_none = 0;
inline constexpr uint32_t k_file_share_read = 1;

/** dwCreationDisposition values. */
inline constexpr uint32_t k_create_new = 1;
inline constexpr uint32_t k_create_always = 2;
inline constexpr uint32_t k_open_existing = 3;
inline constexpr uint32_t k_open_always = 4;

/** dwFlagsAndAttributes values. */
inline constexpr uint32_t k_file_attribute_directory = 0x10;
inline constexpr uint32_t k_file_attribute_normal = 0x80;
inline constexpr uint32_t k_file_flag_sequential_scan = 0x08000000;

/** Sentinels returned by file calls. */
inline constexpr uint32_t k_invalid_file_attributes = 0xffffffffu;
inline constexpr uint32_t k_invalid_file_size = 0xffffffffu;
inline constexpr uint32_t k_invalid_set_file_pointer = 0xffffffffu;
inline constexpr uint32_t k_infinite = 0xffffffffu;

/** INVALID_HANDLE_VALUE. */
inline void *invalid_handle() noexcept { return reinterpret_cast<void *>(~static_cast<uintptr_t>(0)); }

/** Wait results. */
inline constexpr uint32_t k_wait_object_0 = 0;
inline constexpr uint32_t k_wait_abandoned = 0x80;

/** GetExitCodeThread value for a thread that has not finished. */
inline constexpr uint32_t k_still_active = 0x103;

/** Win32 error codes the file code tests. */
inline constexpr uint32_t k_error_handle_eof = 0x26;
inline constexpr uint32_t k_error_already_exists = 0xb7;

/** FormatMessage flags: FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | maximum line width mask. */
inline constexpr uint32_t k_format_message_system_message = 0x12ff;

/** WaitForSingleObject timeout result and the default user locale. */
inline constexpr uint32_t k_wait_timeout = 0x102;
inline constexpr uint32_t k_locale_user_default = 0x400;

/** Window messages, virtual keys and system commands the movie player handles. */
inline constexpr uint32_t k_wm_keydown = 0x100;
inline constexpr uint32_t k_wm_keyup = 0x101;
inline constexpr uint32_t k_wm_char = 0x102;
inline constexpr uint32_t k_wm_syskeydown = 0x104;
inline constexpr uint32_t k_wm_syschar = 0x106;
inline constexpr uint32_t k_wm_syscommand = 0x112;
inline constexpr uint32_t k_vk_escape = 0x1b;
inline constexpr uint32_t k_vk_space = 0x20;
inline constexpr uint32_t k_sc_close = 0xf060;

/** VirtualAlloc: MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE. */
inline constexpr uint32_t k_mem_commit_reserve = 0x3000;
inline constexpr uint32_t k_page_readwrite = 4;

/** FILE_FLAG_OVERLAPPED and MAX_PATH (260). */
inline constexpr uint32_t k_file_flag_overlapped = 0x40000000;
inline constexpr uint32_t k_max_path = 0x104;

/** SetFilePointer move methods. */
inline constexpr uint32_t k_file_begin = 0;
inline constexpr uint32_t k_file_end = 2;

}  // namespace halo::win32
