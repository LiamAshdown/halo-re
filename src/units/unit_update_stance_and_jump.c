// unit_update_stance_and_jump  (Ghidra: FUN_00566de0)
// address 0x566de0, size 1548 bytes
// name confidence: 0.45 (the name predates the rewrite: the function plays a unit's damage "ping" reaction --
//   soft/hard ping or the death/ready transition -- and aims its throw direction)
// rewrite confidence: 0.85
// evidence: types/objects.h object current_shield_damage/current_body_damage (0xe8/0xec), vitality_flags (0x106),
//   parent_object (0x11c), type (0xb4), animation_frame (0xd2); units.h unit animation_state_flags (0x298), 0x28c,
//   animation_state (0x2a3), overlay slot 2 (0x2b2 animation, 0x2b4 frame), current weapon index (0x2f2);
//   tags.h Unit soft_ping_threshold / hard_ping_threshold / hard_death_threshold (0x218 / 0x220 / 0x228),
//   soft/hard ping interrupt ticks (0x2c8 / 0x2ca), unit_flags (0x17c), animation_graph (0x44); Biped flags
//   (0x2f4, bit 10) and the biped's +0x4cc bit 0. ModelAnimations +0x3c/+0x40 (an int16 table the lookups
//   index, 11 entries per facing and 4 facings per class), +0x78 animations (0xb4 each: frame count +0x22,
//   +0x42 compared against the facing-0 entry).
// REWRITTEN (from objdump 0x566de0..0x5673eb). All ten arguments are on the stack (the old header's EAX..EDI
//   register list was wrong); the ninth is a pointer to a 2D throw/aim vector, not an event id. The draft called
//   animation_choose_random_permutation without its EAX (graph) / DX (first animation) arguments (a crash on
//   the first campaign biped), unit_set_or_test_seat_and_weapon_label with 0 instead of 1, and did not model
//   the facing (0..3 from turn_angle), the stance class or the final aim switch.
// blam-cc: stack -> all ten (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_units.h"
#include "fn_math.h"
#include "fn_objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern int16_t network_game_mode;                // 0x00719720, word; 0 = local game
extern char *s_stand;                                // 0x0069fdec "stand"
extern double fabs(double x);


extern uint32_t weapon_must_be_readied(uint32_t weapon_object_index); // 0x4c2ea0, blam-cc: EAX
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, blam-cc: EAX -> animation_graph_tag, DX -> first_animation, stack -> stream

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack
extern void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count);
    // 0x4f6b70, blam-cc: EAX -> object_index, DX -> requested_count
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label,
    uint8_t test_only); // 0x5651e0, blam-cc: EAX -> unit_index, stack -> the rest
extern uint8_t unit_animation_state_is_compatible(const uint8_t *animation_block, int16_t requested_state);
    // 0x565be0, blam-cc: ECX -> animation_block, DX -> requested_state
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern int32_t unit_pick_random_spawned_actor_count(uint32_t unit_index); // 0x568540, blam-cc: EDI
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970, blam-cc: EAX, CX
extern char *unit_get_current_weapon_label(uint32_t unit_index); // 0x56dfd0, blam-cc: EAX


    // 0x56ebd0, blam-cc: EAX -> object_index, stack -> graph, animation_index
extern void unit_set_throw_aim_direction(uint32_t object_index, real_vector2d *direction_xy);
    // 0x5704d0, blam-cc: EAX -> object_index, ECX -> direction_xy

#define OBJECT_U8(o, offset) (*(uint8_t *)((o) + (offset)))
#define OBJECT_I16(o, offset) (*(int16_t *)((o) + (offset)))
#define OBJECT_U16(o, offset) (*(uint16_t *)((o) + (offset)))
#define OBJECT_I32(o, offset) (*(int32_t *)((o) + (offset)))
#define OBJECT_F32(o, offset) (*(float *)((o) + (offset)))

// The graph's int16 table (+0x40, +0x3c entries) at an index, -1 outside it.
static int16_t animation_table_lookup(uint8_t *graph, int32_t index)
{
    if (index < 0 || index >= *(int32_t *)&((ModelAnimations *)graph)->unit_damage.count) {
        return -1;
    }
    return (*(int16_t **)&((ModelAnimations *)graph)->unit_damage.pointer)[index];
}

