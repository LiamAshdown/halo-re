// render_lighting_disable_workaround  (Ghidra: FUN_00511ef0; new name, evidence below)
// address 0x511ef0, size 43 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: types/rasterizer.h globals note the SetRenderState vtable slot (+0xe4) and state
//   0x89 = D3DRS_LIGHTING; render_player_frame.c already names 0x007c118c
//   rasterizer_device_version and 0x0071d174 rasterizer_device (see
//   chimera__rasterizer_set_framebuffer_blend_function.c). Forces fixed-function lighting off
//   for pre-DX9.0a-ish device versions on hardware that needs the debug toggle set.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t console_debug_toggle_6893ec; // 0x006893ec (matches src/rasterizer/rasterizer_model_draw_prepare_states.c)
extern uint32_t rasterizer_device_version;  // 0x007c118c (matches src/render/render_player_frame.c)
extern void *rasterizer_device;             // 0x0071d174

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);

// Disables fixed-function D3D lighting (D3DRS_LIGHTING = 0x89) when the debug toggle is set and
// the device is older than version 0xffff0101.
void render_lighting_disable_workaround(void)
{
    if (console_debug_toggle_6893ec != 0 && rasterizer_device_version < 0xffff0101) {
        void **vtable = *(void ***)rasterizer_device;
        d3d_set_render_state_fn set_render_state = (d3d_set_render_state_fn)vtable[0xe4 / 4];
        set_render_state(rasterizer_device, 0x89, 0);
    }
}

#if 0
Original Ghidra decompilation (0x511ef0):

void FUN_00511ef0(void)

{
  if ((DAT_006893ec != '\0') && (DAT_007c118c < 0xffff0101)) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,0);
  }
  return;
}
#endif
