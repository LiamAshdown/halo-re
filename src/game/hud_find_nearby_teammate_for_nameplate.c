// hud_find_nearby_teammate_for_nameplate  (Ghidra: FUN_0045e340; named per
// out/phase4/game_functions.md)
// address 0x45e340, size 474 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN 2026-09-27 (static loop) from objdump 0x45e340..0x45e51f. The draft dropped every register argument:
//   game_engine_compute_local_player_look_vector takes EAX = the look vector out (local +0x20) and CX = the local
//   player index; the PVS collect (0x4fa1a0) takes EDX = the camera point (EDI kept from unit_get_camera_position)
//   with stack (filter 0x45e2e0, &player_handle, 32, candidates); camera_observer_target_direction takes EAX = the
//   closest-point out, ECX = the look vector, ESI = the camera, stack (candidate, player unit, direction out,
//   distance out, angle out). Constants: 0x672ac0 0.0, 0x672ac4 1.0, 0x673158 (double) 0.13083334, 0x673150 400.0,
//   0x672f98 900.0. With no candidates the function returns -1 directly (0x45e512).
// blam-cc: stack -> player_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;                          // 0x0087a480
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern data_array *object_data;                        // 0x008603b0

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX object, stack mask
extern void unit_get_camera_position(datum_index unit_index, real_point3d *out); // 0x568f80, ECX unit, EDI out
extern void game_engine_compute_local_player_look_vector(real_vector3d *out_forward,
    int16_t local_player_index); // 0x471f40, EAX out, CX local player
extern int32_t object_collect_local_player_relevant_objects(real_point3d *point, uint8_t (*filter)(uint32_t, void *),
    void *filter_context, int32_t max_count, datum_index *out); // 0x4fa1a0, EDX point, stack
extern uint32_t camera_observer_target_direction(real_point3d *candidate_point, real_vector3d *facing,
    real_point3d *reference_position, datum_index object, datum_index exclude_object,
    real_vector3d *out_direction, real *out_distance, real *out_angle); // 0x459cc0, EAX, ECX, ESI, stack
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0
extern uint8_t hud_nameplate_candidate_filter(uint32_t object_index, void *player_handle); // 0x45e2e0

// Finds the teammate biped the player is aiming at (within the 0.1308 aim cone and 20 world units) to show its
// nameplate, preferring the player's cached nameplate target while its weight is positive. Returns the target's
// player index, or -1.
datum_index hud_find_nearby_teammate_for_nameplate(datum_index player_handle)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    datum_index best = (datum_index)0xffffffff;
    real_point3d camera;
    real_vector3d look;
    real_vector3d direction;
    real_point3d closest_point;
    real distance;
    real angle;
    datum_index candidates[32];
    int32_t candidate_count;
    int32_t i;

    if (p->local_player_index != -1) {
        local_player_control *track = &player_control_globals_ptr->local_players[p->local_player_index];

        if (track->nameplate_weight > 0.0f) {
            datum_index target = track->nameplate_target;

            best = object_try_and_get(target, 0xffffffff) != 0 ? target : (datum_index)0xffffffff;
            if (best != (datum_index)0xffffffff) {
                return player_index_from_unit_index(best);
            }
        }
    }

    unit_get_camera_position(p->unit, &camera);
    game_engine_compute_local_player_look_vector(&look, p->local_player_index);
    candidate_count = object_collect_local_player_relevant_objects(&camera, hud_nameplate_candidate_filter,
        &player_handle, 0x20, candidates);
    if (candidate_count <= 0) {
        return best;
    }

    for (i = 0; i < candidate_count; i++) {
        uint8_t *candidate = (uint8_t *)((object_header *)object_data->data)[candidates[i] & 0xffff].data;
        real dx = ((struct object *)candidate)->position.x - camera.x;
        real dy = ((struct object *)candidate)->position.y - camera.y;
        real dz = ((struct object *)candidate)->position.z - camera.z;
        real distance_squared = dz * dz + dx * dx + dy * dy;

        // 0x45e454..0x45e475: a unit whose +0x37c is below 1.0 qualifies outright; otherwise only the player the
        // HUD already tracks (+0x7c).
        if (!(*(real *)(candidate + 0x37c) < 1.0f) &&
            p->nameplate_target_player != player_index_from_unit_index(candidates[i])) {
            continue;
        }
        if (camera_observer_target_direction(&closest_point, &look, &camera, candidates[i], p->unit, &direction,
                &distance, &angle) == 0) {
            continue;
        }
        if ((double)(angle < 0.0f ? -angle : angle) < 0.13083334267139435 && distance_squared < 400.0f && distance_squared < 900.0f) {
            best = candidates[i];
        }
    }

    if (best == (datum_index)0xffffffff) {
        return best;
    }
    return player_index_from_unit_index(best);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
