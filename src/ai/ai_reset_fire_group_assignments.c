// ai_reset_fire_group_assignments  (Ghidra: FUN_0042c940; really the AI's structure bsp deactivate proc, slot 4 of
//   the deactivate table at 0x0069e90c)
// address 0x42c940, size 1358 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// REWRITTEN from objdump 0x42c940..0x42ce8d (the draft called encounter_deactivate and the object deletes without
//   arguments and was believed dead). Before a bsp switch, for every scenario encounter (+0x42c count) that is live
//   (+0x0d) with members (+0x2a):
//   - each member actor (+0x14, next +0x2c) is carried into the unassigned list when it is fighting a player's unit
//     (combat status +0x268 >= 5, target prop (or its pair for kinds 4..5) player-controlled +0x12e, fired recently
//     +0x88 < 90 ticks, within 10), or, for an actor friendly to the player team (enemy bits, team * 10 + 1), when any
//     of its props is a player's unit seen twice (+0x32 >= 2) or within 3;
//   - a swarm carried along keeps only its members in a cluster a local player can see (0x87a478 +0x18 bits); the
//     others become their own actors (actor_new_and_attach_to_unit) or are deleted; nothing is carried when no member
//     can be seen;
//   - a carried actor remembers its encounter/squad (+0x30/+0x38), drops its firing position and position-bound
//     movement, runs its mode's +0x24 proc, leaves the encounter (0x436620), and joins the unassigned list (+0x09 set,
//     +0x10 = 90 when active) with its movement cancelled;
//   then the encounter is deactivated. Finally every unassigned actor clears its target state (0x4286c0) and its props'
//   bsp locations (+0xec, +0xfc, +0x100).
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "game.h"

extern data_array *actor_data;       // 0x00880360
extern data_array *encounter_data;   // 0x008802c8
extern data_array *prop_data;        // 0x008802c0
extern data_array *swarm_data;       // 0x0088035c
extern data_array *object_data;      // 0x008603b0
extern uint8_t *ai_globals_ptr;  // 0x00880354
extern Scenario *global_scenario;
extern uint8_t *team_pair_data;     // 0x006b0b84
extern game_engine_definition *current_game_engine;
extern player_globals *local_player_globals; // 0x0087a478
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index); // 0x427c90, ECX, stack
extern datum_index actor_new_and_attach_to_unit(char reuse_existing, datum_index unit_index, datum_index actor_variant_tag,
    uint32_t encounter_or_none, int16_t squad_index, char ignore_squad, datum_index exclude_actor, char start_active,
    uint16_t unknown_60, int16_t unknown_62, uint16_t unknown_90, uint8_t unknown_68); // 0x426ac0
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0, EDI
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0
extern void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters); // 0x436620, EAX, stack
extern void actor_movement_action_cancel(datum_index actor_index); // 0x428650, EDI
extern void actor_clear_target_state(datum_index actor_index); // 0x4286c0
extern void encounter_deactivate(datum_index encounter_index); // 0x437870, EAX

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)
#define OBJ(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

static uint8_t ai_bsp_actor_should_carry(uint8_t *actor)
{
    if (*(datum_index *)(actor + 0x270) != k_datum_index_none && *(int16_t *)(actor + 0x268) >= 5) {
        uint8_t *target = PROP(*(datum_index *)(actor + 0x270));
        int32_t fired = *(int32_t *)(actor + 0x88);

        if (*(int16_t *)(target + 0x24) >= 4 && *(int16_t *)(target + 0x24) <= 5) {
            target = PROP(*(datum_index *)(target + 0xc));
        }
        return target[0x12e] != 0 && fired != -1 && fired < 0x5a && *(float *)(target + 0x11c) < 10.0f;
    }
    {
        int16_t team = *(int16_t *)(actor + 0x3e);
        uint8_t enemies;
        uint8_t carry = 0;
        datum_index prop_index;

        if (current_game_engine != 0) {
            enemies = team != 1;
        } else {
            int32_t index;

            if (team < 0 || team >= 10) {
                return 0;
            }
            index = team * 10 + 1;
            enemies = (*(uint32_t *)(team_pair_data + 0xa4 + (index >> 5) * 4) & (1u << (index & 0x1f))) == 0;
        }
        if (enemies) {
            return 0;
        }
        for (prop_index = *(datum_index *)(actor + 0x50); prop_index != k_datum_index_none;) {
            uint8_t *p = PROP(prop_index);

            prop_index = *(datum_index *)(p + 8);
            if (p[0x12e] != 0 && (*(int16_t *)(p + 0x32) >= 2 || *(float *)(p + 0x11c) < 3.0f)) {
                carry = 1;
            }
        }
        return carry;
    }
}

