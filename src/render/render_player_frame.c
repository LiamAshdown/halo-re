// render_player_frame  (Ghidra: FUN_0050ba80; named from types/render.h's module header and
// out/phase4/render_types_notes.md, which document this exact address throughout as
// "render_player_frame 0x50ba80" alongside its sibling render_nonplayer_frame 0x50bdc0, and give
// CEA's prototype for its callee render_window as (local_player_index, source_camera,
// source_frustum, rasterizer_camera, rasterizer_frustum, rasterizer_target, has_mirror)).
// address 0x50ba80, size 830 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: types/render.h's module header and the register-conventions section of
//   out/phase4/render_types_notes.md document this function's EAX argument (screenshot tile,
//   may be NULL) and one stack argument (the view). Disassembly (objdump -d -M intel,
//   0x50ba80..0x50bdbe) was used to resolve every hidden register argument Ghidra's decompile
//   left as "()" or "in_EAX": FUN_0050ca90 takes EAX=camera/ECX=bounds-out; the two initial
//   chimera__render_camera_build_frustum calls take ECX=camera/EAX=bounds/ESI=frustum-out/
//   stack=1 (matching the 0x50cc40 register-conventions note); the mirror branch's "in_EAX"
//   pointer is read through EBX, the callee-saved copy of the entry EAX made at 0x50ba96
//   ("mov ebx,eax"); render_camera_mirror (0x50c660) takes EBX=source_camera plus its two stack
//   pointers (confirmed from its own prologue: "rep movsd" of 0x15 dwords from ESI=EBX into
//   EDI=the second stack argument). structure_bsp_mirror_query's two opaque pointer arguments
//   (src/structures/structure_bsp_mirror_query.c) are the source camera and the just-built
//   source frustum, forwarded without being dereferenced here.
// review fix (phase-4 gate, objdump 0x50ba80..0x50bdbd): 0x553490 takes EDX = the source
//   camera (lea edi,[ebp+4]; mov edx,edi at 0x50ba91); the z_far clamp is gated on
//   atmospheric_maximum_density (+0x10, 0x007c3304) == 1.0, not atmospheric_minimum_distance;
//   the mirror result is not pre-seeded with the cluster (0x50bd2f saves the cluster index in
//   its own stack slot).
// register convention: EAX = screenshot_tile (Point2DInt*, may be 0), one stack argument (view).
//   // blam-cc: EAX=screenshot_tile, stack=view
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include "structures.h"
#include "game.h"

extern render_fog render_fog_state;                  // 0x007c32f4, this module (named
                                                     // render_fog_state; a variable cannot share
                                                     // the render_fog typedef's own name in C)
extern uint8_t render_clip_warning;                  // 0x0071cfbd, this module
extern int32_t render_cluster_index;                 // 0x007c3348, structures module
extern int16_t render_cluster_sky_index;             // 0x007c334e, structures module
extern uint32_t rasterizer_device_version;           // 0x007c118c, cache module
extern uint8_t rasterizer_caps_flag_68a;             // 0x0069c68a, rasterizer module
extern game_engine_definition *current_game_engine;  // 0x006f1d20, game module
extern game_engine_state game_engine_state_value;    // 0x0087aa10, game module (renamed to
                                                     // avoid the enum tag, matching
                                                     // src/game/game_engine_begin_end_game_sequence.c)
extern player_globals *local_player_globals;         // 0x0087a478, game module

extern uint8_t *cinematic_globals; // 0x006f187c, UNSURE name (matches
    // src/interface/hud_messaging_update.c and others); byte +9 is a "cinematic playing" flag
    // tested here alongside the multiplayer-round and local-player-count checks

extern int16_t unknown_00696568; // UNSURE name/owner: screenshot tile grid divisor, see below
extern int16_t unknown_00719aac; // UNSURE name/owner: screenshot tile grid divisor, see below

