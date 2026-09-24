// fg_render  (Ghidra: rasterizer_frame_statistics_draw_graph 0x5129a0, plus the tail Ghidra split
// off as a separate function at 0x512b80 and misnamed object_render_state_refresh; CEA
// fg_render(render_graph, render_infos), hint only; renamed)
// address 0x5129a0, size 1004 bytes (0x5129a0..0x512d8b; Ghidra's 0x5129a0 entry covers 480
//   bytes and the rest is its "function" at 0x512b80, which is not a function: nothing calls
//   0x512b80, it is the fall through after the call at 0x512b7d)
// name confidence: 0.7   rewrite confidence: 0.75
// evidence: objdump -d -M intel 0x5129a0..0x512d8b; Ghidra's decompile turns the D3D call
//   return addresses into fake stack locals and invents the text colour stores, so this is
//   written from the disassembly. Device calls go through the IDirect3DDevice9 vtable
//   (0x0071d174): +0x15c SetVertexDeclaration, +0x134 SetSoftwareVertexProcessing, +0x170
//   SetVertexShader, +0x178 SetVertexShaderConstantF, +0x1ac SetPixelShader, +0xe4
//   SetRenderState, +0x10c SetTextureStageState, +0x104 SetTexture, +0x14c DrawPrimitiveUP.
//   - vertex declaration / usage = rasterizer_vertex_declarations[8] (0x006e1af0 / 0x006e1af8,
//     _rasterizer_vertex_type_dynamic_screen), vertex shader = rasterizer_vertex_shaders[35]
//     (0x0069e468), with the (flag ? 0x10 : 0 | usage) & 0x10 idiom of
//     src/rasterizer/rasterizer_decals_draw_cluster.c.
//   - c13..c17 (5 float4 on the stack): c[0] = 2/w, c[3] = -1 - 1/w, c[5] = -2/h,
//     c[7] = 1 + 1/h, c[11] = 0.5, c[15..17] = 1, c[19] = 1, everything else 0, where w and h
//     are the width and height of rasterizer_window.camera.viewport_bounds (0x007c1254..
//     0x007c125a): the pixel to clip space transform for screen vertices. -1.0 and -2.0 are
//     the constants at 0x00672ba8 / 0x00672bb0.
//   - render states: CULLMODE (0x16) NONE, COLORWRITEENABLE (0xa8) 0xf, ALPHABLENDENABLE (0x1b)
//     0, ALPHATESTENABLE (0x0f) 0, ZENABLE (7) 0, ZWRITEENABLE (0x0e) 0, FOGENABLE (0x1c) 0,
//     0x9c POINTSPRITEENABLE 1, 0x9d POINTSCALEENABLE 0; stage 0 colour and alpha
//     SELECTARG2 of DIFFUSE, stage 1 disabled, texture 0 NULL; pixel shader NULL and
//     rasterizer_set_shader_stage_config 0x519200 (AX = 0).
//   - render_graph (BL): DrawPrimitiveUP(LINESTRIP, 0x1ff, frame_graphs[0].vertices, 0x18) and
//     (LINESTRIP, 4, frame_graphs[0].frame_vertices, 0x18).
//   - hud_text_draw_configure 0x4944c0 (1, -1, 0, 0, 5, 0), then the text colour is forced to
//     opaque white (the four 1.0 locals at esp+0x04..0x10 copied to 0x006e4738..0x006e4744)
//     and hud_text_draw_background_mode (0x006e4748) to 0; this happens for render_infos too.
//   - the labels: chimera__draw_8_bit_text 0x5148b0 with EAX = 0 (no clip override), ECX = the
//     label rectangle, stack (0, 0, text): name ("FPS", 0x006bc310) in name_bounds, sprintf
//     0x623693 "%d" (0x0065fb30) of (int)maximum in maximum_bounds, (int)average in
//     average_bounds, both through the 0x200 byte stack buffer.
//   - finally SetSoftwareVertexProcessing(rasterizer_software_vertex_processing).
// register convention: BL = render_graph, AL = render_infos (types/render.h; the caller
//   rasterizer_frame_statistics_draw 0x512e80 loads both from 0x0071d128 / 0x0071d12c).
//   // blam-cc: BL=render_graph, AL=render_infos
// UNSURE: render_infos only enables the shared device setup and the text colour reset; it
//   draws nothing itself here (the table of rates is drawn by 0x512e80).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

extern void *rasterizer_device;                         // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;  // 0x007c1220, rasterizer module
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count];
    // 0x006e1a90, rasterizer module
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders];
    // 0x0069e350, rasterizer module
extern uint8_t rasterizer_software_vertex_processing;   // 0x0069c680, rasterizer module
extern frame_graph frame_graphs[1];                     // 0x006b9260, this module

