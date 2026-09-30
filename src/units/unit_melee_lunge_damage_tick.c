// unit_melee_lunge_damage_tick  (Ghidra: FUN_0056fc80)
// address 0x56fc80, size 700 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// REWRITTEN from objdump 0x56fc80..0x56ff3b (the draft had the callees' arguments guessed and the damage data
//   responsible player wrong). Stack: unit. Only while the unit is in melee state 4 (+0x289), attached to an
//   object (+0x11c, the one being hit) and its tag has melee damage (+0x294). When the countdown (+0x28a) is 0 and
//   that object has collision (0x504e10, EDI object, ECX context), a 0.2 * forward segment centred on the unit
//   is tested against it (0x504f60, flags 3); a hit gives the hit point and the hit surface plane (the node
//   matrix applied to the collision plane, 0x4cbf10, flipped when the record says so). Damage data: the melee
//   damage, the unit as responsible object with its player (+0x218) and team (+0xb8), blend 1/30, multiplier 1,
//   no cluster / material; a hit adds epicentre = origin = the hit point, direction = forward, flag 2, and
//   restarts the countdown at 10. object_apply_damage(dd, object, node, region, material, &plane) -- or
//   (dd, object, -1, -1, -1, 0) without a hit -- then the countdown is decremented.
// blam-cc: stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>
#include <stdint.h>
#include "physics.h"
#include "fn_units.h"
#include "fn_physics.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX

extern uint8_t object_collision_context_test_segment(object_collision_context *context, uint32_t flags,
    real_point3d *origin, real_vector3d *delta, object_node_collision_result *out_result); // 0x504f60, stack
extern void matrix4x3_transform_plane(real_plane3d *out, real_matrix4x3 *m, real_plane3d *plane); // 0x4cbf10, EAX, ECX, EDX
extern void object_apply_damage(damage_data *dd, uint32_t target_object_index, int16_t node_index, int16_t region_index,
    int16_t material_index, uint32_t plane); // 0x4ee5e0, stack

void unit_melee_lunge_damage_tick(uint32_t unit_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    datum_index target = ((unit_object *)obj)->base.parent_object;
    uint8_t hit = 0;
    real_plane3d plane;                     // L14: first the sweep vector, then the hit surface plane
    real_point3d start;
    real_point3d hit_point;
    object_collision_context context;
    object_node_collision_result record;
    damage_data dd;

    if (obj[0x289] != 4 || target == k_datum_index_none || *(datum_index *)&((Unit *)tag)->melee_damage.tag_id == k_datum_index_none) {
        return;
    }
    if (obj[0x28a] == 0 && object_collision_context_build(target, &context)) {
        object_get_position(&start, unit_index);
        plane.normal.i = ((unit_object *)obj)->base.forward.i * 0.2f;
        plane.normal.j = ((unit_object *)obj)->base.forward.j * 0.2f;
        plane.normal.k = ((unit_object *)obj)->base.forward.k * 0.2f;
        start.x -= plane.normal.i * 0.5f;
        start.y -= plane.normal.j * 0.5f;
        start.z -= plane.normal.k * 0.5f;
        if (object_collision_context_test_segment(&context, 3, &start, &plane.normal, &record)) {
            float fraction = *(float *)((uint8_t *)&record + 0x08);

            hit_point.x = plane.normal.i * fraction + start.x;
            hit_point.y = plane.normal.j * fraction + start.y;
            hit_point.z = plane.normal.k * fraction + start.z;
            matrix4x3_transform_plane(&plane,
                (real_matrix4x3 *)((uint8_t *)context.nodes + record.node_index * 0x34),
                *(real_plane3d **)((uint8_t *)&record + 0x0c));
            if (*(int32_t *)((uint8_t *)&record + 0x14) < 0) {
                plane.normal.i = -plane.normal.i;
                plane.normal.j = -plane.normal.j;
                plane.normal.k = -plane.normal.k;
                plane.d = -plane.d;
            }
            hit = 1;
        }
    }
    memset(&dd, 0, sizeof(dd));
    dd.damage_effect_tag = *(datum_index *)&((Unit *)tag)->melee_damage.tag_id;
    dd.material_type = -1;
    dd.location_cluster_index = -1;
    dd.multiplier = 1.0f;
    dd.responsible_object = unit_index;
    dd.team_index = ((unit_object *)obj)->base.owner_team;
    dd.responsible_player = ((unit_object *)obj)->unit.controlling_player;
    dd.random_blend = 0.033333335f;
    if (hit) {
        dd.epicentre = hit_point;
        dd.origin = hit_point;
        dd.direction = *(real_vector3d *)&((unit_object *)obj)->base.forward.i;
        dd.flags |= 2;
        obj[0x28a] = 10;
        object_apply_damage(&dd, ((unit_object *)obj)->base.parent_object, record.node_index, record.region_index,
            *(int16_t *)((uint8_t *)&record + 0x1a), (uint32_t)(uintptr_t)&plane);
    } else {
        object_apply_damage(&dd, ((unit_object *)obj)->base.parent_object, -1, -1, -1, 0);
    }
    obj[0x28a]--;
}

