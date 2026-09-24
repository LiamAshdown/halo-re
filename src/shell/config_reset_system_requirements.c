// config_reset_system_requirements  (Ghidra: config_reset_system_requirements, already named)
// address 0x57cfe0, size 153 bytes
// name confidence: 0.55  rewrite confidence: 0.85
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Resets the block of
//   system-requirement/config globals used by the config.txt parser to their default (mostly
//   zero) state." Every written global matches types/shell.h's config_* / config_maximum_
//   resolution field list by address.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t config_maximum_resolution;                 // 0x0069fe3c
extern int32_t config_linear_texture_addressing;           // 0x00722b28
extern int32_t config_linear_texture_addressing_zoom;      // 0x00722b2c
extern int32_t config_linear_texture_addressing_sun;       // 0x00722b30
extern int32_t config_use_fixed_function;                  // 0x00722b34
extern int32_t config_disable_driver_management;           // 0x00722b38
extern int32_t config_unsupported_card;                    // 0x00722b3c
extern int32_t config_prototype_card;                      // 0x00722b40
extern int32_t config_old_driver;                          // 0x00722b44
extern int32_t config_old_sound_driver;                    // 0x00722b48
extern int32_t config_invalid_driver;                      // 0x00722b4c
extern int32_t config_invalid_sound_driver;                // 0x00722b50
extern int32_t config_disable_buffering;                   // 0x00722b54
extern int32_t config_enable_stop_start;                   // 0x00722b58
extern int32_t config_head_relative_speech;                // 0x00722b5c
extern int32_t config_safe_mode;                           // 0x00722b60
extern int32_t config_force_shader;                        // 0x00722b64
extern int32_t config_use_anisotropic_filter;               // 0x00722b68
extern int32_t config_disable_specular;                     // 0x00722b6c
extern int32_t config_disable_render_targets;                // 0x00722b70
extern int32_t config_disable_alpha_render_targets;          // 0x00722b74
extern int32_t config_use_alternate_convolve_mask;           // 0x00722b78
extern int32_t config_min_max_blend_op_is_broken;             // 0x00722b7c
extern float config_decal_z_bias;                            // 0x00722b80
extern float config_transparent_decal_z_bias;                 // 0x00722b84
extern float config_decal_slope_z_bias;                       // 0x00722b88
extern float config_transparent_decal_slope_z_bias;           // 0x00722b8c

// Resets every system-requirement/config global that config.txt (shell_parse_config_txt) fills
// back to its default state: MaximumResolution to 0x1000, every flag to 0, and the decal
// z-bias floats to their bit-exact defaults (0.0f for the two slope biases).
void config_reset_system_requirements(void)
{
    uint32_t bits;

    config_maximum_resolution = k_shell_config_maximum_resolution_default;
    config_linear_texture_addressing = 0;
    config_linear_texture_addressing_zoom = 0;
    config_linear_texture_addressing_sun = 0;
    config_use_fixed_function = 0;
    config_prototype_card = 0;
    config_disable_driver_management = 0;
    config_use_anisotropic_filter = 0;
    config_disable_specular = 0;
    config_unsupported_card = 0;
    config_enable_stop_start = 0;
    config_head_relative_speech = 0;
    config_disable_render_targets = 0;
    config_disable_alpha_render_targets = 0;
    config_use_alternate_convolve_mask = 0;
    config_old_driver = 0;
    config_old_sound_driver = 0;
    config_invalid_driver = 0;
    config_invalid_sound_driver = 0;
    config_safe_mode = 0;
    config_force_shader = 0;
    config_min_max_blend_op_is_broken = 0;
    config_disable_buffering = 0;

    bits = 0xb866afcd;
    config_decal_z_bias = *(float *)&bits;
    config_decal_slope_z_bias = 0.0f;
    bits = 0xb6a7c5ac;
    config_transparent_decal_z_bias = *(float *)&bits;
    config_transparent_decal_slope_z_bias = 0.0f;
}

#if 0
Original Ghidra decompilation (0x57cfe0):

void config_reset_system_requirements(void)

{
  DAT_0069fe3c = 0x1000;
  DAT_00722b28 = 0;
  DAT_00722b2c = 0;
  DAT_00722b30 = 0;
  DAT_00722b34 = 0;
  DAT_00722b40 = 0;
  DAT_00722b38 = 0;
  DAT_00722b68 = 0;
  DAT_00722b6c = 0;
  DAT_00722b3c = 0;
  DAT_00722b58 = 0;
  DAT_00722b5c = 0;
  DAT_00722b70 = 0;
  DAT_00722b74 = 0;
  DAT_00722b78 = 0;
  DAT_00722b44 = 0;
  DAT_00722b48 = 0;
  DAT_00722b4c = 0;
  DAT_00722b50 = 0;
  DAT_00722b60 = 0;
  DAT_00722b64 = 0;
  DAT_00722b7c = 0;
  DAT_00722b54 = 0;
  DAT_00722b80 = 0xb866afcd;
  DAT_00722b88 = 0;
  DAT_00722b84 = 0xb6a7c5ac;
  DAT_00722b8c = 0;
  return;
}
#endif
