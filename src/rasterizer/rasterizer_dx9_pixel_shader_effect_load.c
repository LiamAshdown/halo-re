// rasterizer_dx9_pixel_shader_effect_load  (Ghidra: rasterizer_dx9_pixel_shader_effect_load, already named)
// address 0x52f980, size 123 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: functions.md summary ("Loads/compiles one pixel-shader effect chunk (param_1=data,
//   param_2=size) into the effect-slot table for the current shader index (register EAX),
//   reporting a fatal error referencing shaders\fx.bin"); FUN_00583f26's 9-argument shape
//   (device, data, size, defines, include, flags, pool, ppEffect, ppErrorBuffer) matches
//   D3DXCreateEffect exactly.
// register convention: EAX -> effect_index; (data, size) on the stack.

#include "tags.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                      // 0x0071d174
extern void *rasterizer_effect_pool;                                  // 0x0071d254
extern d3dx_macro rasterizer_effect_defines[2]; // 0x007c0460
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern const char *rasterizer_shader_file_name;                        // 0x00722bbc UNSURE: last
                                                                        //   shader file name; used
                                                                        //   by the fatal error dialog
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

// blam-cc: D3DXCreateEffect(device, data, size, defines, include, flags, pool, ppEffect, ppErrorBuffer)
extern int32_t D3DXCreateEffect(void *device, const void *data, uint32_t size, const void *defines,
                                void *include, uint32_t flags, void *pool, void *out_effect,
                                void **out_error_buffer); // 0x583f26

// Compiles one pixel-shader effect chunk into rasterizer_effects[effect_index].effect. On failure,
// records "shaders\fx.bin" as the failing file name and raises a fatal error dialog. Any returned
// compilation-error buffer is released either way.
int32_t rasterizer_dx9_pixel_shader_effect_load(int32_t effect_index, const void *data, uint32_t size)
{
    int32_t hr;
    void *error_buffer;

    rasterizer_effects[effect_index].effect = 0;
    error_buffer = 0;
    hr = D3DXCreateEffect(rasterizer_device, data, size, rasterizer_effect_defines, 0, 0,
                          rasterizer_effect_pool, &rasterizer_effects[effect_index].effect, &error_buffer);
    if (hr < 0) {
        rasterizer_shader_file_name = "shaders\\fx.bin";
        shell_display_fatal_error_dialog(0x69, 0x7e, 1);
    }
    if (error_buffer != 0) {
        ((void (__stdcall *)(void *))(*(void ***)error_buffer)[2])(error_buffer); // Release()
    }
    return hr >= 0;
}

#if 0
Original Ghidra decompilation (0x52f980):

bool rasterizer_dx9_pixel_shader_effect_load(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int in_EAX;
  int iVar3;
  int *local_4;

  uVar2 = DAT_0071d254;
  uVar1 = DAT_0071d174;
  (&DAT_0069d410)[in_EAX * 8] = 0;
  local_4 = (int *)0x0;
  iVar3 = FUN_00583f26(uVar1,param_1,param_2,&DAT_007c0460,0,0,uVar2,&DAT_0069d410 + in_EAX * 8,
                       &local_4);
  if (iVar3 < 0) {
    DAT_00722bbc = "shaders\\fx.bin";
    shell_display_fatal_error_dialog(0x69,0x7e,1);
  }
  if (local_4 != (int *)0x0) {
    (**(code **)(*local_4 + 8))(local_4);
  }
  return -1 < iVar3;
}
#endif
