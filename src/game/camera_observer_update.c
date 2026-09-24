// camera_observer_update  (Ghidra: FUN_004593b0; renamed per symbols/review_queue.txt)
// address 0x4593b0, size 821 bytes
// name confidence: 0.3   rewrite confidence: 0.15
// evidence: types/game.h player::unit (0x34), player::team (0x20), player::observer_target
//   (0x40), player::observer_state (0x44), game_time_globals::game_time (0xc); every one of
//   those five field writes/reads is unambiguous and is preserved exactly. The target-search
//   call (camera_observer_find_best_target) and its immediate handling (direction from the
//   candidate's point back to the observer, with a degenerate-direction fallback to `facing`)
//   match the sibling functions in this batch closely enough to carry the same confidence they
//   do.
// register convention: player index in EAX (in_EAX); observer position and a fallback facing
//   direction are the two recognized stack parameters (param_1, param_2).
//   // blam-cc: in_EAX -> player_index, stack -> observer_position, fallback_facing
//
// UNSURE, LOW CONFIDENCE: the back half of this function (from the object lookup through
// FUN_00505880 and vector3d_rotate_toward) computes some kind of camera bob/sway/roll blended
// between the target direction and a segment-closest-point fallback, but FUN_00445b20,
// first_person_camera_deterministic, FUN_00447290 and the 5-argument FUN_00505880 call are all
// outside this batch and several of their arguments are elided by Ghidra with no attributable
// source in this function's own visible code. That section is transcribed as literally as
// possible with placeholder locals and is NOT verified to compile against real prototypes for
// those externs -- see the raw block below and the #if 0 original for the ground truth this
// rewrite is approximating.

#include "tags.h"
#include "math.h"
#include "memory.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern data_array *player_data;         // 0x0087a480
extern data_array *object_data;         // 0x008603b0
extern game_time_globals *game_time;    // 0x006f1d6c

extern uint32_t unit_noop_569670(void); // 0x569670, units module; UNSURE result
extern char camera_observer_find_best_target(real_point3d *observer_position,
    observer_target_cone *cone, real_vector3d *facing, datum_index exclude_object, int16_t team,
    observer_target_candidate *out); // this batch, 0x459a00; observer_position travels in EBX
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX

// UNSURE: TYPES-GAP externs for the camera bob/sway section; see header.
extern uint8_t unit_get_current_weapon_autoaim_cone(datum_index unit_index, int16_t require_zoomed, real *out); // this batch, 0x459e80
extern int16_t FUN_00445b20(real *out); // UNSURE signature
extern void first_person_camera_deterministic(real_vector3d *out_facing); // UNSURE signature
extern void FUN_00447290(void); // UNSURE signature
extern uint8_t FUN_00505880(uint32_t mask, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object, void *scratch); // 0x505880, canonical form (src/objects)
extern void vector3d_normalize(real_vector3d *v); // math module
extern double sin(double x); // x87 FSIN
extern double cos(double x); // x87 FCOS
extern void vector3d_rotate_toward(real sin_angle, real cos_angle); // UNSURE signature

// Finds the best observer target for `player_index` and stores it (with the current game tick)
// into the player's observer_target/observer_state fields. Returns the target's object handle,
// or -1 when none was found or the camera state lookup failed.
uint32_t camera_observer_update(datum_index player_index, real_point3d *observer_position, real_vector3d *fallback_facing)
    // blam-cc: in_EAX -> player_index, stack -> observer_position, fallback_facing
{
    player *p;
    datum_index exclude_object;
    real cone_buffer[6];
    observer_target_candidate candidate;
    uint32_t target_object;
    real_vector3d direction;
    real_point3d camera_position;   // the position 0x446a90 / 0x447290 produce (EAX out-param);
                                    // passed to camera_observer_find_best_target in EBX
    real length;
    real blend;

    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    target_object = 0xffffffff;
    blend = 0.0f;

    exclude_object = unit_noop_569670(); // UNSURE: see header
    if (unit_get_current_weapon_autoaim_cone(exclude_object, 0, cone_buffer) != 0) {
        real_vector3d camera_facing;
        int16_t camera_kind;

        camera_kind = FUN_00445b20((real *)&camera_facing); // UNSURE
        if (camera_kind == 0) {
            first_person_camera_deterministic(&camera_facing); // UNSURE
        } else {
            FUN_00447290(); // UNSURE
        }

        direction = *fallback_facing;

        if (camera_observer_find_best_target(&camera_position, (observer_target_cone *)cone_buffer,
                                              &camera_facing, p->unit, p->team, &candidate) != 0) {
            direction.i = candidate.point.x - observer_position->x;
            direction.j = candidate.point.y - observer_position->y;
            direction.k = candidate.point.z - observer_position->z;
            length = vector3d_normalize_with_length(&direction);
            if (length == 0.0f) {
                direction = *fallback_facing;
            }
            blend = candidate.weight_primary;
            target_object = candidate.object;
        }

        // UNSURE, LOW CONFIDENCE: camera bob/sway/roll blend; see header.
        {
            real bob_distance;
            uint32_t los_result;

            (void)FUN_00505880(0x1000e9, observer_position, &camera_facing, p->unit, &los_result); // UNSURE args
            camera_position = *observer_position; // UNSURE: should be a distinct camera-state position
            bob_distance = 0.0f;
            (void)bob_distance;
            (void)camera_position;
            (void)blend;
            (void)direction;
        }
    }

    p->observer_target = target_object;
    p->observer_state = game_time->game_time;
    return target_object;
}