// 0x42cb6e: split a swarm by visibility; returns 0 when nothing of it can be seen (the actor stays)
static uint8_t ai_bsp_split_swarm(datum_index actor_index, uint8_t *actor)
{
    uint8_t *swarm;
    int16_t count;
    int16_t hidden = 0;
    datum_index hidden_units[16];
    int16_t i;

    if (*(datum_index *)(actor + 0x28) == k_datum_index_none) {
        return 0;
    }
    swarm = (uint8_t *)swarm_data->data + (*(datum_index *)(actor + 0x28) & 0xffff) * 0x98;
    count = *(int16_t *)(swarm + 2);
    for (i = 0; i < count; i++) {
        datum_index unit_index = *(datum_index *)(swarm + 0x18 + i * 4);
        datum_index root = unit_index;
        int16_t cluster;

        while (*(datum_index *)(OBJ(root) + 0x11c) != k_datum_index_none) {
            root = *(datum_index *)(OBJ(root) + 0x11c);
        }
        cluster = *(int16_t *)(OBJ(root) + 0x9c);
        if (cluster == -1 ||
            (*(uint32_t *)&local_player_globals->cluster_pvs[(cluster >> 5)] & (1u << (cluster & 0x1f))) == 0) {
            hidden_units[hidden++] = unit_index;
        }
    }
    if (hidden == 0) {
        return 1;
    }
    if (hidden == count) {
        return 0;
    }
    for (i = 0; i < hidden; i++) {
        datum_index unit_index = hidden_units[i];

        actor = ACTOR(actor_index);
        actor_remove_from_unit_cluster(actor_index, unit_index);
        actor = ACTOR(actor_index);
        if (actor_new_and_attach_to_unit(1, unit_index, *(datum_index *)(actor + 0x5c), *(uint32_t *)(actor + 0x34),
                *(int16_t *)(actor + 0x3a), 0, actor_index, 0, 2, 0, 0xffff, 0) == k_datum_index_none) {
            int32_t kind = *(int32_t *)(OBJ(unit_index) + 4);

            if (kind == 0) {
                object_delete_unparented(unit_index);
                object_delete_recursive(unit_index, 0);
            } else if (kind == 3) {
                object_delete_recursive(unit_index, 0);
            }
        }
    }
    return 1;
}

