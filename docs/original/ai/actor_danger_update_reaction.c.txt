// actor_danger_update_reaction  (Ghidra: actor_danger_update_reaction, renamed)
// address 0x41eda0, size 1533 bytes
// name confidence: 0.45  rewrite confidence: 0.85
// VERIFIED against disassembly 0x41eda0..0x41f39c (2026-09-30); rewritten from it (the draft called object_try_and_get, object_get_root_object_index,
//   actor_evaluate_engagement_reachability and the perception test without their arguments). Once per tick for the
//   actor's registered danger (+0x280 kind 1..3, object +0x28c; a vanished object clears it):
//   - geometry: position -> +0x2b0, velocity -> +0x2bc, end point 45 ticks on -> +0x2c8, midpoint -> +0x2dc,
//     half length plus +0x294 -> +0x2d8, distance from the actor's firing block point -> +0x2d4;
//   - kind 1 (a unit meleeing/firing): noticed already, or its prop (0x43ea80) seen at least twice (+0x30); +0x2e8
//     is the frames left of its animation when it is state 0x19 (0x564390), else -1;
//   - kind 2 (a projectile): +0x2e8 = the object's remaining detonation ticks ((1 - +0x240) / +0x244, rounded)
//     or -1; noticed already or the actor's own, else for an actor neither asleep (+0x6a 1) nor in a sleeping
//     encounter (+0x40), within the projectile tag's +0x19c, perceived (0x41bb30, posture = reachability 0x42b270
//     from the firing block to it, in the root object's cluster) at 2 or better;
//   - kind 3 (a vehicle): drops out when it is barely moving (speed squared under 4.4e-5) or beyond its tag's +4
//     plus 10; noticed already, or its driver's prop seen twice, else perceived as above (unless asleep) or heard
//     (0x41c030 with the tag's +0x182 gate) at 2 or better;
//   - newly noticed: vocalization 0xc (payload 5). +0x286 records noticed, +0x28a the actor's own projectile.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;     // 0x00880360
extern data_array *prop_data;      // 0x008802c0
extern data_array *encounter_data; // 0x008802c8
extern data_array *object_data;    // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern double sqrt(double x);
extern int32_t fistp_round(float x);

extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX
extern uint32_t object_get_root_object_index(uint32_t object_index); // 0x4f6fb0, ECX
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point); // 0x41c1e0, EAX, ECX, EDX
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX
extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying); // 0x42b270, AX, CX, ESI, EDI, stack
extern int16_t actor_dispatch_look_handler_by_posture(int16_t posture, uint32_t actor_index, void *origin, void *target,
    uint8_t stance_a, uint8_t check_facing, uint16_t range_class); // 0x41bb30, EBX, stack
extern uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index,
    void *target_ref, int16_t gate, real_point3d *listener_position); // 0x41c030, stack, EAX, ECX, BX, ESI
extern int32_t unit_get_animation_frames_remaining(uint32_t unit_index, int16_t *out_animation_state); // 0x564390, EAX, stack
extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant, void *context); // 0x4142d0, EAX, stack

#define A_W(o) (*(int16_t *)(actor + (o)))
#define A_D(o) (*(uint32_t *)(actor + (o)))
#define A_F(o) (*(float *)(actor + (o)))

static uint8_t actor_danger_prop_seen_twice(datum_index actor_index, datum_index object_index)
{
    datum_index prop_index = actor_find_prop_for_object(object_index, actor_index);

    if (prop_index == k_datum_index_none) {
        return 0xff;
    }
    return *(int16_t *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * 0x138 + 0x30) >= 2;
}

// the actor is asleep (+0x6a 1) or its encounter sleeps (+0x40)
static uint8_t actor_danger_asleep(uint8_t *actor)
{
    uint8_t *encounter = 0;

    if (A_D(0x34) != 0xffffffff) {
        encounter = (uint8_t *)encounter_data->data + (A_D(0x34) & 0xffff) * 0x6c;
    }
    return A_W(0x6a) == 1 || (encounter != 0 && encounter[0x40] != 0);
}

