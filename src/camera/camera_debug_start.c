// camera_debug_start  (Ghidra: camera_debug_start, already named)
// address 0x444c00, size 327 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: out/phase4/camera_types_notes.md proposes this is really hs `camera_set`: it fills
//   camera_script_globals from a ScenarioCutsceneCameraPoint (position/forward/up/fov, matrix
//   built by matrix4x3_from_euler_angles) or an animation, sets mode 0 and marks it changed,
//   then runs one camera_update(0) tick and forces the local observer to commit immediately
//   (observer_set_command / observer_advance / observer_commit) so the very first frame after a
//   camera_set already shows the new view. Field order/offsets confirmed against
//   camera_script_globals in types/camera.h.
// register convention: cutscene camera point index in AX (in_AX, 16-bit); ticks (an int16:
//   0x444c1e movsx ecx,WORD PTR [ebp+0x8]) and the relative object are cdecl stack parameters, confirmed with objdump (the function
//   loads them at [ebp+8]/[ebp+0xc] relative to the post-prologue frame and returns with a bare
//   `ret`, i.e. the caller cleans up the two stack slots).
//   // blam-cc: AX -> camera_point_index, stack -> (ticks, relative_object)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"

extern Scenario *global_scenario;                    // 0x00746f8c
extern camera_script_globals camera_script;   // 0x006869d0
extern player_globals *local_player_globals;          // 0x0087a478
extern float observer_dt;                             // 0x006ac658
extern observer observers[1];                         // 0x006ac65c

extern void camera_update(float dt);                  // 0x445640, this module
// blam-cc: DX -> local_player_index
extern void observer_set_command(int16_t local_player_index);  // 0x447ab0, this module
// blam-cc: DI -> local_player_index
extern void observer_advance(int16_t local_player_index);      // 0x447b50, this module
// blam-cc: AX -> local_player_index
extern void observer_commit(int16_t local_player_index);       // 0x448900, this module
extern void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll); // 0x4cba10, math module

// hs camera_set: point camera_script_globals at cutscene camera point `camera_point_index` of
// the current scenario (position/orientation/fov), give it `ticks` to live and, when
// `relative_object` is a real object, make the point relative to it. Then runs one camera
// update tick and, if a local player already exists, forces the observer to process and commit
// the new command immediately instead of waiting a frame.
void camera_debug_start(int16_t camera_point_index, int16_t ticks, datum_index relative_object)
{
    ScenarioCutsceneCameraPoint *point;
    real_matrix4x3 matrix; // only the forward and up rows are read back below

    point = (ScenarioCutsceneCameraPoint *)((uint8_t *)global_scenario->cutscene_camera_points.pointer +
                                             camera_point_index * sizeof(ScenarioCutsceneCameraPoint));

    camera_script.mode = _camera_script_mode_point;
    camera_script.changed = 1;
    camera_script.position = point->position;
    camera_script.camera_point_index = camera_point_index;

    matrix4x3_from_euler_angles(&matrix, point->orientation.yaw, point->orientation.pitch,
                                 point->orientation.roll);
    camera_script.forward = *(Vector3D *)&matrix.forward;
    camera_script.up = *(Vector3D *)&matrix.up;

    if (point->field_of_view == 0.0f) {
        camera_script.field_of_view = 1.2217305f; // 70 degrees
    } else {
        camera_script.field_of_view = point->field_of_view;
    }

    camera_script.time_remaining = (float)(ticks / 30);
    camera_script.object = relative_object;

    camera_update(0.0f); // push 0: the float dt argument is 0.0
    observer_dt = 0.0001f;

    if (local_player_globals->local_players[0] != k_datum_index_none) {
        observers[0].updated = 1;
        observer_set_command(0);      // 0x444d10 xor edx,edx
        if (observer_dt != 0.0f) {
            observer_advance(0);      // 0x444d33 xor edi,edi
        }
        observer_commit(0);           // 0x444d3a xor eax,eax
    }
}

#if 0
Original Ghidra decompilation (0x444c00):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void camera_debug_start(short param_1,undefined4 param_2)

{
  short in_AX;
  int iVar1;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;

  iVar1 = in_AX * 0x68 + *(int *)(global_scenario + 0x4f4);
  DAT_006869d2 = 0;
  DAT_006869d1 = 1;
  DAT_006869dc = *(undefined4 *)(iVar1 + 0x28);
  DAT_006869e0 = *(undefined4 *)(iVar1 + 0x2c);
  DAT_006869e4 = *(undefined4 *)(iVar1 + 0x30);
  _DAT_006869d4 = in_AX;
  matrix4x3_from_euler_angles
            (*(undefined4 *)(iVar1 + 0x34),*(undefined4 *)(iVar1 + 0x38),
             *(undefined4 *)(iVar1 + 0x3c));
  DAT_006869e8 = local_38;
  DAT_006869ec = local_34;
  DAT_006869f0 = local_30;
  DAT_006869f4 = local_20;
  DAT_006869f8 = local_1c;
  DAT_006869fc = local_18;
  if (*(float *)(iVar1 + 0x40) == 0.0) {
    DAT_00686a00 = 0x3f9c61aa;
  }
  else {
    DAT_00686a00 = *(undefined4 *)(iVar1 + 0x40);
  }
  _DAT_006869d8 = (float)((int)param_1 / 0x1e);
  DAT_00686a04 = param_2;
  camera_update(0);
  _DAT_006ac658 = 0.0001;
  if (*(int *)(DAT_0087a478 + 4) != -1) {
    DAT_006ac6cc = 1;
    FUN_00447ab0();
    if (_DAT_006ac658 != 0.0) {
      FUN_00447b50();
    }
    FUN_00448900();
  }
  return;
}

Disassembly (objdump -d -M intel, 0x444c00..0x444d47), confirming the register/stack split:

0x444c00: push ebp
0x444c01: mov ebp, esp
0x444c03: sub esp, 0x38
0x444c06: push esi
0x444c07: push edi
0x444c08: mov di, ax                      ; camera_point_index arrives in AX
0x444c16: movsx esi, di
0x444c1e: movsx ecx, word ptr [ebp + 8]    ; ticks (stack arg 1)
0x444c22..0x444c33: imul-based /30 division
0x444c6b..0x444c77: pushes point+0x3c/0x38/0x34 (orientation), lea eax,[ebp-0x38], call matrix4x3_from_euler_angles
0x444cdc: fild dword ptr [ebp + 8]         ; ticks/30 as float
0x444cdf: mov ecx, dword ptr [ebp + 0xc]   ; relative_object (stack arg 2)
0x444cf0: call 0x445640                    ; camera_update(0)
0x444d19: call 0x447ab0
0x444d35: call 0x447b50
0x444d3c: call 0x448900
0x444d46: ret                              ; bare ret -> caller pops both stack args (cdecl)
#endif