void ai_reset_fire_group_assignments(void)
{
    int32_t encounter_count = *(int32_t *)&global_scenario->encounters.count;
    int16_t e;
    datum_index actor_index;

    for (e = 0; e < encounter_count; e++) {
        uint8_t *encounter = (uint8_t *)encounter_data->data + (e & 0xffff) * 0x6c;
        datum_index next;

        if (encounter[0xd] == 0 || *(int16_t *)(encounter + 0x2a) <= 0) {
            continue;
        }
        next = *(datum_index *)(encounter + 0x14);
        while (ai_globals_ptr[1] != 0 && next != k_datum_index_none) {
            uint8_t *actor;
            uint8_t carry;

            actor_index = next;
            actor = ACTOR(actor_index);
            next = *(datum_index *)(actor + 0x2c);
            carry = ai_bsp_actor_should_carry(actor);
            if (!carry) {
                continue;
            }
            if (actor[6] != 0 && !ai_bsp_split_swarm(actor_index, actor)) {
                continue;
            }
            actor = ACTOR(actor_index);
            *(int32_t *)(actor + 0x30) = e;
            *(int16_t *)(actor + 0x38) = *(int16_t *)(actor + 0x3a);
            *(int16_t *)(actor + 0x3b8) = -1;
            if (*(int16_t *)(actor + 0x46c) == 3 || *(int16_t *)(actor + 0x46c) == 4) {
                *(int16_t *)(actor + 0x46c) = 0;
                *(datum_index *)(actor + 0x480) = k_datum_index_none;
            }
            {
                void (*carry_proc)(datum_index) =
                    *(void (**)(datum_index))((uint8_t *)&actor_mode_definitions[*(int16_t *)(actor + 0x6c)] + 0x24);

                if (carry_proc != 0) {
                    carry_proc(actor_index);
                }
            }
            encounter_remove_actor(actor_index, 0);
            if (ai_globals_ptr[1] == 0) {
                break;
            }
            actor = ACTOR(actor_index);
            *(datum_index *)(actor + 0x2c) = *(datum_index *)(ai_globals_ptr + 8);
            *(datum_index *)(ai_globals_ptr + 8) = actor_index;
            actor[9] = 1;
            *(int16_t *)(actor + 0x10) = actor[8] != 0 ? 0x5a : 0;
            actor_movement_action_cancel(actor_index);
        }
        encounter = (uint8_t *)encounter_data->data + (e & 0xffff) * 0x6c;
        *(int16_t *)(encounter + 0xe) = 0;
        encounter_deactivate((datum_index)(int32_t)e);
    }

    // 0x42ce06
    for (actor_index = *(datum_index *)(ai_globals_ptr + 8); actor_index != k_datum_index_none;) {
        datum_index following = *(datum_index *)(ACTOR(actor_index) + 0x2c);
        datum_index prop_index;

        actor_clear_target_state(actor_index);
        for (prop_index = *(datum_index *)(ACTOR(actor_index) + 0x50); prop_index != k_datum_index_none;) {
            uint8_t *p = PROP(prop_index);

            prop_index = *(datum_index *)(p + 8);
            *(int16_t *)(p + 0x100) = -1;
            *(int32_t *)(p + 0xfc) = -1;
            *(int32_t *)(p + 0xec) = -1;
        }
        actor_index = following;
    }
}

#if 0
Original Ghidra decompilation (0x42c940) -- full listing via `python tools/pack.py 0x42c940`:

void FUN_0042c940(void)