extern void render_camera_update_leaf_and_cluster(real_point3d *point); // 0x553490, structures
    // module; blam-cc: EDX -> point (passed straight on to the leaf probe 0x5013a0; the
    // src/structures rewrite reads the global 0x007c3114 instead, which is not set yet here)

extern void scenario_sky_fog_state_update(int16_t local_player_index, render_camera *camera, render_fog *out_fog,
                          int16_t cluster_sky_index); // 0x53e8c0, foreign (scenario module, not
    // yet rewritten in this project); blam-cc: local_player_index/camera/out_fog on the stack in
    // that order, cluster_sky_index in AX (loaded immediately before the call, after the stack
    // arguments are already pushed)

extern void structure_bsp_build_fog_environment(int16_t cluster_index,
                                                 structure_fog_environment *out); // 0x555330,
    // structures module; blam-cc: AX=cluster_index, ESI=out

extern void render_camera_compute_projection_skew(render_camera *camera, float bounds_out[4]);
    // 0x50ca90, this module; blam-cc: EAX=camera, ECX=bounds_out

extern void chimera__render_camera_build_frustum(float *bounds, render_camera *camera,
                                                  render_frustum *frustum_out,
                                                  uint8_t build_projection); // 0x50cc40, this module;
    // blam-cc: EAX=bounds (may be 0), ECX=camera, ESI=frustum_out, stack=validate

extern void render_camera_mirror(render_camera *source_camera, structure_bsp_mirror_result *mirror,
                                  render_camera *out_camera); // 0x50c660, this module;
    // blam-cc: EBX=source_camera, stack=(mirror, out_camera)

extern uint8_t structure_bsp_mirror_query(void *camera_ref, void *camera,
                                           structure_bsp_mirror_result *out); // 0x553560,
    // structures module

extern void render_window(int16_t local_player_index, render_camera *source_camera,
                           render_frustum *source_frustum, render_camera *rasterizer_camera,
                           render_frustum *rasterizer_frustum, int16_t rasterizer_target,
                           uint8_t has_mirror); // 0x50bfb0, this module (cdecl)

