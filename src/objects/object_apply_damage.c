// object_apply_damage
// address 0x4ee5e0, size 2939 bytes
// name confidence: 0.75 (Ghidra-recovered name; out/phase4/objects_types_notes.md calls out
// "chimera__apply_damage 0x4ee5e0 ... is genuinely object_apply_damage")
// rewrite confidence: 0.8
// REWRITTEN from objdump 0x4ee5e0..0x4ef15a. Stack: (damage_data, target, node, region, material, hit plane).
//   The responsible player is dropped when stale. The amount is the damage effect block's (tag +0x1c4) random
//   range (+0x10..+0x14, one LCG step of random_seed_global) blended with its lower bound (+0x0c) by the random
//   blend, times the multiplier; a responsible unit's actor (+0x1f8, else +0x1f4; via +0x328 when set) scales it
//   by perception (0x42aa90); multiplayer applies the game engine scale between the two controlling players,
//   single player the difficulty damage multiplier (0x46fe10) unless the damaging team is a friend of team 1.
//   The damaged list is the target and its parents (only the target for flags 1 or 4) plus the target's damage
//   owner (+0xf0). Without flag 1, a vehicle target first passes rider_damage_fraction (tag +0x184) of the damage
//   (split between player riders in multiplayer) to its seated bipeds by recursion -- AI bipeds only when
//   driving (flag 0x20) -- then the multiplier is reset to 1. Every unit in the list that belongs to a player
//   gets the screen effects (player_effect_mark_damage_direction locally, player_effect_send_network_update to
//   remote players; unowned units only with 0x87abc5, for the first local player). With a positive amount the
//   list is walked from the last entry down: each object with a collision geometry (tag +0x7c) resolves
//   friendly-fire rules (multiplayer 0x6f1cbc / 0x6f1cf4), its region from the node (+0x28c, +0x32), notify
//   flags 0x20 (difficulty scaled) / 0x10 (same or friendly team), its material (the hit material only for the
//   target, else the geometry's indirect material +0x04, else 0x6b8c68), instant kills (flag 4, 0x87abc7 or side
//   effect 2 on a sleeping unit facing away), shields (object_apply_shield_damage) and -- for the target or
//   parents whose geometry passes damage on (flag 2, the target geometry's flag 4 clear) -- the body
//   (object_apply_body_damage, which ends the walk). The first object hurt reports its shield material and
//   vitality into damage_data +0x4c / +0x48; the responsible player is told (0x4ee3c0), a biped hit on the shield
//   flags +0x122, and object_damage_notify_and_impulse runs for every object; notify flag 4 deletes it. The walk
//   stops once the amount is used up.
// blam-cc: stack=(dd, target_object_index, node_index, region_index, material_index, plane)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include <stdint.h>
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern data_array *player_data;     // 0x0087a480
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t *team_pair_data;         // 0x006b0b84, +0xa4: the ten-team friend bitfield (team_a * 10 + team_b)
extern game_main_globals *main_game_globals; // 0x006b0b80
extern uint8_t g_0087abc5;          // 0x0087abc5, cheat: screen effects for unowned units too
extern uint8_t g_0087abc7;          // 0x0087abc7, cheat: player damage kills outright
extern ModelCollisionGeometryMaterial default_collision_material; // 0x006b8c68
extern int16_t network_game_mode;   // 0x00719720, 0 local, 1 client, 2 host
extern uint8_t game_engine_teams_enabled_flag;          // 0x006f1cbc, multiplayer friendly-fire rules enabled
extern uint8_t g_006f1cf4;          // 0x006f1cf4, multiplayer friendly-fire mode
extern player_globals *local_player_globals; // 0x0087a478

extern uint8_t actor_apply_perception_scale(datum_index actor_index, const uint8_t *zone, float *in_out_value); // 0x42aa90, EAX, stack, EDX
extern void player_effect_send_network_update(datum_index player_handle, const real_vector3d *direction,
    const damage_data *dd, float random_blend, float damage_amount); // 0x456bc0, EAX, EBX, stack
extern void player_effect_mark_damage_direction(datum_index player_index, const damage_data *dd,
    const real_vector3d *direction, float random_blend, float damage_amount); // 0x456cf0, EAX, stack
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX
extern float game_engine_compute_time_scale(int32_t param_a, int32_t param_b); // 0x461550, EDX, ESI
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification); // 0x46fe10, stack, CX
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0, stack
extern void object_set_health_frozen_flag(uint32_t object_index); // 0x4eda20, EAX
extern int32_t object_get_controlling_player_index(datum_index object_index); // 0x4ee2e0, EAX
extern void object_notify_pickup_or_refresh_probe(uint32_t object_index, datum_index player_index); // 0x4ee3c0, stack, EDI
extern void object_apply_body_damage(uint32_t target_index, int32_t region_index, int32_t node_index, void *plane,
    uint8_t *geometry, uint8_t *material, uint8_t *effect_block, damage_data *dd, uint32_t *notify_flags,
    float *body_damage_out, float *material_multiplier_out, float damage, uint8_t is_local); // 0x4ef2a0
