// decal_and_font_system_reset  (Ghidra: FUN_00515740, unnamed; named per
// out/phase4/rasterizer_types_notes.md's misattribution table: "Clears the lens flare visibility
// tables, the glyph cache, and loads GlobalsRasterizerData into 0x0071d164")
// address 0x515740, size 148 bytes
// name confidence: 0.55  rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x515740..0x5157d3.)
// evidence: 0x8c0 dwords == sizeof(lens_flare_object_visibility[0x380]) and 0x4002 dwords ==
//   k_lens_flare_marker_visibility_size (both types/rasterizer.h); Globals+0x134/+0x138 is the
//   rasterizer_data TagReflexive's count/pointer. The two 0x0071cfc4/0x0071cfc0 blocks are
//   flagged in the type header as owned elsewhere (cinematic code) and left untyped here; the
//   letterbox height field at +0x74 of the first block is called out explicitly.
// register convention: none -- no parameters.
// reconciled: R80 0x0071cfc4 uint32_t* cinematic_globals -> render.h cinematic_screen_effect_globals *cinematic_screen_effect_state (same dword stores)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Globals *global_globals; // 0x00746fa0
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern lens_flare_object_visibility lens_flare_object_visibility_table[k_lens_flare_object_visibility_slots]; // 0x006bc510
extern uint8_t lens_flare_marker_visibility[0x10008]; // 0x006be810
extern int32_t lens_flare_instance_count; // 0x0071d134
extern cinematic_screen_effect_globals *cinematic_screen_effect_state; // 0x0071cfc4, render.h (0x78 bytes, +0x74 near_clip_distance)
extern float *rasterizer_model_ambient_reflection_tint; // 0x0071cfc0

extern void font_glyph_cache_clear_all(void); // 0x514cb0

// Clears the lens flare visibility tables and active count, clears the font glyph cache, loads
// GlobalsRasterizerData from the Globals tag (or NULL if it has none), and resets two
// cinematic-owned state blocks (including the letterbox height at +0x74 of the first).
void decal_and_font_system_reset(void)
{
    uint8_t *globals = (uint8_t *)global_globals;
    int32_t i;

    if (*(int32_t *)(globals + 0x134) == 0) {
        rasterizer_globals_data = (GlobalsRasterizerData *)((void *)0);
    } else {
        rasterizer_globals_data = (GlobalsRasterizerData *)(*(void **)(globals + 0x138));
    }

    for (i = 0; i < 0x8c0; i++) {
        ((uint32_t *)lens_flare_object_visibility_table)[i] = 0;
    }
    for (i = 0; i < 0x4002; i++) {
        ((uint32_t *)lens_flare_marker_visibility)[i] = 0;
    }
    lens_flare_instance_count = 0;
    font_glyph_cache_clear_all();

    if (cinematic_screen_effect_state != (cinematic_screen_effect_globals *)0) {
        for (i = 0; i < 0x1e; i++) {
            ((uint32_t *)cinematic_screen_effect_state)[i] = 0;
        }
        ((uint32_t *)cinematic_screen_effect_state)[0x19] = 0x3f800000; // 1.0f
        ((uint32_t *)cinematic_screen_effect_state)[0x1a] = 0x3f800000;
        ((uint32_t *)cinematic_screen_effect_state)[0x1b] = 0x3f800000;
        ((uint32_t *)cinematic_screen_effect_state)[0x1c] = 0x3f800000;
    }
    if (rasterizer_model_ambient_reflection_tint != (float *)0) {
        rasterizer_model_ambient_reflection_tint[0] = 0;
        rasterizer_model_ambient_reflection_tint[1] = 0;
        rasterizer_model_ambient_reflection_tint[2] = 0;
        rasterizer_model_ambient_reflection_tint[3] = 0;
    }
    if (cinematic_screen_effect_state != (cinematic_screen_effect_globals *)0) {
        ((uint32_t *)cinematic_screen_effect_state)[0x1d] = 0; // near_clip_distance (+0x74)
    }
}

#if 0
Original Ghidra decompilation (0x515740):

void FUN_00515740(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;

  if (*(int *)(DAT_00746fa0 + 0x134) == 0) {
    DAT_0071d164 = 0;
  }
  else {
    DAT_0071d164 = *(undefined4 *)(DAT_00746fa0 + 0x138);
  }
  puVar2 = &DAT_006bc510;
  for (iVar1 = 0x8c0; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  puVar2 = &DAT_006be810;
  for (iVar1 = 0x4002; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  DAT_0071d134 = 0;
  font_glyph_cache_clear_all();
  puVar2 = DAT_0071cfc4;
  if (DAT_0071cfc4 != (undefined4 *)0x0) {
    puVar3 = DAT_0071cfc4;
    for (iVar1 = 0x1e; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    puVar2[0x19] = 0x3f800000;
    puVar2[0x1a] = 0x3f800000;
    puVar2[0x1b] = 0x3f800000;
    puVar2[0x1c] = 0x3f800000;
  }
  puVar3 = DAT_0071cfc0;
  if (DAT_0071cfc0 != (undefined4 *)0x0) {
    *DAT_0071cfc0 = 0;
    puVar3[1] = 0;
    puVar3[2] = 0;
    puVar3[3] = 0;
  }
  if (puVar2 != (undefined4 *)0x0) {
    puVar2[0x1d] = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
