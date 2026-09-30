// chimera__letterbox  (Ghidra: chimera__letterbox -- Chimera signature name, hint only; not
// renamed here because it is not a FUN_xxxxxx placeholder. Suggested rewrite name per
// out/phase4/cutscene_types_notes.md: cinematic_render.)
// address 0x4499c0, size 1261 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/cutscene_types_notes.md cinematic_globals +0x00 letterbox_scale / +0x04
// letterbox_last_tick / +0x08 show_letterbox / +0x0c titles[4]; types/tags.h ScenarioCutsceneTitle
// (text_bounds 0x28, string_index 0x30, text_style 0x32, justification 0x34, text_flags 0x38,
// text_color 0x3c, shadow_color 0x40, fade_in_time 0x44, up_time 0x48, fade_out_time 0x4c),
// Scenario (cutscene_titles 0x4fc, ingame_help_text 0x590), HUDGlobals.default_chapter_title_bounds
// (offsetof-verified at 0x2dc under -m32 -- see below), types/interface.h widget_memory_pool_valid /
// ui_root_widget (the "a ui widget is open" gate). Ghidra's own decompile silently DROPS the
// Rectangle2D construction for both letterbox bars and the destination-rect selection for the
// title text (it treats those stack writes as dead because it never resolved 0x449780's and
// chimera__draw_16_bit_text's custom register arguments); both are reconstructed here directly
// from objdump -d -M intel 0x4499c0..0x449eac (see the trailer) rather than from the decompiled
// C, which is otherwise followed closely.
// register convention: __cdecl, no parameters; every access is through named globals. The two
// callees this function shares with the rest of the engine (0x449780, chimera__draw_16_bit_text)
// use their own established custom register conventions -- see their prototypes below.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "game.h"
#include "cache.h"
#include "interface.h"
#include "cutscene.h"
#include "fn_cutscene.h"
#include "fn_bitmaps.h"

extern int32_t ROUND(float x); // MSVC round-to-nearest helper
extern float fabsf(float x); // x87 FABS

extern cinematic_globals *cinematic_globals_ptr;      // 0x006f187c
extern game_time_globals *game_time;                  // 0x006f1d6c
extern Scenario *global_scenario;                     // 0x00746f8c
extern tag_instance *tag_instances;                    // 0x0087bc14
extern HUDGlobals *hud_globals_tag_data;               // 0x0071941c
extern uint8_t widget_memory_pool_valid;               // 0x00718fc2
extern widget_instance *ui_root_widget[1];             // 0x00718f94
extern Rectangle2D render_viewport_top;             // 0x007c3140; only .top/.left are read
                                                        // here (see also screen_safe_area_origin
                                                        // / render_viewport_top in other modules
                                                        // for the same packed pair)
extern uint32_t text_shadow_color_argb;                 // 0x0071d144, the text shadow colour this
                                                        // draw sets before chimera__draw_16_bit_text
                                                        // and clears right after

// blam-cc: EAX -> packed_color, ECX -> rect (same prototype as the interface / rasterizer
// callers declare it)
// A generic filled screen rectangle; shared with several interface/rasterizer callers outside
// this module. Misattributed to cutscene by the address-run heuristic (library-style helper with
// no types of its own -- see out/phase4/cutscene_types_notes.md); not rewritten here.
extern void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect); // 0x449780, solid rectangle fill

// blam-cc: EAX -> out, ECX -> packed color (src/bitmaps/color_argb_int_to_real.c, this batch)


// blam-cc: ECX -> font, EAX -> color, stack -> style, justification, flags
// (src/text/text_set_render_context.c, already committed)
extern void text_set_render_context(datum_index font, ColorARGB *color, int16_t style,
    int16_t justification, uint32_t flags); // 0x5563b0

// blam-cc: ECX=list_id, EDX=index (src/text/text_string_list_get_string.c, already committed)
extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index); // 0x5578c0

// blam-cc: EAX -> clip_rect(opt), ECX -> dest_rect_override(opt), stack -> (position_or_color1,
// position_or_color2, text)  (src/rasterizer/chimera__draw_16_bit_text.c, already committed)
extern void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override,
    int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2,
    const int16_t *text); // 0x514ab0

