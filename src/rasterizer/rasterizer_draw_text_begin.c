// rasterizer_draw_text_begin  (Ghidra: FUN_00531b80; the phase 3 rewrite called it
//   rasterizer_first_person_model_render)
// address 0x531b80, size 781 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: raw disassembly (phase 4 review). The only callers are chimera__draw_8_bit_text
//   0x5148b0 and chimera__draw_16_bit_text 0x514ab0, which bracket their glyph draws with this
//   and 0x531e90 (rasterizer_draw_text_end); it first calls chimera__widescreen_text_scaling
//   0x531ab0. Nothing here draws a model. EDI is the text draw state of the caller: up to three
//   bitmap pointers at +0xc (a null entry ends the list), per-stage clamp flags at +0x18,
//   two floats at +0x40 / +0x44 copied into c17.xy, the framebuffer blend mode at +0x88 and a
//   point filtering flag at +0x8a.
//   Phase 4 fixes: the blend function takes the mode word at +0x88 in CX; the vertex
//   declaration is declarations[8] (0x006e1af0), not [0].
// register convention: EDI -> state (ui_quad_render_state, types/interface.h; the same 0x8c byte
//   record the HUD builders hand to rasterizer_ui_quad_draw 0x51c9a0).
// blam-cc: EDI -> state

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"
#include "objects.h"
#include "units.h"
#include "fn_rasterizer.h"

extern uint8_t text_rendering_enabled;   // 0x00689402
extern rasterizer_window_parameters rasterizer_window; // 0x007c1220
extern void *rasterizer_device;               // 0x0071d174
extern uint8_t console_debug_toggle_6893e6;   // 0x006893e6
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern uint8_t rasterizer_software_vertex_processing; // 0x0069c680
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350 (35 at 0x0069e468)
extern float rasterizer_ui_text_constants[20]; // 0x006e1d08

extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x00444550
// blam-cc: CX -> mode


typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_setconst_fn)(void *self, uint32_t reg, const float *data, uint32_t count);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_texture(uint32_t stage, uint32_t texture)
{
    ((d3d_call2_fn)device_vtable()[0x104 / 4])(rasterizer_device, stage, texture);
}
static void set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, stage, type, value);
}
static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}

// Renders a multi-part model (up to 3 attachment parts, consistent with the first-person view
// model) with widescreen-corrected shader constants and per-part texture/blend state.
void rasterizer_draw_text_begin(ui_quad_render_state *state)
{
    uint8_t *context = (uint8_t *)state;
    void *part0;
    int16_t part;

    if (text_rendering_enabled == 0 || rasterizer_window.type != 1) {
        return;
    }

    chimera__widescreen_text_scaling();
    chimera__rasterizer_set_framebuffer_blend_function(((struct ui_quad_render_state *)context)->framebuffer_blend_function);

    part0 = *(void **)(context + 0xc);
    if (part0 != 0) {
        texture_cache_get(part0, 1, 1);
        set_texture(0, *(uint32_t *)((uint8_t *)part0 + 0x28));
    }

    set_render_state(0x16, 3);  // CULLMODE = CCW
    set_render_state(0xa8, 7);  // COLORWRITEENABLE = RGB
    set_render_state(0x1b, 1);  // ALPHABLENDENABLE = TRUE
    set_render_state(0xf, 1);   // ALPHATESTENABLE = TRUE
    set_render_state(0x18, 0);  // ALPHAREF = 0
    set_render_state(0x7, 0);   // ZENABLE = FALSE
    set_render_state(0x1c, 0);  // FOGENABLE = FALSE
    if (console_debug_toggle_6893e6 != 0) {
        set_render_state(8, 3); // FILLMODE SOLID while the wireframe debug toggle is on
    }

    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
        rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
        (rasterizer_software_vertex_processing != 0 ? 0x10u : 0u) | (rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage & 0x10));
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[35].shader);
    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0); // SetPixelShader(NULL)

    rasterizer_ui_text_constants[16] = *(float *)(context + 0x40);
    rasterizer_ui_text_constants[17] = *(float *)(context + 0x44);
    ((d3d_setconst_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, rasterizer_ui_text_constants, 5);

    for (part = 0; part < 3; part++) {
        void *part_texture = *(void **)(context + 0xc + part * 4);
        uint32_t address_mode, filter_value;

        if (part_texture == 0) {
            set_texture(part, 0);
            break;
        }
        texture_cache_get(part_texture, 1, 1);
        set_texture(part, *(uint32_t *)((uint8_t *)part_texture + 0x28));

        address_mode = (context[0x18 + part] == 0) ? 3u : 1u; // CLAMP unless the per-stage flag is set (WRAP)
        set_sampler_state(part, 1, address_mode); // ADDRESSU
        set_sampler_state(part, 2, address_mode); // ADDRESSV
        filter_value = (context[0x8a] == 0) ? 2u : 1u;
        set_sampler_state(part, 5, filter_value); // MAGFILTER
        set_sampler_state(part, 6, filter_value); // MINFILTER
        set_sampler_state(part, 7, filter_value); // MIPFILTER
    }

    set_texture_stage_state(0, 1, 4);
    set_texture_stage_state(0, 2, 2);
    set_texture_stage_state(0, 3, 0);
    set_texture_stage_state(0, 4, 4);
    set_texture_stage_state(0, 5, 2);
    set_texture_stage_state(0, 6, 0);
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
}

#if 0
Original Ghidra decompilation (0x531b80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00531b80(void)

{
  int iVar1;
  short sVar2;
  int iVar3;
  int unaff_EDI;

  if ((DAT_00689402 != '\0') && ((short)DAT_007c1220 == 1)) {
    chimera__widescreen_text_scaling();
    chimera__rasterizer_set_framebuffer_blend_function();
    iVar1 = *(int *)(unaff_EDI + 0xc);
    if (iVar1 != 0) {
      texture_cache_get(1,1);
      (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,0,*(undefined4 *)(iVar1 + 0x28));
    }
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x18,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
    if (DAT_006893e6 != '\0') {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,8,3);
    }
    (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1af0);
    (**(code **)(*DAT_0071d174 + 0x134))
              (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1af8 & 0x10);
    (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,DAT_0069e468);
    (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
    _DAT_006e1d48 = *(undefined4 *)(unaff_EDI + 0x40);
    _DAT_006e1d4c = *(undefined4 *)(unaff_EDI + 0x44);
    (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,&DAT_006e1d08,5);
    sVar2 = 0;
    do {
      iVar3 = (int)sVar2;
      iVar1 = *(int *)(unaff_EDI + 0xc + iVar3 * 4);
      if (iVar1 == 0) {
        (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,(int)sVar2,0);
        break;
      }
      texture_cache_get(1,1);
      (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,iVar3,*(undefined4 *)(iVar1 + 0x28));
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,1,
                 (*(char *)(iVar3 + 0x18 + unaff_EDI) == '\0') * '\x02' + '\x01');
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,2,
                 (*(char *)(iVar3 + 0x18 + unaff_EDI) == '\0') * '\x02' + '\x01');
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,5,(*(char *)(unaff_EDI + 0x8a) == '\0') + '\x01');
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,6,(*(char *)(unaff_EDI + 0x8a) == '\0') + '\x01');
      (**(code **)(*DAT_0071d174 + 0x114))
                (DAT_0071d174,iVar3,7,(*(char *)(unaff_EDI + 0x8a) == '\0') + '\x01');
      sVar2 = sVar2 + 1;
    } while (sVar2 < 3);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,0);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,4);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,6,0);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
  }
  return;
}
#endif