extern float hud_text_draw_color_a;                     // 0x006e4738, interface module
extern float hud_text_draw_color_r;                     // 0x006e473c
extern float hud_text_draw_color_g;                     // 0x006e4740
extern float hud_text_draw_color_b;                     // 0x006e4744
extern int16_t hud_text_draw_background_mode;           // 0x006e4748

extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200; blam-cc: AX -> mode
extern void hud_text_draw_configure(int16_t font_table_index, uint16_t color_or_flags,
    int16_t column, uint32_t unknown_4730, int16_t color_table_index, int16_t color_index);
    // 0x4944c0, interface module (cdecl)
extern void chimera__draw_8_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override,
    Point2DInt *cursor, int32_t flags, const char *text);
    // 0x5148b0, rasterizer module; blam-cc: EAX -> clip_rect_override, ECX -> dest_rect_override,
    // stack -> (cursor, flags, text). src/rasterizer types the two stack slots as opaque
    // uint32_t position_or_color1/2; 0x512e80 passes a Point2DInt out cursor and -4 there.
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 _sprintf

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data,
                                         uint32_t count);
typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t primitive_type,
                                            uint32_t primitive_count, const void *data,
                                            uint32_t stride);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}

// Draws the frame rate graph (when render_graph is set) as two screen space line strips plus its
// "FPS" / maximum / average labels, after putting the device into a plain vertex colour, no
// depth, no blend state.
void fg_render(uint8_t render_graph, uint8_t render_infos) // blam-cc: BL=render_graph, AL=render_infos
{
    float white[4];
    float constants[20];
    char text[0x200];
    float inverse;
    int16_t width;
    int16_t height;

    white[0] = 1.0f;
    white[1] = 1.0f;
    white[2] = 1.0f;
    white[3] = 1.0f;
    if (!render_graph && !render_infos) {
        return;
    }

    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
        rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
        ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
         rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[35].shader);

    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right -
                      rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom -
                       rasterizer_window.camera.viewport_bounds.top);
    inverse = 1.0f / (float)width;
    constants[0] = inverse + inverse;
    constants[1] = 0.0f;
    constants[2] = 0.0f;
    constants[3] = -1.0f - inverse;
    inverse = 1.0f / (float)height;
    constants[4] = 0.0f;
    constants[5] = -2.0f * inverse;
    constants[6] = 0.0f;
    constants[7] = inverse + 1.0f;
    constants[8] = 0.0f;
    constants[9] = 0.0f;
    constants[10] = 0.0f;
    constants[11] = 0.5f;
    constants[12] = 0.0f;
    constants[13] = 0.0f;
    constants[14] = 0.0f;
    constants[15] = 1.0f;
    constants[16] = 1.0f;
    constants[17] = 1.0f;
    constants[18] = 0.0f;
    constants[19] = 1.0f;
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 13, constants, 5);

    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);
    rasterizer_set_shader_stage_config(0);
    set_render_state(0x16, 1);    // D3DRS_CULLMODE, D3DCULL_NONE
    set_render_state(0xa8, 0xf);  // D3DRS_COLORWRITEENABLE, rgba
    set_render_state(0x1b, 0);    // D3DRS_ALPHABLENDENABLE
    set_render_state(0x0f, 0);    // D3DRS_ALPHATESTENABLE
    set_render_state(0x07, 0);    // D3DRS_ZENABLE
    set_render_state(0x0e, 0);    // D3DRS_ZWRITEENABLE
    set_render_state(0x1c, 0);    // D3DRS_FOGENABLE
    set_texture_stage_state(0, 1, 3); // COLOROP SELECTARG2
    set_texture_stage_state(0, 3, 0); // COLORARG2 DIFFUSE
    set_texture_stage_state(0, 4, 3); // ALPHAOP SELECTARG2
    set_texture_stage_state(0, 6, 0); // ALPHAARG2 DIFFUSE
    set_texture_stage_state(1, 1, 1); // COLOROP DISABLE
    set_texture_stage_state(1, 4, 1); // ALPHAOP DISABLE
    ((d3d_call2_fn)device_vtable()[0x104 / 4])(rasterizer_device, 0, 0);
    set_render_state(0x9c, 1);    // D3DRS_POINTSPRITEENABLE
    set_render_state(0x9d, 0);    // D3DRS_POINTSCALEENABLE

    if (render_graph) {
        ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 3, 0x1ff,
            frame_graphs[0].vertices, sizeof(rasterizer_dynamic_screen_vertex));
        ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 3, 4,
            frame_graphs[0].frame_vertices, sizeof(rasterizer_dynamic_screen_vertex));
    }

    hud_text_draw_configure(1, 0xffff, 0, 0, 5, 0);
    hud_text_draw_color_a = white[0];
    hud_text_draw_background_mode = 0;
    hud_text_draw_color_r = white[1];
    hud_text_draw_color_g = white[2];
    hud_text_draw_color_b = white[3];

    if (render_graph) {
        chimera__draw_8_bit_text(0, (int32_t *)&frame_graphs[0].name_bounds, 0, 0,
                                 frame_graphs[0].name);
        sprintf(text, "%d", (int32_t)frame_graphs[0].maximum);
        chimera__draw_8_bit_text(0, (int32_t *)&frame_graphs[0].maximum_bounds, 0, 0, text);
        sprintf(text, "%d", (int32_t)frame_graphs[0].average);
        chimera__draw_8_bit_text(0, (int32_t *)&frame_graphs[0].average_bounds, 0, 0, text);
    }

    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x5129a0):

