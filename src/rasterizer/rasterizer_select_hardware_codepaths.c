// rasterizer_select_hardware_codepaths  (Ghidra: rasterizer_select_hardware_codepaths, already
// named)
// address 0x516810, size 166 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: selects between vendor/driver code paths based on d3d_caps9.max_streams (+0xbc) and
//   .pixel_shader_version (+0xcc), matching the type header's pinned offsets exactly.
// register convention: none -- __cdecl, no parameters.
// UNSURE: after the two callee calls, Ghidra reads extraout_EAX/extraout_ECX -- registers it
//   cannot trace to a real source, only that they are whatever EAX/ECX held on return. The two
//   comparisons that follow are identical in shape to the max_streams/pixel_shader_version
//   checks made just before the calls, so this is modeled as re-reading those same two caps
//   fields rather than trusting an unprovable leftover register value; this is the most likely
//   true behaviour but is not independently confirmed. The function-pointer targets
//   (LAB_/FUN_/DAT_ addresses outside this session's range) are kept as raw code addresses since
//   their real signatures are unknown.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps; // 0x007c10c0
extern void *unknown_007c048c; // 0x007c048c shader_environment self-illumination draw procedure
extern void *unknown_007c0490; // 0x007c0490 UNSURE: draw function pointer
extern void *unknown_007c0494; // 0x007c0494 UNSURE: draw function pointer
extern void *rasterizer_water_draw_procedure; // 0x007bf050

extern void rasterizer_glass_draw_procedures_select(void); // 0x523ec0
extern void rasterizer_shader_environment_select_draw_functions(void); // 0x52b630

extern void rasterizer_shader_environment_self_illumination_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); // 0x51f3e0
extern void rasterizer_light_cone_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); // 0x51dc50
extern void function_do_nothing(void);  // 0x44ad80
// 0x51fd80, 0x51fad0, 0x51e8f0, 0x51e570 and 0x51e2a0 are functions Ghidra never defined (they are
//   only referenced by these immediate stores); see src/rasterizer/README.md, known gaps.
extern uint8_t LAB_0051fd80, LAB_0051fad0, LAB_0051e8f0, LAB_0051e570;
extern void rasterizer_shader_environment_lightmap_draw(void); // 0x51e2a0
extern uint8_t LAB_005358b0, DAT_00535fd0;

// Selects vendor/driver-specific rendering code path function pointers based on the detected
// GPU capability caps (max_streams, pixel_shader_version).
void __cdecl rasterizer_select_hardware_codepaths(void)
{
    if (rasterizer_caps.max_streams < 2) {
        unknown_007c048c = &LAB_0051fd80;
    } else {
        unknown_007c048c = &LAB_0051fad0;
        if (rasterizer_caps.pixel_shader_version > 0xffff0100) {
            unknown_007c048c = (void *)rasterizer_shader_environment_self_illumination_draw;
        }
    }

    rasterizer_glass_draw_procedures_select();
    rasterizer_shader_environment_select_draw_functions();

    if (rasterizer_caps.max_streams < 2) { // UNSURE: re-read, see file header
        unknown_007c0490 = &LAB_0051e8f0;
        if (rasterizer_caps.pixel_shader_version > 0xffff0100) {
            unknown_007c0494 = (void *)rasterizer_light_cone_draw;
            goto set_vertex_buffer_slot;
        }
    } else {
        unknown_007c0490 = &LAB_0051e570;
        if (rasterizer_caps.pixel_shader_version > 0xffff0100) {
            unknown_007c0490 = (void *)rasterizer_shader_environment_lightmap_draw; // 0x51e2a0
            if (rasterizer_caps.pixel_shader_version > 0xffff0100) {
                unknown_007c0494 = (void *)rasterizer_light_cone_draw;
                goto set_vertex_buffer_slot;
            }
        }
    }
    unknown_007c0494 = (void *)function_do_nothing;

set_vertex_buffer_slot:
    rasterizer_water_draw_procedure = &LAB_005358b0;
    if (rasterizer_caps.pixel_shader_version > 0xffff0100) {
        rasterizer_water_draw_procedure = &DAT_00535fd0;
    }
}

#if 0
Original Ghidra decompilation (0x516810):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl rasterizer_select_hardware_codepaths(void)

{
  uint extraout_EAX;
  int extraout_ECX;

  if (DAT_007c117c < 2) {
    _DAT_007c048c = (code *)&LAB_0051fd80;
  }
  else {
    _DAT_007c048c = (code *)&LAB_0051fad0;
    if (0xffff0100 < DAT_007c118c) {
      _DAT_007c048c = FUN_0051f3e0;
    }
  }
  FUN_00523ec0();
  rasterizer_shader_environment_select_draw_functions();
  if (extraout_ECX < 2) {
    _DAT_007c0490 = &LAB_0051e8f0;
LAB_0051687d:
    if (0xffff0100 < extraout_EAX) {
      _DAT_007c0494 = FUN_0051dc50;
      goto LAB_0051689a;
    }
  }
  else {
    _DAT_007c0490 = &LAB_0051e570;
    if (0xffff0100 < extraout_EAX) {
      _DAT_007c0490 = &DAT_0051e2a0;
      goto LAB_0051687d;
    }
  }
  _DAT_007c0494 = FUN_0044ad80;
LAB_0051689a:
  DAT_007bf050 = &LAB_005358b0;
  if (0xffff0100 < extraout_EAX) {
    DAT_007bf050 = &DAT_00535fd0;
  }
  return;
}
#endif
