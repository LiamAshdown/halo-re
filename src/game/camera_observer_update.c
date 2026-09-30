// camera_observer_update  (Ghidra: FUN_004593b0; renamed per symbols/review_queue.txt)
// address 0x4593b0, size 821 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (REWRITTEN from objdump 0x4593b0..0x4596e4; really the
//   player weapon-fire autoaim/magnetism resolver called from trigger_create_projectiles)
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
// collision_test_movement_segment and vector3d_rotate_toward) computes some kind of camera bob/sway/roll blended
// between the target direction and a segment-closest-point fallback, but FUN_00445b20,
// first_person_camera_deterministic, FUN_00447290 and the 5-argument collision_test_movement_segment call are all
// outside this batch and several of their arguments are elided by Ghidra with no attributable
// source in this function's own visible code. That section is transcribed as literally as
// possible with placeholder locals and is NOT verified to compile against real prototypes for
// those externs -- see the raw block below and the #if 0 original for the ground truth this
// rewrite is approximating.

#include <string.h>
#include "tags.h"
#include "math.h"
#include "memory.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "fn_game.h"
#include "fn_units.h"
#include "fn_math.h"

extern data_array *player_data;         // 0x0087a480
extern data_array *object_data;         // 0x008603b0
extern game_time_globals *game_time;    // 0x006f1d6c


extern char camera_observer_find_best_target(real_point3d *observer_position, observer_target_cone *cone,
    real_vector3d *facing, datum_index exclude_object, int16_t team, void *out); // 0x459a00, EBX, stack

extern uint8_t unit_get_current_weapon_autoaim_cone(datum_index unit_index, int16_t require_zoomed, real *out); // 0x459e80, EAX, EDX, EDI
extern int16_t camera_get_seat_camera_state(datum_index unit, int16_t *out_state); // 0x445b20, ECX, stack
extern void first_person_camera_deterministic(Point3D *out_position, datum_index unit, Vector3D *out_direction); // 0x446a90, EAX, ECX, stack
extern void first_person_camera_apply_weapon_offset(real_point3d *position, datum_index unit,
    real_vector3d *aiming_direction); // 0x447290, EAX, EBX, ESI
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object_index, void *result); // 0x505880, stack

extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);


// REWRITTEN from objdump. EAX = the player; stack = (the projectile origin, the aim direction, rotated in place).
//   Using the unit's weapon autoaim cone (0x459e80: EAX = unit_noop(unit), DX = its zoom level +0x320), the
//   first-person camera position/direction (deterministic, or with the weapon offset in a seat) and the best
//   target inside the cone (0x459a00), the aim is pulled toward the target and toward what the camera looks at:
//   a 128-unit segment from the camera (pushed forward by its distance from the unit) finds that point. The
//   aim is rotated toward (target dir * fraction + look dir * (1 - fraction)) by at most cone[4]. The target
//   goes to player +0x40 and the tick to +0x44; the target is returned. The draft called six helpers without
//   arguments.
uint32_t camera_observer_update(datum_index player_index, real_point3d *observer_position, real_vector3d *fallback_facing)
{
    uint8_t *player = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    datum_index unit = ((struct player *)player)->unit;
    datum_index target = (datum_index)k_datum_index_none;
    uint32_t aim_unit = unit_noop_569670(unit);
    uint8_t *aim_unit_obj = (uint8_t *)((object_header *)object_data->data)[aim_unit & 0xffff].data;
    real cone[5];

    if (unit_get_current_weapon_autoaim_cone(aim_unit, (int16_t)(int8_t)aim_unit_obj[0x320], cone)) {
        int16_t seat_state = 0;
        real_point3d camera_position;
        real_vector3d camera_direction;
        real_vector3d target_direction;
        real_vector3d look_direction;
        real_vector3d blend;
        real fraction = 0.0f;
        uint8_t record[0x50]; // [esp+0x74]: the target record / the collision result (+0x00 object, +0x04 point,
                              //   +0x18 hit point, +0x30 magnetism fraction)
        uint8_t *unit_obj;
        real dx, dy, dz, distance;
        real_vector3d camera_forward;
        real_point3d probe_origin;
        real_vector3d probe_delta;

        unit = ((struct player *)player)->unit;
        if (camera_get_seat_camera_state(unit, &seat_state) == 0) {
            first_person_camera_deterministic((Point3D *)&camera_position, unit, (Vector3D *)&camera_direction);
        } else {
            first_person_camera_apply_weapon_offset(&camera_position, unit, &camera_direction);
        }

        target_direction = *fallback_facing;
        memset(record, 0, sizeof(record));
        if (camera_observer_find_best_target(&camera_position, (observer_target_cone *)cone, &camera_direction,
                ((struct player *)player)->unit, (int16_t)*(uint16_t *)&((struct player *)player)->team, record)) {
            target_direction.i = *(real *)(record + 0x04) - observer_position->x;
            target_direction.j = *(real *)(record + 0x08) - observer_position->y;
            target_direction.k = *(real *)(record + 0x0c) - observer_position->z;
            if (vector3d_normalize_with_length(&target_direction) == 0.0f) {
                target_direction = *fallback_facing;
            }
            fraction = *(real *)(record + 0x30);
            target = *(datum_index *)(record + 0x00);
        }

        unit_obj = (uint8_t *)((object_header *)object_data->data)[aim_unit & 0xffff].data;
        dx = camera_position.x - *(real *)(unit_obj + 0x5c);
        dy = camera_position.y - *(real *)(unit_obj + 0x60);
        dz = camera_position.z - *(real *)(unit_obj + 0x64);
        distance = (real)sqrt((double)(dx * dx + dy * dy + dz * dz));
        camera_forward = camera_direction;
        vector3d_normalize_with_length(&camera_forward);
        probe_origin.x = camera_forward.i * distance + camera_position.x;
        probe_origin.y = camera_forward.j * distance + camera_position.y;
        probe_origin.z = camera_forward.k * distance + camera_position.z;
        probe_delta.i = camera_direction.i * 128.0f;
        probe_delta.j = camera_direction.j * 128.0f;
        probe_delta.k = camera_direction.k * 128.0f;
        collision_test_movement_segment(0x1000e9, &probe_origin, &probe_delta, ((struct player *)player)->unit, record);

        look_direction.i = *(real *)(record + 0x18) - observer_position->x;
        look_direction.j = *(real *)(record + 0x1c) - observer_position->y;
        look_direction.k = *(real *)(record + 0x20) - observer_position->z;
        if (vector3d_normalize_with_length(&look_direction) == 0.0f) {
            look_direction = *fallback_facing;
        }
        blend.i = look_direction.i * (1.0f - fraction) + target_direction.i * fraction;
        blend.j = look_direction.j * (1.0f - fraction) + target_direction.j * fraction;
        blend.k = look_direction.k * (1.0f - fraction) + target_direction.k * fraction;
        vector3d_normalize(&blend);
        vector3d_rotate_toward(&blend, fallback_facing, fallback_facing, (real)sin((double)cone[4]),
            (real)cos((double)cone[4]));
    }

    ((struct player *)player)->observer_target = target;
    ((struct player *)player)->observer_state = game_time->game_time;
    return (uint32_t)target;
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