void rasterizer_frame_statistics_draw_graph(void)

{
  int *piVar1;
  char in_AL;
  undefined4 uVar2;
  char unaff_BL;
  char acStack_308 [12];
  int *piStack_2fc;
  undefined4 uStack_2f8;
  undefined4 uStack_2f4;
  undefined4 uStack_2f0;
  int *piStack_2ec;
  undefined4 uStack_2e8;
  undefined4 uStack_2e4;
  int *piStack_2e0;
  undefined4 uStack_2dc;
  undefined4 uStack_2d8;
  int *piStack_2d4;
  undefined4 uStack_2d0;
  undefined4 uStack_2cc;
  int *piStack_2c8;
  undefined4 uStack_2c4;
  undefined4 uStack_2c0;
  int *piStack_2bc;
  undefined4 uStack_2b8;
  undefined4 uStack_2b4;
  int *piStack_2b0;
  undefined4 uStack_2ac;
  undefined4 uStack_2a8;
  int *piStack_2a4;
  undefined4 uStack_2a0;
  undefined4 uStack_29c;
  int *piStack_298;
  undefined4 uStack_294;
  int *piStack_290;
  undefined4 uStack_28c;
  undefined1 *puStack_288;
  undefined4 uStack_284;
  int *piStack_280;
  int iStack_27c;
  int *piStack_278;
  uint uStack_274;
  int *piStack_270;
  undefined4 uStack_26c;
  
  if ((unaff_BL != '\0') || (in_AL != '\0')) {
    uStack_26c = DAT_006e1af0;
    piStack_270 = DAT_0071d174;
    uStack_274 = 0x5129e8;
    (**(code **)(*DAT_0071d174 + 0x15c))();
    uStack_274 = -(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10;
    piStack_278 = DAT_0071d174;
    iStack_27c = 0x512a0f;
    (**(code **)(*DAT_0071d174 + 0x134))();
    iStack_27c = DAT_0069e468;
    piStack_280 = DAT_0071d174;
    uStack_284 = 0x512a24;
    (**(code **)(*DAT_0071d174 + 0x170))();
    iStack_27c = (int)(short)((short)DAT_007c1258 - (short)DAT_007c1254);
    uStack_284 = 5;
    puStack_288 = &stack0xfffffd98;
    uStack_28c = 0xd;
    piStack_290 = DAT_0071d174;
    uStack_294 = 0x512b24;
    (**(code **)(*DAT_0071d174 + 0x178))();
    uStack_294 = 0;
    piStack_298 = DAT_0071d174;
    uStack_29c = 0x512b34;
    (**(code **)(*DAT_0071d174 + 0x1ac))();
    uStack_29c = 0x512b3b;
    rasterizer_set_shader_stage_config();
    uStack_29c = 1;
    uStack_2a0 = 0x16;
    piStack_2a4 = DAT_0071d174;
    uStack_2a8 = 0x512b4d;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_2a8 = 0xf;
    uStack_2ac = 0xa8;
    piStack_2b0 = DAT_0071d174;
    uStack_2b4 = 0x512b62;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_2b4 = 0;
    uStack_2b8 = 0x1b;
    piStack_2bc = DAT_0071d174;
    uStack_2c0 = 0x512b74;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_2c0 = 0;
    uStack_2c4 = 0xf;
    piStack_2c8 = DAT_0071d174;
    uStack_2cc = 0x512b86;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_2cc = 0;
    uStack_2d0 = 7;
    piStack_2d4 = DAT_0071d174;
    uStack_2d8 = 0x512b98;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_2d8 = 0;
    uStack_2dc = 0xe;
    piStack_2e0 = DAT_0071d174;
    uStack_2e4 = 0x512baa;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_2e4 = 0;
    uStack_2e8 = 0x1c;
    piStack_2ec = DAT_0071d174;
    uStack_2f0 = 0x512bbc;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_2f0 = 3;
    uStack_2f4 = 1;
    uStack_2f8 = 0;
    piStack_2fc = DAT_0071d174;
    acStack_308[8] = -0x30;
    acStack_308[9] = '+';
    acStack_308[10] = 'Q';
    acStack_308[0xb] = '\0';
    (**(code **)(*DAT_0071d174 + 0x10c))();
    acStack_308[8] = '\0';
    acStack_308[9] = '\0';
    acStack_308[10] = '\0';
    acStack_308[0xb] = '\0';
    acStack_308[4] = '\x03';
    acStack_308[5] = '\0';
    acStack_308[6] = '\0';
    acStack_308[7] = '\0';
    acStack_308[0] = '\0';
    acStack_308[1] = '\0';
    acStack_308[2] = '\0';
    acStack_308[3] = '\0';
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,3);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,6,0);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
    (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,0,0);
    piVar1 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9d);
    if (unaff_BL != '\0') {
      (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,3,0x1ff,&DAT_006b9280,0x18);
      (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,3,4,&DAT_006bc280,0x18);
    }
    FUN_004944c0(1,0xffffffff,0,0,5,0);
    DAT_006e4738 = 0;
    DAT_006e4748 = 0;
    DAT_006e473c = piVar1;
    DAT_006e4740 = 0x9c;
    DAT_006e4744 = 1;
    if (unaff_BL != '\0') {
      chimera__draw_8_bit_text(0,0,&DAT_006bc310);
      uVar2 = __ftol();
      _sprintf(acStack_308,"%d",uVar2);
      chimera__draw_8_bit_text(0,0,acStack_308);
      uVar2 = __ftol();
      _sprintf(acStack_308,"%d",uVar2);
      chimera__draw_8_bit_text(0,0,acStack_308);
    }
    (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  }
  return;
}