extern void object_apply_shield_damage(uint32_t target_index, uint8_t *geometry, uint8_t *material,
    uint8_t *effect_block, uint32_t *notify_flags, float *shield_damage_out, float *remaining_damage,
    uint8_t is_local, uint8_t apply_state, object_shield_impulse_result *record); // 0x4ef820, EBX record
extern void object_damage_notify_and_impulse(uint32_t target_index, damage_data *dd, uint32_t notify_flags,
    float shield_damage, float body_damage, uint32_t unused_6, int32_t region_index, uint32_t is_local); // 0x4efcf0
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0, EDI
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern uint8_t unit_point_in_front_and_asleep(real_point3d *world_point, uint32_t unit_index); // 0x56bc80, EAX, EDI

static uint8_t *object_get(datum_index object_index)
{
    return (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
}

static uint8_t *tag_get(datum_index tag_index)
{
    return (uint8_t *)tag_instances[tag_index & 0xffff].data;
}

static uint8_t teams_are_friends(int32_t bit)
{
    return (uint8_t)((*(uint32_t *)(team_pair_data + 0xa4 + (bit >> 5) * 4) >> (bit & 0x1f)) & 1);
}

// the inline datum_try_and_get on the player array: the record, or 0 when stale
static player *player_try_get(datum_index player_index)
{
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    int16_t identifier;
    uint8_t *record;

    if (player_index == k_datum_index_none || index < 0 || index >= player_data->maximum_count) {
        return 0;
    }
    record = (uint8_t *)player_data->data + player_data->size * index;
    identifier = *(int16_t *)record;
    if (identifier == 0 || (salt != 0 && identifier != salt)) {
        return 0;
    }
    return (player *)record;
}

void object_apply_damage(damage_data *dd, uint32_t target_object_index, int16_t hit_node_index, int16_t hit_region_index,
    int16_t hit_material_index, uint32_t hit_plane)
{
    datum_index target_index = target_object_index;
    int16_t node_index = hit_node_index;
    int16_t region_index = hit_region_index;
    int16_t material_index = hit_material_index;
    uint8_t *target = object_get(target_index);
    uint8_t *effect_block = tag_get(dd->damage_effect_tag) + 0x1c4;
    int32_t target_role = ((struct object *)target)->network_role;
    uint8_t target_is_local = (target_role == 0 || target_role == 3);
    uint8_t difficulty_scaled = 0;
    uint8_t parents_take_damage = 1;
    uint8_t no_random_range;
    uint8_t reported = 0;
    datum_index list[17];
    int16_t count = 0;
    int16_t remaining;
    uint32_t flags;
    uint8_t *target_tag;
    float amount;

    if (dd->responsible_player != k_datum_index_none && player_try_get(dd->responsible_player) == 0) {
        dd->responsible_player = k_datum_index_none;
    }
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    amount = ((*(float *)(effect_block + 0x14) - *(float *)(effect_block + 0x10)) *
              ((float)(int32_t)(random_seed_global >> 16) * 1.5259022e-05f) + *(float *)(effect_block + 0x10)) *
             dd->random_blend + (1.0f - dd->random_blend) * *(float *)(effect_block + 0x0c);
    amount = amount * dd->multiplier;
    if (dd->responsible_object != k_datum_index_none) {
        uint8_t *responsible = (uint8_t *)object_try_and_get(dd->responsible_object, 3);

        if (responsible != 0) {
            datum_index actor;

            if (*(datum_index *)(responsible + 0x328) != k_datum_index_none) {
                responsible = object_get(*(datum_index *)(responsible + 0x328));
            }
            actor = ((unit_object *)responsible)->unit.swarm_actor_index; // unit+0x1f8 (was cast through struct player)
            if (actor == k_datum_index_none) {
                actor = ((unit_object *)responsible)->unit.actor_index;   // unit+0x1f4
            }
            if (actor != k_datum_index_none) {
                actor_apply_perception_scale(actor, (const uint8_t *)dd, &amount);
            }
        }
    }
    if (current_game_engine != 0) {
        int32_t target_player = object_get_controlling_player_index(target_index);
        int32_t responsible_player = object_get_controlling_player_index(dd->responsible_object);

        amount = game_engine_compute_time_scale(responsible_player, target_player) * amount;
    } else if (dd->team_index != -1) {
        int16_t team = dd->team_index;

        if (team < 0 || team >= 10 || !teams_are_friends(team * 10 + 1)) {
            amount = weapon_get_zoom_fov(0, main_game_globals->difficulty) * amount;
            difficulty_scaled = 1;
        }
    }

    flags = dd->flags;
    if ((flags & 1) || (flags & 4)) {
        list[0] = target_index;
        count = 1;
    } else if (target_index != k_datum_index_none) {
        datum_index id = target_index;

        do {
            list[count++] = id;
            id = *(datum_index *)(object_get(id) + 0x11c);
        } while (id != k_datum_index_none);
    }
    target_tag = tag_get(*(datum_index *)target);
    if (*(datum_index *)(target_tag + 0x7c) != k_datum_index_none) {
        parents_take_damage = (uint8_t)(~(*(uint32_t *)tag_get(*(datum_index *)(target_tag + 0x7c)) >> 4) & 1);
    }
    if (((struct object *)target)->damage_owner != k_datum_index_none) {
        list[count++] = ((struct object *)target)->damage_owner;
    }

    // a vehicle hands rider_damage_fraction of the damage to the bipeds seated in it
    if ((flags & 1) == 0 && ((struct object *)target)->type == 1) {
        float rider_fraction = (1.0f - *(float *)(effect_block + 0x18)) * *(float *)(target_tag + 0x184);
        datum_index child;

        dd->multiplier = rider_fraction;
        if (current_game_engine != 0 && ((struct object *)target)->first_child_object != k_datum_index_none) {
            int32_t players = 0;

            for (child = ((struct object *)target)->first_child_object; child != k_datum_index_none;
                 child = *(datum_index *)(object_get(child) + 0x114)) {
                uint8_t *rider = object_get(child);

                if (((struct object *)rider)->type == 0 && *(datum_index *)(rider + 0x218) != k_datum_index_none) {
                    players++;
                }
            }
            if (players != 0) {
                dd->multiplier = rider_fraction / (float)players;
            }
        }
        for (child = ((struct object *)target)->first_child_object; child != k_datum_index_none;
             child = *(datum_index *)(object_get(child) + 0x114)) {
            uint8_t *rider = object_get(child);

            if (((struct object *)rider)->type != 0) {
                continue;
            }
            if (*(datum_index *)(rider + 0x218) == k_datum_index_none) {
                if (child != *(datum_index *)(target + 0x324)) {
                    continue;
                }
                dd->flags |= 0x20;
            } else {
                dd->flags &= ~0x20u;
            }
            object_apply_damage(dd, child, -1, -1, -1, 0);
            dd->flags &= ~0x20u;
        }
        dd->multiplier = 1.0f;
    }

    // screen effects for the players whose units are hurt
    no_random_range = (*(float *)(effect_block + 0x10) == 0.0f && *(float *)(effect_block + 0x14) == 0.0f);
    {
        int16_t i;

        for (i = 0; i < count; i++) {
            datum_index id = list[i];
            int16_t index = (int16_t)id;
            int16_t salt = (int16_t)(id >> 16);
            uint8_t *header;
            uint8_t *unit;
            datum_index player_index;

            if (id == k_datum_index_none || index < 0 || index >= object_data->maximum_count) {
                continue;
            }
            header = (uint8_t *)object_data->data + object_data->size * index;
            if (*(int16_t *)header == 0 || (salt != 0 && *(int16_t *)header != salt)) {
                continue;
            }
            if (((1u << (header[3] & 0x1f)) & 3) == 0) {
                continue;
            }
            unit = *(uint8_t **)(header + 0x8);
            if (unit == 0) {
                continue;
            }
            player_index = ((unit_object *)unit)->unit.controlling_player;
            if (player_index != k_datum_index_none) {
                switch (network_game_mode) {
                case 0:
                    player_effect_mark_damage_direction(player_index, dd, &dd->direction, dd->random_blend, amount);
                    break;
                case 1:
                    if (no_random_range == 1) {
                        player_effect_mark_damage_direction(player_index, dd, &dd->direction, dd->random_blend,
                            amount);
                    }
                    break;
                case 2:
                    if (no_random_range) {
                        player_effect_mark_damage_direction(player_index, dd, &dd->direction, dd->random_blend,
                            amount);
                    } else {
                        player_effect_send_network_update(player_index, &dd->direction, dd, dd->random_blend,
                            amount);
                    }
                    break;
                }
            } else if (g_0087abc5) {
                datum_index first_local = local_player_globals->local_players[0];

                if (network_game_mode == 0) {
                    player_effect_mark_damage_direction(first_local, dd, &dd->direction, dd->random_blend, amount);
                } else if (network_game_mode == 2) {
                    player_effect_send_network_update(first_local, &dd->direction, dd, dd->random_blend, amount);
                }
            }
        }
    }

    if (!(amount > 0.0f)) {
        return;
    }
    remaining = count;
    for (;;) {
        int16_t before = remaining;
        int16_t i;
        datum_index id;
        uint8_t *obj;
        uint8_t *object_tag;
        uint8_t *geometry;
        uint8_t *material;
        uint32_t notify_flags = 0;
        float shield_damage = 0.0f;
        float body_damage = 0.0f;
        float material_multiplier = 0.0f;
        int32_t region = -1;
        object_shield_impulse_result record;
        uint8_t kill;
        uint8_t friendly = 0;
        uint8_t shield_allowed = 1;
        uint8_t body_allowed = 1;
        uint8_t apply_state;

        remaining--;
        if (before <= 0) {
            break;
        }
        i = remaining;
        id = list[i];
        obj = object_get(id);
        object_tag = tag_get(*(datum_index *)obj);
        if (*(datum_index *)(object_tag + 0x7c) == k_datum_index_none) {
            goto notify;
        }
        geometry = tag_get(*(datum_index *)(object_tag + 0x7c));
        kill = (uint8_t)((dd->flags >> 2) & 1);
        *(datum_index *)record.unknown_00 = id;
        record.shield_damage_dealt = 0.0f;
        record.depleted_this_call = 0;
        if (((object *)obj)->network_role == 3 || ((object *)obj)->network_role == 0) {
            apply_state = 1;
        } else {
            player *responsible = player_try_get(dd->responsible_player);

            apply_state = (responsible != 0 && responsible->local_player_index != -1) ? 0 : 1;
        }
        if (current_game_engine != 0 && game_engine_teams_enabled_flag) {
            datum_index owner = player_index_from_unit_index(id);

            if (owner != k_datum_index_none && owner != dd->responsible_player) {
                player *owner_record = player_try_get(owner);

                if (owner_record != 0) {
                    friendly = (uint8_t)(teams_are_enemies(dd->team_index,
                        *(int16_t *)&((struct player *)owner_record)->team) == 0);
                    if (friendly) {
                        switch (g_006f1cf4) {
                        case 0:
                            shield_allowed = 0;
                            body_allowed = 0;
                            break;
                        case 2:
                            shield_allowed = 1;
                            body_allowed = 0;
                            break;
                        case 3:
                            if ((*(uint8_t *)(effect_block + 0x4) & 0x20) == 0) {
                                shield_allowed = 0;
                                body_allowed = 0;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (node_index >= 0 && node_index < *(int32_t *)(geometry + 0x28c)) {
            *(int16_t *)&region = *(int16_t *)(*(uint8_t **)(geometry + 0x290) + node_index * 0x40 + 0x32);
        }
        if (difficulty_scaled) {
            notify_flags = 0x20;
        }
        if (dd->team_index != -1) {
            int16_t team = ((object *)obj)->owner_team;

            if (current_game_engine != 0) {
                if (team == dd->team_index) {
                    notify_flags |= 0x10;
                }
            } else if (team >= 0 && team < 10 && dd->team_index >= 0 && dd->team_index < 10) {
                if (teams_are_friends(team * 10 + dd->team_index)) {
                    notify_flags |= 0x10;
                }
            }
        }
        if (i == 0 && material_index >= 0 && material_index < *(int32_t *)(geometry + 0x234)) {
            material = *(uint8_t **)(geometry + 0x238) + material_index * 0x48;
        } else if (*(int16_t *)(geometry + 0x4) >= 0 && *(int16_t *)(geometry + 0x4) < *(int32_t *)(geometry + 0x234)) {
            material = *(uint8_t **)(geometry + 0x238) + *(int16_t *)(geometry + 0x4) * 0x48;
        } else {
            material = (uint8_t *)&default_collision_material;
        }
        dd->material_type = *(int16_t *)(material + 0x24);
        if (g_0087abc7 && dd->responsible_player != k_datum_index_none) {
            kill = 1;
        }
        if (*(int16_t *)effect_block == 2 && unit_point_in_front_and_asleep(&dd->origin, id) &&
            (obj[0x107] & 8) == 0) {
            kill = 1;
        }
        if (target_is_local == 1 && kill && (obj[0x106] & 4) == 0 && (!friendly || body_allowed)) {
            ((object *)obj)->body_vitality = 0.0f;
            object_set_health_frozen_flag(id);
            notify_flags |= 0x41;
        }
        if ((dd->flags & 0x20) == 0 && (*(uint32_t *)(effect_block + 0x4) & 0x200) == 0 &&
            ((object *)obj)->maximum_shield_vitality > 0.0f && (!friendly || shield_allowed) && (i == 0 || (*geometry & 1))) {
            object_apply_shield_damage(id, geometry, material, effect_block, &notify_flags, &shield_damage, &amount,
                target_is_local, apply_state, &record);
        }
        if ((i == 0 || (parents_take_damage && (*geometry & 2))) &&
            (*(uint8_t *)(effect_block + 0x4) & 0x40) == 0) {
            if (((*geometry & 0x20) && (*(uint8_t *)(effect_block + 0x4) & 0x20) == 0) ||
                (friendly && !body_allowed)) {
                amount = 0.0f;
            }
            object_apply_body_damage(id, (i == 0) ? region_index : -1, (i == 0) ? node_index : -1,
                (void *)(uintptr_t)((i == 0) ? hit_plane : 0), geometry, material, effect_block, dd, &notify_flags,
                &body_damage, &material_multiplier, amount, target_is_local);
            remaining = 0;
        }
        if (!reported && (shield_damage > 0.0001f || body_damage > 0.0001f)) {
            if (shield_damage > body_damage) {
                dd->material_type = *(int16_t *)(geometry + 0xd2);
                dd->unknown_48 = *(uint32_t *)&((object *)obj)->shield_vitality;
            } else {
                float vitality = ((object *)obj)->body_vitality;

                if (vitality < 0.0f) {
                    vitality = 0.0f;
                } else if (vitality > 1.0f) {
                    vitality = 1.0f;
                }
                *(float *)&dd->unknown_48 = vitality;
            }
            reported = 1;
        }
        object_notify_pickup_or_refresh_probe(id, dd->responsible_player);
        if (shield_damage > 0.0f && ((object *)obj)->type == 0) {
            obj[0x122] = 1;
        }
notify:
        object_damage_notify_and_impulse(id, dd, notify_flags, shield_damage, body_damage,
            *(uint32_t *)&material_multiplier, region, target_is_local);
        if (notify_flags & 4) {
            int32_t role = *(int32_t *)(object_get(id) + 0x4);

            if (role == 0) {
                object_delete_unparented(id);
                object_delete_recursive(id, 0);
            } else if (role == 3) {
                object_delete_recursive(id, 0);
            }
        }
        if (!(amount > 0.0f)) {
            break;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ee5e0):

void object_apply_damage(uint *param_1,uint param_2,short param_3,short param_4,short param_5,
                        uint param_6)

{
  uint *puVar1;
  bool bVar2;
  bool bVar3;
  float fVar4;
  byte *pbVar5;
  uint uVar6;
  char cVar7;
  short sVar8;
  int iVar9;
  uint uVar10;
  short *psVar11;
  short sVar12;
  uint uVar13;
  int iVar14;
  byte bVar15;
  ushort uVar16;
  int iVar17;
  undefined *puVar18;
  short *psVar19;
  bool bVar20;
  float10 fVar21;
  uint auStackY_20044 [32731];
  char local_91;
  byte local_8d;
  float local_8c;
  uint *local_88;
  char local_81;
  int local_80;
  uint local_7c;
  uint local_78;
  byte *local_74;
  uint *local_70;
  short *local_6c;
  uint local_68;
  uint local_64;
  float local_60;
  int local_5c;
  uint local_58;
  uint local_54;
  undefined4 local_50;
  uint local_4c;
  int local_48;
  uint local_44 [17];
  
  iVar17 = DAT_008603b0;
  local_5c = (param_2 & 0xffff) * 0xc;
  iVar14 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_5c) + 4);
  if ((iVar14 == 0) || (local_64 = local_64 & 0xffffff00, iVar14 == 3)) {
    local_64 = CONCAT31(local_64._1_3_,1);
  }
  uVar13 = param_1[2];
  if ((uVar13 != 0xffffffff) &&
     ((((sVar8 = (short)uVar13, sVar8 < 0 || (*(short *)(DAT_0087a480 + 0x20) <= sVar8)) ||
       (sVar8 = *(short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar8 +
                          *(int *)(DAT_0087a480 + 0x34)), sVar8 == 0)) ||
      ((sVar12 = (short)(uVar13 >> 0x10), sVar12 != 0 && (sVar8 != sVar12)))))) {
    param_1[2] = 0xffffffff;
  }
  iVar14 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  psVar19 = (short *)(iVar14 + 0x1c4);
  DAT_00719cd0 = DAT_00719cd0 * 0x19660d + 0x3c6ef35f;
  local_58 = DAT_00719cd0 >> 0x10;
  local_81 = '\0';
  local_8d = 1;
  local_8c = ((1.0 - (float)param_1[0x10]) * *(float *)(iVar14 + 0x1d0) +
             ((float)local_58 * 1.5259022e-05 *
              (*(float *)(iVar14 + 0x1d8) - *(float *)(iVar14 + 0x1d4)) + *(float *)(iVar14 + 0x1d4)
             ) * (float)param_1[0x10]) * (float)param_1[0x11];
  local_6c = psVar19;
  if ((param_1[3] != 0xffffffff) && (iVar9 = object_try_and_get(3), iVar9 != 0)) {
    if (*(uint *)(iVar9 + 0x328) != 0xffffffff) {
      iVar9 = *(int *)(*(int *)(iVar17 + 0x34) + 8 + (*(uint *)(iVar9 + 0x328) & 0xffff) * 0xc);
    }
    iVar17 = *(int *)(iVar9 + 0x1f8);
    if (iVar17 == -1) {
      iVar17 = *(int *)(iVar9 + 500);
    }
    if (iVar17 != -1) {
      FUN_0042aa90(param_1);
    }
  }
  if (DAT_006f1d20 == 0) {
    sVar8 = (short)param_1[4];
    if ((sVar8 == -1) ||
       (((-1 < sVar8 && (sVar8 < 10)) &&
        (iVar17 = sVar8 * 10 + 1,
        (1 << ((byte)iVar17 & 0x1f) & *(uint *)(DAT_006b0b84 + 0xa4 + (iVar17 >> 5) * 4)) != 0))))
    goto LAB_004ee7d4;
    fVar21 = (float10)FUN_0046fe10(0);
    local_81 = '\x01';
  }
  else {
    object_get_controlling_player_index();
    object_get_controlling_player_index();
    fVar21 = (float10)FUN_00461550();
  }
  local_8c = (float)(fVar21 * (float10)local_8c);
LAB_004ee7d4:
  iVar17 = 0;
  uVar13 = param_1[1] & 1;
  local_80 = 0;
  bVar3 = false;
  if ((uVar13 == 0) && ((param_1[1] & 4) == 0)) {
    if (param_2 != 0xffffffff) {
      iVar9 = *(int *)(DAT_008603b0 + 0x34);
      do {
        local_44[(short)iVar17] = param_2;
        param_2 = *(uint *)(*(int *)(iVar9 + 8 + (param_2 & 0xffff) * 0xc) + 0x11c);
        iVar17 = iVar17 + 1;
        local_80 = iVar17;
      } while (param_2 != 0xffffffff);
    }
  }
  else {
    local_44[0] = param_2;
    local_80 = 1;
  }
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_5c);
  uVar10 = *(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x7c);
  if (uVar10 != 0xffffffff) {
    local_8d = ~(byte)(**(uint **)((uVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) >> 4) & 1;
  }
  if (puVar1[0x3c] != 0xffffffff) {
    sVar8 = (short)local_80;
    local_80 = local_80 + 1;
    local_44[sVar8] = puVar1[0x3c];
  }
  uVar16 = (ushort)local_80;
  if ((uVar13 == 0) && ((short)puVar1[0x2d] == 1)) {
    local_88 = (uint *)((1.0 - *(float *)(iVar14 + 0x1dc)) *
                       *(float *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x184)
                       );
    bVar20 = DAT_006f1d20 != 0;
    param_1[0x11] = (uint)local_88;
    if (bVar20) {
      uVar13 = puVar1[0x46];
      iVar14 = 0;
      local_7c = 0;
      if (uVar13 != 0xffffffff) {
        do {
          iVar17 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar13 & 0xffff) * 0xc);
          if ((*(short *)(iVar17 + 0xb4) == 0) && (*(int *)(iVar17 + 0x218) != -1)) {
            iVar14 = iVar14 + 1;
          }
          uVar13 = *(uint *)(iVar17 + 0x114);
        } while (uVar13 != 0xffffffff);
        local_7c = iVar14;
        if (iVar14 != 0) {
          param_1[0x11] = (uint)((float)local_88 / (float)iVar14);
        }
      }
    }
    uVar13 = puVar1[0x46];
    psVar19 = local_6c;
    while (local_6c = psVar19, uVar13 != 0xffffffff) {
      iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar13 & 0xffff) * 0xc);
      if (*(short *)(iVar14 + 0xb4) == 0) {
        if (*(int *)(iVar14 + 0x218) == -1) {
          if (uVar13 != puVar1[0xc9]) goto LAB_004ee9ab;
          uVar10 = param_1[1] | 0x20;
        }
        else {
          uVar10 = param_1[1] & 0xffffffdf;
        }
        param_1[1] = uVar10;
        object_apply_damage(param_1,uVar13,0xffffffff,0xffffffff,0xffffffff,0);
        param_1[1] = param_1[1] & 0xffffffdf;
      }
LAB_004ee9ab:
      psVar19 = local_6c;
      uVar13 = *(uint *)(iVar14 + 0x114);
    }
    uVar16 = (ushort)local_80;
    param_1[0x11] = 0x3f800000;
  }
  fVar4 = local_8c;
  if ((*(float *)(psVar19 + 8) != 0.0) || (bVar20 = true, *(float *)(psVar19 + 10) != 0.0)) {
    bVar20 = false;
  }
  iVar14 = DAT_0087a480;
  uVar13 = local_7c;
  if (0 < (short)uVar16) {
    local_7c = (uint)uVar16;
    local_88 = local_44;
    do {
      uVar13 = *local_88;
      psVar19 = (short *)0x0;
      if (((uVar13 != 0xffffffff) && (sVar8 = (short)uVar13, -1 < sVar8)) &&
         (sVar8 < *(short *)(DAT_008603b0 + 0x20))) {
        psVar11 = (short *)((int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar8 +
                           *(int *)(DAT_008603b0 + 0x34));
        sVar8 = *psVar11;
        if ((sVar8 != 0) && ((sVar12 = (short)(uVar13 >> 0x10), sVar12 == 0 || (sVar8 == sVar12))))
        {
          psVar19 = psVar11;
        }
      }
      if (((psVar19 != (short *)0x0) && ((1 << (*(byte *)((int)psVar19 + 3) & 0x1f) & 3U) != 0)) &&
         (*(int *)(psVar19 + 4) != 0)) {
        if (*(int *)(*(int *)(psVar19 + 4) + 0x218) == -1) {
          if (DAT_0087abc5 != '\0') {
            if (DAT_00719720 == 0) {
              uVar13 = param_1[0x10];
              goto LAB_004eeb0e;
            }
            if (DAT_00719720 == 2) {
              FUN_00456bc0(param_1,param_1[0x10],fVar4);
            }
          }
        }
        else if (DAT_00719720 == 0) {
LAB_004eeabe:
          uVar13 = param_1[0x10];
LAB_004eeb0e:
          FUN_00456cf0(param_1,param_1 + 0xd,uVar13,fVar4);
        }
        else if (DAT_00719720 == 1) {
          if (bVar20) goto LAB_004eeabe;
        }
        else if (DAT_00719720 == 2) {
          if (bVar20) goto LAB_004eeabe;
          FUN_00456bc0(param_1,param_1[0x10]);
        }
      }
      local_88 = local_88 + 1;
      local_7c = local_7c - 1;
      iVar14 = DAT_0087a480;
      uVar13 = 0;
    } while (local_7c != 0);
  }
joined_r0x004eeb40:
  do {
    local_7c = uVar13;
    if ((local_8c <= 0.0) || (sVar8 = (short)local_80, local_80 = local_80 + -1, sVar8 < 1)) {
      DAT_0087a480 = iVar14;
      return;
    }
    local_68 = local_44[(short)local_80];
    local_48 = (local_68 & 0xffff) * 0xc;
    local_70 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_48);
    uVar13 = *(uint *)(*(int *)((*local_70 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x7c);
    local_60 = 0.0;
    local_88 = (uint *)0x0;
    local_58 = 0;
    local_78 = 0;
    local_5c = 0xffffffff;
    DAT_0087a480 = iVar14;
    if (uVar13 != 0xffffffff) {
      local_74 = *(byte **)((uVar13 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      bVar15 = (byte)(param_1[1] >> 2) & 1;
      local_91 = '\0';
      bVar2 = true;
      bVar20 = true;
      local_50 = 0;
      local_4c = local_4c & 0xffffff00;
      local_7c._1_3_ = (uint3)(local_7c >> 8);
      if ((local_70[1] == 3) || (local_70[1] == 0)) {
        local_7c = CONCAT31(local_7c._1_3_,1);
      }
      else {
        uVar13 = param_1[2];
        local_7c = CONCAT31(local_7c._1_3_,1);
        if (((uVar13 != 0xffffffff) && (sVar8 = (short)uVar13, -1 < sVar8)) &&
           (sVar8 < *(short *)(iVar14 + 0x20))) {
          psVar19 = (short *)((int)*(short *)(iVar14 + 0x22) * (int)sVar8 + *(int *)(iVar14 + 0x34))
          ;
          sVar8 = *psVar19;
          if (((sVar8 != 0) &&
              ((sVar12 = (short)(uVar13 >> 0x10), sVar12 == 0 || (sVar8 == sVar12)))) &&
             (psVar19[1] != -1)) {
            local_7c = (uint)local_7c._1_3_ << 8;
          }
        }
      }
      local_54 = local_68;
      if ((((DAT_006f1d20 != 0) && (DAT_006f1cbc != '\0')) &&
          ((uVar13 = FUN_00474db0(local_68), uVar13 != 0xffffffff &&
           ((((uVar13 != param_1[2] && (sVar8 = (short)uVar13, -1 < sVar8)) &&
             (sVar8 < *(short *)(iVar14 + 0x20))) &&
            (sVar8 = *(short *)((int)*(short *)(iVar14 + 0x22) * (int)sVar8 +
                               *(int *)(iVar14 + 0x34)), sVar8 != 0)))))) &&
         ((sVar12 = (short)(uVar13 >> 0x10), sVar12 == 0 || (sVar8 == sVar12)))) {
        cVar7 = FUN_0045bd50();
        local_91 = '\x01' - (cVar7 != '\0');
        if (local_91 != '\0') {
          if (DAT_006f1cf4 == '\0') {
LAB_004eed0b:
            bVar2 = false;
          }
          else {
            if (DAT_006f1cf4 != '\x02') {
              if ((DAT_006f1cf4 != '\x03') || ((*(byte *)(local_6c + 2) & 0x20) != 0))
              goto LAB_004eed15;
              goto LAB_004eed0b;
            }
            bVar2 = true;
          }
          bVar20 = false;
        }
      }
LAB_004eed15:
      if ((-1 < param_3) && ((int)param_3 < *(int *)(local_74 + 0x28c))) {
        local_5c = CONCAT22(local_5c._2_2_,
                            *(undefined2 *)(param_3 * 0x40 + 0x32 + *(int *)(local_74 + 0x290)));
      }
      if (local_81 != '\0') {
        local_78 = 0x20;
      }
      sVar8 = (short)param_1[4];
      if (sVar8 != -1) {
        sVar12 = (short)local_70[0x2e];
        if (DAT_006f1d20 == 0) {
          if ((((sVar12 < 0) || (9 < sVar12)) || (sVar8 < 0)) || (9 < sVar8)) goto LAB_004eedd1;
          iVar14 = (int)sVar8 + sVar12 * 10;
          cVar7 = '\x01' - ((1 << ((byte)iVar14 & 0x1f) &
                            *(uint *)(DAT_006b0b84 + 0xa4 + (iVar14 >> 5) * 4)) != 0);
        }
        else {
          cVar7 = sVar12 != sVar8;
        }
        if (cVar7 == '\0') {
          local_78 = local_78 | 0x10;
        }
      }
LAB_004eedd1:
      if ((((short)local_80 == 0) && (-1 < param_5)) && ((int)param_5 < *(int *)(local_74 + 0x234)))
      {
        puVar18 = (undefined *)(*(int *)(local_74 + 0x238) + param_5 * 0x48);
      }
      else {
        sVar8 = *(short *)(local_74 + 4);
        if ((sVar8 < 0) || (*(int *)(local_74 + 0x234) <= (int)sVar8)) {
          puVar18 = &DAT_006b8c68;
        }
        else {
          puVar18 = (undefined *)(*(int *)(local_74 + 0x238) + sVar8 * 0x48);
        }
      }
      *(undefined2 *)(param_1 + 0x13) = *(undefined2 *)(puVar18 + 0x24);
      if ((DAT_0087abc7 != '\0') && (param_1[2] != 0xffffffff)) {
        bVar15 = 1;
      }
      if (((*local_6c == 2) && (cVar7 = FUN_0056bc80(), cVar7 != '\0')) &&
         ((*(byte *)((int)local_70 + 0x107) & 8) == 0)) {
        bVar15 = 1;
      }
      puVar1 = local_70;
      if ((((char)local_64 == '\x01') && (bVar15 != 0)) &&
         (((*(byte *)((int)local_70 + 0x106) & 4) == 0 && ((local_91 == '\0' || (bVar20)))))) {
        local_70[0x38] = 0;
        FUN_004eda20();
        local_78 = local_78 | 0x41;
      }
      pbVar5 = local_74;
      if ((((((param_1[1] & 0x20) == 0) && ((*(uint *)(local_6c + 2) & 0x200) == 0)) &&
           (0.0 < (float)puVar1[0x37])) && ((local_91 == '\0' || (bVar2)))) &&
         (((short)local_80 == 0 || ((*local_74 & 1) != 0)))) {
        object_apply_shield_damage
                  (local_68,local_74,puVar18,local_6c,&local_78,&local_60,&local_8c,local_64,
                   local_7c);
      }
      sVar8 = (short)local_80;
      if (((sVar8 == 0) || ((local_8d != 0 && ((*pbVar5 & 2) != 0)))) &&
         ((*(uint *)(local_6c + 2) & 0x40) == 0)) {
        if ((((*pbVar5 & 0x20) != 0) && ((*(uint *)(local_6c + 2) & 0x20) == 0)) ||
           ((local_91 != '\0' && (!bVar20)))) {
          local_8c = 0.0;
        }
        if (sVar8 == 0) {
          iVar14 = (int)param_3;
          iVar17 = (int)param_4;
        }
        else {
          iVar14 = -1;
          iVar17 = -1;
        }
        object_apply_body_damage
                  (local_68,iVar17,iVar14,(sVar8 != 0) - 1 & param_6,pbVar5,puVar18,local_6c,param_1
                   ,&local_78,&local_88,&local_58,local_8c,local_64);
        local_80 = 0;
      }
      if ((!bVar3) && ((0.0001 < local_60 || (0.0001 < (float)local_88)))) {
        if (local_60 <= (float)local_88) {
          if (0.0 <= (float)local_70[0x38]) {
            if ((float)local_70[0x38] <= 1.0) {
              uVar13 = local_70[0x38];
            }
            else {
              uVar13 = 0x3f800000;
            }
          }
          else {
            uVar13 = 0;
          }
          param_1[0x12] = uVar13;
        }
        else {
          *(undefined2 *)(param_1 + 0x13) = *(undefined2 *)(pbVar5 + 0xd2);
          param_1[0x12] = local_70[0x39];
        }
        bVar3 = true;
      }
      FUN_004ee3c0(local_54,local_50,local_4c);
      if ((0.0 < local_60) && ((short)local_70[0x2d] == 0)) {
        *(undefined1 *)((int)local_70 + 0x122) = 1;
      }
    }
    uVar6 = local_68;
    uVar10 = local_78;
    object_damage_notify_and_impulse
              (local_68,param_1,local_78,local_60,local_88,local_58,local_5c,local_64);
    iVar14 = DAT_0087a480;
    uVar13 = local_7c;
  } while ((uVar10 & 4) == 0);
  iVar17 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_48) + 4);
  if (iVar17 == 0) {
    FUN_004f5aa0();
  }
  else if (iVar17 != 3) goto joined_r0x004eeb40;
  FUN_004f59d0(uVar6,0);
  iVar14 = DAT_0087a480;
  uVar13 = local_7c;
  goto joined_r0x004eeb40;
}

#endif
