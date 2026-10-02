// rasterizer_set_default_render_states  (Ghidra: rasterizer_set_default_render_states, already
// named)
// address 0x5160d0, size 1845 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: a long, purely mechanical sequence of IDirect3DDevice9::SetRenderState (vtable
//   +0xe4) and ::SetTextureStageState (vtable +0x10c) calls with literal state/value pairs; three
//   states are conditioned on d3d_caps9.raster_caps bits (types/rasterizer.h: 0x04000000
//   DEPTHBIAS gates render state 0xc3, 0x02000000 SLOPESCALEDEPTHBIAS gates 0xaf) or computed
//   from it (state 0x30 <- bit 0x10000). Rewritten as two literal-pair tables plus the three
//   special-cased states, in the exact original order, rather than 90 separate call statements,
//   since every call shares the same device/vtable-slot shape.
// register convention: none -- __cdecl, no parameters.
// UNSURE: the individual D3DRENDERSTATETYPE/D3DSAMPLERSTATETYPE enum names are not asserted,
//   only their numeric values (as Ghidra shows them) and vtable slots.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device; // 0x0071d174
extern d3d_caps9 rasterizer_caps; // 0x007c10c0

typedef int32_t (__stdcall *d3d_set_render_state_fn)(void *device, uint32_t state, uint32_t value);
typedef int32_t (__stdcall *d3d_set_texture_stage_state_fn)(void *device, uint32_t stage, uint32_t type, uint32_t value);

static void set_render_state(uint32_t state, uint32_t value)
{
    void **vtable = *(void ***)rasterizer_device;
    ((d3d_set_render_state_fn)vtable[0xe4 / 4])(rasterizer_device, state, value);
}

static void set_render_states(const uint32_t pairs[][2], int count)
{
    int i;
    for (i = 0; i < count; i++) {
        set_render_state(pairs[i][0], pairs[i][1]);
    }
}

// Initializes the D3D device's render states and texture stage states to their default values after
// device creation/reset.
void __cdecl rasterizer_set_default_render_states(void)
{
    void **vtable;
    static const uint32_t table1[][2] = {
        {7, 1}, {0xe, 1}, {0x17, 4},
    };
    static const uint32_t table2[][2] = {
        {0x34, 0}, {0x35, 1}, {0x36, 1}, {0x37, 1}, {0x38, 8}, {0x39, 0}, {0x3a, 0}, {0x3b, 0},
        {0xf, 0}, {0x19, 5}, {0x18, 0}, {0x1b, 0}, {0x13, 2}, {0x14, 1}, {0xab, 1}, {0xce, 0},
        {0xcf, 2}, {0xd0, 1}, {0xd1, 1}, {0x1c, 0},
    };
    static const uint32_t table3[][2] = {
        {0x22, 0}, {0x23, 0}, {0x8c, 0}, {0x24, 0}, {0x25, 0}, {0x26, 0}, {0x9c, 1}, {0x9d, 0},
        {0x9a, 0}, {0x9b, 0}, {0xa6, 0}, {0x9e, 0}, {0x9f, 0}, {0xa0, 0}, {0xa8, 0xf}, {9, 2},
        {0x16, 3}, {8, 3}, {0x10, 0}, {0x1a, 0}, {0xa1, 1}, {0xa2, 0}, {0xa3, 0}, {0xa5, 0},
        {0x88, 1}, {0x98, 0}, {0x80, 0}, {0x81, 0}, {0x82, 0}, {0x83, 0}, {0x84, 0}, {0x85, 0},
        {0x86, 0}, {0x87, 0}, {0xc6, 0}, {199, 0}, {200, 0}, {0xc9, 0}, {0xca, 0}, {0xcb, 0},
        {0xcc, 0}, {0xcd, 0}, {0x3c, 0}, {0xc2, 0}, {0x97, 0}, {0xa7, 0}, {0xaa, 0}, {0x89, 0},
        {0x8b, 0}, {0x1d, 0}, {0x8d, 0}, {0x8e, 0}, {0x8f, 0}, {0x93, 0}, {0x91, 0}, {0x92, 0},
        {0x94, 0},
    };

    set_render_states(table1, sizeof(table1) / sizeof(table1[0]));

    if ((rasterizer_caps.raster_caps & 0x4000000) != 0) {
        set_render_state(0xc3, 0);
    }
    if ((rasterizer_caps.raster_caps & 0x2000000) != 0) {
        set_render_state(0xaf, 0);
    }

    set_render_states(table2, sizeof(table2) / sizeof(table2[0]));

    set_render_state(0x30, (rasterizer_caps.raster_caps >> 0x10) & 1);

    set_render_states(table3, sizeof(table3) / sizeof(table3[0]));

    vtable = *(void ***)rasterizer_device;
    {
        d3d_set_texture_stage_state_fn set_texture_stage_state = (d3d_set_texture_stage_state_fn)vtable[0x10c / 4];
        set_texture_stage_state(rasterizer_device, 0, 0xb, 0);
        set_texture_stage_state(rasterizer_device, 1, 0xb, 1);
        set_texture_stage_state(rasterizer_device, 2, 0xb, 2);
        set_texture_stage_state(rasterizer_device, 3, 0xb, 3);
        set_texture_stage_state(rasterizer_device, 0, 0x18, 0);
        set_texture_stage_state(rasterizer_device, 1, 0x18, 0);
        set_texture_stage_state(rasterizer_device, 2, 0x18, 0);
        set_texture_stage_state(rasterizer_device, 3, 0x18, 0);
    }
}

#if 0
Original Ghidra decompilation (0x5160d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl rasterizer_set_default_render_states(void)

{
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,4);
  if ((_DAT_007c10e4 & 0x4000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc3,0);
  }
  if ((_DAT_007c10e4 & 0x2000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaf,0);
  }
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x34,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x35,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x36,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x37,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x38,8);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x39,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3a,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3b,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x19,5);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,2);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xce,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xcf,2);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xd0,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xd1,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x30,_DAT_007c10e4 >> 0x10 & 1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x22,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x23,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x8c,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x24,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x25,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x26,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9c,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9d,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9a,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9b,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa6,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9e,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9f,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa0,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,0xf);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,9,2);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,8,3);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x10,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1a,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa1,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa2,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa3,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa5,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x88,1);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x98,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x80,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x81,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x82,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x83,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x84,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x85,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x86,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x87,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc6,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,199,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,200,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc9,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xca,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xcb,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xcc,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xcd,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc2,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x97,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa7,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaa,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x8b,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1d,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x8d,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x8e,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x8f,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x93,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x91,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x92,0);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x94,0);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,0xb,0);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,0xb,1);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,2,0xb,2);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,3,0xb,3);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,0x18,0);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,0x18,0);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,2,0x18,0);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,3,0x18,0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
