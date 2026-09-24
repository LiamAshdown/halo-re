// hs_object_detach_and_place_at_location  (Ghidra: FUN_00487f50)
// address 0x487f50, size 1568 bytes
// name confidence: 0.35 (out/phase4/hs_functions.md: "Detaches an object from its current parent
//   object using full 3D-transform math, then repositions and reorients it based on a scenario
//   location-table entry")
// rewrite confidence: 0.35 -- SEE THE DISCLAIMER BELOW.
// evidence: types/hs.h globals list (object_headers 0x008603b0 stride 0x0c data-at-+8, tag_instances
//   0x0087bc14 stride 0x20 data-at-0x14) and out/phase4/hs_types_notes.md's object field offsets
//   (0xb4 type, 0x114/0x118/0x11c sibling/child/parent -- 0x11c matches index 0x47 here exactly);
//   the location_index * 0x5c + Scenario+0x4e8 addressing matches Scenario::cutscene_flags'
//   documented stride/offset exactly, even though hs_types_notes.md's evidence table only cites
//   0x488870/0x488960 as readers of that table; src/math's already-rewritten matrix4x3_inverse,
//   matrix4x3_transform_normal and matrix4x3_multiply_procedure (0x00696664) fix that stack
//   buffer's shape as a full real_matrix4x3.
// register convention: cutscene_flags index in AX (in_AX); object index, detach flag and reorient
//   flag as the three recognized stack parameters (param_1, param_2, param_3).
//   // blam-cc: AX -> location_index, stack -> (object_index, detach_from_parent, reorient)
//
// DISCLAIMER: this function reaches deep into the (not yet type-recovered) objects/units/game
// modules -- 22 distinct callees, most still FUN_xxxxxx, and dozens of field offsets on records
// this module does not own. Only the offsets out/phase4/hs_types_notes.md or types/cache.h
// directly document are given names; everything else is left as the same raw word-indexed access
// Ghidra produced (`object[N]` on a `uint32_t *`, matching Ghidra's own `puVar2[N]`), each with a
// best-effort comment. Several callees (matrix4x3_inverse's `in`, the bare
// game_engine_compute_look_angles_from_vector()/datum_get()/unit_all_seats_unoccupied()/unit_recompute_seat_occupants()/
// unit_pick_and_ready_next_weapon() calls) had one or more arguments living in registers Ghidra's decompile dropped
// entirely (no in_/unaff_ variable at all); those are marked UNSURE at the call site with the
// best inference available from surrounding code, not a verified fact. Control flow, arithmetic
// and every write this function performs are preserved exactly as decompiled; only naming and
// call-argument reconstruction carry the reduced confidence above.
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "hs.h"

extern double fcos(double x); // 0x0062xxxx CRT (float10 in the original; narrowed to float on store)
extern double fsin(double x);

extern datum_index datum_new(data_array *array);              // memory module, 0x4d0480
extern void *datum_get(datum_index handle, data_array *array); // memory module, 0x4d0680

extern void *object_try_and_get(int32_t type_mask);           // objects module, 0x4f6ec0
extern void object_snap_to_parent_marker_and_detach(datum_index object_index);            // objects module, 0x4f6610
extern void FUN_004f51c0(datum_index object_index, void *param_2, int32_t param_3);
                                                                 // objects module, 0x4f51c0
extern int16_t object_get_node_local_transform(datum_index location_object, void *marker_or_location, void *out_buffer,
    char param_4);                                              // objects module, 0x4f6080
extern void object_reset_velocity_and_wake(datum_index object_index);             // objects module, 0x4f5160
extern uint32_t players_iterate_and_discard(datum_index object_index);         // game module, 0x474db0
extern void FUN_00475c60(uint32_t unit_or_player, uint32_t param_2, void *param_3);
                                                                 // game module, 0x475c60
extern void unit_reset_orientation_and_find_position(datum_index object_index);             // objects module, 0x55add0
extern void object_recalculate_bounding_radius_recursive(datum_index object_index);
                                                                 // objects module, 0x4f82b0
extern void object_for_each_light_attachment(datum_index object_index, int32_t param_2,
    int32_t param_3); // blam-cc: EAX -> object_index (UNSURE, not visible in the decompile);
                       // objects module, 0x4f9a20
