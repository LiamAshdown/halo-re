// actor_refresh_combat_context  (Ghidra: actor_refresh_combat_context, already named)
// address 0x4297a0, size 1845 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN (from objdump 0x4297a0..0x429edd; the draft was confidence 0.2 with argument-less callees). Offsets
//   are the actor's (0x724 each in actor_data):
//   swarm actor (+0x06): every creature (swarm 0x98 each in 0x0088035c, count +0x02, units +0x18, creatures
//   +0x58; creature 0x40 each in 0x00880358) gets its unit position (+0x04) and vehicle (+0x10: the unit's
//   +0x4d8 when it is a biped, else -1); the swarm centre (+0x0c) is their average. The actor's context block
//   (+0x120, 0xa8 bytes) is cleared (+0x158 vehicle and +0x164 = -1) and, with a leader unit (+0x24), filled
//   by actor_fill_unit_position_context.
//   otherwise: actor_fill_unit_position_context(EBX unit, +0x120); the head marker position (0x672034) gives
//   the water/weather test (EBX point, stack: +0x144 location, NULL) into +0x15d; +0x99 is ActorVariant flag
//   bit 21. In a vehicle (parent type 1): +0x158 = the vehicle, +0x15e seat kind (1 driver; 4 or 2/3 by
//   Vehicle flags 0x800/0x1000/0x2000), +0x161 gunner and +0x162 = the actor definition's +0x14c > 0,
//   +0x160 = seat kind <= 1; a vehicle that wants the actor in its own encounter/squad (+0x334/+0x336) moves
//   it there (remembering the old one in +0x40/+0x44/+0x48) through actor_reset_squad_link_for_type_change.
//   Out of a vehicle the remembered squad is restored. Then the attached objects are scanned (enemy bipeds
//   set +0x1b4; a stuck projectile sets +0x1b0), the unit's aim state (+0x15c, +0x164, +0x168) is copied for
//   an on-foot biped, and the facing (+0x174, flattened), aim (+0x180), look (+0x18c), right (+0x198) and up
//   (+0x1a4) vectors and four unit values (+0x1b8..+0x1c4) are refreshed.
// blam-cc: stack -> actor_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include "game.h"
#include <string.h>
#include "units.h"
#include "fn_ai.h"

extern data_array *actor_data;        // 0x00880360
extern data_array *swarm_data;        // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern data_array *encounter_data;    // 0x008802c8
extern encounter_squad_state *encounter_squad_states; // 0x008802cc
extern data_array *object_data;       // 0x008603b0
extern tag_instance *tag_instances;   // 0x0087bc14
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t *team_pair_data; // 0x006b0b84, a bitset at +0xa4 (10 x 10 teams)
extern const real_point3d *global_zero_vector3d_pointer; // 0x006966f8
extern const real_vector3d *global_forward3d_pointer;   // 0x00696718
extern const real_vector3d *global_up3d_pointer;        // 0x00696720
extern char ai_marker_name_b[];    // 0x00672034

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, blam-cc: EAX, ECX

    // 0x4296c0, blam-cc: EBX -> unit_index, stack -> out_context
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker,
    uint32_t flags); // 0x4f6080
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out); // 0x53ed60, blam-cc: EBX -> point, stack -> leaf, weather_index_out
extern void *actor_get_actor_definition(datum_index actor_index); // 0x40fa70, blam-cc: EAX
extern void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index,
    int16_t squad_index); // 0x4290f0, blam-cc: EAX, EBX, stack
extern void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out); // 0x569720, ECX, EAX
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, blam-cc: ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
    // 0x4052c0, blam-cc: EAX -> out, ECX -> a, stack -> b
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, blam-cc: ECX

#define A_U8(offset) (*(uint8_t *)(self + (offset)))
#define A_I16(offset) (*(int16_t *)(self + (offset)))
#define A_I32(offset) (*(int32_t *)(self + (offset)))

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

