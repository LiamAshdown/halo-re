// hud_find_nearby_teammate_for_nameplate  (Ghidra: FUN_0045e340; named per
// out/phase4/game_functions.md)
// address 0x45e340, size 474 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Searches nearby units for the closest one within the
// player's aim cone and short range, used to pick a HUD nameplate target"); types/game.h player
// (local_player_index +0x02, unit +0x34, unknown_7c "read by the nameplate HUD (0x45e520)").
// register convention: player handle is this function's one explicit stack parameter.
// RESOLVED (phase 4 review): Ghidra's "local_player_index * 0x40 + DAT_006b145c + 0x38/0x3c" IS
// player_control_globals->local_players[local_player_index].nameplate_target / _weight -- the
// compiler folded the 0x10-byte header into the field offset (0x10 + 0x28 == 0x38,
// 0x10 + 0x2c == 0x3c). game_engine_init_player_look_state_from_object (0x470e80) writes the
// same slot: "puVar1 = in_AX * 0x40 + 0x10 + DAT_006b145c" then "puVar1[10] = 0xffffffff",
// i.e. record +0x28 seeded to -1, with +0x2c left 0.0 by the zeroing loop. Both fields are now
// named in types/game.h local_player_control.
// UNSURE: game_engine_compute_local_player_look_vector, object_collect_local_player_relevant_objects, camera_observer_target_direction and players_iterate_and_discard are outside this batch's
// assigned range and are called here exactly as Ghidra shows them, including the elided/implicit
// arguments noted inline.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"

extern data_array *player_data;                          // 0x0087a480
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern data_array *object_headers;                        // 0x008603b0

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void unit_get_camera_position(datum_index unit_index, real_point3d *out); // 0x568f80,
    // blam-cc: ECX -> unit_index, EDI -> out (matches src/units/unit_get_camera_position.c)
extern void game_engine_compute_local_player_look_vector(void);                           // 0x471f40, not in this batch
extern int object_collect_local_player_relevant_objects(void *callback, datum_index *player_handle, int32_t max_count,
    datum_index *out_candidates); // 0x4fa1a0, not in this batch; UNSURE exact signature
extern char camera_observer_target_direction(datum_index candidate, datum_index reference_unit, void *out1,
    void *out2, float *out_angle); // 0x459cc0, not in this batch
extern datum_index players_iterate_and_discard(datum_index object_or_unit); // 0x474db0, not in this batch

// Ghidra could not resolve the callback address used by object_collect_local_player_relevant_objects; kept as an opaque symbol.
extern void LAB_0045e2e0(void);

// Finds the nearest visible teammate within a short cone/range of `player`'s aim, preferring the
// player's own last-frame local look-target when it is still valid.
datum_index hud_find_nearby_teammate_for_nameplate(datum_index player_handle)
{
    player *p;
    local_player_control *track;
    real_point3d camera;
    datum_index candidates[32];
    int candidate_count;
    int i;
    datum_index best;
    object *obj;

    p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    best = (datum_index)0xffffffff;

    if (p->local_player_index != -1) {
        track = &player_control_globals_ptr->local_players[p->local_player_index];
        if (0.0f < track->nameplate_weight) {
            best = track->nameplate_target;
            obj = object_try_and_get(best, 0xffffffff); // UNSURE: type_mask 0xffffffff (_object_mask_all)
            if (obj == 0) {
                best = (datum_index)0xffffffff;
            }
            if (best != (datum_index)0xffffffff) {
                return players_iterate_and_discard(best);
            }
        }
    }

    // objdump 0x45e3bc..0x45e3c3: ECX = p->unit, EDI = &camera. Both arguments are registers
    // Ghidra drops; the out-parameter reading is confirmed by src/units/unit_get_camera_position.c.
    unit_get_camera_position(p->unit, &camera);
    game_engine_compute_local_player_look_vector();
    candidate_count = object_collect_local_player_relevant_objects(&LAB_0045e2e0, &player_handle, 0x20, candidates);

    for (i = 0; i < candidate_count; i++) {
        object *candidate_obj = ((object_header *)object_headers->data)[candidates[i] & 0xffff].data;
        float dx = candidate_obj->position.x - camera.x; // UNSURE camera fields (see note above)
        float dy = candidate_obj->position.y - camera.y;
        float dz = candidate_obj->position.z - camera.z;
        float dist2 = dy * dy + dx * dx + dz * dz; // order preserved from Ghidra

        void *unused1;
        void *unused2;
        float angle;

        if ((*(float *)((uint8_t *)candidate_obj + 0x37c) < 1.0f ||
             p->unknown_7c == players_iterate_and_discard(candidates[i])) &&
            camera_observer_target_direction(candidates[i], p->unit, &unused1, &unused2, &angle) != 0 &&
            (angle < 0.0f ? -angle : angle) < 0.13083334f &&
            dist2 < 400.0f && dist2 < 900.0f) {
            best = candidates[i];
        }
    }

    if (best != (datum_index)0xffffffff) {
        return players_iterate_and_discard(best);
    }
    return best;
}

#if 0
Original Ghidra decompilation (0x45e340), from tools/pack.py 0x45e340:

uint FUN_0045e340(uint param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  uint local_c4;
  float local_bc;
  int local_b8;
  float local_b4;
  float local_b0;
  float local_ac;
  undefined1 local_a8 [16];
  undefined1 local_98 [24];
  uint local_80 [32];

  iVar5 = (param_1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  local_c4 = 0xffffffff;
  if ((*(short *)(iVar5 + 2) != -1) &&
     (iVar6 = *(short *)(iVar5 + 2) * 0x40 + DAT_006b145c, 0.0 < *(float *)(iVar6 + 0x3c))) {
    local_c4 = *(uint *)(iVar6 + 0x38);
    iVar6 = object_try_and_get(0xffffffff);
    if (iVar6 == 0) {
      local_c4 = 0xffffffff;
    }
    if (local_c4 != 0xffffffff) goto LAB_0045e4fb;
  }
  unit_get_camera_position();
  FUN_00471f40();
  local_b8 = FUN_004fa1a0(&LAB_0045e2e0,&param_1,0x20,local_80);
  iVar6 = DAT_008603b0;
  iVar9 = 0;
  if (0 < local_b8) {
    do {
      iVar7 = *(int *)(*(int *)(iVar6 + 0x34) + 8 + (local_80[iVar9] & 0xffff) * 0xc);
      fVar1 = *(float *)(iVar7 + 0x5c) - local_b4;
      fVar2 = *(float *)(iVar7 + 0x60) - local_b0;
      fVar3 = *(float *)(iVar7 + 100) - local_ac;
      fVar1 = fVar2 * fVar2 + fVar1 * fVar1 + fVar3 * fVar3;
      if ((((*(float *)(iVar7 + 0x37c) < 1.0) ||
           (iVar7 = FUN_00474db0(local_80[iVar9]), *(int *)(iVar5 + 0x7c) == iVar7)) &&
          (cVar4 = FUN_00459cc0(local_80[iVar9],*(undefined4 *)(iVar5 + 0x34),local_98,local_a8,
                                &local_bc), cVar4 != '\0')) &&
         (((ABS(local_bc) < 0.13083334 && (fVar1 < 400.0)) && (fVar1 < 900.0)))) {
        local_c4 = local_80[iVar9];
      }
      iVar9 = iVar9 + 1;
    } while (iVar9 < local_b8);
    if (local_c4 != 0xffffffff) {
LAB_0045e4fb:
      uVar8 = FUN_00474db0(local_c4);
      return uVar8;
    }
  }
  return local_c4;
}
#endif
