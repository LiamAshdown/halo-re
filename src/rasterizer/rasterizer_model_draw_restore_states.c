// rasterizer_model_draw_restore_states  (Ghidra: FUN_0052b530, unnamed)
// address 0x52b530, size 255 bytes
// name confidence: 0.4   rewrite confidence: 0.75
//   Phase 4 rename: the counterpart of rasterizer_model_draw_prepare_states 0x526f50 (clears the
//   active model context); it is not specific to shader_environment.
// evidence: mirrors the frustum-z-refresh gate at the top of rasterizer_model_draw_prepare_states
//   (0x526f50) exactly (same globals, same sign-bit-of-flags-byte test), then, for pre-ps_1_1
//   devices with the model context's node_parts_fixed_function_fog flag set, resets the world
//   transform to the identity matrix, and finally clears rasterizer_active_model_context.
// Phase 4 review fix: the stage configuration restored is mode 2 (the earlier file had 1).
// register convention: none -- no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t console_debug_toggle_6893ec;             // 0x006893ec UNSURE, see rasterizer_model_draw_prepare_states.c
extern rasterizer_model_draw_context *rasterizer_active_model_context; // 0x0071d1f0
extern uint8_t unknown_0071d1fd;                          // 0x0071d1fd UNSURE, see rasterizer_model_draw_prepare_states.c
extern d3d_caps9 rasterizer_caps;                         // 0x007c10c0
extern void *rasterizer_device;                           // 0x0071d174

extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200
extern void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far); // 0x518f40

typedef int32_t (*d3d_set_transform_fn)(void *device, uint32_t state, const void *matrix);

// Performs end-of-pass cleanup for the shader_environment renderer, restoring shared render state
// and clearing the active-object pointer.
void rasterizer_model_draw_restore_states(void)
{
    rasterizer_model_draw_context *context = (rasterizer_model_draw_context *)rasterizer_active_model_context;

    if (console_debug_toggle_6893ec == 0) {
        return;
    }

    if ((int8_t)context->flags < 0 && unknown_0071d1fd == 0) {
        rasterizer_set_shader_stage_config(2);   // mov eax, 2 at 0x52b553
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }

    if (rasterizer_caps.pixel_shader_version < 0xffff0101 && (context->flags & 0x200) != 0) {
        float identity[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        void **vtable = *(void ***)rasterizer_device;
        ((d3d_set_transform_fn)vtable[0xb0 / 4])(rasterizer_device, 0x100 /* D3DTS_WORLD */, identity);
    }

    rasterizer_active_model_context = 0;
}

#if 0
Original Ghidra decompilation (0x52b530):

void FUN_0052b530(void)

{
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  if (DAT_006893ec != '\0') {
    if (((char)*DAT_0071d1f0 < '\0') && (DAT_0071d1fd == '\0')) {
      rasterizer_set_shader_stage_config();
      chimera__rasterizer_set_frustum_z_func(0,0);
    }
    if ((DAT_007c118c < 0xffff0101) && ((*DAT_0071d1f0 & 0x200) != 0)) {
      local_8 = 0;
      local_c = 0;
      local_10 = 0;
      local_14 = 0;
      local_1c = 0;
      local_20 = 0;
      local_24 = 0;
      local_28 = 0;
      local_30 = 0;
      local_34 = 0;
      local_38 = 0;
      local_3c = 0;
      local_4 = 0x3f800000;
      local_18 = 0x3f800000;
      local_2c = 0x3f800000;
      local_40 = 0x3f800000;
      (**(code **)(*DAT_0071d174 + 0xb0))(DAT_0071d174,0x100,&local_40);
    }
    DAT_0071d1f0 = (uint *)0x0;
  }
  return;
}
#endif