void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction,
    uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle,
    int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still)
{
    uint8_t *obj = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 0xc + 8);
    uint8_t *unit_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t forced = force_ready;
    uint8_t soft_ping;
    uint8_t hard_ping;
    int32_t weapon_class = weapon_class_index;
    int32_t facing;
    int32_t stance_class;
    datum_index graph_tag;
    uint8_t *graph;
    int16_t new_state;
    int16_t animation;
    uint8_t allowed;
    double turn;

    if (forced) {
        allow_death_reaction = 0;
        soft_ping = 1;
        hard_ping = OBJECT_F32(unit_tag, 0x228) > 0.0f && OBJECT_F32(obj, 0xec) > OBJECT_F32(unit_tag, 0x228);
    } else if (allow_death_reaction) {
        forced = 1;
        soft_ping = 1;
        hard_ping = 0;
    } else {
        soft_ping = OBJECT_F32(obj, 0xec) > OBJECT_F32(unit_tag, 0x218) ||
            OBJECT_F32(obj, 0xe8) > OBJECT_F32(unit_tag, 0x218);
        hard_ping = OBJECT_F32(obj, 0xec) > OBJECT_F32(unit_tag, 0x220);
        if (ignore_disoriented || (int8_t)OBJECT_U8(obj, 0x204) < 0) {
            hard_ping = 0;
        }
    }
    if (force_reaction) {
        hard_ping = 1;
        soft_ping = 1;
    }
    if (weapon_class_index == -1) {
        weapon_class = 0;
    }
    turn = fabs(turn_angle);
    if (turn < 0.78539818525314331) {       // 0x672fc0
        facing = 3;
    } else if (turn > 2.1598450094461441) {   // 0x6731c0
        facing = 0;
    } else {
        facing = turn_angle > 0.0f ? 1 : 2;
    }
    if (current_game_engine != 0 && (int16_t)weapon_class == 2 && hard_ping && forced) {
        facing = 1;
    }
    if (require_still && !soft_ping && !forced) {
        return;
    }
    graph_tag = *(datum_index *)&((struct Object *)unit_tag)->animation_graph.tag_id;
    graph = (uint8_t *)tag_instances[graph_tag & 0xffff].data;

    if (!hard_ping && !forced) {
        // refresh the idle overlay (slot 2) once it has run past the soft ping interrupt ticks
        if (OBJECT_I16(obj, 0x2b2) != -1 && OBJECT_I16(obj, 0x2b4) <= OBJECT_I16(unit_tag, 0x2c8)) {
            return;
        }
        animation = animation_choose_random_permutation(graph_tag,
            animation_table_lookup(graph, (int16_t)(facing * 0xb + weapon_class)), 1);
        if (animation == -1) {
            return;
        }
        OBJECT_I16(obj, 0x2b2) = animation;
        OBJECT_I16(obj, 0x2b4) = 0;
        return;
    }

    new_state = forced ? 0x19 : 0x17;
    if (forced) {
        stance_class = hard_ping + 2;
        allowed = 1;
    } else {
        stance_class = 1;
        allowed = unit_animation_state_is_compatible(obj + 0x298, new_state) ? 1 : 0;
    }
    if (OBJECT_U8(obj, 0x2a3) == 0x17 && OBJECT_I16(obj, 0xd2) > OBJECT_I16(unit_tag, 0x2ca)) {
        allowed = 1;
    }
    if (!forced) {
        if (OBJECT_U8(obj, 0x106) & 4) {
            allowed = 0;
        }
        if (OBJECT_I32(obj, 0x11c) != -1) {
            return;
        }
    }
    if (!allowed) {
        return;
    }
    if (forced) {
        unit_set_or_test_seat_and_weapon_label(unit_index, s_stand, unit_get_current_weapon_label(unit_index), 1);
    }
    if (new_state == 0x19 && OBJECT_I16(obj, 0xb4) == 0 && (OBJECT_U8(obj, 0x4cc) & 1) &&
        (OBJECT_I32(unit_tag, 0x2f4) & 0x400) == 0) {
        new_state = 0x18;
        if (unit_try_set_animation_state(unit_index, 0x18)) {
            goto aim;
        }
    }

    animation = animation_choose_random_permutation(graph_tag,
        animation_table_lookup(graph, (int16_t)((facing + stance_class * 4) * 0xb + weapon_class)), 1);
    if (animation == -1) {
        if (forced) {
            OBJECT_U16(obj, 0x298) = (uint16_t)((OBJECT_U16(obj, 0x298) & 0xfff7) | 4);
            if (OBJECT_U8(unit_tag, 0x17c) & 2) {
                object_delete_teardown(unit_index);
                unit_pick_random_spawned_actor_count(unit_index);
            }
        }
    } else {
        uint8_t *animation_data = *(uint8_t **)&((ModelAnimations *)graph)->animations.pointer + animation * 0xb4;

        if (OBJECT_U8(obj, 0x2a3) == 0x21) {
            unit_release_thrown_grenade(unit_index, 1);
        }
        object_copy_default_node_transforms(unit_index, 3);
        OBJECT_U8(obj, 0x2a3) = (uint8_t)new_state;
        unit_set_custom_animation(unit_index, graph_tag, animation);
        OBJECT_U8(obj, 0x298) |= 1;
        if (forced) {
            uint8_t keep_still = suppress_shield_check || allow_death_reaction;

            if (!keep_still && network_game_mode != 0) {
                datum_index weapon = unit_get_weapon_object_index(unit_index, OBJECT_I16(obj, 0x2f2));

                if (object_try_and_get(weapon, 4) != 0 && weapon_must_be_readied(weapon) == 1) {
                    keep_still = 1;
                }
            }
            if (keep_still) {
                OBJECT_U8(obj, 0x28c) = 0;
            } else {
                int16_t frames = *(int16_t *)(animation_data + 0x22);
                int8_t ticks = (int8_t)random_int_range((int16_t)(frames >> 2),
                    (int16_t)((frames >> 1) + (frames >> 2)));

                OBJECT_U8(obj, 0x28c) = (uint8_t)(ticks > 1 ? ticks : 1);
            }
        }
        if ((int16_t)facing != 0 &&
            *(int16_t *)(animation_data + 0x42) ==
                animation_table_lookup(graph, (int16_t)stance_class * 0x2c + (int16_t)weapon_class)) {
            facing = 0;
        }
        if (forced) {
            if ((int16_t)facing == 3) {
                OBJECT_U8(obj, 0x298) |= 8;
            } else {
                OBJECT_U8(obj, 0x298) &= 0xf7;
            }
        }
    }