extern char unit_update_animation_state_machine(datum_index object_index);             // units module, 0x565420
extern void unit_try_set_animation_state(uint32_t unit_index, int32_t state);
                                                                 // units module, 0x565f90
extern char unit_all_seats_unoccupied(void); // UNSURE: zero visible arguments; units module, 0x566910
extern void unit_dispatch_scripted_event_9(int32_t param_1);                      // units module, 0x56c370
extern void unit_recompute_seat_occupants(void); // UNSURE: zero visible arguments; units module, 0x56ce30
extern void unit_pick_and_ready_next_weapon(void); // UNSURE: zero visible arguments; units module, 0x56d6a0
extern void player_update_history_free_all(void *history);      // game module, 0x4e6f20
extern void game_engine_compute_look_angles_from_vector(void);
    // UNSURE: zero visible arguments; game module, 0x470d80
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in);   // math module, 0x4cb7a0
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal,
    real_matrix4x3 *m);                                                   // math module, 0x4cbec0
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b,
    real_matrix4x3 *out);                                                 // 0x00696664

extern data_array *object_headers;   // 0x008603b0, stride 0x0c, object data pointer at +0x08
extern tag_instance *tag_instances;  // 0x0087bc14
extern Scenario *global_scenario;    // 0x00746f8c
extern data_array *players;          // 0x0087a480, stride 0x200
extern uint8_t network_game_active;  // 0x00719720, DAT_00719720: nonzero in a network game

// game_time_globals: defined in types/game.h (R32 replaced hs.h's partial game_time_globals)
extern game_time_globals *game_time; // 0x006f1d6c
extern void *unknown_0071c2d8;       // 0x0071c2d8, UNSURE: some per-game(?) record; +0xf48 passed
                                      // to player_update_history_free_all when nonzero

