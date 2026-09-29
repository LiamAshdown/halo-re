// object_find_nearest_squad_member  (Ghidra: object_find_nearest_squad_member; named from out/phase2/results/ai_02.json)
// address 0x41c2c0, size 492 bytes
// name confidence: 0.45   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x41c2c0..0x41c4ab; swarm unit / component arrays un-swapped; 0.36 / 2.25 constants)
// evidence: out/phase2/results/ai_02.json -- searches either a linked-object list
//   (actor.cluster_unit_index, chained through object+0x1fc, when the actor has no swarm) or a
//   swarm's member array (when it does) for the object nearest a caller point, applying a
//   same-object distance penalty (0.36x) or, for a swarm member marked with header flag bit 1,
//   a 2.25x penalty, and optionally stamping the winning (and, in the linked-list path, every
//   visited) object's cluster_stamp for shared group ownership.
// register convention: EAX -> actor_index; param_1 (a point-bearing record, position at +0xc),
//   param_2 (an object/unit index compared for the same-object penalty) and param_3 (char,
//   stamp_group) are Ghidra's recognized stack parameters.
//   // blam-cc: EAX -> actor_index, stack -> reference, exclude_index, stamp_group
//
// UNSURE: param_1's own type is not identified; its position is read at a fixed +0xc/+0x10/+0x14
// offset triple, kept as a raw cast.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern data_array *object_data;          // 0x008603b0
extern int32_t object_cluster_stamp;     // 0x008603cc

extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900

// blam-cc: EAX -> actor_index, stack -> reference, exclude_index, stamp_group
// Finds the nearest object to a given point among either a raw object linked list or a squad's
// (swarm's) member list, optionally marking group ownership. Returns the winning object/unit
// index, or k_datum_index_none if the list was empty.
datum_index object_find_nearest_squad_member(datum_index actor_index, void *reference, datum_index exclude_index, char stamp_group)
{
    actor *self;
    datum_index swarm_index;
    swarm *group;
    int16_t i;
    swarm_component *component;
    object *obj;
    object *stamp_target;
    datum_index cursor;
    float dx, dy, dz;
    float dist_sq;
    datum_index best;
    float best_dist;
    float rx, ry, rz;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    swarm_index = self->swarm_index;
    best = k_datum_index_none;
    best_dist = 3.4028235e+38f;
    rx = *(float *)((uint8_t *)reference + 0xc);
    ry = *(float *)((uint8_t *)reference + 0x10);
    rz = *(float *)((uint8_t *)reference + 0x14);

    if (swarm_index == k_datum_index_none) {
        cursor = self->cluster_unit_index;
        if (cursor == k_datum_index_none) {
            return k_datum_index_none;
        }
        do {
            real_point3d position;
            obj = ((object_header *)object_data->data)[cursor & 0xffff].data;
            object_get_position(&position, cursor);
            dx = rx - position.x;
            dy = ry - position.y;
            dz = rz - position.z;
            dist_sq = dx * dx + dy * dy + dz * dz;
            if (cursor == exclude_index) {
                dist_sq = dist_sq * 0.36f;
            }
            if (dist_sq < best_dist) {
                best = cursor;
                best_dist = dist_sq;
            }
            if (stamp_group != 0) {
                if (obj->cluster_stamp != object_cluster_stamp) {
                    obj->cluster_stamp = object_cluster_stamp;
                }
            }
            cursor = *(datum_index *)((uint8_t *)obj + 0x1fc);
        } while (cursor != k_datum_index_none);
        return best;
    }

    group = (swarm *)((uint8_t *)swarm_data->data + (swarm_index & 0xffff) * sizeof(swarm));
    if (0 < group->component_count) {
        for (i = 0; i < group->component_count; i++) {
            // 0x41c336..0x41c342: the component RECORD comes from component_index (+0x58); the unit index (+0x18) is
            // what is excluded, returned and stamped. FIXED 2026-09-27: the draft had the two arrays swapped.
            component = (swarm_component *)((uint8_t *)swarm_component_data->data +
                                            (group->component_index[i] & 0xffff) * sizeof(swarm_component));
            dx = rx - component->position.x;
            dy = ry - component->position.y;
            dz = rz - component->position.z;
            dist_sq = dx * dx + dy * dy + dz * dz;

            if ((*((uint8_t *)component + 2) & 2) == 0) {
                if (group->unit_index[i] == exclude_index) {
                    dist_sq = dist_sq * 0.36f;
                }
            } else {
                dist_sq = dist_sq * 2.25f;
            }

            if (dist_sq < best_dist) {
                best = group->unit_index[i];
                best_dist = dist_sq;
            }

            if (stamp_group != 0) {
                stamp_target = ((object_header *)object_data->data)[group->unit_index[i] & 0xffff].data;
                if (stamp_target->cluster_stamp != object_cluster_stamp) {
                    stamp_target->cluster_stamp = object_cluster_stamp;
                }
            }
        }
        return best;
    }

    return k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x41c2c0):

