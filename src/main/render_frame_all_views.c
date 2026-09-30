// render_frame_all_views  (Ghidra: render_frame_all_views, already named)
// address 0x4c9260, size 714 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: matches the given name; confirmed field-by-field against objdump -d -M intel
// bin/halo.exe at 0x4c9260..0x4c9529. render_views[2] (0x00719b70, types/render.h render_view,
// "the local player views followed by the trailing non player view") and every camera-default
// global reuse src/main/render_view_camera_fill.c's names (that function, FUN_004c9050, is
// called directly here per local player view; the trailing view repeats its NULL-observer branch
// inline, plus a leading viewport_split_rect_compute call the per-player loop also makes).
// observers[1].camera (0x006ac6d0 = observers base 0x006ac65c + offsetof(observer, camera)=0x74,
// types/camera.h) supplies the observer_camera* for a resolved local_player_index.
// current_game_engine/game_engine_state_value/local_player_globals/widget_memory_pool_valid/
// ui_root_widget/cinematic_globals_ptr reuse src/interface/ui_error_modal_update.c's names.
// screenshots (0x007196e0) reuses src/shell/shell_winmain.c's name; render_frame (Ghidra:
// render_views_draw_all) reuses src/render/render_frame.c's renamed signature.
// register convention: cdecl, two ordinary float stack parameters (time_since_tick,
// phase 4 review (disassembly 0x4c9260..0x4c9529: view loop, trailing view, screenshot dispatch match; the screenshot inputs are now input_globals fields and the FOV factor is the float 0.6375f.
// time_since_frame), forwarded to render_frame's last two parameters.
// UNSURE: 0x0068943c (gates whether a resolved local_player_index is cached across views in the
// same frame) has no established name anywhere; declared as an opaque byte. 0x007127d2 and
// 0x007124aa ("the screenshot input state" per out/phase4/main_types_notes.md) are
// input_globals.system_key_states[2] (print) and input_globals.states[0].buttons[0x12] (phase 4
// review; they were TYPES-GAP bytes). EBX (render_frame's screenshot_tile) is
// confirmed 0 (NULL) at this call site (`xor ebx,ebx` at 0x4c9509, immediately before the call).
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)
// reconciled: R11 0x0069c65c/0x0069c660 externs default_clip_near/far -> rasterizer.h rasterizer_default_z_near/z_far

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "cache.h"
#include "game.h"
#include "camera.h"
#include "rasterizer.h"
#include "render.h"
#include "networking.h"
#include "saved_games.h"
#include "input.h"
#include "main.h"
#include <string.h>
#include "units.h"
#include "cutscene.h"
#include "fn_sound.h"

extern int32_t player_effect_reentry_count; // 0x00719ccc, foreign (effects module)
extern main_globals main_globals_data;      // 0x00719700
extern render_view render_views[2];         // 0x00719b70

extern game_engine_definition *current_game_engine; // 0x006f1d20, foreign
extern game_engine_state game_engine_state_value;   // 0x0087aa10, foreign (game module)
extern player_globals *local_player_globals;        // 0x0087a478, foreign (game module)
extern uint8_t widget_memory_pool_valid;            // 0x00718fc2, foreign (interface module)
extern widget_instance *ui_root_widget[1];          // 0x00718f94, foreign (interface module)
extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c
extern observer observers[1];                       // 0x006ac65c, foreign (camera module)
extern uint8_t render_view_local_player_sticky;     // 0x0068943c, TYPES-GAP, UNSURE identity
extern int32_t screenshots;                         // 0x007196e0, foreign (shell module)
extern input_abstraction_globals input_globals;     // 0x00710328, foreign (input module)

extern const real_point3d *global_zero_vector3d_pointer; // 0x006966f8 -> (0,0,0), foreign (math module)
extern const real_vector3d *global_forward3d_pointer;    // 0x00696718 -> (1,0,0), foreign (math module)
extern const real_vector3d *global_up3d_pointer;         // 0x00696720 -> (0,0,1), foreign (math module)
extern float rasterizer_default_z_near;                          // 0x0069c65c, foreign (rasterizer module)
extern float rasterizer_default_z_far;                           // 0x0069c660, foreign (rasterizer module)
extern uint8_t unknown_00873d30;                          // TYPES-GAP, UNSURE identity

extern double tan(double x);
extern double atan2(double y, double x);


extern void viewport_split_rect_compute(int32_t view_count, int32_t view_index,
    Rectangle2D *window, Rectangle2D *out_viewport); // 0x4c8da0, this module
extern void render_view_camera_fill(observer_camera *observer, render_view *view); // 0x4c9050, this module
extern void render_frame(Point2DInt *screenshot_tile, render_view *views, int16_t count,
    Point2DInt *screenshot_page, float time_since_tick, float time_since_frame); // 0x50bea0, foreign (render module)
extern void screenshot_render(render_view *views); // 0x4ca1a0, this module

