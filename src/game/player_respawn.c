// player_respawn  (Ghidra: player_respawn, already named)
// address 0x477ea0, size 1010 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// REWRITTEN (from objdump 0x477ea0..0x478291; the draft passed the player index to observer_new, which takes
//   EDX = &observers[local player] (0x006ac65c, stride 0x29c), and dropped several register arguments).
//   Player (0x200 bytes): +0x02 local player index, +0x20 team, +0x24 interaction object, +0x28 interaction
//   type (word), +0x34 unit, +0x68 cleared at the end, +0xae deaths.
//   Single player (no game engine) with a local player: the local player's unit slot (0x0087a478 +0x8) is
//   cleared. A dead unit (+0x106 bit 2) is deleted and a fresh one spawned; a living one (a revert) is marked
//   pending delete together with its held weapon (+0x2f8[+0x2f2]), each with its light attachments dropped
//   (header flag bit 0 cleared, datum byte +2 bit 1 set) when its tag has a light (+0x34 != -1), and the
//   local player keeps controlling it (ESI unit, DI local index) without a new spawn.
//   Otherwise, as a local game or network server (0x00719720 == 0 or 2): a random starting location and the
//   globals player unit tag (+0x174 -> +0xc; the multiplayer unit +0x168 -> +0x1c under a game engine) place
//   a new unit facing (cos, sin, 0) of the location's facing with the global up vector and the player's
//   colour; it is tied to the player (+0xc0 owner, +0xb8 team, +0x218 controlling player), refreshed with CL 1,
//   the look state initialised for a local player, and in single player the starting profile applied (1 when
//   there are several profiles and the player has died, else 0). A network server also sends its grenade
//   counts, a unit update and its weapon loadout.
// blam-cc: EAX -> player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "camera.h"
#include "networking.h"

extern data_array *player_data;                     // 0x0087a480
extern data_array *object_data;                     // 0x008603b0
extern uint8_t *local_player_globals;               // 0x0087a478
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern int16_t network_game_mode;                   // 0x00719720
extern Globals *global_globals;
extern Scenario *global_scenario;
extern const real_vector3d *global_up3d_pointer;    // 0x00696720
extern uint8_t network_message_scratch;          // 0x00871de0
extern tag_instance *tag_instances;                 // 0x0087bc14
extern network_server_globals *network_server; // 0x0071c2d4
extern observer observers[];                        // 0x006ac65c, stride 0x29c

extern void object_mark_pending_delete(uint32_t object_index); // 0x4f50f0, blam-cc: EAX
extern void object_delete(uint32_t object_index); // 0x4f5bd0, blam-cc: EAX
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
    int32_t invoke_callback); // 0x4f9a20, blam-cc: EAX, stack
extern void local_player_set_controlled_unit(datum_index new_unit, int16_t local_player_index); // 0x474fc0, ESI, DI
extern int16_t player_pick_random_starting_location(datum_index player_handle); // 0x4776d0, stack
extern ScenarioPlayerStartingLocation *game_get_player_starting_location(int16_t index); // 0x477640, CX
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
    datum_index role); // 0x4f53a0, blam-cc: EAX, stack
extern real *game_engine_get_player_color(uint32_t player_index, real *out_rgb); // 0x463290, EAX, ESI
extern void object_placement_data_set_change_colors(real *color, object_placement_data *placement); // 0x477670, EAX, ECX
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern void unit_refresh_targeting_flag_and_weapons(uint32_t unit_index, uint8_t initial_targeting_flag); // 0x569bf0, stack, CL
extern void game_engine_init_player_look_state_from_object(datum_index unit, int16_t local_player_index); // 0x470e80, EDX, AX
extern void unit_apply_starting_profile(int16_t starting_profile_index, datum_index unit_handle,
    uint8_t reset_stats); // 0x473c50, EAX, ECX, stack
extern void game_engine_apply_player_grenade_counts(uint32_t player_index); // 0x4613c0, EAX
extern void object_type_override_call_0x68(uint32_t object_index); // 0x4f4560, ESI
// RESOLVED 2026-09-28: unit_build_network_update now takes (unit, buffer, 0x7ff8) and returns the encoded bit
// count, as the binary pushes and tests (the buffer is the network scratch 0x871de0, named below after another use).
extern int32_t unit_build_network_update(uint32_t object_index, int32_t buffer, int32_t bit_budget); // 0x55aed0
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, network_server_globals *server,
    int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused); // 0x4e1a80, EAX, ECX
