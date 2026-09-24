// rasterizer_light_disable_all  (Ghidra: already named)
// address 0x526700, size 93 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: types/rasterizer.h globals: 0x007c118c is rasterizer_caps.pixel_shader_version
//   (compared against 0xffff0101, ps_1_1, throughout this module), 0x007c1160 is
//   rasterizer_caps.max_active_lights, 0x0071d174 is rasterizer_device, 0x0069c708 is the 0x44
//   byte D3DMATERIAL9 rasterizer_default_material, and 0x007c3084 is
//   rasterizer_fixed_function_light_count. Device vtable+0xc4 is SetMaterial (D3DMATERIAL9*) and
//   +0xd4 is LightEnable(DWORD Index, BOOL Enable), matching IDirect3DDevice9's SDK vtable layout
//   (index 49 and 53 respectively).
// register convention: none -- no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;              // 0x007c10c0
extern void *rasterizer_device;                // 0x0071d174
extern uint8_t rasterizer_default_material[0x44]; // 0x0069c708 D3DMATERIAL9
extern int32_t rasterizer_fixed_function_light_count; // 0x007c3084

typedef int32_t (__stdcall *d3d_set_material_fn)(void *device, const void *material);
typedef int32_t (__stdcall *d3d_light_enable_fn)(void *device, uint32_t index, int32_t enable);

// Resets the default material and disables every fixed-function Direct3D light, for the
// pre-pixel-shader lighting fallback used when the device has no ps_1_1 support.
void rasterizer_light_disable_all(void)
{
    uint32_t i;
    void **vtable;

    if (rasterizer_caps.pixel_shader_version < 0xffff0101 && rasterizer_caps.max_active_lights != 0) {
        vtable = *(void ***)rasterizer_device;
        ((d3d_set_material_fn)vtable[0xc4 / 4])(rasterizer_device, rasterizer_default_material);

        rasterizer_fixed_function_light_count = 0;

        i = 0;
        if (rasterizer_caps.max_active_lights != 0) {
            do {
                vtable = *(void ***)rasterizer_device;
                ((d3d_light_enable_fn)vtable[0xd4 / 4])(rasterizer_device, i, 0);
                i = i + 1;
            } while (i < rasterizer_caps.max_active_lights);
        }
    }
}

#if 0
Original Ghidra decompilation (0x526700):

/* WARNING: Unknown calling convention -- yet parameter storage is locked */

void rasterizer_light_disable_all(void)

{
  uint uVar1;

  if ((DAT_007c118c < 0xffff0101) && (DAT_007c1160 != 0)) {
    (**(code **)(*DAT_0071d174 + 0xc4))(DAT_0071d174,&DAT_0069c708);
    uVar1 = 0;
    DAT_007c3084 = 0;
    if (DAT_007c1160 != 0) {
      do {
        (**(code **)(*DAT_0071d174 + 0xd4))(DAT_0071d174,uVar1,0);
        uVar1 = uVar1 + 1;
      } while (uVar1 < DAT_007c1160);
    }
  }
  return;
}
#endif
