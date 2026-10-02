// rasterizer_detail_objects_begin  (Ghidra: FUN_0051b3f0, unnamed)
// address 0x51b3f0, size 758 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: gated by the 0x00689404 toggle and render_local_view_count() <= 1 (a single local view). Sets
//   the fixed render state bundle for detail objects (alpha blend srcalpha/invsrcalpha, z test
//   lessequal without z writes, no fog, no culling), clamps and filters sampler 0, uploads six
//   vec4 at vertex shader register 0xd, binds the detail object vertex buffer (0x0071d1c8,
//   stride 0x14 = the detail_object vertex type 11) as stream 0 and sets the stage 0/1
//   combiners. It is the setup half of rasterizer_detail_objects_draw 0x51b890, whose records
//   index Scenario.detail_object_collection_palette (+0x3c0). The phase 2 summary called these
//   static decals. Ghidra lost every call argument; the calls and the constant block are read
//   from the raw code 0x51b3f0..0x51b6e5.
// register convention: none, __cdecl with no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device;                                     // 0x0071d174
extern void *rasterizer_detail_object_vertex_buffer;                // 0x0071d1c8
extern uint8_t console_debug_toggle_689404;                         // 0x00689404 detail objects enable
extern float console_debug_value_689430;                            // 0x00689430 UNSURE: copied into c14.w

extern int16_t render_local_view_count(void);                                  // 0x4c9220, UNSURE: local player count

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3d_set_stream_source_fn)(void *self, uint32_t stream, void *buffer, uint32_t offset, uint32_t stride);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}

static void rasterizer_set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}

static void rasterizer_set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, type, value);
}

void rasterizer_detail_objects_begin(void)
{
    float constants[24];

    if (console_debug_toggle_689404 == 0 || render_local_view_count() > 1) {
        return;
    }

    rasterizer_set_render_state(0x16, 1);                       // D3DRS_CULLMODE none
    rasterizer_set_render_state(0xa8, 7);                       // D3DRS_COLORWRITEENABLE rgb
    rasterizer_set_render_state(0x1b, 1);                       // D3DRS_ALPHABLENDENABLE
    rasterizer_set_render_state(0x13, 5);                       // D3DRS_SRCBLEND srcalpha
    rasterizer_set_render_state(0x14, 6);                       // D3DRS_DESTBLEND invsrcalpha
    rasterizer_set_render_state(0xab, 1);                       // D3DRS_BLENDOP add
    rasterizer_set_render_state(0xf, 0);                        // D3DRS_ALPHATESTENABLE
    rasterizer_set_render_state(7, 1);                          // D3DRS_ZENABLE
    rasterizer_set_render_state(0x17, 4);                       // D3DRS_ZFUNC lessequal
    rasterizer_set_render_state(0xe, 0);                        // D3DRS_ZWRITEENABLE
    rasterizer_set_render_state(0x1c, 0);                       // D3DRS_FOGENABLE
    rasterizer_set_sampler_state(0, 1, 3);                      // ADDRESSU clamp
    rasterizer_set_sampler_state(0, 2, 3);                      // ADDRESSV clamp
    rasterizer_set_sampler_state(0, 5, 2);                      // MAGFILTER linear
    rasterizer_set_sampler_state(0, 6, 2);                      // MINFILTER linear
    rasterizer_set_sampler_state(0, 7, 2);                      // MIPFILTER linear

    // c13..c18
    constants[0] = 255.01f;                                     // 0x437f028f
    constants[1] = 0.0f;
    constants[2] = 1.0f;
    constants[3] = 1.0f;
    constants[4] = 8.0f;
    constants[5] = 8.0f;
    constants[6] = 8.0f;
    constants[7] = console_debug_value_689430;
    constants[8] = 1.0f;
    constants[9] = 1.0f;
    constants[10] = 0.5f;
    constants[11] = 0.0f;
    constants[12] = 0.0f;
    constants[13] = 1.0f;
    constants[14] = -0.5f;
    constants[15] = 0.0f;
    constants[16] = 0.0f;
    constants[17] = 0.0f;
    constants[18] = -0.5f;
    constants[19] = 1.0f;
    constants[20] = 1.0f;
    constants[21] = 0.0f;
    constants[22] = 0.5f;
    constants[23] = 1.0f;
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, constants, 6);

    ((d3d_set_stream_source_fn)device_vtable()[0x190 / 4])(rasterizer_device, 0, rasterizer_detail_object_vertex_buffer, 0, 0x14);
    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);  // SetPixelShader(NULL)
    rasterizer_set_texture_stage_state(0, 1, 4);                // COLOROP modulate
    rasterizer_set_texture_stage_state(0, 2, 2);                // COLORARG1 texture
    rasterizer_set_texture_stage_state(0, 3, 0);                // COLORARG2 diffuse
    rasterizer_set_texture_stage_state(0, 4, 4);                // ALPHAOP modulate
    rasterizer_set_texture_stage_state(0, 5, 2);                // ALPHAARG1 texture
    rasterizer_set_texture_stage_state(0, 6, 0);                // ALPHAARG2 diffuse
    rasterizer_set_texture_stage_state(1, 1, 1);                // stage 1 COLOROP disable
    rasterizer_set_texture_stage_state(1, 4, 1);                // stage 1 ALPHAOP disable
}

#if 0
Original Ghidra decompilation (0x51b3f0):

void FUN_0051b3f0(void)