aim:
    if (throttle == 0 || (OBJECT_I32(unit_tag, 0x17c) & 0x200) || OBJECT_I16(obj, 0xb4) != 0 ||
        OBJECT_I32(obj, 0x11c) != -1 || (!hard_ping && !forced)) {
        return;
    }
    {
        real_vector2d direction;

        switch ((int16_t)facing) {
        case 0:
            direction.i = -throttle->i;
            direction.j = -throttle->j;
            break;
        case 1:
            direction.i = -throttle->j;
            direction.j = throttle->i;
            break;
        case 2:
            direction.i = throttle->j;
            direction.j = -throttle->i;
            break;
        default: // 3 (facing is always 0..3)
            direction = *throttle;
            break;
        }
        unit_set_throw_aim_direction(unit_index, &direction);
    }
}

#if 0
Original Ghidra decompilation (0x566de0):

void FUN_00566de0(uint param_1,char param_2,char param_3,char param_4,char param_5,char param_6,
                 float param_7,short param_8,int param_9,char param_10)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  bool bVar5;
  char cVar6;
  char cVar7;
  short sVar8;
  undefined4 uVar9;
  bool bVar10;
  short sVar11;
  short sVar12;
  int iVar13;

  iVar13 = (param_1 & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar13);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (param_2 == '\0') {
    if (param_3 == '\0') {
      if ((*(float *)(iVar2 + 0x218) < (float)puVar1[0x3b]) ||
         (*(float *)(iVar2 + 0x218) < (float)puVar1[0x3a])) {
        bVar5 = true;
      }
      else {
        bVar5 = false;
      }
      bVar10 = *(float *)(iVar2 + 0x220) < (float)puVar1[0x3b];
      if ((param_5 == '\0') && (-1 < (char)puVar1[0x81])) goto LAB_00566eca;
    }
    else {
      param_2 = '\x01';
      bVar5 = true;
    }
LAB_00566ec4:
    bVar10 = false;
  }
  else {
    param_3 = '\0';
    bVar5 = true;
    if ((*(float *)(iVar2 + 0x228) <= 0.0) || ((float)puVar1[0x3b] <= *(float *)(iVar2 + 0x228)))
    goto LAB_00566ec4;
    bVar10 = true;
  }