void actor_refresh_combat_context(datum_index actor_index)
{
    uint8_t *self = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = (uint8_t *)tag_instances[((struct actor *)self)->actor_definition_tag & 0xffff].data;
    uint8_t *unit;
    uint8_t *parent = 0;
    datum_index parent_index;
    datum_index child;

    if (A_U8(0x06)) {
        uint8_t *swarm = (uint8_t *)swarm_data->data + (((struct actor *)self)->swarm_index & 0xffff) * 0x98;
        real_point3d *center = (real_point3d *)(swarm + 0xc);
        int16_t count = ((struct swarm *)swarm)->component_count;
        int16_t i;

        *center = *global_zero_vector3d_pointer;
        for (i = 0; i < count; i++) {
            uint8_t *creature = (uint8_t *)swarm_component_data->data +
                (*(datum_index *)(swarm + 0x58 + i * 4) & 0xffff) * 0x40;
            datum_index creature_unit = *(datum_index *)(swarm + 0x18 + i * 4);
            uint8_t *creature_object = object_get(creature_unit);
            datum_index vehicle = ((struct object *)creature_object)->type == 0 ?
                *(datum_index *)(creature_object + 0x4d8) : k_datum_index_none;

            object_get_position((real_point3d *)(creature + 4), creature_unit);
            *(datum_index *)(creature + 0x10) = vehicle;
            center->x = *(float *)(creature + 4) + center->x;
            center->y = *(float *)(creature + 8) + center->y;
            center->z = *(float *)(creature + 0xc) + center->z;
        }
        if (count > 0) {
            float scale = 1.0f / (float)count;

            center->x = scale * center->x;
            center->y = scale * center->y;
            center->z = scale * center->z;
        }
        memset(self + 0x120, 0, 0x2a * 4);
        A_I32(0x158) = -1;
        A_I32(0x164) = -1;
        if (A_I32(0x24) != -1) {
            actor_fill_unit_position_context(A_I32(0x24), (actor_unit_position_context *)(self + 0x120));
        }
        return;
    }

    unit = object_get(A_I32(0x18));
    parent_index = ((unit_object *)unit)->base.parent_object;
    if (parent_index != k_datum_index_none) {
        parent = object_get(parent_index);
    }
    actor_fill_unit_position_context(A_I32(0x18), (actor_unit_position_context *)(self + 0x120));
    {
        object_marker marker;
        real_point3d head;

        object_get_node_local_transform(A_I32(0x18), ai_marker_name_b, &marker, 1);
        head = marker.node_transform.position;
        A_U8(0x15d) = scenario_location_get_water_and_weather(&head, (bsp_leaf_reference *)(self + 0x144), 0);
    }
    A_U8(0x99) = (uint8_t)((*(uint32_t *)actor_tag >> 21) & 1);

    if (parent != 0 && *(int16_t *)(parent + 0xb4) == 1) {
        uint8_t *vehicle_tag = (uint8_t *)tag_instances[*(datum_index *)parent & 0xffff].data;
        uint32_t vehicle_flags;

        A_U8(0x161) = 0;
        A_U8(0x162) = 0;
        A_I16(0x15e) = 0;
        A_I32(0x158) = parent_index;
        if (*(int32_t *)(parent + 0x324) == A_I32(0x18)) {
            A_I16(0x15e) = 1;
            vehicle_flags = *(uint32_t *)(vehicle_tag + 0x2f0);
            if (vehicle_flags & 0x800) {
                if (vehicle_flags & 0x1000) {
                    A_I16(0x15e) = 4;
                    A_U8(0x99) = 1;
                } else if (vehicle_flags & 0x2000) {
                    A_I16(0x15e) = (int16_t)((~(vehicle_flags >> 14) & 1) | 2);
                }
            }
        }
        if (*(int32_t *)(parent + 0x328) == A_I32(0x18)) {
            A_U8(0x161) = 1;
            A_U8(0x162) = *(float *)((uint8_t *)actor_get_actor_definition(actor_index) + 0x14c) > 0.0f;
        }
        A_U8(0x160) = A_I16(0x15e) <= 1;
        if (*(int16_t *)(parent + 0x334) != -1) {
            datum_index encounter = A_I32(0x34);
            int16_t wanted_encounter = *(int16_t *)(parent + 0x334);
            int16_t wanted_squad = *(int16_t *)(parent + 0x336);
            uint8_t move = 1;

            if ((encounter & 0xffff) == (uint32_t)(int32_t)wanted_encounter) {
                if (wanted_squad == -1 || A_I16(0x3a) == wanted_squad) {
                    move = 0;
                } else {
                    uint8_t *encounter_record = (uint8_t *)encounter_data->data + (encounter & 0xffff) * 0x6c;

                    if (((struct encounter *)encounter_record)->follow_mode > 0) {
                        int16_t first = ((struct encounter *)encounter_record)->first_squad;
                        uint8_t *states = (uint8_t *)encounter_squad_states;

                        if (states[(int16_t)(first + A_I16(0x3a)) * 0x20 + 0x10] != 0 &&
                            states[(int16_t)(first + wanted_squad) * 0x20 + 0x10] != 0) {
                            move = 0;
                        }
                    }
                }
            }
            if (move) {
                if (A_U8(0x40) == 0) {
                    A_I32(0x44) = encounter;
                    A_I16(0x48) = A_I16(0x3a);
                    A_U8(0x40) = 1;
                    if (encounter != k_datum_index_none) {
                        *((uint8_t *)encounter_data->data + (encounter & 0xffff) * 0x6c + 0x1e) = 1;
                    }
                }
                actor_reset_squad_link_for_type_change(actor_index, wanted_encounter, wanted_squad);
            }
        }
    } else {
        A_I32(0x158) = -1;
        A_I16(0x15e) = 0;
        A_U8(0x160) = 0;
        A_U8(0x161) = 0;
        if (A_U8(0x40)) {
            actor_reset_squad_link_for_type_change(actor_index, A_I32(0x44), A_I16(0x48));
            A_U8(0x40) = 0;
        }
    }

    A_U8(0x1b5) = unit[0x28b] > 0;
    A_U8(0x1b4) = 0;
    A_I32(0x1b0) = -1;
    for (child = ((unit_object *)unit)->base.first_child_object; child != k_datum_index_none;
         child = *(datum_index *)(object_get(child) + 0x114)) {
        uint8_t *child_object = object_get(child);
        int16_t type = ((struct object *)child_object)->type;

        if (type == 0) {
            int16_t actor_team = A_I16(0x3e);
            int16_t child_team = ((struct object *)child_object)->owner_team;
            uint8_t enemy;

            if (current_game_engine != 0) {
                enemy = actor_team != child_team;
            } else if (actor_team < 0 || actor_team >= 10 || child_team < 0 || child_team >= 10) {
                enemy = 1;
            } else {
                int32_t bit = actor_team * 10 + child_team;

                enemy = (((uint32_t *)(team_pair_data + 0xa4))[bit >> 5] & (1u << (bit & 0x1f))) == 0;
            }
            if (enemy) {
                A_U8(0x1b4) = 1;
            }
        } else if (type == 5) {
            if ((int8_t)child_object[0x22c] < 0 || (A_I16(0x280) == 2 && child == A_I32(0x28c))) {
                A_I32(0x1b0) = child;
            }
        }
    }

    A_U8(0x15c) = 0;
    A_I32(0x164) = -1;
    if (((unit_object *)unit)->base.type == 0 && A_I32(0x158) == -1) {
        uint8_t *unit_object = object_get(A_I32(0x18));

        if ((int8_t)unit_object[0x501] >= 6) {
            A_U8(0x15c) = 1;
        }
        A_I32(0x164) = *(int32_t *)(unit_object + 0x4dc);
        *(real_vector3d *)&((struct actor *)self)->unknown_168 = *(real_vector3d *)(unit_object + 0x4e0);
    }
    unit_get_forward_vector_or_marker_normal(A_I16(0x15e) > 0 ? A_I32(0x158) : A_I32(0x18),
        (real_vector3d *)(self + 0x174));
    if (A_U8(0x99) == 0) {
        if (vector2d_normalize_with_length((real_vector2d *)(self + 0x174)) > 0.0f) {
            ((struct actor *)self)->facing.k = 0.0f;
        } else {
            *(real_vector3d *)&((struct actor *)self)->facing.i = *global_forward3d_pointer;
        }
    }
    if (A_U8(0x161)) {
        uint8_t *vehicle = object_get(A_I32(0x158));
        uint8_t *vehicle_tag = (uint8_t *)tag_instances[*(datum_index *)vehicle & 0xffff].data;

        if (*(uint32_t *)(vehicle_tag + 0x2f0) & 0x100) {
            unit_get_forward_vector_or_marker_normal(A_I32(0x18), (real_vector3d *)(self + 0x180));
        } else {
            *(real_vector3d *)&((struct actor *)self)->facing_unknown_180.i = *(real_vector3d *)&((vehicle_object *)vehicle)->unit.aiming_vector.i;
        }
    } else {
        *(real_vector3d *)&((struct actor *)self)->facing_unknown_180.i = *(real_vector3d *)&((unit_object *)unit)->unit.aiming_vector.i;
    }
    *(real_vector3d *)&((struct actor *)self)->facing_unknown_18c.i = *(real_vector3d *)&((unit_object *)unit)->unit.looking_vector.i;
    vector3d_cross_product((real_vector3d *)(self + 0x198), (real_vector3d *)(self + 0x18c), global_up3d_pointer);
    vector3d_normalize_with_length((real_vector3d *)(self + 0x198));
    vector3d_cross_product((real_vector3d *)(self + 0x1a4), (real_vector3d *)(self + 0x198),
        (real_vector3d *)(self + 0x18c));
    A_I32(0x1b8) = *(int32_t *)&((unit_object *)unit)->base.body_vitality;
    A_I32(0x1bc) = *(int32_t *)&((unit_object *)unit)->base.shield_vitality;
    A_I32(0x1c0) = *(int32_t *)&((unit_object *)unit)->base.recent_body_damage;
    A_I32(0x1c4) = *(int32_t *)&((unit_object *)unit)->base.recent_shield_damage;
}

