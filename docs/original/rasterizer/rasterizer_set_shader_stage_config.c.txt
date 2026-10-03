// rasterizer_set_shader_stage_config  (Ghidra: rasterizer_set_shader_stage_config, already
// named)
// address 0x519200, size 704 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: a mode-keyed table of SetRenderState (vtable +0xe4) calls for texture stage states
//   (0x34-0x3a, 0x3b), cached against rasterizer_shader_stage_config (0x0069c6ac,
//   types/rasterizer.h) to skip redundant re-application; matches its own name exactly.
// register convention: mode in in_AX (the 16-bit half of EAX). // blam-cc: AX -> mode
// UNSURE: a debug toggle (0x006893ef) forces mode to 0 when clear; not documented in
//   types/rasterizer.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t unknown_006893ef; // 0x006893ef UNSURE: debug toggle
extern int16_t rasterizer_shader_stage_config; // 0x0069c6ac
extern void *rasterizer_device; // 0x0071d174

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

// blam-cc: AX -> mode
// Applies a cached, mode-dependent bundle of texture stage render states, doing nothing if
// `mode` already matches the cached configuration.
void rasterizer_set_shader_stage_config(int16_t mode)
{
    uint32_t value_3a;
    uint32_t final_state;
    uint32_t final_value;

    if (unknown_006893ef == 0) {
        mode = 0;
    }
    if (mode == rasterizer_shader_stage_config) {
        return;
    }

    // FIXED (objdump 0x519237..0x5194b1): the device and its vtable are loaded per call inside the cases, so an
    // out-of-range mode (the default path, 0x519221 ja 0x5194b7) never touches the device
    #define set_render_state(device, state, value)         ((d3d_set_render_state_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, (state), (value))

    switch (mode) {
    case 0:
        final_state = 0x34;
        final_value = 0;
        goto apply_final;
    case 1:
        set_render_state(rasterizer_device, 0x34, 1);
        set_render_state(rasterizer_device, 0x35, 1);
        set_render_state(rasterizer_device, 0x36, 1);
        set_render_state(rasterizer_device, 0x37, 3);
        set_render_state(rasterizer_device, 0x38, 8);
        set_render_state(rasterizer_device, 0x39, 1);
        set_render_state(rasterizer_device, 0x3a, 1);
        final_value = 1;
        goto set_3b;
    case 2:
        set_render_state(rasterizer_device, 0x34, 1);
        set_render_state(rasterizer_device, 0x35, 1);
        set_render_state(rasterizer_device, 0x36, 1);
        set_render_state(rasterizer_device, 0x37, 1);
        set_render_state(rasterizer_device, 0x38, 3);
        set_render_state(rasterizer_device, 0x39, 0);
        value_3a = 1;
        break;
    case 3:
        set_render_state(rasterizer_device, 0x34, 1);
        set_render_state(rasterizer_device, 0x35, 1);
        set_render_state(rasterizer_device, 0x36, 1);
        set_render_state(rasterizer_device, 0x37, 1);
        set_render_state(rasterizer_device, 0x38, 6);
        set_render_state(rasterizer_device, 0x39, 0);
        value_3a = 1;
        break;
    case 4:
        set_render_state(rasterizer_device, 0x34, 1);
        set_render_state(rasterizer_device, 0x35, 1);
        set_render_state(rasterizer_device, 0x36, 1);
        set_render_state(rasterizer_device, 0x37, 3);
        set_render_state(rasterizer_device, 0x38, 3);
        set_render_state(rasterizer_device, 0x39, 2);
        set_render_state(rasterizer_device, 0x3a, 1);
        final_value = 2;
        goto set_3b;
    case 5:
        set_render_state(rasterizer_device, 0x34, 1);
        set_render_state(rasterizer_device, 0x35, 1);
        set_render_state(rasterizer_device, 0x36, 1);
        set_render_state(rasterizer_device, 0x37, 1);
        set_render_state(rasterizer_device, 0x38, 3);
        set_render_state(rasterizer_device, 0x39, 0);
        value_3a = 3;
        break;
    default:
        goto done;
    }
    set_render_state(rasterizer_device, 0x3a, value_3a);
    final_value = 0;
set_3b:
    final_state = 0x3b;
apply_final:
    set_render_state(rasterizer_device, final_state, final_value);
done:
    rasterizer_shader_stage_config = mode;
    #undef set_render_state
}

#if 0
Original Ghidra decompilation (0x519200):

void rasterizer_set_shader_stage_config(void)

{
  short in_AX;
  undefined4 uVar1;
  undefined4 uVar2;

  if (DAT_006893ef == '\0') {
    in_AX = 0;
  }
  if (in_AX == DAT_0069c6ac) {
    return;
  }
  switch(in_AX) {
  case 0:
    uVar1 = 0;
    uVar2 = 0x34;
    goto LAB_005194a9;
  case 1:
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x34,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x35,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x36,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x37,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x38,8);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x39,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3a,1);
    uVar1 = 1;
    goto LAB_005194a7;
  case 2:
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x34,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x35,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x36,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x37,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x38,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x39,0);
    uVar1 = 1;
    break;
  case 3:
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x34,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x35,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x36,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x37,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x38,6);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x39,0);
    uVar1 = 1;
    break;
  case 4:
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x34,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x35,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x36,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x37,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x38,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x39,2);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3a,1);
    uVar1 = 2;
    goto LAB_005194a7;
  case 5:
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x34,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x35,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x36,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x37,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x38,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x39,0);
    uVar1 = 3;
    break;
  default:
    goto switchD_00519227_default;
  }
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3a,uVar1);
  uVar1 = 0;
LAB_005194a7:
  uVar2 = 0x3b;
LAB_005194a9:
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,uVar2,uVar1);
switchD_00519227_default:
  DAT_0069c6ac = in_AX;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