// Draws one local player's window: resolves the camera's leaf/cluster and fog, clamps the far
// clip plane to the fog and to the camera's own near plane, computes the asymmetric projection
// bounds (subdividing them into a screenshot tile when one was requested), builds the source and
// rasterizer frustums, then attempts a mirror/portal reflection pass before drawing the main
// scene.
void render_player_frame(Point2DInt *screenshot_tile, render_view *view) // blam-cc: EAX=screenshot_tile, stack=view
{
    render_camera *source_camera = &view->source_camera;
    uint8_t has_mirror = 0;
    float frustum_bounds[4];
    render_frustum source_frustum;
    render_frustum rasterizer_frustum;
    uint8_t attempt_mirror;

    render_camera_update_leaf_and_cluster(&source_camera->position);

    render_fog_state.unknown_02 = 0;
    scenario_sky_fog_state_update(view->local_player_index, source_camera, &render_fog_state, render_cluster_sky_index);

    // Reuses render_fog as a structure_fog_environment for their shared "planar fog" span: the
    // fields from planar_mode (+0x1c) through planar_maximum_depth (+0x44) line up exactly
    // between the two structs (types/rasterizer.h render_fog, types/structures.h
    // structure_fog_environment).
    structure_bsp_build_fog_environment(render_cluster_index,
                                         (structure_fog_environment *)&render_fog_state);

    if (render_fog_state.atmospheric_maximum_distance != 0.0f && render_cluster_sky_index == -1 &&
        render_fog_state.atmospheric_maximum_distance < render_fog_state.planar_maximum_distance) {
        render_fog_state.planar_maximum_distance = render_fog_state.atmospheric_maximum_distance;
    }
    if (render_fog_state.atmospheric_maximum_density == 1.0f &&
        render_fog_state.atmospheric_maximum_distance != 0.0f) {
        float z_far = render_fog_state.atmospheric_maximum_distance;
        if (source_camera->z_far <= render_fog_state.atmospheric_maximum_distance) {
            z_far = source_camera->z_far;
        }
        source_camera->z_far = z_far;
    }
    if (render_fog_state.planar_mode == 2 && render_fog_state.planar_maximum_distance != 0.0f) {
        float z_far = render_fog_state.planar_maximum_distance;
        if (source_camera->z_far <= render_fog_state.planar_maximum_distance) {
            z_far = source_camera->z_far;
        }
        source_camera->z_far = z_far;
    }
    // Written in the binary as (z_far < z_near) != (z_far == z_near), which is exactly
    // z_far <= z_near; kept as a plain <= here since both compile to the same two-way branch.
    if (source_camera->z_far <= source_camera->z_near) {
        if (!render_clip_warning) {
            render_clip_warning = 1;
        }
        source_camera->z_far = source_camera->z_near + 0.01f;
    }

    render_camera_compute_projection_skew(source_camera, frustum_bounds);

    if (screenshot_tile != 0) {
        int32_t tile_total = (int32_t)unknown_00696568 * (int32_t)unknown_00719aac;
        if (tile_total > 0) {
            // UNSURE: subdivides the asymmetric projection bounds into a screenshot tile grid;
            // the identity of unknown_00696568/unknown_00719aac (both int16 globals) is not
            // recovered from this module alone. Arithmetic preserved exactly.
            float step_x = (frustum_bounds[1] - frustum_bounds[0]) / (float)tile_total;
            float step_y = (frustum_bounds[3] - frustum_bounds[2]) / (float)tile_total;
            float x0 = (float)screenshot_tile->x * step_x + frustum_bounds[0];
            float y0 = (float)(tile_total - screenshot_tile->y - 1) * step_y + frustum_bounds[2];
            frustum_bounds[0] = x0;
            frustum_bounds[2] = y0;
            frustum_bounds[1] = x0 + step_x;
            frustum_bounds[3] = y0 + step_y;
        }
    }

    chimera__render_camera_build_frustum(frustum_bounds, source_camera, &source_frustum, 1);
    chimera__render_camera_build_frustum(frustum_bounds, &view->rasterizer_camera,
                                          &rasterizer_frustum, 1);

    // UNSURE: this gate always evaluates true in this retail build. types/game.h notes that
    // player_globals only ever has one local player slot here, which makes the final re-check of
    // local_player_count against 1 unreachable (it can only fail once local_player_count is already known
    // to equal 1). Reproduced exactly rather than simplified away, in case a splitscreen build
    // gave the second comparison a different operand.
    attempt_mirror = 1;
    if (!(current_game_engine != 0 && game_engine_state_value >= _game_engine_state_ended &&
          game_engine_state_value <= _game_engine_state_post_game)) {
        if (cinematic_globals[9] == 0) {
            int16_t local_player_count = local_player_globals->local_player_count;
            if (local_player_count == 1 && local_player_count != 1) {
                attempt_mirror = 0; // never reachable; see the UNSURE note above
            }
        }
    }

    if (attempt_mirror) {
        structure_bsp_mirror_result mirror_result;
        if (structure_bsp_mirror_query(source_camera, &source_frustum, &mirror_result) &&
            rasterizer_device_version > 0xffff0100 && !rasterizer_caps_flag_68a) {
            render_camera mirror_camera;
            render_frustum mirror_frustum;
            int32_t saved_cluster_index = render_cluster_index;

            render_camera_mirror(source_camera, &mirror_result, &mirror_camera);
            chimera__render_camera_build_frustum(frustum_bounds, &mirror_camera, &mirror_frustum, 1);
            render_cluster_index = mirror_result.cluster_index;
            render_window(-1, &mirror_camera, &mirror_frustum, &mirror_camera, &mirror_frustum,
                          _render_target_mirror, 0);
            render_cluster_index = saved_cluster_index;
            has_mirror = 1;
        }
    }

    render_window(view->local_player_index, source_camera, &source_frustum,
                  &view->rasterizer_camera, &rasterizer_frustum, _render_target_main, has_mirror);
}