static uint8_t actor_danger_stance(datum_index actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    return A_W(0x6e) >= 2 ? 2 : (A_W(0x6a) >= 3);
}

void actor_danger_update_reaction(datum_index actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *object;
    uint32_t block[14];
    real_point3d *position = (real_point3d *)(actor + 0x2b0);
    real_point3d *block_point = (real_point3d *)&block[3];
    uint8_t noticed = 0;
    uint8_t own = 0;

    if (A_W(0x280) <= 0) {
        return;
    }
    object = (uint8_t *)object_try_and_get(A_D(0x28c), 0xffffffff);
    if (object == 0) {
        A_W(0x280) = 0;
        return;
    }
    object_get_position(position, A_D(0x28c));
    actor_get_firing_positions(actor_index, block, position);
    *(real_vector3d *)(actor + 0x2bc) = *(real_vector3d *)&((struct object *)object)->velocity.i;
    {
        float dx = position->x - block_point->x;
        float dy = position->y - block_point->y;
        float dz = position->z - block_point->z;

        A_F(0x2d4) = (float)sqrt((double)(dz * dz + dy * dy + dx * dx));
    }
    A_F(0x2c8) = A_F(0x2bc) * 45.0f + position->x;
    A_F(0x2cc) = A_F(0x2c0) * 45.0f + position->y;
    A_F(0x2d0) = A_F(0x2c4) * 45.0f + position->z;
    A_F(0x2dc) = (A_F(0x2c8) + position->x) * 0.5f;
    A_F(0x2e0) = (position->y + A_F(0x2cc)) * 0.5f;
    A_F(0x2e4) = (position->z + A_F(0x2d0)) * 0.5f;
    {
        float dx = position->x - A_F(0x2dc);
        float dy = position->y - A_F(0x2e0);
        float dz = position->z - A_F(0x2e4);

        A_F(0x2d8) = (float)sqrt((double)(dz * dz + dy * dy + dx * dx)) + A_F(0x294);
    }

    switch (A_W(0x280)) {
    case 1: { // 0x41f2e4
        int16_t state = 0;
        int32_t frames;

        noticed = actor[0x286];
        if (!noticed) {
            uint8_t seen = actor_danger_prop_seen_twice(actor_index, A_D(0x28c));

            if (seen != 0xff) {
                noticed = seen;
            }
        }
        frames = unit_get_animation_frames_remaining(A_D(0x28c), &state);
        A_W(0x2e8) = state == 0x19 ? (int16_t)frames : -1;
        break;
    }
    case 2: { // 0x41f139
        uint8_t *tag;
        int16_t cluster;
        int16_t status;

        if (A_D(0x18) != 0xffffffff && *(uint32_t *)&((struct object *)object)->parent_object == A_D(0x18)) {
            own = 1;
        }
        if (*(float *)(object + 0x240) > 0.0f && *(float *)(object + 0x244) > 0.0f) {
            A_W(0x2e8) = (int16_t)fistp_round((1.0f - *(float *)(object + 0x240)) / *(float *)(object + 0x244));
        } else {
            A_W(0x2e8) = -1;
        }
        if (actor[0x286] != 0 || own) {
            noticed = 1;
            break;
        }
        if (actor_danger_asleep(actor)) {
            break;
        }
        tag = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
        if (!(A_F(0x2d4) < *(float *)(tag + 0x19c))) {
            break;
        }
        cluster = ((struct object *)object)->location_cluster_index;
        if (*(uint32_t *)&((struct object *)object)->parent_object != 0xffffffff) {
            uint8_t *root = (uint8_t *)((object_header *)object_data->data)[object_get_root_object_index(A_D(0x28c)) & 0xffff].data;

            cluster = ((struct object *)root)->location_cluster_index;
        }
        status = (int16_t)actor_evaluate_engagement_reachability(*(int16_t *)((uint8_t *)block + 0x28), cluster,
            position, (real_point3d *)block, 0, 0, A_D(0x28c), A_D(0x158) != 0xffffffff);
        actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
        if (actor_dispatch_look_handler_by_posture(status, actor_index, block, position, 0, 1,
                actor_danger_stance(actor_index)) >= 2) {
            noticed = 1;
        }
        break;
    }
    case 3: { // 0x41ef40
        uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
        float vi = ((struct object *)object)->velocity.i;
        float vj = ((struct object *)object)->velocity.j;
        float vk = ((struct object *)object)->velocity.k;
        uint8_t asleep;
        uint8_t *location;
        int16_t status;

        if (vi * vi + vj * vj + vk * vk < 4.4444445e-05f || ((struct Object *)tag)->bounding_radius + 10.0f < A_F(0x2d4)) {
            A_W(0x280) = 0;
            break;
        }
        noticed = actor[0x286];
        if (noticed) {
            break;
        }
        if (*(uint32_t *)(object + 0x324) != 0xffffffff) {
            uint8_t seen = actor_danger_prop_seen_twice(actor_index, *(uint32_t *)(object + 0x324));

            if (seen != 0xff) {
                noticed = seen;
                break;
            }
        }
        asleep = actor_danger_asleep(actor);
        location = object + 0x98;
        if (*(uint32_t *)&((struct object *)object)->parent_object != 0xffffffff) {
            location = (uint8_t *)((object_header *)object_data->data)[object_get_root_object_index(A_D(0x28c)) & 0xffff].data + 0x98;
        }
        status = (int16_t)actor_evaluate_engagement_reachability(*(int16_t *)((uint8_t *)block + 0x28),
            *(int16_t *)(location + 4), position, (real_point3d *)block, 0, 0, A_D(0x28c), A_D(0x158) != 0xffffffff);
        if (!asleep &&
            actor_dispatch_look_handler_by_posture(status, actor_index, block, position, 0, 1,
                actor_danger_stance(actor_index)) >= 2) {
            noticed = 1;
            break;
        }
        if ((int16_t)actor_target_hearing_check(location, status, actor_index, block, *(int16_t *)(tag + 0x182),
                position) >= 2) {
            noticed = 1;
        }
        break;
    }
    default:
        break;
    }

    actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    if (noticed && actor[0x286] == 0) {
        uint8_t payload[0x10];

        memset(payload, 0, sizeof(payload));
        *(int16_t *)payload = 5;
        actor_begin_vocalization(actor_index, 0xc, 1, payload);
    }
    actor[0x286] = noticed;
    actor[0x28a] = own;
}