Original Ghidra decompilation of the tail (0x512b80, not a function):

void object_render_state_refresh(void)

{
  int *piVar1;
  undefined4 uVar2;
  int in_EDX;
  char unaff_BL;
  char acStack_40 [12];
  int *piStack_34;
  undefined4 uStack_30;
  undefined4 uStack_2c;
  undefined4 uStack_28;
  int *piStack_24;
  undefined4 uStack_20;
  undefined4 uStack_1c;
  int *piStack_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  int *piStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0x512b86;
  (**(code **)(in_EDX + 0xe4))();
  uStack_4 = 0;
  uStack_8 = 7;
  piStack_c = DAT_0071d174;
  uStack_10 = 0x512b98;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_10 = 0;
  uStack_14 = 0xe;
  piStack_18 = DAT_0071d174;
  uStack_1c = 0x512baa;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_1c = 0;
  uStack_20 = 0x1c;
  piStack_24 = DAT_0071d174;
  uStack_28 = 0x512bbc;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_28 = 3;
  uStack_2c = 1;
  uStack_30 = 0;
  piStack_34 = DAT_0071d174;
  acStack_40[8] = -0x30;
  acStack_40[9] = '+';
  acStack_40[10] = 'Q';
  acStack_40[0xb] = '\0';
  (**(code **)(*DAT_0071d174 + 0x10c))();
  acStack_40[8] = '\0';
  acStack_40[9] = '\0';
  acStack_40[10] = '\0';
  acStack_40[0xb] = '\0';
  acStack_40[4] = '\x03';
  acStack_40[5] = '\0';
  acStack_40[6] = '\0';
  acStack_40[7] = '\0';
  acStack_40[0] = '\0';
  acStack_40[1] = '\0';
  acStack_40[2] = '\0';
  acStack_40[3] = '\0';
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,3);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,6,0);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
  (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
  (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,0,0);
  piVar1 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x9d);
  if (unaff_BL != '\0') {
    (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,3,0x1ff,&DAT_006b9280,0x18);
    (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,3,4,&DAT_006bc280,0x18);
  }
  FUN_004944c0(1,0xffffffff,0,0,5,0);
  DAT_006e4738 = 0;
  DAT_006e4748 = 0;
  DAT_006e473c = piVar1;
  DAT_006e4740 = 0x9c;
  DAT_006e4744 = 1;
  if (unaff_BL != '\0') {
    chimera__draw_8_bit_text(0,0,&DAT_006bc310);
    uVar2 = __ftol();
    _sprintf(acStack_40,"%d",uVar2);
    chimera__draw_8_bit_text(0,0,acStack_40);
    uVar2 = __ftol();
    _sprintf(acStack_40,"%d",uVar2);
    chimera__draw_8_bit_text(0,0,acStack_40);
  }
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  return;
}
#endif
