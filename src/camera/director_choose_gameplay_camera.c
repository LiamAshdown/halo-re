// director_choose_gameplay_camera  (Ghidra: FUN_00445dc0; renamed)
// address 0x445dc0, size 376 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: camera_update calls it for director_globals.mode 0 and 1 (following / orbiting).
//   With reset set it forces the first person pov. Otherwise, unless hs camera_control holds
//   the camera, it lets director_update_seat_camera (0x445c00, not forced) track the seat, then
//   switches to the dead camera (dead_camera_new 0x4450e0 + pov 0x445380) while the player is
//   dead, and back to the first / third person pov once the player has a unit again.
//   "Dead" is player.unit == -1 with player.deaths (+0xae) > 0 (0x445e2d..0x445e3e), so a
//   player who has not spawned yet keeps the gameplay camera. chimera
//   sig__spectate_death_cam_full_sig lands inside this function (0x445e06).
// register convention (objdump 0x445dc1 and the caller 0x4456ad): local player index in DI
//   (movsx ebp,di; the caller does xor edi,edi), reset flag as the only stack argument.
//   // blam-cc: DI -> local_player_index, stack -> reset
// The local player to player lookup (0x445dfa) is local_player_globals->local_players[i] for
// 0 <= i < 1, else -1, the inline form of local_player_get_player_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *hs_camera_control_pointer;                 // 0x0087bc0c, hs module
extern player_globals *local_player_globals;               // 0x0087a478
extern data_array *player_data;                            // 0x0087a480
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern director directors[1];                              // 0x006ac560

// blam-cc: AX -> local_player_index, BL -> force
extern void director_update_seat_camera(int16_t local_player_index, uint8_t force); // 0x445c00, this module
// blam-cc: ECX -> unit, stack -> out_state; result in AX
extern int16_t camera_get_seat_camera_state(datum_index unit, int16_t *out_state);  // 0x445b20, this module
// blam-cc: EAX -> self, DX -> local_player_index, stack -> unit
extern dead_camera_data *dead_camera_new(dead_camera_data *self, int16_t local_player_index,
    datum_index unit);                                                              // 0x4450e0, this module
extern void camera_track_compute_pov(director_camera_data *data, camera_input *input,
    observer_command *command); // 0x445380, this module (the dead camera pov)
extern void camera_first_person_compute_pov(director_camera_data *data, camera_input *input,
    observer_command *command); // 0x446d60, this module
extern void camera_third_person_compute_pov(director_camera_data *data, camera_input *input,
    observer_command *command); // 0x447370, this module

// blam-cc: DI -> local_player_index, stack -> reset
void director_choose_gameplay_camera(int16_t local_player_index, uint8_t reset)
{
    director *director = &directors[local_player_index];
    datum_index player_index;
    player *player_record;
    uint8_t player_is_dead;

    if (reset) {
        director->data.first_person.field_of_view = 0.0f;
        director->pov_proc = camera_first_person_compute_pov;
        director->look_scale = 1.0f;
        director->unknown_c0 = 0;
        return;
    }

    if (local_player_index == -1 || local_player_index >= 1) {
        player_index = k_datum_index_none;
    } else {
        player_index = local_player_globals->local_players[local_player_index];
    }
    // no salt check: indexes the players array directly, as the original does
    player_record = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    player_is_dead = (player_record->unit == k_datum_index_none && player_record->deaths > 0);

    if (*hs_camera_control_pointer != 0) {
        return;
    }

    director_update_seat_camera(local_player_index, 0);

    if (player_is_dead) {
        if (director->pov_proc != camera_track_compute_pov) {
            dead_camera_new(&director->data.dead, local_player_index, k_datum_index_none);
            director->unknown_c0 = 0;
            director->pov_proc = camera_track_compute_pov;
            director->look_scale = 1.0f;
            director->transition_time = 1.0f;
        }
    } else if (director->pov_proc == camera_track_compute_pov) {
        int16_t seat_camera_state;
        int16_t third_person = camera_get_seat_camera_state(
            player_control_globals_ptr->local_players[local_player_index].unit, &seat_camera_state);

        if (third_person == 1) {
            third_person_camera_data *third = &director->data.third_person;

            third->unit = k_datum_index_none;
            third->seat_index = -1;
            third->initialized = 0;
            third->unknown_01 = 0;
            third->crouch_or_jump = 0;
            third->unknown_03 = 0;
            third->unknown_04 = 0;
            third->pitch_offset = 0.0f;
            third->yaw_offset = 0.0f;
            third->distance_scale = 1.0f;
            director->look_scale = 1.0f;
            director->pov_proc = camera_third_person_compute_pov;
            director->unknown_c0 = 0;
            director->seat_camera_state = seat_camera_state;
        } else {
            director->data.first_person.field_of_view = 0.0f;
            director->pov_proc = camera_first_person_compute_pov;
            director->look_scale = 1.0f;
            director->unknown_c0 = 0;
            director->seat_camera_state = seat_camera_state;
        }
    }
}

