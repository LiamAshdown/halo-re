// render_view_camera_fill  (Ghidra: FUN_004c9050; still unnamed -> renamed)
// address 0x4c9050, size 459 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: out/phase4/main_types_notes.md "Register arguments confirmed at call sites":
// "FUN_004c9050 0x4c9050: EAX = observer_camera or NULL, ECX = render_view (0x4c93ad)". Every
// field offset below is confirmed against objdump -d -M intel bin/halo.exe at
// 0x4c9050..0x4c921a (both branches, byte for byte). Default vectors when observer is NULL:
// global_zero_vector3d_pointer (0x006966f8 -> (0,0,0)), global_forward3d_pointer
// (0x00696718 -> (1,0,0)) and global_up3d_pointer (0x00696720 -> (0,0,1)), all three already
// named this way in src/camera/observer_new.c's own header comment. game_time (0x006f1d6c),
// current_game_engine-adjacent console_globals.active (0x006b7020, console_globals in
// types/main.h) and camera_get_type_for_player (0x445ac0) reuse established names/signatures.
// matrix4x3_from_forward_up_position and matrix4x3_multiply reuse
// src/math/matrix4x3_from_forward_up_position.c and other files' established signatures; the
// indirect call through PTR_matrix4x3_multiply_00696664 resolves statically to matrix4x3_multiply
// itself (0x4cc0d0), per this batch's own callee list.
// register convention: EAX -> observer (nullable), ECX -> view.
// phase 4 review (disassembly 0x4c9050..0x4c921a): every store, the FOV formula and all five
// callee register shapes re-checked; the two float constants are now written as floats.
// UNSURE: matrix4x3_extract_forward_up_position (0x4cbd90) and player_effect_build_camera_shake_matrix (the per-player
// camera shake matrix builder) are outside this batch and not documented anywhere; declared here
// with their best-recovered register conventions from the call sites at 0x4c9139..0x4c9178
// (player_effect_build_camera_shake_matrix: CX -> local_player_index, stack -> out; matrix4x3_extract_forward_up_position:
// EAX -> up_out, ECX -> forward_out, stack -> matrix then position_out, mirroring
// matrix4x3_from_forward_up_position's own EAX/ECX/ESI/stack shape in reverse).
// RESOLVED (R11): 0x0069c65c/0x0069c660 (camera->z_near/z_far source) are the default near/far
// clip pair, now rasterizer.h rasterizer_default_z_near/_far (formerly "rasterizer_letterbox_height").
// 0x00873d30 (gates whether source_camera is refreshed from
// rasterizer_camera) has no established name anywhere; declared as an opaque byte.
// reconciled: R11 0x0069c65c/0x0069c660 externs default_clip_near/far -> rasterizer.h rasterizer_default_z_near/z_far

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "camera.h"
#include "rasterizer.h"
#include "render.h"
#include "main.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_time_globals *game_time;         // 0x006f1d6c, foreign (game module)
extern console_globals console_globals_data; // 0x006b7020, this module's own header type
extern float rasterizer_default_z_near;              // 0x0069c65c, foreign (rasterizer module); rasterizer.h (R11)
extern float rasterizer_default_z_far;               // 0x0069c660, foreign (rasterizer module); rasterizer.h (R11)
extern uint8_t unknown_00873d30;             // TYPES-GAP, UNSURE identity

extern const real_point3d *global_zero_vector3d_pointer; // 0x006966f8 -> (0,0,0), foreign (math module)
extern const real_vector3d *global_forward3d_pointer;    // 0x00696718 -> (1,0,0), foreign (math module)
extern const real_vector3d *global_up3d_pointer;         // 0x00696720 -> (0,0,1), foreign (math module)

extern double tan(double x);
extern double atan2(double y, double x);

extern int16_t camera_get_type_for_player(int16_t local_player_index); // 0x445ac0, foreign (camera module)
    // blam-cc: CX -> local_player_index (0x4c90d4 mov cx,[edi], still live at 0x4c9124)