// Builds this frame's render_views array (one entry per local player, plus a trailing
// non-player view) and dispatches to render_frame (or, when a screenshot is pending, to
// screenshot_render instead). Each local player view's camera is filled by
// render_view_camera_fill from that player's observer, unless the game engine is showing
// end-of-game results for 2..3 local players (in which case every view is forced to the
// trailing non-player camera instead, one entry only).
void render_frame_all_views(float time_since_tick, float time_since_frame)
{
    uint8_t showing_results;
    int16_t local_player_count_field;
    int32_t view_count;
    int32_t i;
    int16_t resolved_local_player_index;
    render_view *view;

    player_effect_reentry_count = player_effect_reentry_count + 1;
    sound_update();

    showing_results = (current_game_engine != 0 && (int32_t)game_engine_state_value > 1 &&
                        (int32_t)game_engine_state_value < 4)
                          ? 1
                          : 0;

    local_player_count_field = local_player_globals->local_player_count;
    view_count = 1; // every branch of the original clamp converges on 1 here (k_maximum_local_players)

    if (widget_memory_pool_valid != 0 && ui_root_widget[0] != 0) {
        strstr(ui_root_widget[0]->name, "error_modal");
    }
    if (showing_results || cinematic_globals_ptr->in_progress != 0) {
        view_count = 1;
    }

    resolved_local_player_index = -1;
    for (i = 0; i < view_count; i++) {
        view = &render_views[i];
        viewport_split_rect_compute(view_count, i, &view->rasterizer_camera.window_bounds,
                                     &view->rasterizer_camera.viewport_bounds);

        if (showing_results || i >= view_count) {
            view->local_player_index = -1;
        } else {
            int16_t candidate;

            if (render_view_local_player_sticky == 0 || resolved_local_player_index == -1) {
                if (main_globals_data.game_connection == 3) {
                    candidate = 0;
                } else {
                    candidate = -1;
                    if (local_player_globals->local_players[0] != (datum_index)-1 &&
                        resolved_local_player_index < 0) {
                        candidate = 0;
                    }
                }
            } else {
                candidate = resolved_local_player_index;
            }
            view->local_player_index = candidate;
            resolved_local_player_index = candidate;
        }

        render_view_camera_fill(view->local_player_index != -1
                                     ? &observers[view->local_player_index].camera
                                     : 0,
                                 view);
        view->nonplayer = 0;
    }

    // Trailing non-player view (render_view_camera_fill's own NULL-observer branch, inlined):
    view = &render_views[view_count];
    viewport_split_rect_compute(1, 0, &view->rasterizer_camera.window_bounds,
                                 &view->rasterizer_camera.viewport_bounds);
    view->local_player_index = -1;
    view->nonplayer = 1;
    view->rasterizer_camera.position = *global_zero_vector3d_pointer;
    view->rasterizer_camera.forward.i = global_forward3d_pointer->i;
    view->rasterizer_camera.forward.j = global_forward3d_pointer->j;
    view->rasterizer_camera.forward.k = global_forward3d_pointer->k;
    view->rasterizer_camera.up.i = global_up3d_pointer->i;
    view->rasterizer_camera.up.j = global_up3d_pointer->j;
    view->rasterizer_camera.up.k = global_up3d_pointer->k;
    view->rasterizer_camera.z_near = rasterizer_default_z_near;
    view->rasterizer_camera.mirrored = 0;
    view->rasterizer_camera.z_far = rasterizer_default_z_far;
    view->rasterizer_camera.vertical_field_of_view =
        (float)(2.0 * atan2(tan(0.6981316804885864) * 0.6375f, 1.0)); // 0x00673098 (40 deg), 0x00673090 float, 0x00672af8
    if (unknown_00873d30 == 0) {
        view->source_camera = view->rasterizer_camera;
    }

    if (screenshots == 0) {
        render_frame(0, render_views, (int16_t)(view_count + 1), 0, time_since_tick,
                     time_since_frame);
        player_effect_reentry_count = player_effect_reentry_count - 1;
        return;
    }

    if (input_globals.system_key_states[2] == 0 &&       // 0x007127d2 print screen hold count
        input_globals.states[0].buttons[0x12] == 0) {   // 0x007124aa local player 0 button 18
        if (main_globals_data.screenshot_tile_count < 1) {
            render_frame(0, render_views, (int16_t)(view_count + 1), 0, time_since_tick,
                         time_since_frame);
            player_effect_reentry_count = player_effect_reentry_count - 1;
            return;
        }
    } else {
        main_globals_data.screenshot_tile_count = 1;
    }
    screenshot_render(render_views);
    player_effect_reentry_count = player_effect_reentry_count - 1;
}

#if 0
Original Ghidra decompilation (0x4c9260):

