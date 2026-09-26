// rasterizer_screen_flash_render  (Ghidra: rasterizer_screen_flash_render, already named)
// address 0x52ed00, size 2656 bytes
// name confidence: 0.8   rewrite confidence: 0.55
// evidence: cea-pdb name match; the six technique handles it selects between (screen_flash_techniques,
//   set up by rasterizer_screen_flash_init_shaders 0x52ec40) and the render_screen_flash fields it
//   reads (rasterizer_window.screen_flash, 0x007c1458..0x007c146c) confirm the identification.
//   Ghidra's decompilation of this function is almost entirely unusable (every SetRenderState /
//   ID3DXEffect call below the color computation is rendered with denormalized-float stack
//   "arguments" that are really embedded return addresses and reused scratch slots), so this
//   rewrite is instead built directly from the raw disassembly (objdump -d bin/halo.exe,
//   0x52ed00..0x52f75f), instruction by instruction, including working out the ESP-relative
//   offsets across the two places the compiler pushes an argument before finishing a scratch
//   buffer that a later call still points into (the vertex/pixel shader constant blocks below).
// register convention: none -- __cdecl, no arguments.
// UNSURE: several things are transcribed exactly as found without a full explanation:
//   - After the initial null checks the raw code does `xor eax,eax; cmp eax,0x69e270; je ...`
//     before the real `mov eax,[0x69e270]; test eax,eax; je ...` null check. The first compare is
//     always false (eax is freshly zeroed and 0x69e270 is a nonzero immediate), so it can never
//     branch; it is omitted here as dead code rather than transcribed literally.
//   - Case 3 ("Max") and case 4 ("Min") each have a sub-path (taken when unknown_00722b7c is
//     nonzero) that renders with an additive blend and the FlashLighten technique
//     (screen_flash_techniques[0]) instead of their own technique -- confirmed by direct
//     disassembly of the SetTechnique argument, not a copy/paste mistake in this rewrite.
//   - Case 4's fallback path (src_blend_caps bit clear) sets TEXTUREFACTOR to the current color
//     twice in a row (once inline, once at the branch merge point); the duplicate call is in the
//     original code and is preserved rather than folded away.
//   - Phase 4 review: the 0xffffffff dword at +0xc of each quad vertex is the packed white
//     colour of rasterizer_dynamic_screen_vertex (x, y, z, color, u, v), not a float; and
//     0x0069e270 is rasterizer_effects[115].effect (0x0069d410 + 115 * 0x20).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t console_debug_toggle_689427;                          // 0x00689427
extern rasterizer_window_parameters rasterizer_window;                // 0x007c1220
extern void *rasterizer_device;                                       // 0x0071d174 IDirect3DDevice9
extern void *rasterizer_screen_flash_effect;                          // 0x0069e270 = rasterizer_effects[115].effect
extern void *screen_flash_techniques[6];                              // 0x0071d23c, FlashLighten .. FlashTint
extern d3d_caps9 rasterizer_caps;                                     // 0x007c10c0
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern uint8_t rasterizer_software_vertex_processing;                 // 0x0069c680
extern int32_t unknown_00722b7c; // 0x00722b7c flag selecting the additive + BLENDFACTOR fallback for the Max and Min flash blends

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c); // COM: __stdcall
typedef int32_t (__stdcall *d3d_setconst_fn)(void *self, uint32_t reg, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t primitive_type, uint32_t primitive_count,
                                            const void *data, uint32_t stride);
typedef int32_t (__stdcall *d3dx_effect_settechnique_fn)(void *effect, void *technique);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *pass_count, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}