#if 0
Original Ghidra decompilation (0x56fc80):

void FUN_0056fc80(uint param_1)

{
  uint *puVar1;
  bool bVar2;
  char cVar3;
  int iVar4;
  uint *puVar5;
  uint uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  float *pfVar9;
  float local_4b4;
  float fStack_4b0;
  float fStack_4ac;
  float fStack_4a8;
  float local_4a4;
  float fStack_4a0;
  float fStack_49c;
  float fStack_498;
  float fStack_494;
  float fStack_490;
  int local_48c;
  uint auStack_488 [4];
  undefined2 uStack_478;
  undefined2 uStack_470;
  float fStack_46c;
  float fStack_468;
  float fStack_464;
  float fStack_460;
  float fStack_45c;
  float fStack_458;
  uint uStack_454;
  uint uStack_450;
  uint uStack_44c;
  undefined4 uStack_448;
  undefined4 uStack_444;
  undefined2 uStack_43c;
  undefined1 local_430 [16];
  undefined2 local_420;
  undefined2 uStack_41e;
  undefined2 uStack_41c;
  float fStack_418;
  int iStack_40c;
  undefined4 uStack_406;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  local_48c = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((*(char *)((int)puVar1 + 0x289) == '\x04') && (puVar1[0x47] != 0xffffffff)) &&
     (*(int *)(local_48c + 0x294) != -1)) {
    bVar2 = false;
    if (*(char *)((int)puVar1 + 0x28a) == '\0') {
      cVar3 = FUN_00504e10();
      if (cVar3 != '\0') {
        object_get_position();
        local_4b4 = (float)puVar1[0x1d] * 0.2;
        fStack_4b0 = (float)puVar1[0x1e] * 0.2;
        fStack_4ac = (float)puVar1[0x1f] * 0.2;
        local_4a4 = local_4a4 - local_4b4 * 0.5;
        fStack_4a0 = fStack_4a0 - fStack_4b0 * 0.5;
        fStack_49c = fStack_49c - fStack_4ac * 0.5;
        cVar3 = FUN_00504f60(local_430,3,&local_4a4,&local_4b4,&local_420);
        if (cVar3 != '\0') {
          fStack_498 = local_4b4 * fStack_418 + local_4a4;
          fStack_494 = fStack_4b0 * fStack_418 + fStack_4a0;
          fStack_490 = fStack_4ac * fStack_418 + fStack_49c;
          matrix4x3_transform_plane();
          if (iStack_40c < 0) {
            local_4b4 = -local_4b4;
            fStack_4b0 = -fStack_4b0;
            fStack_4ac = -fStack_4ac;
            fStack_4a8 = -fStack_4a8;
          }
          bVar2 = true;
        }
      }
    }
    uStack_478 = (undefined2)puVar1[0x2e];
    puVar5 = auStack_488;
    for (iVar4 = 0x15; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    auStack_488[0] = *(uint *)(local_48c + 0x294);
    auStack_488[2] = puVar1[0x86];
    uStack_43c = 0xffff;
    uStack_470 = 0xffff;
    uStack_444 = 0x3f800000;
    auStack_488[3] = param_1;
    uStack_448 = 0x3d088889;
    if (bVar2) {
      fStack_458 = fStack_490;
      fStack_464 = fStack_490;
      fStack_460 = fStack_498;
      fStack_46c = fStack_498;
      uStack_454 = puVar1[0x1d];
      fStack_45c = fStack_494;
      fStack_468 = fStack_494;
      uStack_450 = puVar1[0x1e];
      uStack_44c = puVar1[0x1f];
      auStack_488[1] = auStack_488[1] | 2;
      pfVar9 = &local_4b4;
      uVar7 = CONCAT22(uStack_41e,local_420);
      uVar6 = puVar1[0x47];
      uVar8 = CONCAT22(uStack_41c,uStack_41e);
      *(undefined1 *)((int)puVar1 + 0x28a) = 10;
    }
    else {
      pfVar9 = (float *)0x0;
      uStack_406 = 0xffffffff;
      uVar8 = 0xffffffff;
      uVar7 = 0xffffffff;
      uVar6 = puVar1[0x47];
    }
    object_apply_damage(auStack_488,uVar6,uVar7,uVar8,uStack_406,pfVar9);
    *(char *)((int)puVar1 + 0x28a) = *(char *)((int)puVar1 + 0x28a) + -1;
  }
  return;
}
#endif
