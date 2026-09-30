// rasterizer_begin_frame  (Ghidra: rasterizer_begin_frame, already named)
// address 0x5175c0, size 266 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: copies the caller's window parameters block (0x96 dwords == sizeof
//   rasterizer_window_parameters) into rasterizer_window, then zeroes every per-frame dynamic
//   geometry/transparent-group/light table types/rasterizer.h documents, matching the type
//   header's own note for this exact function.
// register convention: source window parameters in the recognized stack parameter.
// UNSURE: 0x0071d160/0x0071d1b4/0x0071d1b2/0x0071d1b1/0x0071d276 are not documented in
//   types/rasterizer.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern uint32_t rasterizer_scratch_memory_used; // 0x0071d140
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count]; // 0x006d98e8
extern int32_t rasterizer_dynamic_vertex_slot_count; // 0x006dd9d8
extern int32_t rasterizer_dynamic_index_count;       // 0x006e09e4
extern int32_t rasterizer_dynamic_index_slot_count;  // 0x006e09e0
extern int32_t transparent_geometry_group_count;     // 0x0071d154
extern uint32_t unknown_0071d160; // 0x0071d160 UNSURE
extern uint32_t transparent_geometry_group_drawn_bits[12]; // 0x006d983c
extern int32_t transparent_geometry_group_secondary_count; // 0x0071d158
extern int32_t rasterizer_light_count; // 0x007c1480
extern uint8_t unknown_0071d276;                                    // 0x0071d276 UNSURE
extern uint16_t unknown_0071d1b4; // 0x0071d1b4 UNSURE (written as a word)
extern uint8_t rasterizer_render_target_capture_done;      // 0x0071d1b2, set by 0x519b00
extern uint8_t rasterizer_render_target_capture_requested; // 0x0071d1b1, see 0x519b00
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893e6;                         // 0x006893e6 wireframe
extern void *rasterizer_device;   // 0x0071d174


// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode);       // 0x519200

extern uint32_t color_rgb_float_to_int(const ColorRGB *color); // 0x4ab5d0
extern void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear_target); // 0x52ccc0, blam-cc: AX target_index
// blam-cc: stack -> (z_near, z_far) as raw float bits


typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

// Latches the current frame's window/camera/frustum/fog parameters into rasterizer_window and
// resets every per-frame subsystem (dynamic geometry slots, transparent groups, lights).
void __cdecl rasterizer_begin_frame(rasterizer_window_parameters *source)
{
    void **vtable;
    int i;

    rasterizer_window = *source;

    rasterizer_scratch_memory_used = 0;

    for (i = 0; i < k_rasterizer_vertex_type_count; i++) {
        rasterizer_dynamic_vertex_caches[i].used = 0;
    }
    rasterizer_dynamic_vertex_slot_count = 0;
    rasterizer_dynamic_index_count = 0;
    rasterizer_dynamic_index_slot_count = 0;
    transparent_geometry_group_count = 0;
    unknown_0071d160 = 0;
    for (i = 0; i < 12; i++) {
        transparent_geometry_group_drawn_bits[i] = 0;
    }
    transparent_geometry_group_secondary_count = 0;
    rasterizer_light_count = 0;
    rasterizer_light_disable_all();

    unknown_0071d276 = 0;
    unknown_0071d1b4 = 0;
    rasterizer_render_target_capture_done = 0;
    rasterizer_render_target_capture_requested = 0;

    rasterizer_set_shader_stage_config(0);
    rasterizer_set_fog_constants(&source->fog);

    {
        uint32_t clear_color = (console_debug_toggle_6893e4 == 1) ? 0 : color_rgb_float_to_int(&rasterizer_window.fog.atmospheric_color);
        if (rasterizer_window.type == 1 || rasterizer_window.type == 2) {
            rasterizer_render_target_set_active(rasterizer_window.type, clear_color, source->clear_target == 0);
        }
    }

    chimera__rasterizer_set_frustum_z_func(0xbf800000, 0xbf800000); // -1.0f, -1.0f

    vtable = *(void ***)rasterizer_device;
    ((d3d_set_render_state_fn)vtable[0xe4 / 4])(rasterizer_device, 8, 3 - (uint32_t)(console_debug_toggle_6893e6 != 0));
}

#if 0
Original Ghidra decompilation (0x5175c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl rasterizer_begin_frame(short *param_1)

{
  undefined4 *puVar1;
  int iVar2;
  uint uVar3;
  short *psVar4;

  psVar4 = param_1;
  puVar1 = &DAT_007c1220;
  for (iVar2 = 0x96; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar1 = *(undefined4 *)psVar4;
    psVar4 = psVar4 + 2;
    puVar1 = puVar1 + 1;
  }
  DAT_0071d140 = 0;
  puVar1 = &DAT_006d98e8;
  do {
    *puVar1 = 0;
    puVar1 = puVar1 + 3;
  } while ((int)puVar1 < 0x6d99d8);
  DAT_006dd9d8 = 0;
  DAT_006e09e4 = 0;
  DAT_006e09e0 = 0;
  DAT_0071d154 = 0;
  _DAT_0071d160 = 0;
  puVar1 = &DAT_006d983c;
  for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar1 = 0;
    puVar1 = puVar1 + 1;
  }
  DAT_0071d158 = 0;
  DAT_007c1480 = 0;
  rasterizer_light_disable_all();
  DAT_0071d276 = 0;
  _DAT_0071d1b4 = 0;
  DAT_0071d1b2 = 0;
  DAT_0071d1b1 = 0;
  rasterizer_set_shader_stage_config();
  FUN_005176d0();
  if (DAT_006893e4 == 1) {
    uVar3 = 0;
  }
  else {
    uVar3 = color_rgb_float_to_int((float *)&DAT_007c140c);
  }
  if ((*param_1 == 1) || (*param_1 == 2)) {
    FUN_0052ccc0(uVar3,*(char *)((int)param_1 + 5) == '\0');
  }
  chimera__rasterizer_set_frustum_z_func(0xbf800000,0xbf800000);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,8,3 - (uint)(DAT_006893e6 != '\0'));
  return;
}
#endif