#if 0
Original Ghidra decompilation (0x50ba80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0050ba80(undefined2 *param_1)

{
  undefined2 *puVar1;
  short sVar2;
  float fVar3;
  char cVar4;
  short *in_EAX;
  int iVar5;
  short local_540;
  float local_538;
  float local_534;
  float local_530;
  float local_52c;
  undefined4 local_528;
  float local_524;
  float local_520;
  float local_51c;
  float local_518;
  short local_50c;
  undefined1 local_508 [88];
  undefined1 local_4b0 [400];
  undefined1 local_320 [400];
  undefined1 local_190 [400];

  puVar1 = param_1 + 2;
  local_528 = 0;
  FUN_00553490();
  DAT_007c32f4._2_2_ = 0;
  FUN_0053e8c0(*param_1,puVar1,&DAT_007c32f4);
  FUN_00555330();
  if (((DAT_007c330c != 0.0) && (DAT_007c334e == -1)) && (DAT_007c330c < _DAT_007c3334)) {
    _DAT_007c3334 = DAT_007c330c;
  }
  if ((_DAT_007c3304 == 1.0) && (DAT_007c330c != 0.0)) {
    fVar3 = DAT_007c330c;
    if (*(float *)(param_1 + 0x22) <= DAT_007c330c) {
      fVar3 = *(float *)(param_1 + 0x22);
    }
    *(float *)(param_1 + 0x22) = fVar3;
  }
  if ((DAT_007c3310 == 2) && (_DAT_007c3334 != 0.0)) {
    fVar3 = _DAT_007c3334;
    if (*(float *)(param_1 + 0x22) <= _DAT_007c3334) {
      fVar3 = *(float *)(param_1 + 0x22);
    }
    *(float *)(param_1 + 0x22) = fVar3;
  }
  if (*(float *)(param_1 + 0x22) < *(float *)(param_1 + 0x20) !=
      (*(float *)(param_1 + 0x22) == *(float *)(param_1 + 0x20))) {
    if (DAT_0071cfbd == '\0') {
      DAT_0071cfbd = '\x01';
    }
    *(float *)(param_1 + 0x22) = *(float *)(param_1 + 0x20) + 0.01;
  }
  FUN_0050ca90();
  if ((in_EAX != (short *)0x0) && (iVar5 = (int)(short)DAT_00696568 * (int)DAT_00719aac, 0 < iVar5))
  {
    local_520 = (local_534 - local_538) / (float)iVar5;
    local_518 = (local_52c - local_530) / (float)iVar5;
    local_524 = (float)(int)*in_EAX * local_520 + local_538;
    local_51c = (float)((iVar5 - in_EAX[1]) + -1) * local_518 + local_530;
    local_520 = local_524 + local_520;
    local_518 = local_51c + local_518;
  }
  chimera__render_camera_build_frustum(1);
  chimera__render_camera_build_frustum(1);
  if ((((((DAT_006f1d20 != 0) && (1 < DAT_0087aa10)) && (DAT_0087aa10 < 4)) ||
       ((*(char *)(DAT_006f187c + 9) != '\0' || (sVar2 = *(short *)(DAT_0087a478 + 0xc), sVar2 < 1))
       )) || ((1 < sVar2 || (sVar2 == 1)))) &&
     (((cVar4 = FUN_00553560(puVar1,local_320,&local_524), iVar5 = _DAT_007c3348, cVar4 != '\0' &&
       (0xffff0100 < DAT_007c118c)) && (DAT_0069c68a == '\0')))) {
    FUN_0050c660(&local_524,local_508);
    chimera__render_camera_build_frustum(1);
    _DAT_007c3348 = (int)local_50c;
    render_scene_draw(0xffffffff,local_508,local_4b0,local_508,local_4b0,2,0);
    local_540 = (short)iVar5;
    _DAT_007c3348 = (int)local_540;
    local_528 = 1;
  }
  render_scene_draw(*param_1,puVar1,local_320,param_1 + 0x2c,local_190,1,local_528);
  return;
}

Relevant disassembly (objdump -d -M intel, 0x50ba80..0x50bdbe) used to resolve the hidden
register arguments to FUN_0050ca90, chimera__render_camera_build_frustum (0x50cc40) and
render_camera_mirror (0x50c660):

0050ba80:
  sub    esp,0x540
  push   ebx
  push   ebp
  mov    ebp,DWORD PTR [esp+0x54c]      ; ebp = param_1 (view*)
  push   esi
  push   edi
  lea    edi,[ebp+0x4]                  ; edi = &view->source_camera
  mov    edx,edi
  mov    ebx,eax                        ; ebx = entry EAX (screenshot_tile)
  mov    BYTE PTR [esp+0x28],0x0
  call   0x553490
  ...
  lea    ecx,[esp+0x18]                 ; &local_538 (frustum_bounds)
  mov    eax,edi                        ; source_camera
  call   0x50ca90                       ; blam-cc: EAX=camera, ECX=bounds_out
  test   ebx,ebx                        ; the decompiler's "in_EAX" null check is really EBX
  je     0x50bc77
  movsx  eax,WORD PTR ds:0x696568
  movsx  edx,WORD PTR ds:0x719aac
  imul   eax,edx
  ...
  movsx  ecx,WORD PTR [ebx]             ; *in_EAX, read through EBX
  ...
  movsx  edx,WORD PTR [ebx+0x2]         ; in_EAX[1], read through EBX
  ...
  lea    ecx,[esp+0x54]                 ; &source_frustum
  mov    eax,edi                        ; source_camera        (folded into "mov eax,edi" above)
  lea    esi,[esp+0x234]                ; &source_frustum out buffer (local_320)
  push   0x1
  call   0x50cc40                       ; blam-cc: ECX=camera, EAX=bounds, ESI=frustum_out, stack=1
  lea    ecx,[ebp+0x58]                 ; &view->rasterizer_camera
  push   0x1
  lea    esi,[esp+0x3c4]                ; &rasterizer_frustum out buffer (local_190)
  lea    eax,[esp+0x1c]                 ; frustum_bounds again
  call   0x50cc40
  ...
  lea    eax,[esp+0x2c]                 ; &mirror_result
  push   eax
  lea    ecx,[esp+0x234]                ; &source_frustum (forwarded opaque)
  push   ecx
  push   edi                            ; source_camera (forwarded opaque)
  call   0x553560                       ; structure_bsp_mirror_query(camera_ref, camera, out)
  ...
  lea    eax,[esp+0x48]                 ; &mirror_camera (out)
  push   eax
  lea    ecx,[esp+0x30]                 ; &mirror_result
  push   ecx
  mov    ebx,edi                        ; EBX = source_camera
  call   0x50c660                       ; blam-cc: EBX=source_camera, stack=(mirror, out_camera)
  ...
  push   edx                            ; has_mirror
  push   0x1                            ; _render_target_main
  lea    eax,[esp+0x3c8]
  push   eax                            ; &rasterizer_frustum
  xor    edx,edx
  mov    dx,WORD PTR [ebp+0x0]          ; view->local_player_index
  lea    eax,[ebp+0x58]
  push   eax                            ; &view->rasterizer_camera
  lea    ecx,[esp+0x240]
  push   ecx                            ; &source_frustum
  push   edi                            ; source_camera
  push   edx                            ; local_player_index
  call   0x50bfb0                       ; render_window(...)

render_camera_mirror's own prologue (0x50c660) confirms EBX is a real register parameter:

0050c660:
  sub    esp,0x3c
  mov    eax,DWORD PTR [esp+0x40]       ; stack arg 1 = mirror result
  ...
  push   ebp
  mov    ebp,DWORD PTR [esp+0x48]       ; stack arg 2 = out camera
  push   esi
  ...
  mov    ecx,0x15
  mov    esi,ebx                        ; source = EBX
  mov    edi,ebp                        ; dest = out camera
  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]   ; copies the whole render_camera (0x15 dwords)
#endif