extern void player_effect_build_camera_shake_matrix(void *out_shake_matrix, int16_t local_player_index); // 0x457390,
    // blam-cc: stack -> out_shake_matrix, CX -> local_player_index (the definition's parameter order)
extern void matrix4x3_from_forward_up_position(real_vector3d *up, real_vector3d *forward,
    real_point3d *position, real_matrix4x3 *out); // 0x4cbd60, foreign (math module)
extern void matrix4x3_extract_forward_up_position(real_vector3d *up_out, real_vector3d *forward_out,
    real_matrix4x3 *matrix, real_point3d *position_out); // 0x4cbd90, foreign, UNSURE signature
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0, foreign

// blam-cc: EAX -> observer, ECX -> view
// Fills view->rasterizer_camera's position/forward/up/field_of_view either from `observer`
// (an observer_camera) or, when observer is NULL, from the engine's default zero/forward/up
// vectors and a fixed 40-degree-ish field of view (identical formula to
// src/main/render_pregame_view_initialize.c's default camera). When an observer is supplied and
// the view belongs to a real local player (local_player_index != -1) with the console closed and
// the game not paused, recomputes the field of view from the observer's own FOV adjusted to the
// view's current aspect ratio, and -- unless the camera type is the dead-camera type (3) -- folds
// in that player's camera shake by building an orientation matrix, multiplying it by the shake
// matrix, and extracting the result back into position/forward/up. Finally seeds z_near/z_far
// from the default clip distances, clears mirrored, and (unless unknown_00873d30 is set)
// refreshes view->source_camera to match the just-built rasterizer_camera.
void render_view_camera_fill(observer_camera *observer, render_view *view)
{
    render_camera *camera = &view->rasterizer_camera;

    if (observer != 0) {
        camera->position.x = observer->position.x;
        camera->position.y = observer->position.y;
        camera->position.z = observer->position.z;
        camera->forward.i = observer->forward.i;
        camera->forward.j = observer->forward.j;
        camera->forward.k = observer->forward.k;
        camera->up.i = observer->up.i;
        camera->up.j = observer->up.j;
        camera->up.k = observer->up.k;

        {
            int32_t width = camera->viewport_bounds.right - camera->viewport_bounds.left;
            int32_t height = camera->viewport_bounds.bottom - camera->viewport_bounds.top;
            double half_fov_tan = tan((double)observer->field_of_view * 0.5);
            camera->vertical_field_of_view =
                (float)(2.0 * atan2(((double)height / (double)width) * half_fov_tan * 0.85f, 1.0)); // 0x00672d28 float
        }

        if (view->local_player_index != -1 && console_globals_data.active == 0 &&
            game_time->paused == 0) {
            if (camera_get_type_for_player(view->local_player_index) != 3) {
                uint8_t shake_matrix[56];
                real_matrix4x3 orientation;

                player_effect_build_camera_shake_matrix(shake_matrix, view->local_player_index);
                matrix4x3_from_forward_up_position((real_vector3d *)&observer->up,
                                                    (real_vector3d *)&observer->forward,
                                                    (real_point3d *)observer, &orientation);
                matrix4x3_multiply(&orientation, (real_matrix4x3 *)shake_matrix, &orientation);
                matrix4x3_extract_forward_up_position(&camera->up, &camera->forward, &orientation,
                                                       &camera->position);
            }
        }
    } else {
        camera->position = *global_zero_vector3d_pointer;
        camera->forward = *global_forward3d_pointer;
        camera->up = *global_up3d_pointer;
        camera->vertical_field_of_view =
            (float)(2.0 * atan2(tan(0.6981316804885864) * 0.6375f, 1.0)); // 0x00673098, 0x00673090 float
    }

    camera->z_far = rasterizer_default_z_far;
    camera->mirrored = 0;
    camera->z_near = rasterizer_default_z_near;

    if (unknown_00873d30 == 0) {
        view->source_camera = view->rasterizer_camera;
    }
}

#if 0
Original Ghidra decompilation (0x4c9050):

void FUN_004c9050(void)