// Detaches `object_index` from its parent object (if any), computing the exact position/rotation
// it needs so it keeps its world-space placement, then always repositions/reorients it onto
// Scenario::cutscene_flags[location_index] (used as a facing direction, and -- when `reorient` is
// set -- also as an absolute placement for a "control" object obtained via object_try_and_get).
// `detach_from_parent` gates the parent-detach half of the function; `reorient` gates the second
// half's placement writes. See the DISCLAIMER above: most object/unit field names below are the
// same raw word offsets Ghidra decompiled, not recovered struct fields.
void hs_object_detach_and_place_at_location(int16_t location_index, datum_index object_index,
    char detach_from_parent, char reorient)
{
    uint32_t *object;
    uint32_t *parent;
    uint32_t *parent_record; // re-fetched `object` after calls that may reallocate object_headers
    ScenarioCutsceneFlag *location;
    int32_t child_data_offset;
    real_vector3d facing;          // local_dc/local_d8/local_d4 (first use: world facing vector)
    real_point3d parent_position;  // local_dc/local_d8/local_d4 (second use, after detach: parent
                                    // object's own position, read via the seat/marker lookup)
    datum_index parent_handle;     // local_d0
    int32_t unit_data;             // local_cc: nonzero once resolved to a live unit's data array element
    real_vector3d delta;           // local_c8/local_c4/local_c0
    real_vector3d combined_delta;  // local_bc/local_b8/local_b4
    uint8_t seat_transform[96];    // local_78[96], out buffer for object_get_node_local_transform
    real_point3d seat_position;    // local_18/local_14/local_10, read back out of seat_transform's
                                    // tail by the caller (UNSURE of the exact sub-offset)
    real_matrix4x3 scratch_matrix; // local_b0..uStack_8c
    uint32_t player_or_unit;       // uVar10
    void *control;                 // iVar15 after object_try_and_get(3)
    real_vector3d *place_out;      // pfVar11

    object = (uint32_t *)((uint8_t *)object_headers->data + (object_index & 0xffff) * 0x0c + 8);
    if (object_index == k_datum_index_none) {
        return;
    }

    location = (ScenarioCutsceneFlag *)((uint8_t *)global_scenario->cutscene_flags.pointer +
        location_index * 0x5c);
    unit_data = 0;

    if (detach_from_parent != 0) {
        child_data_offset = (object_index & 0xffff) * 0x0c;
        object = *(uint32_t **)((uint8_t *)object_headers->data + child_data_offset + 8);
        if (object[0x47] != 0xffffffff) { // object->parent (0x11c) is valid
            control = object_try_and_get(3);
            if (control == 0) {
                object_snap_to_parent_marker_and_detach(object_index);
            } else if (network_game_active != 1) {
                parent_handle = object[0x47];
                if (parent_handle != k_datum_index_none && (int16_t)object[0xbc] != -1) {
                    // parent's object record, and this object's seat/marker index (object[0xbc])
                    parent = *(uint32_t **)((uint8_t *)object_headers->data +
                        (parent_handle & 0xffff) * 0x0c + 8);
                    parent_record = *(uint32_t **)((uint8_t *)object_headers->data +
                        child_data_offset + 8);
                    parent_record = (uint32_t *)((int16_t)((uint8_t *)parent_record)[0x1f2] +
                        (uint32_t)parent_record); // UNSURE: byte at object+0x1f2 reinterpreted as
                                                   // a signed offset applied to the object pointer
                                                   // itself; preserved exactly as decompiled
                    object_get_node_local_transform(parent_handle,
                        (uint8_t *)tag_instances[(*parent & 0xffff) & 0xffff].data + 0x2e8 + 0x24 +
                            (int16_t)object[0xbc] * 0x11c,
                        seat_transform, 1);
                    delta.i = *(float *)((uint8_t *)parent_record + 0x28) - seat_position.x;
                    delta.j = *(float *)((uint8_t *)parent_record + 0x2c) - seat_position.y;
                    child_data_offset = *(int32_t *)((uint8_t *)tag_instances[
                        (*(uint32_t *)((uint8_t *)tag_instances[(*object & 0xffff) & 0xffff].data +
                            0x34) & 0xffff) & 0xffff].data + 0xbc);
                    delta.k = *(float *)((uint8_t *)parent_record + 0x30) - seat_position.z;
                    parent_position.x = *(float *)((uint8_t *)(uint32_t)child_data_offset + 0x28);
                    parent_position.y = *(float *)((uint8_t *)(uint32_t)child_data_offset + 0x2c);
                    parent_position.z = *(float *)((uint8_t *)(uint32_t)child_data_offset + 0x30);

                    if (parent[0xc9] == object_index && ((uint8_t *)parent)[0x2a3] != '%' &&
                        object[0x47] != 0xffffffff) {
                        unit_try_set_animation_state(object[0x47], 0x25);
                    }

                    object[0xcb] = parent_handle;
                    object[0xcc] = game_time->game_time; // UNSURE: field name guessed from
                                                              // types/tags.h game_time_globals
                    if (object[0xc9] == object_index) {
                        object[0xc9] = 0xffffffff;
                    }
                    if (object[0xca] == object_index) {
                        object[0xca] = 0xffffffff;
                    }
                    object_snap_to_parent_marker_and_detach(object_index);

                    combined_delta.i = delta.i + *(float *)&object[0x17];
                    combined_delta.j = delta.j + *(float *)&object[0x18];
                    combined_delta.k = (delta.k + *(float *)&object[0x19]) - parent_position.z;

                    FUN_004f51c0(object_index, (void *)0, 0);

                    parent_record = *(uint32_t **)((uint8_t *)object_headers->data +
                        child_data_offset + 8); // NOTE: reuses child_data_offset, matching the
                                                  // decompile exactly (see UNSURE below)
                    matrix4x3_multiply_procedure(
                        (real_matrix4x3 *)((int16_t)((uint8_t *)parent_record)[0x1f2] +
                            (uint32_t)parent_record),
                        (real_matrix4x3 *)((uint8_t *)&parent_position + 0x68), &scratch_matrix);
                    object[0x1d] = ((uint32_t *)&scratch_matrix)[0];
                    object[0x1e] = ((uint32_t *)&scratch_matrix)[1];
                    object[0x1f] = ((uint32_t *)&scratch_matrix)[2];
                    object[0x20] = ((uint32_t *)&scratch_matrix)[3];
                    object[0x21] = ((uint32_t *)&scratch_matrix)[4];
                    object[0x22] = ((uint32_t *)&scratch_matrix)[5];

                    parent = *(uint32_t **)((uint8_t *)object_headers->data + child_data_offset + 8);
                    control = tag_instances[(*parent & 0xffff) & 0xffff].data;
                    if (*(int32_t *)((uint8_t *)control + 0x34) != -1) {
                        if ((parent[4] & 1) != 0) {
                            object_for_each_light_attachment(object_index, 0, 1);
                        }
                        if (*(int32_t *)((uint8_t *)control + 0x34) != -1) {
                            parent[4] = parent[4] & 0xfffffffe;
                            *((uint8_t *)object_headers->data + child_data_offset + 2) |= 2;
                        }
                    }
                    *(int16_t *)&object[0xbc] = -1;
                    ((uint8_t *)object)[0x2a7] = 2;
                    if (parent[0xc9] == object_index) {
                        parent[0xc9] = 0xffffffff;
                    }
                    if (parent[0xca] == object_index) {
                        parent[0xca] = 0xffffffff;
                    }
                    unit_recompute_seat_occupants();
                    unit_pick_and_ready_next_weapon();
                    unit_update_animation_state_machine(object_index);
                    place_out = (real_vector3d *)((int16_t)((uint8_t *)object)[0x1ea] + 0x10 +
                        (uint32_t)object);
                    *place_out = *(real_vector3d *)&parent_position; // stored as (dc, d8, d4) triple
                    if ((int16_t)object[0x2d] == 0) { // object->type (0xb4) == biped
                        unit_reset_orientation_and_find_position(object_index);
                    }
                    object_recalculate_bounding_radius_recursive(object_index);
                    if (unit_all_seats_unoccupied() == 1) {
                        control = object_try_and_get(2);
                        if (control != 0) {
                            *(uint32_t *)((uint8_t *)control + 0x5ac) = game_time->game_time;
                        }
                    }
                    if (network_game_active == 1) {
                        control = datum_get(k_datum_index_none, players);
                        // UNSURE: the handle argument (EDX) is not visible anywhere in the
                        // decompile; `players` (ESI) is confirmed by an otherwise-dead
                        // `iVar9 = DAT_0087a480` load immediately before this call. This is the
                        // one call in the function whose semantics could not be recovered.
                        if (control != 0 && *(int16_t *)((uint8_t *)control + 2) == -1) {
                            *(uint32_t *)((uint8_t *)control + 0x180) = 0;
                            *(uint32_t *)((uint8_t *)control + 0x17c) = 0;
                            *(uint32_t *)((uint8_t *)control + 0x1e0) = 0;
                            *(uint32_t *)((uint8_t *)control + 0x1dc) = 0;
                        }
                    }
                }
                if (object[1] == 0) {
                    unit_dispatch_scripted_event_9(1);
                }
                if (network_game_active == 1) {
                    player_or_unit = object[0x86];
                    if (player_or_unit != 0xffffffff) {
                        int16_t slot = (int16_t)player_or_unit;
                        if (slot >= 0 && slot < players->maximum_count) {
                            int16_t *element = (int16_t *)((uint8_t *)players->data +
                                slot * players->size);
                            int16_t identifier = *element;
                            if (identifier != 0) {
                                int16_t salt = (int16_t)(player_or_unit >> 0x10);
                                if ((salt == 0 || identifier == salt) &&
                                    *(int16_t *)((uint8_t *)element + 2) != -1 &&
                                    unknown_0071c2d8 != 0) {
                                    player_update_history_free_all(
                                        *(void **)((uint8_t *)unknown_0071c2d8 + 0xf48));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    facing.k = (float)fsin((double)location->facing.pitch);
    {
        double cos_pitch = fcos((double)location->facing.pitch);
        double cos_yaw = fcos((double)location->facing.yaw);
        double sin_yaw = fsin((double)location->facing.yaw);
        facing.i = (float)(cos_yaw * cos_pitch);
        facing.j = (float)(sin_yaw * cos_pitch);
    }
    // UNSURE: `location->facing.pitch`/`.j` above stand in for the two source
    // floats read at cutscene_flag+0x30/+0x34 (yaw, pitch); the actual ScenarioCutsceneFlag
    // field names may not match this module's use.

    object_reset_velocity_and_wake(object_index);
    control = object_try_and_get(3);
    if (control == 0) {
        if (reorient != 0 && unit_data == 0) {
            place_out = &facing;
            goto place;
        }
    } else {
        player_or_unit = players_iterate_and_discard(object_index);
        if (*(int32_t *)((uint8_t *)control + 0x11c) == -1) {
            delta = facing;
        } else {
            matrix4x3_inverse(&scratch_matrix, (real_matrix4x3 *)control);
            // UNSURE: `control` reinterpreted directly as a real_matrix4x3* for the `in` argument
            // is a guess -- the true offset of a matrix inside whatever object_try_and_get(3)
            // returns could not be recovered.
            matrix4x3_transform_normal(&delta, &facing, &scratch_matrix);
        }
        if (reorient != 0) {
            *(float *)((uint8_t *)control + 0x224) = delta.i;
            *(float *)((uint8_t *)control + 0x228) = delta.j;
            *(float *)((uint8_t *)control + 0x22c) = delta.k;
            *(float *)((uint8_t *)control + 0x230) = delta.i;
            *(float *)((uint8_t *)control + 0x254) = delta.i;
            *(float *)((uint8_t *)control + 0x234) = delta.j;
            *(float *)((uint8_t *)control + 600)   = delta.j;
            *(float *)((uint8_t *)control + 0x238) = delta.k;
            *(float *)((uint8_t *)control + 0x25c) = delta.k;
        }
        if (player_or_unit == 0xffffffff) {
            if (reorient != 0 && unit_data == 0) {
                place_out = &facing;
                goto place;
            }
            return;
        }
        unit_data = (player_or_unit & 0xffff) * 0x200 + (uint32_t)players->data;
        if (detach_from_parent != 0) {
            FUN_00475c60(player_or_unit, 0xffffffff,
                (uint8_t *)location + 0x24);
        }
        if (reorient != 0) {
            if (*(int16_t *)((uint8_t *)unit_data + 2) != -1) {
                game_engine_compute_look_angles_from_vector();
                // UNSURE: zero visible arguments; presumably (unit_data, &facing) or similar.
            }
            if (reorient != 0 && unit_data == 0) {
                place_out = &facing;
                goto place;
            }
            FUN_004f51c0(object_index, (void *)0, 0);
            return;
        }
        if (reorient != 0 && unit_data == 0) {
            place_out = &facing;
            goto place;
        }
        FUN_004f51c0(object_index, (void *)0, 0);
        return;
    }
    place_out = (real_vector3d *)0;
place:
    FUN_004f51c0(object_index, place_out, 0);
}

#if 0
Original Ghidra decompilation (0x487f50):

void FUN_00487f50(uint param_1,char param_2,char param_3)

{
  byte *pbVar1;
  uint *puVar2;
  uint *puVar3;
  uint *puVar4;
  float fVar5;
  float fVar6;
  char cVar7;
  short in_AX;
  int iVar8;
  int iVar9;
  uint uVar10;
  float *pfVar11;
  short sVar12;
  short sVar13;
  int iVar14;
  int iVar15;
  float10 fVar16;
  float10 fVar17;
  float local_dc;
  float local_d8;
  float local_d4;
  uint local_d0;
  int local_cc;
  float local_c8;
  float local_c4;
  float local_c0;
  float local_bc;
  float local_b8;
  float local_b4;
  undefined1 local_b0 [4];
  uint uStack_ac;
  uint uStack_a8;
  uint uStack_a4;
  uint uStack_94;
  uint uStack_90;
  uint uStack_8c;
  undefined1 local_78 [96];
  float local_18;
  float local_14;
  float local_10;
  iVar15 = DAT_008603b0;
  if (param_1 == 0xffffffff) {
    return;
  }
  iVar8 = in_AX * 0x5c + *(int *)(DAT_00746f8c + 0x4e8);
  local_cc = 0;
  if (param_2 != '\0') {
    iVar14 = (param_1 & 0xffff) * 0xc;
    puVar2 = *(uint **)(iVar14 + 8 + *(int *)(DAT_008603b0 + 0x34));
    if (puVar2[0x47] != 0xffffffff) {
      iVar9 = object_try_and_get(3);
      if (iVar9 == 0) {
        FUN_004f6610(param_1);
      }
      else if (DAT_00719720 != 1) {
        local_d0 = puVar2[0x47];
        iVar9 = DAT_0087a480;
        if ((local_d0 != 0xffffffff) && ((short)puVar2[0xbc] != -1)) {
          puVar3 = *(uint **)(*(int *)(iVar15 + 0x34) + 8 + (local_d0 & 0xffff) * 0xc);
          iVar15 = *(int *)(*(int *)(iVar15 + 0x34) + 8 + iVar14);
          iVar15 = *(short *)(iVar15 + 0x1f2) + iVar15;
          FUN_004f6080(local_d0,*(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                        0x2e8) + 0x24 + (short)puVar2[0xbc] * 0x11c,local_78,1);
          local_c8 = *(float *)(iVar15 + 0x28) - local_18;
          local_c4 = *(float *)(iVar15 + 0x2c) - local_14;
          iVar9 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 +
                                                       DAT_0087bc14) + 0x34) & 0xffff) * 0x20 + 0x14
                                   + DAT_0087bc14) + 0xbc);
          local_c0 = *(float *)(iVar15 + 0x30) - local_10;
          local_dc = *(float *)(iVar9 + 0x28);
          local_d8 = *(float *)(iVar9 + 0x2c);
          local_d4 = *(float *)(iVar9 + 0x30);
          if ((puVar3[0xc9] == param_1) &&
             ((*(char *)((int)puVar3 + 0x2a3) != '%' && (puVar2[0x47] != 0xffffffff)))) {
            unit_try_set_animation_state(puVar2[0x47],0x25);
          }
          iVar15 = DAT_006f1d6c;
          puVar2[0xcb] = local_d0;
          puVar2[0xcc] = *(uint *)(iVar15 + 0xc);
          if (puVar2[0xc9] == param_1) {
            puVar2[0xc9] = 0xffffffff;
          }
          if (puVar2[0xca] == param_1) {
            puVar2[0xca] = 0xffffffff;
          }
          FUN_004f6610(param_1);
          local_bc = local_c8 + (float)puVar2[0x17];
          local_b8 = local_c4 + (float)puVar2[0x18];
          local_b4 = (local_c0 + (float)puVar2[0x19]) - local_d4;
          FUN_004f51c0(param_1,0,0);
          iVar15 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar14);
          (*(code *)PTR_matrix4x3_multiply_00696664)
                    (*(short *)(iVar15 + 0x1f2) + iVar15,iVar9 + 0x68,local_b0);
          puVar2[0x1d] = uStack_ac;
          puVar2[0x1e] = uStack_a8;
          puVar2[0x1f] = uStack_a4;
          puVar2[0x20] = uStack_94;
          puVar2[0x21] = uStack_90;
          iVar15 = DAT_008603b0;
          puVar2[0x22] = uStack_8c;
          puVar4 = *(uint **)(*(int *)(iVar15 + 0x34) + 8 + iVar14);
          iVar15 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          if (*(int *)(iVar15 + 0x34) != -1) {
            if ((puVar4[4] & 1) != 0) {
              object_for_each_light_attachment(0,1);
            }
            if (*(int *)(iVar15 + 0x34) != -1) {
              iVar15 = *(int *)(DAT_008603b0 + 0x34);
              puVar4[4] = puVar4[4] & 0xfffffffe;
              pbVar1 = (byte *)(iVar15 + iVar14 + 2);
              *pbVar1 = *pbVar1 | 2;
            }
          }
          *(undefined2 *)(puVar2 + 0xbc) = 0xffff;
          *(undefined1 *)((int)puVar2 + 0x2a7) = 2;
          if (puVar3[0xc9] == param_1) {
            puVar3[0xc9] = 0xffffffff;
          }
          if (puVar3[0xca] == param_1) {
            puVar3[0xca] = 0xffffffff;
          }
          FUN_0056ce30();
          FUN_0056d6a0();
          FUN_00565420(param_1);
          pfVar11 = (float *)(*(short *)((int)puVar2 + 0x1ea) + 0x10 + (int)puVar2);
          *pfVar11 = local_dc;
          pfVar11[1] = local_d8;
          pfVar11[2] = local_d4;
          if ((short)puVar2[0x2d] == 0) {
            FUN_0055add0(param_1);
          }
          object_recalculate_bounding_radius_recursive(param_1);
          cVar7 = FUN_00566910();
          if ((cVar7 == '\x01') && (iVar15 = object_try_and_get(2), iVar15 != 0)) {
            *(undefined4 *)(iVar15 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
          }
          iVar9 = DAT_0087a480;
          if (((DAT_00719720 == 1) && (iVar15 = datum_get(), iVar15 != 0)) &&
             (*(short *)(iVar15 + 2) == -1)) {
            *(undefined4 *)(iVar15 + 0x180) = 0;
            *(undefined4 *)(iVar15 + 0x17c) = 0;
            *(undefined4 *)(iVar15 + 0x1e0) = 0;
            *(undefined4 *)(iVar15 + 0x1dc) = 0;
          }
        }
        if (puVar2[1] == 0) {
          FUN_0056c370(1);
          iVar9 = DAT_0087a480;
        }
        if (((DAT_00719720 == 1) && (uVar10 = puVar2[0x86], uVar10 != 0xffffffff)) &&
           ((sVar13 = (short)uVar10, -1 < sVar13 && (sVar13 < *(short *)(iVar9 + 0x20))))) {
          iVar15 = (int)*(short *)(iVar9 + 0x22) * (int)sVar13;
          sVar13 = *(short *)(iVar15 + *(int *)(iVar9 + 0x34));
          if (((sVar13 != 0) &&
              ((sVar12 = (short)(uVar10 >> 0x10), sVar12 == 0 || (sVar13 == sVar12)))) &&
             ((*(short *)(iVar15 + *(int *)(iVar9 + 0x34) + 2) != -1 && (DAT_0071c2d8 != 0)))) {
            player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
          }
        }
      }
    }
  }
  fVar16 = (float10)fcos((float10)*(float *)(iVar8 + 0x34));
  fVar17 = (float10)fcos((float10)*(float *)(iVar8 + 0x30));
  local_dc = (float)(fVar17 * fVar16);
  fVar17 = (float10)fsin((float10)*(float *)(iVar8 + 0x30));
  local_d8 = (float)(fVar17 * fVar16);
  fVar16 = (float10)fsin((float10)*(float *)(iVar8 + 0x34));
  local_d4 = (float)fVar16;
  FUN_004f5160(param_1);
  iVar15 = object_try_and_get(3);
  if (iVar15 == 0) {
LAB_00488529:
    if ((param_3 != '\0') && (local_cc == 0)) {
      pfVar11 = &local_dc;
      goto LAB_00488540;
    }
  }
  else {
    uVar10 = FUN_00474db0(param_1);
    fVar6 = local_d8;
    fVar5 = local_dc;
    if (*(int *)(iVar15 + 0x11c) == -1) {
      local_c8 = local_dc;
      local_c4 = local_d8;
      local_c0 = local_d4;
    }
    else {
      matrix4x3_inverse();
      matrix4x3_transform_normal(local_b0);
    }
    if (param_3 != '\0') {
      *(float *)(iVar15 + 0x224) = fVar5;
      *(float *)(iVar15 + 0x228) = fVar6;
      *(float *)(iVar15 + 0x22c) = local_d4;
      *(float *)(iVar15 + 0x230) = fVar5;
      *(float *)(iVar15 + 0x254) = fVar5;
      *(float *)(iVar15 + 0x234) = fVar6;
      *(float *)(iVar15 + 600) = fVar6;
      *(float *)(iVar15 + 0x238) = local_d4;
      *(float *)(iVar15 + 0x25c) = local_d4;
    }
    if (uVar10 == 0xffffffff) goto LAB_00488529;
    local_cc = (uVar10 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
    if (param_2 != '\0') {
      FUN_00475c60(uVar10,0xffffffff,iVar8 + 0x24);
    }
    if (param_3 != '\0') {
      if (*(short *)(local_cc + 2) != -1) {
        game_engine_compute_look_angles_from_vector();
      }
      goto LAB_00488529;
    }
  }
  pfVar11 = (float *)0x0;
LAB_00488540:
  FUN_004f51c0(param_1,pfVar11,0);
  return;
}
#endif