{
  short sVar1;
  int *piStack_134;
  undefined4 uStack_130;
  undefined4 uStack_12c;
  undefined4 uStack_128;
  int *piStack_124;
  undefined4 uStack_120;
  undefined4 uStack_11c;
  undefined4 uStack_118;
  int *piStack_114;
  undefined4 uStack_110;
  undefined4 uStack_10c;
  undefined4 uStack_108;
  int *piStack_104;
  undefined4 uStack_100;
  undefined4 uStack_fc;
  undefined4 uStack_f8;
  int *piStack_f4;
  undefined4 uStack_f0;
  undefined4 uStack_ec;
  undefined4 uStack_e8;
  int *piStack_e4;
  undefined4 uStack_e0;
  undefined4 uStack_dc;
  int *piStack_d8;
  undefined4 uStack_d4;
  undefined4 uStack_d0;
  int *piStack_cc;
  undefined4 uStack_c8;
  undefined4 uStack_c4;
  int *piStack_c0;
  undefined4 uStack_bc;
  undefined4 uStack_b8;
  int *piStack_b4;
  undefined4 uStack_b0;
  undefined4 uStack_ac;
  int *piStack_a8;
  undefined4 uStack_a4;
  undefined4 uStack_a0;
  int *piStack_9c;
  undefined4 uStack_98;
  undefined4 uStack_94;
  int *piStack_90;
  undefined4 uStack_8c;
  undefined4 uStack_88;
  int *piStack_84;
  undefined4 uStack_80;
  undefined4 uStack_7c;
  int *piStack_78;
  undefined4 uStack_74;
  undefined4 uStack_70;
  int *piStack_6c;
  undefined4 uStack_68;
  undefined4 uStack_64;
  
  if (DAT_00689404 != '\0') {
    uStack_64 = 0x51b405;
    sVar1 = FUN_004c9220();
    if (sVar1 < 2) {
      uStack_64 = 1;
      uStack_68 = 0x16;
      piStack_6c = DAT_0071d174;
      uStack_70 = 0x51b421;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_70 = 7;
      uStack_74 = 0xa8;
      piStack_78 = DAT_0071d174;
      uStack_7c = 0x51b436;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_7c = 1;
      uStack_80 = 0x1b;
      piStack_84 = DAT_0071d174;
      uStack_88 = 0x51b448;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_88 = 5;
      uStack_8c = 0x13;
      piStack_90 = DAT_0071d174;
      uStack_94 = 0x51b45a;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_94 = 6;
      uStack_98 = 0x14;
      piStack_9c = DAT_0071d174;
      uStack_a0 = 0x51b46c;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_a0 = 1;
      uStack_a4 = 0xab;
      piStack_a8 = DAT_0071d174;
      uStack_ac = 0x51b481;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_ac = 0;
      uStack_b0 = 0xf;
      piStack_b4 = DAT_0071d174;
      uStack_b8 = 0x51b493;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_b8 = 1;
      uStack_bc = 7;
      piStack_c0 = DAT_0071d174;
      uStack_c4 = 0x51b4a5;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_c4 = 4;
      uStack_c8 = 0x17;
      piStack_cc = DAT_0071d174;
      uStack_d0 = 0x51b4b7;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_d0 = 0;
      uStack_d4 = 0xe;
      piStack_d8 = DAT_0071d174;
      uStack_dc = 0x51b4c9;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_dc = 0;
      uStack_e0 = 0x1c;
      piStack_e4 = DAT_0071d174;
      uStack_e8 = 0x51b4db;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_e8 = 3;
      uStack_ec = 1;
      uStack_f0 = 0;
      piStack_f4 = DAT_0071d174;
      uStack_f8 = 0x51b4ef;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_f8 = 3;
      uStack_fc = 2;
      uStack_100 = 0;
      piStack_104 = DAT_0071d174;
      uStack_108 = 0x51b503;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_108 = 2;
      uStack_10c = 5;
      uStack_110 = 0;
      piStack_114 = DAT_0071d174;
      uStack_118 = 0x51b517;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_118 = 2;
      uStack_11c = 6;
      uStack_120 = 0;
      piStack_124 = DAT_0071d174;
      uStack_128 = 0x51b52b;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_128 = 2;
      uStack_12c = 7;
      uStack_130 = 0;
      piStack_134 = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0x114))();
      uStack_118 = DAT_00689430;
      piStack_134 = (int *)0x437f028f;
      uStack_130 = 0;
      uStack_12c = 0x3f800000;
      uStack_128 = 0x3f800000;
      piStack_124 = (int *)0x41000000;
      uStack_120 = 0x41000000;
      uStack_11c = 0x41000000;
      piStack_114 = (int *)0x3f800000;
      uStack_110 = 0x3f800000;
      uStack_10c = 0x3f000000;
      uStack_108 = 0;
      piStack_104 = (int *)0x0;
      uStack_100 = 0x3f800000;
      uStack_fc = 0xbf000000;
      uStack_f8 = 0;
      piStack_f4 = (int *)0x0;
      uStack_f0 = 0;
      uStack_ec = 0xbf000000;
      uStack_e8 = 0x3f800000;
      piStack_e4 = (int *)0x3f800000;
      uStack_e0 = 0;
      uStack_dc = 0x3f000000;
      piStack_d8 = (int *)0x3f800000;
      (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,&piStack_134,6);
      (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,DAT_0071d1c8,0,0x14);
      (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,0);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,6,0);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