// Packs four [0,1] channels (already multiplied by the flash intensity) into 0xAARRGGBB the same
// way the CRT __ftol-based sequence in the original code does (C's (int32_t) cast truncates
// toward zero, exactly like __ftol).
static uint32_t pack_argb_bytes(float alpha, float red, float green, float blue)
{
    uint32_t a = (uint32_t)(int32_t)(alpha * 255.0f) & 0xff;
    uint32_t r = (uint32_t)(int32_t)(red * 255.0f) & 0xff;
    uint32_t g = (uint32_t)(int32_t)(green * 255.0f) & 0xff;
    uint32_t b = (uint32_t)(int32_t)(blue * 255.0f) & 0xff;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// The clamped-alpha color variant used by the "Max"/"Min" additive fallback: the alpha channel is
// floored to 0.25 before scaling so the additive blend is never fully invisible.
static uint32_t pack_argb_bytes_clamped_alpha(ColorARGB color, float intensity)
{
    float alpha_i = color.alpha * intensity;
    if (alpha_i < 0.25f) {
        alpha_i = 0.25f;
    }
    return pack_argb_bytes(alpha_i, color.red * intensity, color.green * intensity, color.blue * intensity);
}

// Renders the full-screen flash post-process effect (Lighten/Darken/Max/Min/Invert/Tint) for the
// active render_screen_flash entry of the current window parameters.
void rasterizer_screen_flash_render(void)
{
    render_screen_flash *flash;
    ColorARGB color;
    float intensity;
    uint32_t current_color;   // esi in the original: color, unmodified, packed to 0xAARRGGBB
    uint32_t inverted_color;  // edi in the original: (1-channel) inverted color, same packing
    void *effect;
    void **effect_vt;
    void *technique;
    int16_t width, height;
    float constants[5][4];
    float vs_quad[4][6];
    float ps_constants[2][4];
    union { uint32_t bits; float f; } neg_one_bits;
    uint32_t pass_count, pass;

    if (console_debug_toggle_689427 == 0) {
        return;
    }
    flash = &rasterizer_window.screen_flash;
    if (flash->type == 0) {
        return;
    }

    color = flash->color;
    intensity = flash->intensity;
    current_color = pack_argb_bytes(color.alpha * intensity, color.red * intensity, color.green * intensity,
                                    color.blue * intensity);
    inverted_color = pack_argb_bytes(color.alpha * intensity, (1.0f - color.red) * intensity,
                                     (1.0f - color.green) * intensity, (1.0f - color.blue) * intensity);

    effect = rasterizer_screen_flash_effect;
    if (effect != 0) {
        set_render_state(0x16, 3);   // CULLMODE = D3DCULL_CCW
        set_render_state(0xa8, 7);   // COLORWRITEENABLE = RGB
        set_render_state(0x1b, 1);   // ALPHABLENDENABLE = TRUE
        set_render_state(0x1c, 0);   // FOGENABLE = FALSE

        effect_vt = *(void ***)effect;
        switch (flash->type) {
        case 1: // Lighten
            set_render_state(0x13, 2);  // SRCBLEND = ONE
            set_render_state(0x14, 6);  // DESTBLEND = SRCCOLOR
            set_render_state(0xab, 1);  // BLENDOP = ADD
            set_render_state(0x3c, current_color); // TEXTUREFACTOR
            technique = screen_flash_techniques[0];
            ((d3dx_effect_settechnique_fn)effect_vt[0xec / 4])(effect, technique);
            break;

        case 2: // Darken
            set_render_state(0x13, 2);  // SRCBLEND = ONE
            set_render_state(0x14, 6);  // DESTBLEND = SRCCOLOR
            set_render_state(0xab, 3);  // BLENDOP = MIN
            set_render_state(0x3c, current_color);
            technique = screen_flash_techniques[1];
            ((d3dx_effect_settechnique_fn)effect_vt[0xec / 4])(effect, technique);
            break;

        case 3: // Max
            if (unknown_00722b7c != 0) {
                uint32_t clamped = pack_argb_bytes_clamped_alpha(color, intensity);
                set_render_state(0x13, 5);  // SRCBLEND = SRCALPHA
                set_render_state(0x14, 6);  // DESTBLEND = SRCCOLOR
                set_render_state(0xab, 1);  // BLENDOP = ADD
                set_render_state(0x3c, clamped);
                technique = screen_flash_techniques[0]; // FlashLighten, per the raw code
            } else if ((rasterizer_caps.src_blend_caps & 0x2000) != 0) {
                set_render_state(0x13, 0xa);   // SRCBLEND = INVDESTCOLOR
                set_render_state(0x14, 0xf);   // DESTBLEND = BOTHINVSRCALPHA
                set_render_state(0xab, 5);     // BLENDOP = MAX
                set_render_state(0xc1, current_color); // BLENDFACTOR
                set_render_state(0x3c, current_color); // TEXTUREFACTOR
                technique = screen_flash_techniques[2]; // FlashMax
            } else {
                set_render_state(0x13, 0xa);   // SRCBLEND = INVDESTCOLOR
                set_render_state(0x14, 2);     // DESTBLEND = ZERO
                set_render_state(0xab, 5);     // BLENDOP = MAX
                set_render_state(0x3c, current_color); // TEXTUREFACTOR
                technique = screen_flash_techniques[2]; // FlashMax
            }
            ((d3dx_effect_settechnique_fn)effect_vt[0xec / 4])(effect, technique);
            break;

        case 4: // Min
            if (unknown_00722b7c != 0) {
                uint32_t clamped = pack_argb_bytes_clamped_alpha(color, intensity);
                set_render_state(0x13, 5);  // SRCBLEND = SRCALPHA
                set_render_state(0x14, 6);  // DESTBLEND = SRCCOLOR
                set_render_state(0xab, 1);  // BLENDOP = ADD
                set_render_state(0x3c, clamped);
                technique = screen_flash_techniques[0]; // FlashLighten, per the raw code
            } else if ((rasterizer_caps.src_blend_caps & 0x2000) != 0) {
                set_render_state(0x13, 0xa);   // SRCBLEND = INVDESTCOLOR
                set_render_state(0x14, 0xf);   // DESTBLEND = BOTHINVSRCALPHA
                set_render_state(0xab, 4);     // BLENDOP = MIN
                set_render_state(0xc1, current_color); // BLENDFACTOR
                set_render_state(0x3c, current_color); // TEXTUREFACTOR (raw code sets this twice:
                                                        // once here, once more below at the merge
                                                        // point; both are preserved)
            } else {
                set_render_state(0x13, 0xa);   // SRCBLEND = INVDESTCOLOR
                set_render_state(0x14, 2);     // DESTBLEND = ZERO
                set_render_state(0xab, 4);     // BLENDOP = MIN
            }
            set_render_state(0x3c, current_color); // TEXTUREFACTOR (merge point; see the note above)
            technique = screen_flash_techniques[3]; // FlashMin
            ((d3dx_effect_settechnique_fn)effect_vt[0xec / 4])(effect, technique);
            break;

        case 5: // Invert
            if ((rasterizer_caps.src_blend_caps & 0x2000) != 0) {
                set_render_state(0x13, 0xa);   // SRCBLEND = INVDESTCOLOR
                set_render_state(0x14, 0xf);   // DESTBLEND = BOTHINVSRCALPHA
                set_render_state(0xab, 1);     // BLENDOP = ADD
                set_render_state(0xc1, current_color); // BLENDFACTOR
            } else {
                set_render_state(0x13, 0xa);   // SRCBLEND = INVDESTCOLOR
                set_render_state(0x14, 2);     // DESTBLEND = ZERO
                set_render_state(0xab, 1);     // BLENDOP = ADD
            }
            set_render_state(0x3c, current_color); // TEXTUREFACTOR
            technique = screen_flash_techniques[4]; // FlashInvert
            ((d3dx_effect_settechnique_fn)effect_vt[0xec / 4])(effect, technique);
            break;

        case 6: // Tint
        default: // type > 6 falls through the jump table's bounds check straight to the common tail
            if (flash->type == 6) {
                if ((rasterizer_caps.src_blend_caps & 0x2000) != 0) {
                    set_render_state(0x13, 2);     // SRCBLEND = ONE
                    set_render_state(0x14, 0xf);   // DESTBLEND = BOTHINVSRCALPHA
                    set_render_state(0xab, 1);     // BLENDOP = ADD
                    set_render_state(0xc1, inverted_color); // BLENDFACTOR
                } else {
                    set_render_state(0x13, 2);     // SRCBLEND = ONE
                    set_render_state(0x14, 2);     // DESTBLEND = ZERO
                    set_render_state(0xab, 1);     // BLENDOP = ADD
                }
                set_render_state(0x3c, inverted_color); // TEXTUREFACTOR
                technique = screen_flash_techniques[5]; // FlashTint
                ((d3dx_effect_settechnique_fn)effect_vt[0xec / 4])(effect, technique);
            }
            break;
        }
    }

    // Common tail: draw the full-screen quad through the flash pixel/vertex shader pair,
    // regardless of whether a technique was selected above (the null-effect and out-of-range-type
    // cases both fall straight through to here too).
    set_render_state(0xf, 0); // ALPHATESTENABLE = FALSE
    set_render_state(0x7, 0); // ZENABLE = FALSE

    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device,
        rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);

    {
        uint32_t usage = rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage;
        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
            (rasterizer_software_vertex_processing != 0 ? 0x10u : 0u) | (usage & 0x10));
    }

    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[35].shader);

    // Vertex shader constant register 0xd, 5 vec4: the screen-pixel to [-1,1] clip-space transform
    // (with the D3D9 half-pixel offset baked in), plus two fixed identity-ish rows.
    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
    {
        float inv_w = 1.0f / (float)width;
        float inv_h = 1.0f / (float)height;
        constants[0][0] = inv_w + inv_w;
        constants[0][1] = 0.0f;
        constants[0][2] = 0.0f;
        constants[0][3] = -1.0f - inv_w;
        constants[1][0] = 0.0f;
        constants[1][1] = -2.0f * inv_h;
        constants[1][2] = 0.0f;
        constants[1][3] = 1.0f + inv_h;
        constants[2][0] = 0.0f;
        constants[2][1] = 0.0f;
        constants[2][2] = 0.5f;
        constants[2][3] = 0.0f;
        constants[3][0] = 0.0f;
        constants[3][1] = 0.0f;
        constants[3][2] = 0.0f;
        constants[3][3] = 1.0f;
        constants[4][0] = 0.0f;
        constants[4][1] = 0.0f;
        constants[4][2] = 0.0f;
        constants[4][3] = 1.0f;
    }
    ((d3d_setconst_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &constants[0][0], 5);

    // Recompute width/height as plain floats and build the quad's 4 vertices (screen pixel corner
    // position, z, a raw -1 marker read by the vertex shader as an integer, and a 0/1 corner flag
    // pair): (0,0)-(W,0)-(W,H)-(0,H), stride 0x18 (6 floats), drawn as a triangle fan.
    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
    neg_one_bits.bits = 0xffffffffu;
    {
        float w = (float)width;
        float h = (float)height;
        float n1 = neg_one_bits.f;
        vs_quad[0][0] = 0.0f; vs_quad[0][1] = 0.0f; vs_quad[0][2] = 0.0f; vs_quad[0][3] = n1; vs_quad[0][4] = 0.0f; vs_quad[0][5] = 0.0f;
        vs_quad[1][0] = w;    vs_quad[1][1] = 0.0f; vs_quad[1][2] = 0.0f; vs_quad[1][3] = n1; vs_quad[1][4] = 1.0f; vs_quad[1][5] = 0.0f;
        vs_quad[2][0] = w;    vs_quad[2][1] = h;    vs_quad[2][2] = 0.0f; vs_quad[2][3] = n1; vs_quad[2][4] = 1.0f; vs_quad[2][5] = 1.0f;
        vs_quad[3][0] = 0.0f; vs_quad[3][1] = h;    vs_quad[3][2] = 0.0f; vs_quad[3][3] = n1; vs_quad[3][4] = 0.0f; vs_quad[3][5] = 1.0f;
    }

    // Pixel shader constant register 0, 2 vec4: the plain and inverted flash colors as floats
    // (0..1 channels already scaled by intensity, not the packed byte form above).
    ps_constants[0][0] = color.red * intensity;
    ps_constants[0][1] = color.green * intensity;
    ps_constants[0][2] = color.blue * intensity;
    ps_constants[0][3] = color.alpha * intensity;
    ps_constants[1][0] = (1.0f - color.red) * intensity;
    ps_constants[1][1] = (1.0f - color.green) * intensity;
    ps_constants[1][2] = (1.0f - color.blue) * intensity;
    ps_constants[1][3] = color.alpha * intensity;
    if (rasterizer_caps.pixel_shader_version > 0xffff0100) {
        ((d3d_setconst_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 0, &ps_constants[0][0], 2);
    }

    effect = rasterizer_screen_flash_effect;
    effect_vt = *(void ***)effect;
    pass_count = 0;
    ((d3dx_effect_begin_fn)effect_vt[0x100 / 4])(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        ((d3dx_effect_pass_fn)effect_vt[0x104 / 4])(effect, 0); // BeginPass(0); the raw code passes a
                                                                 // literal 0 every iteration, not `pass`
        ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, &vs_quad[0][0], 0x18); // DrawPrimitiveUP(TRIANGLEFAN)
    }
    ((d3dx_effect_end_fn)effect_vt[0x108 / 4])(effect);

    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x52ed00):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void rasterizer_screen_flash_render(void)