void render_frame_all_views(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  bool bVar2;
  short sVar3;
  undefined *puVar4;
  undefined *puVar5;
  char cVar6;
  short sVar7;
  int iVar8;
  short *psVar9;
  undefined4 *puVar10;
  int iVar11;
  undefined4 *puVar12;
  float10 fVar13;
  int local_18;
  int local_10;

  DAT_00719ccc = DAT_00719ccc + 1;
  sound_update();
  bVar2 = false;
  if (((DAT_006f1d20 != 0) && (1 < DAT_0087aa10)) && (DAT_0087aa10 < 4)) {
    bVar2 = true;
  }
  sVar7 = *(short *)(DAT_0087a478 + 0xc);
  sVar3 = -1;
  if (sVar7 < 1) {
    local_10 = 1;
  }
  else if (sVar7 < 2) {
    local_10 = (int)sVar7;
  }
  else {
    local_10 = 1;
  }
  if ((DAT_00718fc2 != '\0') && (DAT_00718f94 != 0)) {
    FUN_00625430(*(undefined4 *)(DAT_00718f94 + 4),"error_modal");
  }
  if ((bVar2) || (*(char *)(DAT_006f187c + 9) != '\0')) {
    local_10 = 1;
  }
  local_18 = 0;
  if (0 < local_10) {
    psVar9 = &DAT_00719b70;
    do {
      viewport_split_rect_compute(psVar9 + 0x42);
      if ((bVar2) || (local_10 <= local_18)) {
        *psVar9 = -1;
      }
      else {
        if ((DAT_0068943c == '\0') || (sVar7 = sVar3, sVar3 == -1)) {
          if (DAT_00719720 == 3) {
            sVar7 = 0;
          }
          else {
            sVar7 = -1;
            if ((*(int *)(DAT_0087a478 + 4) != -1) && (sVar3 < 0)) {
              sVar7 = 0;
            }
          }
        }
        *psVar9 = sVar7;
        sVar3 = sVar7;
      }
      FUN_004c9050();
      local_18 = local_18 + 1;
      *(undefined1 *)(psVar9 + 1) = 0;
      psVar9 = psVar9 + 0x56;
    } while (local_18 < local_10);
  }
  iVar11 = local_10 * 0xac;
  viewport_split_rect_compute(&DAT_00719bf4 + iVar11);
  puVar4 = PTR_DAT_006966f8;
  (&DAT_00719b70)[local_10 * 0x56] = 0xffff;
  fVar13 = (float10)fptan((float10)0.6981316804885864);
  (&DAT_00719b72)[iVar11] = 1;
  *(undefined4 *)(iVar11 + 0x719bc8) = *(undefined4 *)puVar4;
  *(undefined4 *)(iVar11 + 0x719bcc) = *(undefined4 *)(puVar4 + 4);
  puVar5 = PTR_DAT_00696718;
  *(undefined4 *)(iVar11 + 0x719bd0) = *(undefined4 *)(puVar4 + 8);
  *(undefined4 *)(iVar11 + 0x719bd4) = *(undefined4 *)puVar5;
  *(undefined4 *)(iVar11 + 0x719bd8) = *(undefined4 *)(puVar5 + 4);
  *(undefined4 *)(iVar11 + 0x719bdc) = *(undefined4 *)(puVar5 + 8);
  puVar4 = PTR_DAT_00696720;
  *(undefined4 *)(iVar11 + 0x719be0) = *(undefined4 *)PTR_DAT_00696720;
  *(undefined4 *)(iVar11 + 0x719be4) = *(undefined4 *)(puVar4 + 4);
  uVar1 = *(undefined4 *)(puVar4 + 8);
  *(undefined4 *)(iVar11 + 0x719c04) = DAT_0069c65c;
  cVar6 = DAT_00873d30;
  *(undefined4 *)(iVar11 + 0x719be8) = uVar1;
  uVar1 = DAT_0069c660;
  *(undefined1 *)(iVar11 + 0x719bec) = 0;
  *(undefined4 *)(iVar11 + 0x719c08) = uVar1;
  fVar13 = (float10)fpatan(fVar13 * (float10)0.63750005,(float10)1.0);
  *(float *)(iVar11 + 0x719bf0) = (float)(fVar13 + fVar13);
  if (cVar6 == '\0') {
    puVar10 = (undefined4 *)(iVar11 + 0x719bc8);
    puVar12 = (undefined4 *)(iVar11 + 0x719b74);
    for (iVar8 = 0x15; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar12 = *puVar10;
      puVar10 = puVar10 + 1;
      puVar12 = puVar12 + 1;
    }
  }
  if (DAT_007196e0 == 0) {
LAB_004c9505:
    render_views_draw_all(&DAT_00719b70,local_10 + 1,0,param_1,param_2);
    DAT_00719ccc = DAT_00719ccc + -1;
    return;
  }
  if ((DAT_007127d2 == '\0') && (DAT_007124aa == '\0')) {
    if (DAT_00719aac < 1) goto LAB_004c9505;
  }
  else {
    DAT_00719aac = 1;
  }
  screenshot_render(&DAT_00719b70);
  DAT_00719ccc = DAT_00719ccc + -1;
  return;
}
#endif