extern void game_engine_send_unit_weapon_loadout(uint32_t unit_index, datum_index player_handle,
    int32_t value, int32_t machine_index); // 0x477a80, EAX, stack
extern void observer_new(observer *this); // 0x447740, blam-cc: EDX

extern double cos(double x);
extern double sin(double x);

// Drops a pending-delete object's light attachments, as both halves of the revert branch do.
static void player_respawn_drop_lights(datum_index object_index)
{
    uint8_t *header = (uint8_t *)object_data->data + (object_index & 0xffff) * 0xc;
    uint8_t *obj = *(uint8_t **)(header + 8);
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;

    if (*(int32_t *)(tag + 0x34) == -1) {
        return;
    }
    if (*(uint8_t *)(obj + 0x10) & 1) {
        object_for_each_light_attachment(object_index, 0, 1);
    }
    if (*(int32_t *)(tag + 0x34) != -1) {
        *(uint32_t *)(obj + 0x10) &= ~1u;
        header = (uint8_t *)object_data->data + (object_index & 0xffff) * 0xc; // reloaded at 0x478085
        header[2] |= 2;
    }
}

void player_respawn(uint32_t player_index)
{
    uint8_t *p = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;

    if (current_game_engine == 0 && *(int16_t *)(p + 2) != -1) {
        datum_index *slot = (datum_index *)(local_player_globals + 8 + *(int16_t *)(p + 2) * 4);
        datum_index existing_unit = *slot;

        *slot = k_datum_index_none;
        if (existing_unit != k_datum_index_none) {
            uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + (existing_unit & 0xffff) * 0xc + 8);

            if ((unit[0x106] & 4) == 0) {
                datum_index held_weapon = k_datum_index_none;
                int16_t weapon_index = *(int16_t *)(unit + 0x2f2);

                if (weapon_index != -1) {
                    held_weapon = *(datum_index *)(unit + 0x2f8 + weapon_index * 4);
                }
                object_mark_pending_delete(existing_unit);
                player_respawn_drop_lights(existing_unit);
                local_player_set_controlled_unit(existing_unit, *(int16_t *)(p + 2));
                if (held_weapon != k_datum_index_none) {
                    player_respawn_drop_lights(held_weapon);
                }
                goto reset_player_state;
            }
            object_delete(existing_unit);
        }
    }

    if (network_game_mode == 2 || network_game_mode == 0) {
        int16_t location_index = player_pick_random_starting_location(player_index);
        datum_index unit_tag;
        ScenarioPlayerStartingLocation *location;
        object_placement_data placement;
        real color_buffer[3];
        real color[3];
        real *player_color;
        real facing;
        datum_index new_unit;
        uint8_t *unit;

        if (location_index == -1) {
            goto reset_player_state;
        }
        unit_tag = *(datum_index *)((uint8_t *)global_globals->player_information.pointer + 0xc);
        if (unit_tag == k_datum_index_none) {
            goto reset_player_state;
        }
        location = game_get_player_starting_location(location_index);
        if (current_game_engine != 0) {
            unit_tag = *(datum_index *)((uint8_t *)global_globals->multiplayer_information.pointer + 0x1c);
        }
        object_placement_data_initialize(&placement, unit_tag, k_datum_index_none);
        placement.position = *(real_point3d *)location;
        facing = *(real *)((uint8_t *)location + 0xc);
        placement.forward.i = (real)cos(facing);
        placement.forward.j = (real)sin(facing);
        placement.forward.k = 0.0f;
        placement.up = *global_up3d_pointer;
        player_color = game_engine_get_player_color(player_index, color_buffer);
        color[0] = player_color[0];
        color[1] = player_color[1];
        color[2] = player_color[2];
        object_placement_data_set_change_colors(color, &placement);

        new_unit = object_new_with_datum_role_control(&placement, 3);
        if (new_unit == k_datum_index_none) {
            goto reset_player_state;
        }
        unit = (uint8_t *)object_try_and_get(new_unit, 3);
        if (unit == 0) {
            goto reset_player_state;
        }
        p = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
        *(uint32_t *)(unit + 0xc0) = player_index;
        *(int16_t *)(unit + 0xb8) = *(int16_t *)(p + 0x20);
        *(uint32_t *)(unit + 0x218) = player_index;
        *(datum_index *)(p + 0x34) = new_unit;
        unit_refresh_targeting_flag_and_weapons(new_unit, 1);
        if (*(int16_t *)(p + 2) != -1) {
            game_engine_init_player_look_state_from_object(new_unit, *(int16_t *)(p + 2));
        }
        if (current_game_engine == 0) {
            int32_t profile_count = *(int32_t *)&global_scenario->player_starting_profile.count;

            if (profile_count > 1 && *(int16_t *)(p + 0xae) > 0) {
                unit_apply_starting_profile(1, *(datum_index *)(p + 0x34), 1);
            } else if (profile_count != 0) {
                unit_apply_starting_profile(0, *(datum_index *)(p + 0x34), 1);
            }
        }
        if (network_game_mode == 2) {
            int32_t team = *(int32_t *)(p + 0x20);
            int32_t encoded_bits;

            game_engine_apply_player_grenade_counts(player_index);
            *(uint32_t *)(unit + 4) = 0;
            object_type_override_call_0x68(new_unit);
            encoded_bits = unit_build_network_update(new_unit, (int32_t)&network_message_scratch, 0x7ff8);
            if (encoded_bits > 0) {
                network_session_broadcast_to_flagged(encoded_bits, (network_server_globals *)network_server,
                    1, &network_message_scratch, 1, 0, 0, 3);
            }
            *(uint32_t *)(p + 0x68) = 0;
            game_engine_send_unit_weapon_loadout(new_unit, player_index, team, -1);
        }
    }