#if 0
Original Ghidra decompilation (0x4297a0):

void actor_refresh_combat_context(uint param_1)

{
  float *pfVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  float fVar6;
  undefined *puVar7;
  undefined1 uVar8;
  short sVar9;
  int iVar10;
  uint uVar11;
  uint *puVar12;
  int iVar13;
  int iVar14;
  undefined4 *puVar15;
  char cVar16;
  float10 fVar17;
  undefined4 local_7c;
  undefined1 local_6c [108];

  puVar7 = PTR_DAT_006966f8;
  iVar13 = (param_1 & 0xffff) * 0x724;
  iVar14 = iVar13 + *(int *)(DAT_00880360 + 0x34);
  puVar3 = *(uint **)((*(uint *)(iVar13 + 0x58 + *(int *)(DAT_00880360 + 0x34)) & 0xffff) * 0x20 +
                      0x14 + DAT_0087bc14);
  if (*(char *)(iVar14 + 6) == '\0') {
    iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar14 + 0x18) & 0xffff) * 0xc)
    ;
    uVar5 = *(uint *)(iVar13 + 0x11c);
    if (uVar5 == 0xffffffff) {
      puVar12 = (uint *)0x0;
    }
    else {
      puVar12 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc);
    }
    FUN_004296c0(iVar14 + 0x120);
    object_get_node_local_transform(*(undefined4 *)(iVar14 + 0x18),&DAT_00672034,local_6c,1);
    uVar8 = FUN_0053ed60(iVar14 + 0x144,0);
    *(undefined1 *)(iVar14 + 0x15d) = uVar8;
    *(byte *)(iVar14 + 0x99) = (byte)(*puVar3 >> 0x15) & 1;
    if ((puVar12 == (uint *)0x0) || ((short)puVar12[0x2d] != 1)) {
      *(undefined4 *)(iVar14 + 0x158) = 0xffffffff;
      *(undefined2 *)(iVar14 + 0x15e) = 0;
      *(undefined1 *)(iVar14 + 0x160) = 0;
      *(undefined1 *)(iVar14 + 0x161) = 0;
      if (*(char *)(iVar14 + 0x40) != '\0') {
        FUN_004290f0(*(undefined2 *)(iVar14 + 0x48));
        *(undefined1 *)(iVar14 + 0x40) = 0;
      }
    }
    else {
      iVar10 = *(int *)((*puVar12 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      *(undefined1 *)(iVar14 + 0x161) = 0;
      *(undefined1 *)(iVar14 + 0x162) = 0;
      *(undefined2 *)(iVar14 + 0x15e) = 0;
      *(uint *)(iVar14 + 0x158) = uVar5;
      if (puVar12[0xc9] == *(uint *)(iVar14 + 0x18)) {
        *(undefined2 *)(iVar14 + 0x15e) = 1;
        uVar5 = *(uint *)(iVar10 + 0x2f0);
        if ((uVar5 & 0x800) != 0) {
          if ((uVar5 & 0x1000) == 0) {
            if ((uVar5 & 0x2000) != 0) {
              *(ushort *)(iVar14 + 0x15e) = (byte)~(byte)(uVar5 >> 0xe) & 1 | 2;
            }
          }
          else {
            *(undefined2 *)(iVar14 + 0x15e) = 4;
            *(undefined1 *)(iVar14 + 0x99) = 1;
          }
        }
      }
      if (puVar12[0xca] == *(uint *)(iVar14 + 0x18)) {
        *(undefined1 *)(iVar14 + 0x161) = 1;
        iVar10 = actor_get_actor_definition();
        *(bool *)(iVar14 + 0x162) = 0.0 < *(float *)(iVar10 + 0x14c);
      }
      *(bool *)(iVar14 + 0x160) = *(short *)(iVar14 + 0x15e) < 2;
      if ((short)puVar12[0xcd] != -1) {
        uVar5 = *(uint *)(iVar14 + 0x34);
        uVar11 = uVar5 & 0xffff;
        if ((uVar11 != (int)(short)puVar12[0xcd]) ||
           (((*(short *)((int)puVar12 + 0x336) != -1 &&
             (*(short *)(iVar14 + 0x3a) != *(short *)((int)puVar12 + 0x336))) &&
            ((iVar10 = uVar11 * 0x6c + *(int *)(DAT_008802c8 + 0x34), *(short *)(iVar10 + 0x62) < 1
             || ((sVar9 = *(short *)(iVar10 + 4),
                 *(char *)((short)(*(short *)(iVar14 + 0x3a) + sVar9) * 0x20 + 0x10 + DAT_008802cc)
                 == '\0' ||
                 (*(char *)((short)(sVar9 + *(short *)((int)puVar12 + 0x336)) * 0x20 + 0x10 +
                           DAT_008802cc) == '\0')))))))) {
          if (*(char *)(iVar14 + 0x40) == '\0') {
            *(uint *)(iVar14 + 0x44) = uVar5;
            *(undefined2 *)(iVar14 + 0x48) = *(undefined2 *)(iVar14 + 0x3a);
            *(undefined1 *)(iVar14 + 0x40) = 1;
            if (uVar5 != 0xffffffff) {
              *(undefined1 *)(uVar11 * 0x6c + 0x1e + *(int *)(DAT_008802c8 + 0x34)) = 1;
            }
          }
          FUN_004290f0(*(undefined2 *)((int)puVar12 + 0x336));
        }
      }
    }
    *(bool *)(iVar14 + 0x1b5) = *(char *)(iVar13 + 0x28b) != '\0';
    *(undefined1 *)(iVar14 + 0x1b4) = 0;
    *(undefined4 *)(iVar14 + 0x1b0) = 0xffffffff;
    uVar5 = *(uint *)(iVar13 + 0x118);
    while (uVar5 != 0xffffffff) {
      iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc);
      if (*(short *)(iVar10 + 0xb4) == 0) {
        sVar9 = *(short *)(iVar10 + 0xb8);
        sVar2 = *(short *)(iVar14 + 0x3e);
        if (DAT_006f1d20 == 0) {
          if ((((-1 < sVar2) && (sVar2 < 10)) && (-1 < sVar9)) && (sVar9 < 10)) {
            iVar4 = (int)sVar9 + sVar2 * 10;
            cVar16 = '\x01' - ((1 << ((byte)iVar4 & 0x1f) &
                               *(uint *)(DAT_006b0b84 + 0xa4 + (iVar4 >> 5) * 4)) != 0);
            goto LAB_00429ca3;
          }
        }
        else {
          cVar16 = sVar2 != sVar9;
LAB_00429ca3:
          if (cVar16 == '\0') goto LAB_00429cb0;
        }
        *(undefined1 *)(iVar14 + 0x1b4) = 1;
      }
      else {
LAB_00429cb0:
        if ((*(short *)(iVar10 + 0xb4) == 5) &&
           ((*(char *)(iVar10 + 0x22c) < '\0' ||
            ((*(short *)(iVar14 + 0x280) == 2 && (uVar5 == *(uint *)(iVar14 + 0x28c))))))) {
          *(uint *)(iVar14 + 0x1b0) = uVar5;
        }
      }
      uVar5 = *(uint *)(iVar10 + 0x114);
    }
    *(undefined1 *)(iVar14 + 0x15c) = 0;
    *(undefined4 *)(iVar14 + 0x164) = 0xffffffff;
    if ((*(short *)(iVar13 + 0xb4) == 0) && (*(int *)(iVar14 + 0x158) == -1)) {
      iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                       (*(uint *)(iVar14 + 0x18) & 0xffff) * 0xc);
      if ('\x05' < *(char *)(iVar10 + 0x501)) {
        *(undefined1 *)(iVar14 + 0x15c) = 1;
      }
      *(undefined4 *)(iVar14 + 0x164) = *(undefined4 *)(iVar10 + 0x4dc);
      *(undefined4 *)(iVar14 + 0x168) = *(undefined4 *)(iVar10 + 0x4e0);
      *(undefined4 *)(iVar14 + 0x16c) = *(undefined4 *)(iVar10 + 0x4e4);
      *(undefined4 *)(iVar14 + 0x170) = *(undefined4 *)(iVar10 + 0x4e8);
    }
    FUN_00569720();
    if (*(char *)(iVar14 + 0x99) == '\0') {
      fVar17 = (float10)vector2d_normalize_with_length();
      puVar7 = PTR_DAT_00696718;
      if (fVar17 <= (float10)0.0) {
        *(undefined4 *)(iVar14 + 0x174) = *(undefined4 *)PTR_DAT_00696718;
        *(undefined4 *)(iVar14 + 0x178) = *(undefined4 *)(puVar7 + 4);
        *(undefined4 *)(iVar14 + 0x17c) = *(undefined4 *)(puVar7 + 8);
      }
      else {
        *(undefined4 *)(iVar14 + 0x17c) = 0;
      }
    }
    if (*(char *)(iVar14 + 0x161) == '\0') {
      *(undefined4 *)(iVar14 + 0x180) = *(undefined4 *)(iVar13 + 0x23c);
      *(undefined4 *)(iVar14 + 0x184) = *(undefined4 *)(iVar13 + 0x240);
      *(undefined4 *)(iVar14 + 0x188) = *(undefined4 *)(iVar13 + 0x244);
    }
    else {
      puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                         (*(uint *)(iVar14 + 0x158) & 0xffff) * 0xc);
      if ((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f0) & 0x100) ==
          0) {
        *(uint *)(iVar14 + 0x180) = puVar3[0x8f];
        *(uint *)(iVar14 + 0x184) = puVar3[0x90];
        *(uint *)(iVar14 + 0x188) = puVar3[0x91];
      }
      else {
        FUN_00569720();
      }
    }
    *(undefined4 *)(iVar14 + 0x18c) = *(undefined4 *)(iVar13 + 0x260);
    *(undefined4 *)(iVar14 + 400) = *(undefined4 *)(iVar13 + 0x264);
    *(undefined4 *)(iVar14 + 0x194) = *(undefined4 *)(iVar13 + 0x268);
    vector3d_cross_product(PTR_DAT_00696720);
    vector3d_normalize_with_length();
    vector3d_cross_product((undefined4 *)(iVar14 + 0x18c));
    *(undefined4 *)(iVar14 + 0x1b8) = *(undefined4 *)(iVar13 + 0xe0);
    *(undefined4 *)(iVar14 + 0x1bc) = *(undefined4 *)(iVar13 + 0xe4);
    *(undefined4 *)(iVar14 + 0x1c0) = *(undefined4 *)(iVar13 + 0xf8);
    *(undefined4 *)(iVar14 + 0x1c4) = *(undefined4 *)(iVar13 + 0xf4);
  }
  else {
    iVar13 = (*(uint *)(iVar14 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    pfVar1 = (float *)(iVar13 + 0xc);
    *pfVar1 = *(float *)PTR_DAT_006966f8;
    *(undefined4 *)(iVar13 + 0x10) = *(undefined4 *)(puVar7 + 4);
    *(undefined4 *)(iVar13 + 0x14) = *(undefined4 *)(puVar7 + 8);
    sVar9 = 0;
    if (0 < *(short *)(iVar13 + 2)) {
      do {
        iVar10 = (*(uint *)(iVar13 + 0x58 + sVar9 * 4) & 0xffff) * 0x40 +
                 *(int *)(DAT_00880358 + 0x34);
        iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                        (*(uint *)(iVar13 + 0x18 + sVar9 * 4) & 0xffff) * 0xc);
        local_7c = 0xffffffff;
        if (*(short *)(iVar4 + 0xb4) == 0) {
          local_7c = *(undefined4 *)(iVar4 + 0x4d8);
        }
        object_get_position();
        *(undefined4 *)(iVar10 + 0x10) = local_7c;
        sVar9 = sVar9 + 1;
        *pfVar1 = *(float *)(iVar10 + 4) + *pfVar1;
        *(float *)(iVar13 + 0x10) = *(float *)(iVar10 + 8) + *(float *)(iVar13 + 0x10);
        *(float *)(iVar13 + 0x14) = *(float *)(iVar10 + 0xc) + *(float *)(iVar13 + 0x14);
      } while (sVar9 < *(short *)(iVar13 + 2));
    }
    if (0 < *(short *)(iVar13 + 2)) {
      fVar6 = 1.0 / (float)(int)*(short *)(iVar13 + 2);
      *pfVar1 = fVar6 * *pfVar1;
      *(float *)(iVar13 + 0x10) = fVar6 * *(float *)(iVar13 + 0x10);
      *(float *)(iVar13 + 0x14) = fVar6 * *(float *)(iVar13 + 0x14);
    }
    puVar15 = (undefined4 *)(iVar14 + 0x120);
    for (iVar13 = 0x2a; iVar13 != 0; iVar13 = iVar13 + -1) {
      *puVar15 = 0;
      puVar15 = puVar15 + 1;
    }
    *(undefined4 *)(iVar14 + 0x158) = 0xffffffff;
    *(undefined4 *)(iVar14 + 0x164) = 0xffffffff;
    if (*(int *)(iVar14 + 0x24) != -1) {
      FUN_004296c0((undefined4 *)(iVar14 + 0x120));
      return;
    }
  }
  return;
}
#endif