uint FUN_0041c2c0(int param_1,uint param_2,char param_3)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  uint in_EAX;
  int iVar5;
  int iVar6;
  int iVar7;
  short sVar8;
  uint uVar9;
  int iVar10;
  uint local_18;
  float local_14;
  float local_c;
  float local_8;
  float local_4;

  iVar10 = DAT_00880358;
  iVar1 = DAT_008603cc;
  iVar6 = DAT_008603b0;
  iVar5 = (in_EAX & 0xffff) * 0x724;
  uVar9 = *(uint *)(iVar5 + 0x28 + *(int *)(DAT_00880360 + 0x34));
  local_18 = 0xffffffff;
  local_14 = 3.4028235e+38;
  if (uVar9 == 0xffffffff) {
    uVar9 = *(uint *)(iVar5 + *(int *)(DAT_00880360 + 0x34) + 0x24);
    if (uVar9 != 0xffffffff) {
      do {
        iVar10 = (uVar9 & 0xffff) * 0xc;
        iVar1 = *(int *)(*(int *)(iVar6 + 0x34) + 8 + iVar10);
        object_get_position();
        fVar2 = *(float *)(param_1 + 0xc) - local_c;
        fVar4 = *(float *)(param_1 + 0x10) - local_8;
        fVar3 = *(float *)(param_1 + 0x14) - local_4;
        fVar2 = fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3;
        if (uVar9 == param_2) {
          fVar2 = fVar2 * 0.36;
        }
        if (fVar2 < local_14) {
          local_18 = uVar9;
          local_14 = fVar2;
        }
        if ((param_3 != '\0') &&
           (iVar10 = *(int *)(*(int *)(iVar6 + 0x34) + 8 + iVar10),
           *(int *)(iVar10 + 0x14) != DAT_008603cc)) {
          *(int *)(iVar10 + 0x14) = DAT_008603cc;
        }
        uVar9 = *(uint *)(iVar1 + 0x1fc);
      } while (uVar9 != 0xffffffff);
      return local_18;
    }
  }
  else {
    iVar6 = (uVar9 & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    sVar8 = 0;
    if (0 < *(short *)(iVar6 + 2)) {
      do {
        iVar7 = (int)sVar8;
        iVar5 = (*(uint *)(iVar6 + 0x58 + iVar7 * 4) & 0xffff) * 0x40 + *(int *)(iVar10 + 0x34);
        fVar2 = *(float *)(param_1 + 0xc) - *(float *)(iVar5 + 4);
        fVar3 = *(float *)(param_1 + 0x10) - *(float *)(iVar5 + 8);
        fVar4 = *(float *)(param_1 + 0x14) - *(float *)(iVar5 + 0xc);
        fVar2 = fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3;
        if ((*(byte *)(iVar5 + 2) & 2) == 0) {
          if (*(uint *)(iVar6 + 0x18 + iVar7 * 4) == param_2) {
            fVar2 = fVar2 * 0.36;
          }
        }
        else {
          fVar2 = fVar2 * 2.25;
        }
        if (fVar2 < local_14) {
          local_18 = *(uint *)(iVar6 + 0x18 + iVar7 * 4);
          local_14 = fVar2;
        }
        if ((param_3 != '\0') &&
           (iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                            (*(uint *)(iVar6 + 0x18 + iVar7 * 4) & 0xffff) * 0xc),
           *(int *)(iVar5 + 0x14) != iVar1)) {
          *(int *)(iVar5 + 0x14) = iVar1;
        }
        sVar8 = sVar8 + 1;
      } while (sVar8 < *(short *)(iVar6 + 2));
      return local_18;
    }
  }
  return 0xffffffff;
}
#endif