// Per-frame cinematic update: steps the letterbox fade towards show_letterbox (1 second full
// travel) and draws both bars while the fade is above 0, then advances and draws every queued
// cutscene title. The fade/draw half is skipped entirely while a ui widget is open (the letterbox
// clock is simply not advanced that frame); the title loop always runs.
void chimera__letterbox(void)
{
    int16_t i;

    if (cinematic_globals_ptr->show_letterbox != 0 || cinematic_globals_ptr->letterbox_scale > 0.0f) {
        // "a ui widget is open" gate: this build has one controller-slot root widget
        // (ui_root_widget[1]); the original loops over it as an array.
        uint8_t widget_open = widget_memory_pool_valid != 0 && ui_root_widget[0] != (widget_instance *)0;

        if (!widget_open) {
            int32_t old_tick = cinematic_globals_ptr->letterbox_last_tick;
            float delta = (float)(game_time->game_time - old_tick) * (1.0f / k_cinematic_ticks_per_second);
            cinematic_globals_ptr->letterbox_last_tick = game_time->game_time;

            if (cinematic_globals_ptr->show_letterbox == 0) {
                float scale = cinematic_globals_ptr->letterbox_scale - delta;
                if (scale <= 0.0f) { // fcom / test ah,0x41 / je at 0x449a49: <= 0 stores +0.0
                    scale = 0.0f;
                }
                cinematic_globals_ptr->letterbox_scale = scale;
            } else {
                float scale = cinematic_globals_ptr->letterbox_scale + delta;
                if (scale > 1.0f) {
                    scale = 1.0f;
                }
                cinematic_globals_ptr->letterbox_scale = scale;
            }

            if (cinematic_globals_ptr->letterbox_scale > 0.0f) {
                // 0.125 is the .rdata float at 0x00672cbc (fmul at 0x449a7e), so each bar is 60
                // pixels of the 480 line screen at full scale. Both products are rounded to float
                // (fstp [esp+0x10]) before use, as the two C multiplies are here.
                float bar_height = cinematic_globals_ptr->letterbox_scale * 0.125f *
                    (float)k_cinematic_letterbox_screen_height;
                Rectangle2D bar;

                bar.left = (int16_t)ROUND((float)render_viewport_top.left);
                bar.right = (int16_t)ROUND((float)k_cinematic_letterbox_screen_width);
                bar.top = (int16_t)ROUND((float)render_viewport_top.top);
                bar.bottom = (int16_t)ROUND((float)render_viewport_top.top + bar_height);
                ui_draw_filled_rectangle((uint32_t)k_cinematic_letterbox_color, &bar);

                bar.left = (int16_t)ROUND((float)render_viewport_top.left);
                bar.right = (int16_t)ROUND((float)k_cinematic_letterbox_screen_width);
                bar.top = (int16_t)ROUND((float)k_cinematic_letterbox_screen_height - bar_height);
                bar.bottom = (int16_t)k_cinematic_letterbox_bottom_edge;
                ui_draw_filled_rectangle((uint32_t)k_cinematic_letterbox_color, &bar);
            }
        }
    }

    for (i = 0; i < k_cinematic_title_slot_count; i += 1) {
        cinematic_title_slot *slot = &cinematic_globals_ptr->titles[i];
        // HUDGlobals +0x54 is fullscreen_font.tag_id: the TagDependency starts at 0x48 and its
        // tag_id is the dependency's last dword (+0x0c).
        datum_index fullscreen_font = *(datum_index *)&hud_globals_tag_data->fullscreen_font.tag_id;

        if (slot->title_index == k_cinematic_title_none || fullscreen_font == (datum_index)k_datum_index_none) {
            continue;
        }

        {
            ScenarioCutsceneTitle *title =
                &((ScenarioCutsceneTitle *)global_scenario->cutscene_titles.pointer)[slot->title_index];
            datum_index help_text_list = *(datum_index *)&global_scenario->ingame_help_text.tag_id;

            // string_index is uint16_t in tags.h but the binary tests it signed (test ax,ax / jl
            // at 0x449c04, movsx before the count compare at 0x449c24)
            if (help_text_list == (datum_index)k_datum_index_none || (int16_t)title->string_index < 0) {
                continue;
            }

            {
                UnicodeStringList *help_text_data =
                    (UnicodeStringList *)tag_instances[help_text_list & 0xffff].data;
                if ((int32_t)(int16_t)title->string_index >= (int32_t)help_text_data->strings.count) {
                    continue;
                }
            }

            {
                float ticks = (float)slot->ticks;
                float fade;
                ColorARGB tint;
                int32_t shadow_alpha;
                uint16_t *help_text;
                Rectangle2D *dest_rect;
                int16_t new_ticks;

                if (ticks < title->fade_in_time) {
                    fade = ticks / title->fade_in_time;
                    if (fade < 0.0f) fade = 0.0f;
                    else if (fade > 1.0f) fade = 1.0f;
                } else if (ticks > title->up_time) {
                    fade = 1.0f - (ticks - title->up_time) / title->fade_out_time;
                    if (fade < 0.0f) fade = 0.0f;
                    else if (fade > 1.0f) fade = 1.0f;
                } else {
                    fade = 1.0f;
                }

                color_argb_int_to_real(&tint, *(uint32_t *)&title->text_color);
                tint.alpha *= fade;

                if (fabsf(tint.red - 1.0f) < 0.0001f && fabsf(tint.green - 1.0f) < 0.0001f &&
                    fabsf(tint.blue - 1.0f) < 0.0001f) {
                    // pure white is dimmed so on-screen title text stays readable
                    if (tint.red > 0.8f) tint.red = 0.8f;
                    if (tint.green > 0.8f) tint.green = 0.8f;
                    if (tint.blue > 0.8f) tint.blue = 0.8f;
                }

                text_set_render_context(fullscreen_font, &tint, (int16_t)(title->text_style - 1),
                    title->justification, title->text_flags);

                shadow_alpha = ROUND((float)title->shadow_color.alpha * fade);
                if (shadow_alpha < 0) {
                    shadow_alpha = 0;
                } else if (shadow_alpha > 0xff) {
                    shadow_alpha = 0xff;
                }
                text_shadow_color_argb =
                    (*(uint32_t *)&title->shadow_color & 0xffffff) | ((uint32_t)shadow_alpha << 24);

                help_text = text_string_list_get_string(help_text_list, (int16_t)title->string_index);
                dest_rect = (title->text_bounds.right == title->text_bounds.left ||
                             title->text_bounds.bottom == title->text_bounds.top)
                    ? &hud_globals_tag_data->default_chapter_title_bounds
                    : &title->text_bounds;
                chimera__draw_16_bit_text((Rectangle2D *)0, (int32_t *)dest_rect, 0, 0,
                    (const int16_t *)help_text);
                text_shadow_color_argb = 0;

                new_ticks = slot->ticks + (game_time->paused == 0 ? game_time->ticks_this_frame : 0);
                slot->ticks = new_ticks;
                if (title->up_time + title->fade_out_time <= (float)new_ticks) {
                    slot->title_index = k_cinematic_title_none;
                    slot->ticks = k_cinematic_title_none;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4499c0), from tools/pack.py 0x4499c0:

void __cdecl chimera__letterbox(void)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  short sVar4;
  int *piVar5;
  int iVar6;
  undefined4 uVar7;
  float *pfVar8;
  int iVar9;
  undefined2 *puVar10;
  float local_34;
  int local_30;
  int local_2c;
  float local_c;
  float local_8;
  float local_4;

  pfVar8 = DAT_006f187c;
  pfVar1 = DAT_006f187c + 2;
  if ((*(char *)pfVar1 != '\0') || (0.0 < *DAT_006f187c)) {
    if (DAT_00718fc2 != '\0') {
      piVar5 = &DAT_00718f94;
      do {
        if (*piVar5 != 0) goto LAB_00449ba4;
        piVar5 = piVar5 + 1;
      } while ((int)piVar5 < 0x718f98);
    }
    fVar2 = *(float *)(DAT_006f1d6c + 0xc);
    fVar3 = DAT_006f187c[1];
    DAT_006f187c[1] = fVar2;
    fVar2 = (float)((int)fVar2 - (int)fVar3) * 0.033333335;
    if (*(char *)pfVar1 == '\0') {
      fVar2 = *pfVar8 - fVar2;
      *pfVar8 = fVar2;
      if (fVar2 <= 0.0) {
        fVar2 = 0.0;
      }
    }
    else {
      fVar2 = fVar2 + *pfVar8;
      *pfVar8 = fVar2;
      if (1.0 < fVar2) {
        fVar2 = 1.0;
      }
    }
    *pfVar8 = fVar2;
    if (0.0 < *pfVar8) {
      FUN_00449780();
      FUN_00449780();
      pfVar8 = DAT_006f187c;
    }
  }
LAB_00449ba4:
  local_30 = 0xc;
  local_2c = 4;
  do {
    puVar10 = (undefined2 *)(local_30 + (int)pfVar8);
    if ((*(short *)(local_30 + (int)pfVar8) != -1) && (*(int *)(DAT_0071941c + 0x54) != -1)) {
      iVar9 = *(short *)(local_30 + (int)pfVar8) * 0x60 + *(int *)(global_scenario + 0x500);
      if ((*(uint *)(global_scenario + 0x590) != 0xffffffff) &&
         ((-1 < *(short *)(iVar9 + 0x30) &&
          ((int)*(short *)(iVar9 + 0x30) <
           **(int **)((*(uint *)(global_scenario + 0x590) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14))))
         ) {
        local_34 = 1.0;
        fVar2 = (float)(int)(short)puVar10[1];
        if (*(float *)(iVar9 + 0x44) <= fVar2) {
          if (*(float *)(iVar9 + 0x48) < fVar2) {
            local_34 = 1.0 - (fVar2 - *(float *)(iVar9 + 0x48)) / *(float *)(iVar9 + 0x4c);
            goto LAB_00449c8a;
          }
        }
        else {
          local_34 = fVar2 / *(float *)(iVar9 + 0x44);
LAB_00449c8a:
          if (0.0 <= local_34) {
            if (1.0 < local_34) {
              local_34 = 1.0;
            }
          }
          else {
            local_34 = 0.0;
          }
        }
        color_argb_int_to_real();
        if (((ABS(local_c - 1.0) < 0.0001) && (ABS(local_8 - 1.0) < 0.0001)) &&
           (ABS(local_4 - 1.0) < 0.0001)) {
          if (0.8 < local_c) {
            local_c = 0.8;
          }
          if (0.8 < local_8) {
            local_8 = 0.8;
          }
          if (0.8 < local_4) {
            local_4 = 0.8;
          }
        }
        text_set_render_context
                  (*(short *)(iVar9 + 0x32) + -1,*(undefined2 *)(iVar9 + 0x34),
                   *(undefined4 *)(iVar9 + 0x38));
        if ((int)ROUND((float)*(byte *)(iVar9 + 0x43) * local_34) < 0) {
          iVar6 = 0;
        }
        else {
          iVar6 = 0xff;
          if ((int)ROUND((float)*(byte *)(iVar9 + 0x43) * local_34) < 0x100) {
            iVar6 = (int)ROUND((float)*(byte *)(iVar9 + 0x43) * local_34);
          }
        }
        DAT_0071d144 = *(uint *)(iVar9 + 0x40) & 0xffffff | iVar6 << 0x18;
        uVar7 = text_string_list_get_string();
        chimera__draw_16_bit_text(0,0,uVar7);
        DAT_0071d144 = 0;
        if (*(char *)(DAT_006f1d6c + 2) == '\0') {
          sVar4 = *(short *)(DAT_006f1d6c + 0x10);
        }
        else {
          sVar4 = 0;
        }
        puVar10[1] = puVar10[1] + sVar4;
        pfVar8 = DAT_006f187c;
        fVar2 = *(float *)(iVar9 + 0x4c) + *(float *)(iVar9 + 0x48);
        if (fVar2 < (float)(int)(short)puVar10[1] != (fVar2 == (float)(int)(short)puVar10[1])) {
          *puVar10 = 0xffff;
          puVar10[1] = 0xffff;
        }
      }
    }
    local_30 = local_30 + 4;
    local_2c = local_2c + -1;
    if (local_2c == 0) {
      return;
    }
  } while( true );
}

objdump excerpt reconstructing the Rectangle2D build Ghidra dropped (0x449a75..0x449b99), the
static screen-bounds origin (0x007c3140/+2, top/left only), the 640.0f / 480.0f screen size
literals, and the two calls this feeds (ECX=&rect, EAX=0xff000000):

  449a75: fld    st(0)                     ; letterbox_scale (already on the FPU stack)
  449a77: movsx  ecx,WORD PTR ds:0x7c3142  ; screen bounds .left
  449a7e: fmul   DWORD PTR ds:0x672cbc     ; * 0.125
  449a94: fld    DWORD PTR [esp+0x18]      ; roundtrip float(int(bounds.left)) == ROUND()
  449a98: fistp  DWORD PTR [esp+0x14]
  449aa1: mov    WORD PTR [esp+0x2e],dx    ; rect.left = ROUND(bounds.left)
  449aa6: mov    DWORD PTR [esp+0x18],0x44204000 ; 640.0f
  449ac6: mov    WORD PTR [esp+0x32],ax    ; rect.right = ROUND(640.0f)
  449ab6: movsx  ecx,WORD PTR ds:0x7c3140  ; screen bounds .top
  449ae6: fmul   DWORD PTR ds:0x672b9c     ; (scale*0.125) * 480.0f  -> bar_height
  449af5: mov    WORD PTR [esp+0x2c],dx    ; rect.top = ROUND(bounds.top)
  449b02: fadd   DWORD PTR [esp+0x10]      ; bounds.top + bar_height
  449b17: mov    WORD PTR [esp+0x30],cx    ; rect.bottom = ROUND(bounds.top + bar_height)
  449b1c: mov    eax,0xff000000
  449b21: lea    ecx,[esp+0x2c]
  449b25: call   0x449780                  ; draw top bar
  449b5f: fld    DWORD PTR ds:0x672b9c     ; 480.0f
  449b65: fsub   DWORD PTR [esp+0x10]      ; 480.0f - bar_height  (bar_height reused, not
                                           ; recomputed -- same letterbox_scale both times)
  449b8d: mov    WORD PTR [esp+0x2c],dx    ; rect.top = ROUND(480.0f - bar_height)
  449b92: mov    WORD PTR [esp+0x30],0x1e1 ; rect.bottom = 481 (k_cinematic_letterbox_bottom_edge,
                                           ; a plain immediate, not FPU-rounded like the rest)
  449b99: call   0x449780                  ; draw bottom bar

objdump excerpt for the title text-bounds fallback Ghidra also dropped (0x449c2f..0x449c59; esi
is the ScenarioCutsceneTitle*, i.e. text_bounds in types/tags.h at +0x28..+0x2f):

  449c2f: mov    cx,WORD PTR [esi+0x2e]    ; title->text_bounds.right
  449c33: cmp    cx,WORD PTR [esi+0x2a]    ; vs .left
  449c37: lea    ebx,[esi+0x28]            ; ebx = &title->text_bounds (tentative dest rect)
  449c42: je     0x449c4d
  449c44: mov    dx,WORD PTR [ebx+0x4]     ; .bottom
  449c48: cmp    dx,WORD PTR [ebx]         ; vs .top
  449c4b: jne    0x449c59                  ; not empty: keep ebx = &title->text_bounds
  449c4d: mov    ebx,DWORD PTR ds:0x71941c
  449c53: add    ebx,0x2dc                 ; else ebx = &hud_globals->default_chapter_title_bounds
  449c59: ...
  ; ecx (dest_rect_override) at the call site 0x449e33 is this ebx.
#endif