#if 0
Original Ghidra decompilation (0x445dc0):

void FUN_00445dc0(ushort param_1)

{
  short sVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  short unaff_DI;

  iVar5 = (int)unaff_DI;
  iVar2 = iVar5 * 0xf8;
  if ((char)param_1 != '\0') {
    *(undefined4 *)(&DAT_006ac56c + iVar2) = 0;
    (&DAT_006ac568)[iVar5 * 0x3e] = camera_first_person_compute_pov;
    *(undefined4 *)(&DAT_006ac624 + iVar2) = 0x3f800000;
    (&DAT_006ac620)[iVar2] = 0;
    return;
  }
  if ((unaff_DI == -1) || (0 < unaff_DI)) {
    uVar3 = 0xffffffff;
  }
  else {
    uVar3 = *(uint *)(DAT_0087a478 + 4 + iVar5 * 4);
  }
  iVar4 = (uVar3 & 0xffff) * 0x200;
  if ((*(int *)(iVar4 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) != -1) ||
     (param_1 = CONCAT11(param_1._1_1_,1),
     *(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34) + 0xae) < 1)) {
    param_1 = param_1 & 0xff00;
  }
  if (*DAT_0087bc0c == '\0') {
    FUN_00445c00();
    if ((char)param_1 == '\0') {
      if ((code *)(&DAT_006ac568)[iVar5 * 0x3e] == camera_track_compute_pov) {
        sVar1 = FUN_00445b20(&param_1);
        if (sVar1 == 1) {
          *(undefined4 *)(&DAT_006ac574 + iVar2) = 0xffffffff;
          *(undefined2 *)(&DAT_006ac578 + iVar2) = 0xffff;
          (&DAT_006ac56c)[iVar2] = 0;
          (&DAT_006ac56d)[iVar2] = 0;
          (&DAT_006ac56e)[iVar2] = 0;
          (&DAT_006ac56f)[iVar2] = 0;
          *(undefined2 *)(&DAT_006ac570 + iVar2) = 0;
          *(undefined4 *)(&DAT_006ac580 + iVar2) = 0;
          *(undefined4 *)(&DAT_006ac57c + iVar2) = 0;
          *(undefined4 *)(&DAT_006ac584 + iVar2) = 0x3f800000;
          *(undefined4 *)(&DAT_006ac624 + iVar2) = 0x3f800000;
          (&DAT_006ac568)[iVar5 * 0x3e] = camera_third_person_compute_pov;
          (&DAT_006ac620)[iVar2] = 0;
          *(ushort *)(&DAT_006ac5b4 + iVar2) = param_1;
          return;
        }
        *(undefined4 *)(&DAT_006ac56c + iVar2) = 0;
        (&DAT_006ac568)[iVar5 * 0x3e] = camera_first_person_compute_pov;
        *(undefined4 *)(&DAT_006ac624 + iVar2) = 0x3f800000;
        (&DAT_006ac620)[iVar2] = 0;
        *(ushort *)(&DAT_006ac5b4 + iVar2) = param_1;
      }
    }
    else if ((code *)(&DAT_006ac568)[iVar5 * 0x3e] != camera_track_compute_pov) {
      camera_shake_initialize(0xffffffff);
      (&DAT_006ac620)[iVar2] = 0;
      (&DAT_006ac568)[iVar5 * 0x3e] = camera_track_compute_pov;
      *(undefined4 *)(&DAT_006ac624 + iVar2) = 0x3f800000;
      (&DAT_006ac564)[iVar5 * 0x3e] = 0x3f800000;
      return;
    }
  }
  return;
}

objdump for the register arguments Ghidra dropped:
  445e51: xor bl,bl / mov eax,edi / call 0x445c00       ; director_update_seat_camera(i, 0)
  445e74: push -1 / lea eax,[esi+0xc] / mov edx,edi / call 0x4450e0   ; dead_camera_new
  445ea9: mov edx,ds:0x6b145c / shl ebp,6 / mov ecx,[edx+ebp+0x10] / call 0x445b20
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