LAB_00566eca:
  if (param_6 != '\0') {
    bVar5 = true;
    bVar10 = true;
  }
  if (param_8 == -1) {
    param_8 = 0;
  }
  if (0.7853982 <= ABS(param_7)) {
    if (ABS(param_7) <= 2.159845) {
      sVar8 = 1;
      if (param_7 <= 0.0) {
        sVar8 = 2;
      }
    }
    else {
      sVar8 = 0;
    }
  }
  else {
    sVar8 = 3;
  }
  if ((((DAT_006f1d20 != 0) && (param_8 == 2)) && (bVar10)) && (param_2 != '\0')) {
    sVar8 = 1;
  }
  if (((param_10 != '\0') && (!bVar5)) && (param_2 == '\0')) {
    return;
  }
  iVar3 = *(int *)((*(uint *)(iVar2 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((!bVar10) && (param_2 == '\0')) {
    if ((*(short *)((int)puVar1 + 0x2b2) != -1) &&
       ((short)puVar1[0xad] <= *(short *)(iVar2 + 0x2c8))) {
      return;
    }
    sVar8 = FUN_004d6280(1);
    if (sVar8 == -1) {
      return;
    }
    *(short *)((int)puVar1 + 0x2b2) = sVar8;
    *(undefined2 *)(puVar1 + 0xad) = 0;
    return;
  }
  cVar7 = (param_2 != '\0') * '\x02' + '\x17';
  if (param_2 == '\0') {
    param_7._0_2_ = 1;
    cVar6 = FUN_00565be0();
    sVar12 = 1;
    if (cVar6 != '\0') goto LAB_00567031;
    bVar5 = false;
  }
  else {
    sVar12 = bVar10 + 2;
LAB_00567031:
    param_7._0_2_ = sVar12;
    bVar5 = true;
  }
  if ((*(char *)((int)puVar1 + 0x2a3) == '\x17') &&
     (*(short *)(iVar2 + 0x2ca) < *(short *)((int)puVar1 + 0xd2))) {
    bVar5 = true;
  }
  if (param_2 == '\0') {
    if ((*(byte *)((int)puVar1 + 0x106) & 4) != 0) {
      bVar5 = false;
    }
    if (puVar1[0x47] != 0xffffffff) {
      return;
    }
  }
  if (!bVar5) {
    return;
  }
  if (param_2 != '\0') {
    uVar9 = unit_get_current_weapon_label(1);
    unit_set_or_test_seat_and_weapon_label(PTR_s_stand_0069fdec,uVar9);
  }
  if ((((cVar7 == '\x19') && ((short)puVar1[0x2d] == 0)) &&
      (puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar13), (puVar4[0x133] & 1) != 0))
     && ((*(uint *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 0x400) == 0
        )) {
    cVar7 = '\x18';
    cVar6 = unit_try_set_animation_state(param_1,0x18);
    if (cVar6 != '\0') goto LAB_00567314;
  }
  uVar9 = FUN_004d6280(1);
  sVar12 = (short)uVar9;
  if (sVar12 == -1) {
    if ((param_2 != '\0') &&
       (*(ushort *)(puVar1 + 0xa6) = (ushort)puVar1[0xa6] & 0xfff7 | 4,
       (*(byte *)(iVar2 + 0x17c) & 2) != 0)) {
      object_delete_teardown();
      FUN_00568540();
    }
    goto LAB_00567314;
  }
  if (*(char *)((int)puVar1 + 0x2a3) == '!') {
    unit_release_thrown_grenade(param_1,1);
  }
  FUN_004f6b70();
  *(char *)((int)puVar1 + 0x2a3) = cVar7;
  unit_set_custom_animation(*(undefined4 *)(iVar2 + 0x44),uVar9);
  *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] | 1;
  if (param_2 != '\0') {
    if ((param_4 == '\0') && (param_3 == '\0')) {
      if (DAT_00719720 != 0) {
        unit_get_weapon_object_index();
        iVar13 = object_try_and_get(4);
        if ((iVar13 != 0) && (cVar7 = FUN_004c2ea0(), cVar7 == '\x01')) goto LAB_00567261;
      }
      sVar11 = *(short *)(sVar12 * 0xb4 + *(int *)(iVar3 + 0x78) + 0x22);
      cVar7 = random_int_range(CONCAT22(sVar11 >> 0xf,sVar11 >> 1) + (uint)(ushort)(sVar11 >> 2));
      *(char *)(puVar1 + 0xa3) = cVar7;
      if (cVar7 < '\x02') {
        cVar7 = '\x01';
      }
      *(char *)(puVar1 + 0xa3) = cVar7;
    }
    else {
LAB_00567261:
      *(undefined1 *)(puVar1 + 0xa3) = 0;
    }
  }
  if (sVar8 != 0) {
    iVar13 = param_7._0_2_ * 0x2c + (int)param_8;
    if ((iVar13 < 0) || (*(int *)(iVar3 + 0x3c) <= iVar13)) {
      sVar11 = -1;
    }
    else {
      sVar11 = *(short *)(*(int *)(iVar3 + 0x40) + iVar13 * 2);
    }
    if (*(short *)(sVar12 * 0xb4 + *(int *)(iVar3 + 0x78) + 0x42) == sVar11) {
      sVar8 = 0;
    }
  }
  if (param_2 != '\0') {
    if (sVar8 == 3) {
      *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] | 8;
    }
    else {
      *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] & 0xf7;
    }
  }
LAB_00567314:
  if ((((param_9 != 0) && ((*(uint *)(iVar2 + 0x17c) & 0x200) == 0)) && ((short)puVar1[0x2d] == 0))
     && ((puVar1[0x47] == 0xffffffff && ((bVar10 || (param_2 != '\0')))))) {
    switch(sVar8) {
    case 0:
      FUN_005704d0();
      return;
    case 1:
      FUN_005704d0();
      return;
    case 2:
      break;
    case 3:
    }
    FUN_005704d0();
  }
  return;
}
#endif
