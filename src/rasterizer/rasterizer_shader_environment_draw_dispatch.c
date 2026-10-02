// rasterizer_shader_environment_draw_dispatch  (Ghidra: FUN_0052b050, unnamed)
// address 0x52b050, size 296 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: rebuilt from the raw disassembly (phase 4 review; the earlier file passed six
//   arguments where the binary passes eight, dropped EBX and the position, and swapped the
//   overlay shader for the part shader). Callers FUN_0046b2f0 and flag_render 0x4fc350 load
//   EBX right before the call. With the active model draw context (0x0071d1f0) it first queues
//   the overlay shader of the context (group_parameters.shader, skipped for a plasma overlay
//   whose function value is exactly 0) and gives that group a scratch copy of the two dwords at
//   context +0xac as its function source; then in mode 1 it queues the part itself and requests a
//   frame capture, and in mode 0 it draws immediately through the environment (0x007c0470) or
//   the generic (0x007c0474) draw procedure chosen by 0x52b630.
// register convention: EBX -> dynamic_vertex_slot, stack -> (shader, frame, index_buffer,
//   dynamic_index_slot, primitive_count, vertex_buffer).
// blam-cc: EBX -> dynamic_vertex_slot, stack -> (shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer)
// UNSURE name: nothing here is specific to shader_environment; it is the draw entry for one
//   model geometry part of any shader type.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t console_debug_toggle_6893ec;                 // 0x006893ec
extern rasterizer_model_draw_context *rasterizer_active_model_context; // 0x0071d1f0
extern int16_t rasterizer_active_model_mode;                // 0x0071d1f8
extern uint8_t rasterizer_render_target_capture_requested;  // 0x0071d1b1
extern void *shader_environment_draw_simple;                // 0x007c0470 procedure for shader_type 3
extern void *shader_environment_draw;                       // 0x007c0474 procedure for the other types
extern void debug_fp_dispatch_note(int32_t toggle, int32_t mode, int32_t shader_type, int32_t primitives,
    void *draw, void *draw_simple, void *overlay); // TEMPORARY

// blam-cc: EAX -> link, stack -> (shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot, position)
extern transparent_geometry_group *rasterizer_transparent_geometry_group_build(
    transparent_geometry_group_link *link, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
    int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
    int32_t dynamic_vertex_slot, const real_point3d *position); // 0x52b180
// blam-cc: EAX -> source, ECX -> size
extern void *chimera__rasterizer_memory_alloc(const void *source, uint32_t size); // 0x514560

typedef void (*rasterizer_part_draw_procedure)(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
                                               int32_t dynamic_index_slot, int32_t primitive_count,
                                               rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot);

void rasterizer_shader_environment_draw_dispatch(int32_t dynamic_vertex_slot, uint8_t *shader, int16_t frame,
                                                 rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
                                                 int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    rasterizer_model_draw_context *context;
    uint8_t *overlay;

    debug_fp_dispatch_note(console_debug_toggle_6893ec, rasterizer_active_model_mode, *(int16_t *)&((struct Shader *)shader)->shader_type,
        primitive_count, shader_environment_draw, shader_environment_draw_simple,
        rasterizer_active_model_context ? (void *)(uintptr_t)rasterizer_active_model_context->group_parameters.shader : 0);
        // TEMPORARY first-person diagnostics
    if (!console_debug_toggle_6893ec) {
        return;
    }
    context = rasterizer_active_model_context;
    overlay = (uint8_t *)(uintptr_t)context->group_parameters.shader;
    if (overlay != NULL) {
        int16_t source = *(int16_t *)(overlay + 0x2c);                 // plasma intensity_source
        const float *function_values = (const float *)(uintptr_t)context->group_parameters.function_values;
        uint8_t hidden = (*(int16_t *)(overlay + 0x24) == 0xb && source >= 1 && source <= 4 &&
                          function_values != NULL && function_values[source - 1] == 0.0f);

        if (!hidden) {
            transparent_geometry_group *group =
                rasterizer_transparent_geometry_group_build(NULL, overlay, frame, index_buffer, dynamic_index_slot,
                                                            primitive_count, vertex_buffer, dynamic_vertex_slot,
                                                            &context->center);

            context = rasterizer_active_model_context;
            if (group != NULL) {
                group->lighting_extra =
                    (uint32_t)(uintptr_t)chimera__rasterizer_memory_alloc(&context->group_parameters.unknown_20, 8);
            }
        }
    }
    if (rasterizer_active_model_mode == 1) {
        rasterizer_transparent_geometry_group_build(NULL, shader, frame, index_buffer, dynamic_index_slot,
                                                    primitive_count, vertex_buffer, dynamic_vertex_slot,
                                                    &context->center);
        rasterizer_render_target_capture_requested = 1;
        return;
    }
    if (rasterizer_active_model_mode == 0) {
        if (*(int16_t *)&((struct Shader *)shader)->shader_type == 3) {
            ((rasterizer_part_draw_procedure)shader_environment_draw_simple)(
                shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
        } else {
            ((rasterizer_part_draw_procedure)shader_environment_draw)(
                shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
        }
    }
}

#if 0
Original Ghidra decompilation (0x52b050):

void FUN_0052b050(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  
  if (DAT_006893ec != '\0') {
    iVar2 = *(int *)(DAT_0071d1f0 + 0xa8);
    if (((iVar2 != 0) &&
        ((((*(short *)(iVar2 + 0x24) != 0xb || (sVar1 = *(short *)(iVar2 + 0x2c), sVar1 < 1)) ||
          (4 < sVar1)) ||
         ((*(int *)(DAT_0071d1f0 + 0xb0) == 0 ||
          (*(float *)(*(int *)(DAT_0071d1f0 + 0xb0) + -4 + sVar1 * 4) != 0.0)))))) &&
       (iVar2 = FUN_0052b180(iVar2,param_2,param_3,param_4,param_5,param_6), iVar2 != 0)) {
      uVar3 = chimera__rasterizer_memory_alloc();
      *(undefined4 *)(iVar2 + 0x74) = uVar3;
    }
    if (DAT_0071d1f8 == 1) {
      FUN_0052b180(param_1,param_2,param_3,param_4,param_5,param_6);
      DAT_0071d1b1 = 1;
      return;
    }
    if (DAT_0071d1f8 == 0) {
      if (*(short *)(param_1 + 0x24) == 3) {
        (*DAT_007c0470)();
        return;
      }
      (*DAT_007c0474)(param_1,param_2,param_3,param_4,param_5,param_6);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
