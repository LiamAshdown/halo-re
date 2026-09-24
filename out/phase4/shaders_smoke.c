// Phase 4 syntax gate for types/shaders.h. shaders.h holds no pointer members, so every size
// and offset check below fires on the 64-bit host compiler as well.
#include "tags.h"
#include "memory.h"
#include "shaders.h"

#define CHECK(name, expr) typedef char name[(expr) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// the module records
CHECK(check_channel_size, sizeof(shader_animation_channel) == 0x10);
CHECK(check_channel_period, OFF(shader_animation_channel, period) == 0x04);
CHECK(check_channel_scale, OFF(shader_animation_channel, scale) == 0x0c);
CHECK(check_anim_size, sizeof(shader_texture_animation) == 0x38);
CHECK(check_anim_v, OFF(shader_texture_animation, v) == 0x10);
CHECK(check_anim_rot, OFF(shader_texture_animation, rotation) == 0x20);
CHECK(check_anim_center, OFF(shader_texture_animation, rotation_center) == 0x30);
CHECK(check_xform_size, sizeof(shader_texture_transform) == 0x20);
CHECK(check_xform_v, OFF(shader_texture_transform, v_row) == 0x10);
CHECK(check_effect_size, sizeof(shader_effect) == 0xb4);
CHECK(check_effect_type, OFF(shader_effect, shader_type) == 0x24);
CHECK(check_effect_flags, OFF(shader_effect, flags) == 0x28);
CHECK(check_effect_fade, OFF(shader_effect, framebuffer_fade_mode) == 0x2c);
CHECK(check_effect_secondary_id, OFF(shader_effect, secondary_map) + OFF(TagDependency, tag_id) == 0x58);
CHECK(check_effect_anchor, OFF(shader_effect, anchor) == 0x5c);
CHECK(check_effect_anim, OFF(shader_effect, texture_animation) == 0x60);
CHECK(check_effect_98, OFF(shader_effect, unknown_98) == 0x98);
CHECK(check_effect_zsprite, OFF(shader_effect, zsprite_radius_scale) == 0x9c);

// shader_effect against every tags.h copy of the block
CHECK(check_lightning_shader, sizeof(LightningShader) == sizeof(shader_effect)
      && OFF(LightningShader, shader_type) == OFF(shader_effect, shader_type)
      && OFF(LightningShader, anchor) == OFF(shader_effect, anchor)
      && OFF(LightningShader, u_animation_source) == OFF(shader_effect, texture_animation)
      && OFF(LightningShader, zsprite_radius_scale) == OFF(shader_effect, zsprite_radius_scale));
CHECK(check_particle_block, OFF(Particle, shader_type) == 0xb0 + 0x24);
CHECK(check_contrail_block, OFF(Contrail, shader_type) == 0x84 + 0x24
      && OFF(Contrail, u_animation_source) == 0x84 + 0x60);

// the tag layouts every offset in shaders.h rests on
CHECK(check_shdr_size, sizeof(Shader) == 0x28);
CHECK(check_shdr_type, OFF(Shader, shader_type) == 0x24);
CHECK(check_soso_translucency, OFF(ShaderModel, translucency) == 0x38);
CHECK(check_soso_anim, OFF(ShaderModel, u_animation_source) == 0xfc
      && OFF(ShaderModel, rotation_animation_center) == 0xfc + 0x30);
CHECK(check_schi_flags, OFF(ShaderTransparentChicago, shader_transparent_chicago_flags) == 0x29
      && OFF(ShaderTransparentChicago, first_map_type) == 0x2a);
CHECK(check_scex_flags, OFF(ShaderTransparentChicagoExtended, shader_transparent_chicago_extended_flags) == 0x29
      && OFF(ShaderTransparentChicagoExtended, first_map_type) == 0x2a);
CHECK(check_sotr_flags, OFF(ShaderTransparentGeneric, shader_transparent_generic_flags) == 0x29
      && OFF(ShaderTransparentGeneric, first_map_type) == 0x2a);
CHECK(check_schi_map_anim, OFF(ShaderTransparentChicagoMap, u_animation_source) == 0xa4
      && sizeof(ShaderTransparentChicagoMap) == 0xa4 + sizeof(shader_texture_animation));
CHECK(check_schi_map_uv, OFF(ShaderTransparentChicagoMap, map_u_offset) == 0x5c
      && OFF(ShaderTransparentChicagoMap, map_rotation) == 0x64);
CHECK(check_sotr_map_anim, OFF(ShaderTransparentGenericMap, u_animation_source) == 0x2c
      && sizeof(ShaderTransparentGenericMap) == 0x2c + sizeof(shader_texture_animation));
CHECK(check_sgla_flags, OFF(ShaderTransparentGlass, shader_transparent_glass_flags) == 0x28);
CHECK(check_smet_flags, OFF(ShaderTransparentMeter, meter_flags) == 0x28);
CHECK(check_senv_scroll, OFF(ShaderEnvironment, u_animation_function) == 0x150
      && OFF(ShaderEnvironment, u_animation_period) == 0x154
      && OFF(ShaderEnvironment, u_animation_scale) == 0x158
      && OFF(ShaderEnvironment, v_animation_function) == 0x15c
      && OFF(ShaderEnvironment, v_animation_period) == 0x160
      && OFF(ShaderEnvironment, v_animation_scale) == 0x164);
CHECK(check_senv_detail, OFF(ShaderEnvironment, secondary_detail_map_scale) == 0xc8
      && OFF(ShaderEnvironment, micro_detail_map_scale) == 0xf8);

// constants
CHECK(check_digits, _numeric_countdown_timer_hour_tens + 2 == k_numeric_countdown_timer_digit_count);
CHECK(check_lit, _shader_transparent_lit_bit == 4 && _shader_transparent_draw_before_water_bit == 0x10);
