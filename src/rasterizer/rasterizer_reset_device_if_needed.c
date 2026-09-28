// rasterizer_reset_device_if_needed  (Ghidra: rasterizer_reset_device_if_needed, already named)
// address 0x517500, size 177 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: resets several pre-ps_1_1 debug toggles, then when rasterizer_device_lost is set
//   copies the cached present parameters onto the stack and calls rasterizer_device_reset with
//   it (matching that function's own extra_params/local_38 pairing), then TestCooperativeLevel
//   (device vtable +0xa4) and sets the "update pending" flag rasterizer_service_deferred_
//   windowed_ops.c reads.
// register convention: none -- __cdecl, no parameters.
// UNSURE: the seven 0x006893xx/0x0068941d debug toggles reset for pre-ps_1_1 hardware are not
//   documented in types/rasterizer.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps; // 0x007c10c0
extern uint8_t unknown_006893f6, unknown_006893f7, unknown_006893f8, console_debug_toggle_6893f9; // UNSURE debug toggles
extern uint8_t unknown_006893fd, unknown_0068941d, console_debug_toggle_6893f2; // UNSURE debug toggles
extern uint8_t rasterizer_device_lost; // 0x007c10b0
extern d3d_present_parameters rasterizer_present_parameters; // 0x007c04a0
extern void *rasterizer_device; // 0x0071d174
extern uint8_t rasterizer_in_scene;                                 // 0x0071d16f set after BeginScene, cleared after EndScene

extern uint8_t rasterizer_device_reset(uint32_t *present_parameters); // 0x515d90, stack -> present_parameters

typedef int32_t (__stdcall *d3d_call0_fn)(void *device);

// Resets pre-ps_1_1 debug toggles, resets the D3D device if it was flagged lost, and reports
// whether the device is usable afterward (also flagging a deferred update when it is).
uint8_t __cdecl rasterizer_reset_device_if_needed(void)
{
    uint8_t usable = 1;
    int32_t hr;
    void **vtable;

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        unknown_006893f6 = 0;
        unknown_006893f7 = 0;
        unknown_006893f8 = 0;
        console_debug_toggle_6893f9 = 0;
        unknown_006893fd = 0;
        unknown_0068941d = 0;
        console_debug_toggle_6893f2 = 0;
    }

    if (rasterizer_device_lost != 0) {
        uint32_t present_params_copy[14];
        int32_t i;
        uint32_t *src = (uint32_t *)&rasterizer_present_parameters;
        for (i = 0; i < 0xe; i++) {
            present_params_copy[i] = src[i];
        }
        rasterizer_device_lost = (uint8_t)(1 - (rasterizer_device_reset(present_params_copy) != 0));
        usable = (rasterizer_device_lost == 0);
        if (rasterizer_device_lost != 0) {
            return usable;
        }
    }

    vtable = *(void ***)rasterizer_device;
    hr = ((d3d_call0_fn)vtable[0xa4 / 4])(rasterizer_device);
    if (hr < 0) {
        return 0;
    }
    if (usable == 0) {
        return 0;
    }
    rasterizer_in_scene = 1;
    return usable;
}

#if 0
Original Ghidra decompilation (0x517500):

bool __cdecl rasterizer_reset_device_if_needed(void)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  bool bVar5;
  undefined4 local_38 [14];

  bVar5 = true;
  if (DAT_007c118c < 0xffff0101) {
    DAT_006893f6 = 0;
    DAT_006893f7 = 0;
    DAT_006893f8 = 0;
    DAT_006893f9 = 0;
    DAT_006893fd = 0;
    DAT_0068941d = 0;
    DAT_006893f2 = 0;
  }
  if (DAT_007c10b0 != '\0') {
    puVar3 = &DAT_007c04a0;
    puVar4 = local_38;
    for (iVar2 = 0xe; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    cVar1 = rasterizer_device_reset(local_38);
    DAT_007c10b0 = '\x01' - (cVar1 != '\0');
    bVar5 = DAT_007c10b0 == '\0';
    if (DAT_007c10b0 != '\0') {
      return bVar5;
    }
  }
  iVar2 = (**(code **)(*DAT_0071d174 + 0xa4))(DAT_0071d174);
  if (iVar2 < 0) {
    return false;
  }
  if (bVar5 == false) {
    return false;
  }
  DAT_0071d16f = 1;
  return bVar5;
}
#endif