{
  float fVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  undefined4 uVar6;
  uint uVar7;
  int *piVar8;
  int *piVar9;
  int *piStack_104;
  int **ppiStack_100;
  uint uStack_fc;
  int *piStack_f8;
  undefined4 uStack_f4;
  int *piStack_f0;
  int *piStack_ec;
  int *piStack_e8;
  undefined4 uStack_e4;
  int *piStack_e0;
  undefined4 uStack_dc;
  float fStack_d8;
  int *piStack_d4;
  undefined4 uStack_d0;
  float fStack_cc;
  int *piStack_c8;
  float fStack_c4;
  float fStack_c0;
  int *piStack_bc;
  undefined4 uStack_b8;
  undefined4 uStack_b4;
  int *piStack_b0;
  undefined4 uStack_ac;
  undefined4 uStack_a8;
  int *piStack_a4;
  int *piStack_a0;
  uint uStack_9c;

  if (DAT_00689427 == '\0') {
    return;
  }
  if (DAT_007c1458 == 0) {
    return;
  }
  uStack_9c = 0x52ed49;
  iVar2 = __ftol();
  uStack_9c = 0x52ed73;
  uVar3 = __ftol();
  uStack_9c = 0x52ed8d;
  uVar4 = __ftol();
  uStack_9c = 0x52eda4;
  uVar5 = __ftol();
  uVar7 = ((uVar3 & 0xff | iVar2 << 8) << 8 | uVar4 & 0xff) << 8 | uVar5 & 0xff;
  uStack_9c = 0x52edfc;
  uVar3 = __ftol();
  uStack_9c = 0x52ee18;
  uVar4 = __ftol();
  uStack_9c = 0x52ee31;
  uVar5 = __ftol();
  uVar3 = ((uVar3 & 0xff | iVar2 << 8) << 8 | uVar4 & 0xff) << 8 | uVar5 & 0xff;
  if (DAT_0069e270 == (int **)0x0) goto LAB_0052f740;
  uStack_9c = 3;
  piStack_a0 = (int *)0x16;
  piStack_a4 = DAT_0071d174;
  uStack_a8 = 0x52eea6;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_a8 = 7;
  uStack_ac = 0xa8;
  piStack_b0 = DAT_0071d174;
  uStack_b4 = 0x52eebb;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  uStack_b4 = 1;
  uStack_b8 = 0x1b;
  piStack_bc = DAT_0071d174;
  fStack_c0 = 7.616176e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  fStack_c0 = 0.0;
  fStack_c4 = 3.92364e-44;
  piStack_c8 = DAT_0071d174;
  fStack_cc = 7.616201e-39;
  (**(code **)(*DAT_0071d174 + 0xe4))();
  switch(DAT_007c1458) {
  ... (the full 391-line body Ghidra produced here is a near-total loss: every SetRenderState /
       ID3DXEffect call after this point is shown with stack-slot "arguments" that are denormalized
       floats reinterpreting embedded return addresses and reused scratch, not real parameters; see
       out/phase2/rasterizer/03.md around line 561 for the raw text. This rewrite was built instead
       from the disassembly, as described in the file header above.)
  }
  ...
  return;
}
#endif