{
  short sVar1;
  ushort uVar2;
  uint uVar3;
  uint uVar4;
  bool bVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  uint uVar10;
  ushort uVar11;
  uint uVar12;
  uint *puVar13;
  ushort uVar14;
  int iVar15;
  char cVar16;
  uint local_68;
  uint local_60;
  uint local_44;
  uint local_40 [16];

  iVar6 = global_scenario;
  uVar14 = 0;
  if (0 < *(int *)(global_scenario + 0x42c)) {
    local_68 = 0;
    do {
      iVar9 = *(int *)(DAT_008802c8 + 0x34);
      if ((*(char *)((uint)uVar14 * 0x6c + 0xd + iVar9) != '\0') &&
         (0 < *(short *)((uint)uVar14 * 0x6c + iVar9 + 0x2a))) {
        uVar3 = local_44;
        if (*(char *)(DAT_00880354 + 1) != '\0') {
          if (local_68 == 0xffffffff) {
            uVar3 = *(uint *)(DAT_00880354 + 8);
          }
          else {
            uVar3 = *(uint *)((local_68 & 0xffff) * 0x6c + 0x14 + iVar9);
          }
        }
LAB_0042c9c6:
        while ((local_44 = uVar3, *(char *)(DAT_00880354 + 1) != '\0' && (local_44 != 0xffffffff)))
        {
          iVar9 = *(int *)(DAT_00880360 + 0x34);
          uVar12 = local_44 & 0xffff;
          iVar15 = uVar12 * 0x724;
          uVar10 = *(uint *)(iVar15 + 0x270 + iVar9);
          uVar3 = *(uint *)(iVar15 + 0x2c + iVar9);
          iVar15 = iVar15 + iVar9;
          bVar5 = false;
          if ((uVar10 == 0xffffffff) || (*(short *)(iVar15 + 0x268) < 5)) {
            sVar1 = *(short *)(iVar15 + 0x3e);
            if (DAT_006f1d20 == 0) goto LAB_0042cac2;
            cVar16 = sVar1 != 1;
            goto LAB_0042cb01;
          }
          iVar9 = *(int *)(DAT_008802c0 + 0x34);
          iVar8 = (uVar10 & 0xffff) * 0x138;
          sVar1 = *(short *)(iVar8 + 0x24 + iVar9);
          iVar8 = iVar8 + iVar9;
          if ((3 < sVar1) && (sVar1 < 6)) {
            iVar8 = (*(uint *)(iVar8 + 0xc) & 0xffff) * 0x138 + iVar9;
          }
          if ((((*(char *)(iVar8 + 0x12e) != '\0') && (*(int *)(iVar15 + 0x88) != -1)) &&
              (*(int *)(iVar15 + 0x88) < 0x5a)) && (*(float *)(iVar8 + 0x11c) < 10.0))
          goto LAB_0042cb6e;
        }
      }
      *(undefined2 *)((local_68 & 0xffff) * 0x6c + 0xe + *(int *)(DAT_008802c8 + 0x34)) = 0;
      squad_deactivate();
      uVar14 = uVar14 + 1;
      local_68 = (uint)(short)uVar14;
    } while ((int)local_68 < *(int *)(iVar6 + 0x42c));
  }
  uVar3 = *(uint *)(DAT_00880354 + 8);
  iVar6 = DAT_00880360;
  while (uVar3 != 0xffffffff) {
    iVar15 = (uVar3 & 0xffff) * 0x724;
    uVar10 = *(uint *)(iVar15 + 0x2c + *(int *)(iVar6 + 0x34));
    FUN_004286c0(uVar3);
    iVar6 = DAT_00880360;
    iVar9 = DAT_008802c0;
    uVar12 = *(uint *)(iVar15 + 0x50 + *(int *)(DAT_00880360 + 0x34));
    while (uVar3 = uVar10, uVar12 != 0xffffffff) {
      iVar15 = (uVar12 & 0xffff) * 0x138 + *(int *)(iVar9 + 0x34);
      uVar12 = *(uint *)(iVar15 + 8);
      *(undefined2 *)(iVar15 + 0x100) = 0xffff;
      *(undefined4 *)(iVar15 + 0xfc) = 0xffffffff;
      *(undefined4 *)(iVar15 + 0xec) = 0xffffffff;
    }
  }
  return;
LAB_0042cac2:
  if ((-1 < sVar1) && (sVar1 < 10)) {
    iVar8 = sVar1 * 10 + 1;
    cVar16 = '\x01' - ((1 << ((byte)iVar8 & 0x1f) &
                       *(uint *)(DAT_006b0b84 + 0xa4 + (iVar8 >> 5) * 4)) != 0);
LAB_0042cb01:
    if (cVar16 == '\0') {
      uVar10 = *(uint *)(uVar12 * 0x724 + 0x50 + iVar9);
      while (uVar10 != 0xffffffff) {
        iVar9 = *(int *)(DAT_008802c0 + 0x34);
        iVar8 = (uVar10 & 0xffff) * 0x138;
        uVar10 = *(uint *)(iVar8 + 8 + iVar9);
        if ((*(char *)(iVar8 + 0x12e + iVar9) != '\0') &&
           ((1 < *(short *)(iVar8 + iVar9 + 0x32) || (*(float *)(iVar8 + iVar9 + 0x11c) < 3.0)))) {
          bVar5 = true;
        }
      }
      if (bVar5) {
LAB_0042cb6e:
        if (*(char *)(iVar15 + 6) != '\0') {
          if (*(uint *)(iVar15 + 0x28) == 0xffffffff) goto LAB_0042c9c6;
          iVar9 = (*(uint *)(iVar15 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
          uVar2 = *(ushort *)(iVar9 + 2);
          uVar11 = 0;
          if (0 < (short)uVar2) {
            iVar8 = *(int *)(DAT_008603b0 + 0x34);
            puVar13 = (uint *)(iVar9 + 0x18);
            local_60 = (uint)uVar2;
            do {
              uVar7 = *puVar13;
              uVar10 = 0xffffffff;
              while (uVar4 = uVar7, uVar4 != 0xffffffff) {
                uVar10 = uVar4;
                uVar7 = *(uint *)(*(int *)(iVar8 + 8 + (uVar4 & 0xffff) * 0xc) + 0x11c);
              }
              iVar9 = *(int *)(iVar8 + 8 + (uVar10 & 0xffff) * 0xc);
              if ((*(short *)(iVar9 + 0x9c) == -1) ||
                 (sVar1 = *(short *)(iVar9 + 0x9c),
                 (*(uint *)(DAT_0087a478 + 0x18 + ((int)sVar1 >> 5) * 4) & 1 << ((byte)sVar1 & 0x1f)
                 ) == 0)) {
                iVar9 = (int)(short)uVar11;
                uVar11 = uVar11 + 1;
                local_40[iVar9] = *puVar13;
              }
              puVar13 = puVar13 + 1;
              local_60 = local_60 - 1;
            } while (local_60 != 0);
            if (uVar11 != 0) {
              if (uVar11 == uVar2) goto LAB_0042c9c6;
              if (0 < (short)uVar11) {
                local_60 = (uint)uVar11;
                puVar13 = local_40;
                do {
                  uVar10 = *puVar13;
                  actor_remove_from_unit_cluster(uVar10);
                  iVar9 = actor_new_and_attach_to_unit
                                    (1,uVar10,*(undefined4 *)(iVar15 + 0x5c),
                                     *(undefined4 *)(iVar15 + 0x34),*(undefined2 *)(iVar15 + 0x3a),0
                                     ,local_44,0,2,0,0xffffffff,0);
                  if (iVar9 == -1) {
                    iVar9 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                             (uVar10 & 0xffff) * 0xc) + 4);
                    if (iVar9 == 0) {
                      object_delete_unparented();
                    }
                    else if (iVar9 != 3) goto LAB_0042cce1;
                    object_delete_recursive(uVar10,0);
                  }
LAB_0042cce1:
                  puVar13 = puVar13 + 1;
                  local_60 = local_60 - 1;
                } while (local_60 != 0);
              }
            }
          }
        }
        *(uint *)(iVar15 + 0x30) = local_68;
        iVar8 = DAT_00880360;
        *(undefined2 *)(iVar15 + 0x38) = *(undefined2 *)(iVar15 + 0x3a);
        iVar9 = *(int *)(iVar8 + 0x34);
        iVar15 = uVar12 * 0x724;
        sVar1 = *(short *)(iVar9 + 0x46c + iVar15);
        iVar9 = iVar9 + iVar15;
        *(undefined2 *)(iVar9 + 0x3b8) = 0xffff;
        if ((sVar1 == 3) || (sVar1 == 4)) {
          *(undefined2 *)(iVar9 + 0x46c) = 0;
          *(undefined4 *)(iVar9 + 0x480) = 0xffffffff;
        }
        if (*(code **)(&DAT_00655278 + *(short *)(*(int *)(iVar8 + 0x34) + 0x6c + iVar15) * 0x38) !=
            (code *)0x0) {
          (**(code **)(&DAT_00655278 + *(short *)(*(int *)(iVar8 + 0x34) + 0x6c + iVar15) * 0x38))
                    (local_44);
        }
        squad_remove_actor(0);
        iVar9 = DAT_00880354;
        if (*(char *)(DAT_00880354 + 1) != '\0') {
          iVar15 = *(int *)(DAT_00880360 + 0x34) + iVar15;
          *(undefined4 *)(iVar15 + 0x2c) = *(undefined4 *)(DAT_00880354 + 8);
          *(uint *)(iVar9 + 8) = local_44;
          *(undefined1 *)(iVar15 + 9) = 1;
          *(ushort *)(iVar15 + 0x10) = -(ushort)(*(char *)(iVar15 + 8) != '\0') & 0x5a;
          FUN_00428650();
        }
      }
    }
  }
  goto LAB_0042c9c6;
}
#endif