#if 0
Original Ghidra decompilation (0x41eda0):

void FUN_0041eda0(uint param_1)

{
  float *pfVar1;
  float fVar2;
  uint *puVar3;
  float fVar4;
  ushort uVar5;
  undefined1 uVar6;
  short sVar7;
  undefined2 uVar8;
  uint *puVar9;
  int iVar10;
  uint uVar11;
  undefined4 uVar12;
  float fVar13;
  int iVar14;
  undefined8 uVar15;
  char local_5a;
  char local_59;
  undefined2 local_58;
  uint *local_54;
  int local_50;
  float local_4c;
  undefined2 local_48 [8];
  undefined1 local_38 [12];
  float local_2c;
  float local_28;
  float local_24;

  local_50 = (param_1 & 0xffff) * 0x724;
  iVar14 = *(int *)(DAT_00880360 + 0x34) + local_50;
  if (*(short *)(iVar14 + 0x280) < 1) {
    return;
  }
  puVar9 = (uint *)object_try_and_get(0xffffffff);
  if (puVar9 == (uint *)0x0) {
    *(undefined2 *)(iVar14 + 0x280) = 0;
    return;
  }
  pfVar1 = (float *)(iVar14 + 0x2b0);
  local_54 = puVar9;
  object_get_position();
  actor_get_firing_positions();
  puVar3 = local_54;
  *(float *)(iVar14 + 700) = (float)puVar9[0x1a];
  *(uint *)(iVar14 + 0x2c0) = puVar9[0x1b];
  *(uint *)(iVar14 + 0x2c4) = puVar9[0x1c];
  local_28 = *(float *)(iVar14 + 0x2b4) - local_28;
  local_24 = *(float *)(iVar14 + 0x2b8) - local_24;
  *(float *)(iVar14 + 0x2d4) =
       SQRT((*pfVar1 - local_2c) * (*pfVar1 - local_2c) + local_28 * local_28 + local_24 * local_24)
  ;
  *(float *)(iVar14 + 0x2c8) = *(float *)(iVar14 + 700) * 45.0 + *pfVar1;
  *(float *)(iVar14 + 0x2cc) = *(float *)(iVar14 + 0x2c0) * 45.0 + *(float *)(iVar14 + 0x2b4);
  *(float *)(iVar14 + 0x2d0) = *(float *)(iVar14 + 0x2c4) * 45.0 + *(float *)(iVar14 + 0x2b8);
  *(float *)(iVar14 + 0x2dc) = (*(float *)(iVar14 + 0x2c8) + *pfVar1) * 0.5;
  *(float *)(iVar14 + 0x2e0) = (*(float *)(iVar14 + 0x2b4) + *(float *)(iVar14 + 0x2cc)) * 0.5;
  *(float *)(iVar14 + 0x2e4) = (*(float *)(iVar14 + 0x2b8) + *(float *)(iVar14 + 0x2d0)) * 0.5;
  fVar2 = *pfVar1 - *(float *)(iVar14 + 0x2dc);
  fVar4 = *(float *)(iVar14 + 0x2b4) - *(float *)(iVar14 + 0x2e0);
  fVar13 = *(float *)(iVar14 + 0x2b8) - *(float *)(iVar14 + 0x2e4);
  sVar7 = *(short *)(iVar14 + 0x280);
  local_5a = '\0';
  local_59 = '\0';
  *(float *)(iVar14 + 0x2d8) =
       SQRT(fVar2 * fVar2 + fVar4 * fVar4 + fVar13 * fVar13) + *(float *)(iVar14 + 0x294);
  if (sVar7 == 1) {
    local_5a = *(char *)(iVar14 + 0x286);
    if ((local_5a == '\0') &&
       (uVar11 = FUN_0043ea80(*(undefined4 *)(iVar14 + 0x28c)), uVar11 != 0xffffffff)) {
      local_5a = 1 < *(short *)((uVar11 & 0xffff) * 0x138 + 0x30 + *(int *)(DAT_008802c0 + 0x34));
    }
    uVar8 = FUN_00564390(&local_58);
    if (local_58 != 0x19) {
      uVar8 = 0xffff;
    }
    *(undefined2 *)(iVar14 + 0x2e8) = uVar8;
LAB_0041f353:
    if (local_5a == '\0') goto LAB_0041f381;
  }
  else if (sVar7 == 2) {
    if ((*(uint *)(iVar14 + 0x18) != 0xffffffff) && (local_54[0x47] == *(uint *)(iVar14 + 0x18))) {
      local_59 = '\x01';
    }
    if (((float)local_54[0x90] <= 0.0) || ((float)local_54[0x91] <= 0.0)) {
      *(undefined2 *)(iVar14 + 0x2e8) = 0xffff;
    }
    else {
      local_4c = (1.0 - (float)local_54[0x90]) / (float)local_54[0x91];
      local_54 = (uint *)(int)ROUND(local_4c);
      *(undefined2 *)(iVar14 + 0x2e8) = local_54._0_2_;
    }
    if ((*(char *)(iVar14 + 0x286) == '\0') && (local_59 == '\0')) {
      local_5a = '\0';
      if (*(uint *)(iVar14 + 0x34) == 0xffffffff) {
        iVar10 = 0;
      }
      else {
        iVar10 = (*(uint *)(iVar14 + 0x34) & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
      }
      if ((*(short *)(iVar14 + 0x6a) == 1) ||
         (((iVar10 != 0 && (*(char *)(iVar10 + 0x40) != '\0')) ||
          (*(float *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x19c) <=
           *(float *)(iVar14 + 0x2d4))))) goto LAB_0041f381;
      if (puVar3[0x47] != 0xffffffff) {
        object_get_root_object_index();
      }
      FUN_0042b270(0,0,*(undefined4 *)(iVar14 + 0x28c),*(int *)(iVar14 + 0x158) != -1);
      iVar10 = *(int *)(DAT_00880360 + 0x34) + local_50;
      if (*(short *)(iVar10 + 0x6e) < 2) {
        uVar6 = 2 < *(short *)(iVar10 + 0x6a);
      }
      else {
        uVar6 = 2;
      }
      sVar7 = FUN_0041bb30(param_1,local_38,pfVar1,0,1,uVar6);
      if (sVar7 < 2) goto LAB_0041f381;
      local_5a = '\x01';
    }
    else {
      local_5a = '\x01';
    }
  }
  else {
    if (sVar7 != 3) goto LAB_0041f381;
    fVar2 = (float)puVar9[0x1a];
    fVar13 = *(float *)((*local_54 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (((float)puVar9[0x1c] * (float)puVar9[0x1c] +
         (float)puVar9[0x1b] * (float)puVar9[0x1b] + fVar2 * fVar2 < 4.4444445e-05) ||
       (*(float *)((int)fVar13 + 4) + 10.0 < *(float *)(iVar14 + 0x2d4))) {
      *(undefined2 *)(iVar14 + 0x280) = 0;
      goto LAB_0041f381;
    }
    local_5a = *(char *)(iVar14 + 0x286);
    local_4c = fVar13;
    if (local_5a == '\0') {
      if (local_54[0xc9] != 0xffffffff) {
        uVar15 = FUN_0043ea80(local_54[0xc9]);
        fVar13 = (float)((ulonglong)uVar15 >> 0x20);
        if ((uint)uVar15 != 0xffffffff) {
          local_5a = 1 < *(short *)(((uint)uVar15 & 0xffff) * 0x138 + 0x30 +
                                   *(int *)(DAT_008802c0 + 0x34));
          goto LAB_0041f353;
        }
      }
      if (*(uint *)(iVar14 + 0x34) == 0xffffffff) {
        iVar10 = 0;
      }
      else {
        fVar13 = *(float *)(DAT_008802c8 + 0x34);
        iVar10 = (*(uint *)(iVar14 + 0x34) & 0xffff) * 0x6c + (int)fVar13;
      }
      uVar5 = local_58 >> 8;
      local_58 = local_58 & 0xff00;
      if ((*(short *)(iVar14 + 0x6a) == 1) || ((iVar10 != 0 && (*(char *)(iVar10 + 0x40) != '\0'))))
      {
        local_58 = CONCAT11((char)uVar5,1);
      }
      if (puVar3[0x47] != 0xffffffff) {
        uVar11 = object_get_root_object_index();
        fVar13 = (float)((uVar11 & 0xffff) * 3);
        puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar11 & 0xffff) * 0xc);
      }
      local_54 = puVar3 + 0x26;
      uVar12 = FUN_0042b270(0,0,*(undefined4 *)(iVar14 + 0x28c),
                            CONCAT31((int3)((uint)fVar13 >> 8),*(int *)(iVar14 + 0x158) != -1));
      puVar9 = puVar3 + 0x26;
      if ((char)local_58 == '\0') {
        iVar10 = *(int *)(DAT_00880360 + 0x34) + local_50;
        if (*(short *)(iVar10 + 0x6e) < 2) {
          uVar6 = 2 < *(short *)(iVar10 + 0x6a);
        }
        else {
          uVar6 = 2;
        }
        sVar7 = FUN_0041bb30(param_1,local_38,pfVar1,0,1,uVar6);
        puVar9 = local_54;
        if (1 < sVar7) {
          local_5a = '\x01';
          goto LAB_0041f35b;
        }
      }
      sVar7 = FUN_0041c030(puVar9,uVar12);
      if (sVar7 < 2) goto LAB_0041f381;
      local_5a = '\x01';
    }
  }
LAB_0041f35b:
  if (*(char *)(iVar14 + 0x286) == '\0') {
    local_48[0] = 5;
    FUN_004142d0(0xc,1,local_48);
  }
LAB_0041f381:
  *(char *)(iVar14 + 0x286) = local_5a;
  *(char *)(iVar14 + 0x28a) = local_59;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