{
  undefined *puVar1;
  undefined *puVar2;
  undefined4 uVar3;
  short sVar4;
  undefined4 *in_EAX;
  short *in_ECX;
  int iVar5;
  short *psVar6;
  bool bVar7;
  float10 fVar8;
  undefined1 local_7c [56];
  undefined1 local_44 [60];
  int local_8;

  puVar1 = PTR_DAT_006966f8;
  psVar6 = in_ECX + 0x2c;
  if (in_EAX == (undefined4 *)0x0) {
    fVar8 = (float10)fptan((float10)0.6981316804885864);
    *(undefined4 *)psVar6 = *(undefined4 *)PTR_DAT_006966f8;
    *(undefined4 *)(in_ECX + 0x2e) = *(undefined4 *)(puVar1 + 4);
    puVar2 = PTR_DAT_00696718;
    *(undefined4 *)(in_ECX + 0x30) = *(undefined4 *)(puVar1 + 8);
    *(undefined4 *)(in_ECX + 0x32) = *(undefined4 *)puVar2;
    *(undefined4 *)(in_ECX + 0x34) = *(undefined4 *)(puVar2 + 4);
    *(undefined4 *)(in_ECX + 0x36) = *(undefined4 *)(puVar2 + 8);
    puVar1 = PTR_DAT_00696720;
    *(undefined4 *)(in_ECX + 0x38) = *(undefined4 *)PTR_DAT_00696720;
    *(undefined4 *)(in_ECX + 0x3a) = *(undefined4 *)(puVar1 + 4);
    *(undefined4 *)(in_ECX + 0x3c) = *(undefined4 *)(puVar1 + 8);
    fVar8 = (float10)fpatan(fVar8 * (float10)0.63750005,(float10)1.0);
    *(float *)(in_ECX + 0x40) = (float)(fVar8 + fVar8);
  }
  else {
    *(undefined4 *)psVar6 = *in_EAX;
    *(undefined4 *)(in_ECX + 0x2e) = in_EAX[1];
    *(undefined4 *)(in_ECX + 0x30) = in_EAX[2];
    *(undefined4 *)(in_ECX + 0x32) = in_EAX[8];
    *(undefined4 *)(in_ECX + 0x34) = in_EAX[9];
    *(undefined4 *)(in_ECX + 0x36) = in_EAX[10];
    *(undefined4 *)(in_ECX + 0x38) = in_EAX[0xb];
    *(undefined4 *)(in_ECX + 0x3a) = in_EAX[0xc];
    *(undefined4 *)(in_ECX + 0x3c) = in_EAX[0xd];
    fVar8 = (float10)fptan((float10)(float)in_EAX[0xe] * (float10)0.5);
    local_8 = (int)in_ECX[0x45] - (int)in_ECX[0x43];
    fVar8 = (float10)fpatan(((float10)((int)in_ECX[0x44] - (int)in_ECX[0x42]) / (float10)local_8) *
                            fVar8 * (float10)0.85,(float10)1.0);
    *(float *)(in_ECX + 0x40) = (float)(fVar8 + fVar8);
    if (((*in_ECX != -1) && (DAT_006b7020 == '\0')) && (*(char *)(DAT_006f1d6c + 2) == '\0')) {
      sVar4 = camera_get_type_for_player();
      if (sVar4 != 3) {
        FUN_00457390(local_7c);
        matrix4x3_from_forward_up_position(local_44);
        (*(code *)PTR_matrix4x3_multiply_00696664)(local_44,local_7c,local_44);
        matrix4x3_extract_forward_up_position(local_44,psVar6);
      }
    }
  }
  uVar3 = DAT_0069c65c;
  *(undefined4 *)(in_ECX + 0x4c) = DAT_0069c660;
  bVar7 = DAT_00873d30 == '\0';
  *(undefined1 *)(in_ECX + 0x3e) = 0;
  *(undefined4 *)(in_ECX + 0x4a) = uVar3;
  if (bVar7) {
    for (iVar5 = 0x15; in_ECX = in_ECX + 2, iVar5 != 0; iVar5 = iVar5 + -1) {
      *(undefined4 *)in_ECX = *(undefined4 *)psVar6;
      psVar6 = psVar6 + 2;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
