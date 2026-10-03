/**
 * @file include/halo/shell/layout.hpp
 * Named constants for the shell module: string resource ids, Direct3D/DirectDraw capability bits, memory classes and the MSVC string layout.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include "halo/core/datum.hpp"
#include "halo/core/flags.hpp"
#include "halo/core/win32_constants.hpp"
#include "halo/render/d3d9.hpp"

namespace halo::shell {

/** String resource ids of the fatal error dialog messages (strings.dll). */
inline constexpr uint32_t k_string_insufficient_memory = 0x65;
inline constexpr uint32_t k_string_insufficient_cpu = 0x66;
inline constexpr uint32_t k_string_previous_run_crashed = 0x6a;
inline constexpr uint32_t k_string_d3d9_missing = 0x6b;
inline constexpr uint32_t k_string_insufficient_disk_space = 0x6d;
inline constexpr uint32_t k_string_exception_title = 0x77;
inline constexpr uint32_t k_string_exception_gathering = 0x78;
inline constexpr uint32_t k_string_display_unsupported = 0x79;
inline constexpr uint32_t k_string_dsound_missing = 0x7b;
inline constexpr uint32_t k_string_dinput8_missing = 0x7c;
inline constexpr uint32_t k_string_shfolder_missing = 0x7d;
inline constexpr uint32_t k_string_direct3d_create_failed = 0x81;
inline constexpr uint32_t k_string_eula_name = 0x84;
inline constexpr uint32_t k_string_safe_mode_requested = 0x87;
inline constexpr uint32_t k_string_strings_dll_invalid = 0x88;
inline constexpr uint32_t k_string_single_instance = 0x92;
inline constexpr uint32_t k_string_product_id_missing = 0xa0;

/** String resource ids of the help (readme) file names the dialogs offer. */
inline constexpr uint32_t k_help_file_memory = 0x6e;
inline constexpr uint32_t k_help_file_cpu = 0x6f;
inline constexpr uint32_t k_help_file_crash = 0x73;
inline constexpr uint32_t k_help_file_disk_space = 0x76;
inline constexpr uint32_t k_help_file_directx = 0x7a;
inline constexpr uint32_t k_help_file_general = 0x7e;
inline constexpr uint32_t k_help_file_direct3d = 0x82;

/** Resource ids of the title strings: fatal at +1 from the base. */
inline constexpr uint32_t k_string_error_title_base = 0x7f;

/** Resource id of the fatal error dialog template and of the application icon. */
inline constexpr uintptr_t k_fatal_error_dialog_resource = 0x66;

/** Win32 predefined ids and virtual keys. */
inline constexpr uintptr_t k_idc_arrow = 0x7f00;
inline constexpr int32_t k_vk_control = 0x11;
inline constexpr uint32_t k_wm_initdialog = 0x110;
inline constexpr uint32_t k_wm_command = 0x111;

/** Direct3DCreate9 argument (D3D_SDK_VERSION). */
inline constexpr uint32_t k_d3d_sdk_version = 0x1f;

/** Process creation and priority flags. */
inline constexpr uint32_t k_process_all_access = 0x1f0fff;
inline constexpr uint32_t k_create_default_error_mode = 0x04000000;
inline constexpr uint32_t k_normal_priority_class = 0x20;
inline constexpr uint32_t k_idle_priority_class = 0x40;
inline constexpr uint32_t k_realtime_priority_class = 0x100;
inline constexpr uint32_t k_gmem_zeroinit = 0x40;
inline constexpr uint32_t k_file_map_read_write = 6;

/** Registry hives and access masks. */
inline constexpr uintptr_t k_hkey_current_user = 0x80000001;
inline constexpr uintptr_t k_hkey_local_machine = 0x80000002;
inline constexpr uint32_t k_key_read = 0x20019;
inline constexpr uint32_t k_key_write = 0x20006;
inline constexpr uint32_t k_reg_dword = 4;

/** DDSCAPS_* combinations the video memory probe queries. */
inline constexpr uint32_t k_ddscaps_primary_video_memory = 0x4200;
inline constexpr uint32_t k_ddscaps_local_texture_3d_device = 0x10007000;
inline constexpr uint32_t k_ddscaps_local_texture = 0x10005000;
inline constexpr uint32_t k_ddscaps_local_offscreen_plain = 0x10004040;

/** Physical memory classes in megabytes. */
inline constexpr uint32_t k_memory_class_small_mb = 0x80;
inline constexpr uint32_t k_memory_class_medium_mb = 0x100;
inline constexpr uint32_t k_memory_class_large_mb = 0x200;

/** Video memory estimates of shared-memory graphics, in bytes. */
inline constexpr uint32_t k_video_memory_8mb = 0x800000;
inline constexpr uint32_t k_video_memory_16mb = 0x1000000;
inline constexpr uint32_t k_video_memory_32mb = 0x2000000;
inline constexpr uint32_t k_video_memory_64mb = 0x4000000;
inline constexpr uint32_t k_video_memory_2gb = 0x80000000u;

/** config.txt ForceShader values that are not plain pixel shader versions. */
inline constexpr int32_t k_force_shader_2_0a = 0x270e;
inline constexpr int32_t k_force_shader_none = 0x270f;

/** Bit pattern defaults of the decal z bias floats. */
inline constexpr uint32_t k_decal_z_bias_default_bits = 0xb866afcd;
inline constexpr uint32_t k_transparent_decal_z_bias_default_bits = 0xb6a7c5ac;

/** Layout of the MSVC std::string the shell code reimplements. */
inline constexpr uint32_t k_string_inline_capacity = 0xf;
inline constexpr uint32_t k_string_npos = 0xffffffffu;

/** Two characters packed the way a 16-bit load of "ab" reads them (little endian: first character in the low byte). */
constexpr uint16_t char_pair(char first, char second) noexcept {
    return static_cast<uint16_t>(static_cast<uint8_t>(first) | (static_cast<uint8_t>(second) << 8));
}

/** Windows build numbers the hardware requirement parser maps to OS names. */
inline constexpr uint32_t k_windows_95_build = 950;
inline constexpr uint32_t k_windows_98_build = 951;
inline constexpr uint32_t k_windows_98se_build = 1999;
inline constexpr uint32_t k_windows_me_build = 2223;
inline constexpr uint32_t k_windows_xp_build = 2600;

/** Words of stack the integrity guard fills with a marker. */
inline constexpr int32_t k_stack_guard_words = 0x800;
inline constexpr uint32_t k_stack_guard_marker = 0xeeeeeeee;

/** Byte offset inside the guard buffer of the page that gets protected. */
inline constexpr size_t k_stack_guard_page_offset = 0x1000;

/** The size of the profile directory buffer. */
inline constexpr int32_t k_profile_directory_buffer_size = 0x105;

}  // namespace halo::shell