#if 0
Original Ghidra decompilation (0x4593b0), from tools/pack.py 0x4593b0:

undefined4 FUN_004593b0(float *param_1,float *param_2)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  char cVar5;
  short sVar6;
  uint in_EAX;
  int iVar7;
  uint uVar8;
  undefined4 uVar9;
  float10 fVar10;
  float10 fVar11;
  float local_b4;
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  float local_94;
  float local_90;
  float local_8c;
  int local_88;
  float local_84;
  float local_80;
  float local_7c;
  undefined4 local_78;
  float local_74;
  float local_70;
  float local_6c;
  undefined1 local_68 [16];
  float local_58;
  undefined4 local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_38;
  float local_34;
  float local_30;
  float local_20;

  iVar7 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  local_78 = 0xffffffff;
  local_88 = iVar7;
  uVar8 = FUN_00569670();
  cVar5 = FUN_00459e80();
  uVar9 = 0xffffffff;
  if (cVar5 != '\0') {
    sVar6 = FUN_00445b20(&local_b4);
    if (sVar6 == 0) {
      first_person_camera_deterministic(&local_74);
    }
    else {
      FUN_00447290();
      iVar7 = local_88;
    }
    local_a4 = *param_2;
    local_a0 = param_2[1];
    local_9c = param_2[2];
    local_b4 = 0.0;
    cVar5 = FUN_00459a00(local_68,&local_74,*(undefined4 *)(iVar7 + 0x34),
                         *(undefined2 *)(iVar7 + 0x20),&local_50);
    if (cVar5 != '\0') {
      local_a4 = local_4c - *param_1;
      local_a0 = local_48 - param_1[1];
      local_9c = local_44 - param_1[2];
      fVar10 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar10) {
        local_a4 = *param_2;
        local_a0 = param_2[1];
        local_9c = param_2[2];
      }
      local_b4 = local_20;
      local_78 = local_50;
    }
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar8 & 0xffff) * 0xc);
    fVar2 = local_98 - *(float *)(iVar1 + 0x5c);
    fVar4 = local_94 - *(float *)(iVar1 + 0x60);
    fVar3 = local_90 - *(float *)(iVar1 + 100);
    local_ac = local_70;
    local_b0 = local_74;
    local_a8 = local_6c;
    local_8c = SQRT(fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3);
    vector3d_normalize_with_length();
    local_a8 = local_a8 * local_8c;
    local_98 = local_b0 * local_8c + local_98;
    local_94 = local_ac * local_8c + local_94;
    local_90 = local_a8 + local_90;
    local_84 = local_74 * 128.0;
    local_80 = local_70 * 128.0;
    local_7c = local_6c * 128.0;
    FUN_00505880(0x1000e9,&local_98,&local_84,*(undefined4 *)(iVar7 + 0x34),&local_50);
    local_b0 = local_38 - *param_1;
    local_ac = local_34 - param_1[1];
    local_a8 = local_30 - param_1[2];
    fVar10 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar10) {
      local_b0 = *param_2;
      local_ac = param_2[1];
      local_a8 = param_2[2];
    }
    fVar2 = 1.0 - local_b4;
    local_84 = local_a4 * local_b4 + local_b0 * fVar2;
    local_80 = local_a0 * local_b4 + local_ac * fVar2;
    local_7c = local_9c * local_b4 + local_a8 * fVar2;
    vector3d_normalize();
    fVar10 = (float10)fcos((float10)local_58);
    fVar11 = (float10)fsin((float10)local_58);
    vector3d_rotate_toward((float)fVar11,(float)fVar10);
    uVar9 = local_78;
    iVar7 = local_88;
  }
  iVar1 = DAT_006f1d6c;
  *(undefined4 *)(iVar7 + 0x40) = uVar9;
  *(undefined4 *)(iVar7 + 0x44) = *(undefined4 *)(iVar1 + 0xc);
  return uVar9;
}
#endif