reset_player_state:
    p = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    *(uint32_t *)(p + 0x68) = 0;
    *(uint16_t *)(p + 0x28) = 0;
    *(datum_index *)(p + 0x24) = k_datum_index_none;
    if (*(int16_t *)(p + 2) != -1) {
        observer_new(&observers[*(int16_t *)(p + 2)]);
    }
}

#if 0
Original Ghidra decompilation (0x477ea0), from tools/pack.py 0x477ea0:

void player_respawn(void)

{
  byte *pbVar1;
  uint uVar2;
  uint *puVar3;
  short sVar4;
  uint in_EAX;
  int iVar5;
  undefined4 *puVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  float10 fVar11;
  uint local_b0;
  undefined1 local_9c [24];
  undefined4 local_84;
  undefined4 local_80;
  undefined4 local_7c;
  float local_68;
  float local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;

  iVar5 = (in_EAX & 0xffff) * 0x200;
  iVar8 = *(int *)(DAT_0087a480 + 0x34) + iVar5;
  if ((DAT_006f1d20 == 0) && (*(short *)(iVar8 + 2) != -1)) {
    uVar2 = *(uint *)(DAT_0087a478 + 8 + *(short *)(iVar8 + 2) * 4);
    *(undefined4 *)(DAT_0087a478 + 8 + *(short *)(iVar8 + 2) * 4) = 0xffffffff;
    if (uVar2 != 0xffffffff) {
      iVar9 = (uVar2 & 0xffff) * 0xc;
      if ((*(byte *)(*(int *)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x106) & 4) == 0) {
        iVar7 = *(int *)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34));
        sVar4 = *(short *)(iVar7 + 0x2f2);
        local_b0 = 0xffffffff;
        if (sVar4 != -1) {
          local_b0 = *(uint *)(iVar7 + 0x2f8 + sVar4 * 4);
        }
        object_mark_pending_delete();
        puVar3 = *(uint **)(iVar9 + 8 + *(int *)(DAT_008603b0 + 0x34));
        iVar7 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if (*(int *)(iVar7 + 0x34) != -1) {
          if ((puVar3[4] & 1) != 0) {
            object_for_each_light_attachment(0,1);
          }
          if (*(int *)(iVar7 + 0x34) != -1) {
            iVar7 = *(int *)(DAT_008603b0 + 0x34);
            puVar3[4] = puVar3[4] & 0xfffffffe;
            pbVar1 = (byte *)(iVar7 + iVar9 + 2);
            *pbVar1 = *pbVar1 | 2;
          }
        }
        local_player_set_controlled_unit();
        if (local_b0 != 0xffffffff) {
          iVar7 = (local_b0 & 0xffff) * 0xc;
          puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
          iVar9 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if (*(int *)(iVar9 + 0x34) != -1) {
            if ((puVar3[4] & 1) != 0) {
              object_for_each_light_attachment(0,1);
            }
            if (*(int *)(iVar9 + 0x34) != -1) {
              iVar9 = *(int *)(DAT_008603b0 + 0x34);
              puVar3[4] = puVar3[4] & 0xfffffffe;
              pbVar1 = (byte *)(iVar9 + iVar7 + 2);
              *pbVar1 = *pbVar1 | 2;
            }
          }
        }
        goto LAB_0047824b;
      }
      object_delete();
    }
  }
  if ((((DAT_00719720 == 2) || (DAT_00719720 == 0)) && (sVar4 = FUN_004776d0(), sVar4 != -1)) &&
     (iVar9 = *(int *)(*(int *)(DAT_00746fa0 + 0x174) + 0xc), iVar9 != -1)) {
    puVar6 = (undefined4 *)FUN_00477640();
    if (DAT_006f1d20 != 0) {
      iVar9 = *(int *)(*(int *)(DAT_00746fa0 + 0x168) + 0x1c);
    }
    object_placement_data_initialize(iVar9,0xffffffff);
    local_84 = *puVar6;
    local_80 = puVar6[1];
    local_7c = puVar6[2];
    fVar11 = (float10)fcos((float10)(float)puVar6[3]);
    local_60 = 0;
    local_68 = (float)fVar11;
    fVar11 = (float10)fsin((float10)(float)puVar6[3]);
    local_64 = (float)fVar11;
    local_5c = *(undefined4 *)PTR_DAT_00696720;
    local_58 = *(undefined4 *)(PTR_DAT_00696720 + 4);
    local_54 = *(undefined4 *)(PTR_DAT_00696720 + 8);
    game_engine_get_player_color();
    FUN_00477670();
    iVar9 = object_new_with_datum_role_control(local_9c,3);
    if ((iVar9 != -1) && (iVar7 = object_try_and_get(3), iVar7 != 0)) {
      iVar10 = *(int *)(DAT_0087a480 + 0x34) + iVar5;
      *(uint *)(iVar7 + 0xc0) = in_EAX;
      *(undefined2 *)(iVar7 + 0xb8) = *(undefined2 *)(iVar10 + 0x20);
      *(uint *)(iVar7 + 0x218) = in_EAX;
      *(int *)(iVar10 + 0x34) = iVar9;
      FUN_00569bf0(iVar9);
      if (*(short *)(iVar10 + 2) != -1) {
        game_engine_init_player_look_state_from_object();
      }
      if ((DAT_006f1d20 == 0) &&
         (((1 < *(int *)(global_scenario + 0x348) && (0 < *(short *)(iVar10 + 0xae))) ||
          (*(int *)(global_scenario + 0x348) != 0)))) {
        FUN_00473c50(1);
      }
      if (DAT_00719720 == 2) {
        game_engine_apply_player_grenade_counts();
        *(undefined4 *)(iVar7 + 4) = 0;
        object_type_override_call_0x68();
        iVar9 = FUN_0055aed0(iVar9,&DAT_00871de0,0x7ff8);
        if (0 < iVar9) {
          FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
        }
        *(undefined4 *)(iVar10 + 0x68) = 0;
        FUN_00477a80();
      }
    }
  }
LAB_0047824b:
  iVar9 = DAT_0087a480;
  *(undefined4 *)(iVar8 + 0x68) = 0;
  iVar5 = *(int *)(iVar9 + 0x34) + iVar5;
  *(undefined2 *)(iVar5 + 0x28) = 0;
  *(undefined4 *)(iVar5 + 0x24) = 0xffffffff;
  if (*(short *)(iVar8 + 2) != -1) {
    FUN_00447740();
  }
  return;
}
#endif
