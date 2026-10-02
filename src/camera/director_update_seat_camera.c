// director_update_seat_camera  (Ghidra: FUN_00445c00; renamed)
// address 0x445c00, size 184 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: reads the local player's unit from player_control_globals (+0x10 + i*0x40), asks
//   camera_get_seat_camera_state (0x445b20, ECX = unit, stack = &state) whether the seat wants
//   a third person camera, and flips director.pov_proc between the first person (0x446d60)
//   and third person (0x447370) procedures. The third person branch writes the exact
//   initialisation types/camera.h documents for third_person_camera_data (bytes +0..+3 = 0,
//   word +4 = 0, +8 = -1, word +0xc = -1, +0x10 = +0x14 = 0, +0x18 = 1.0).
// register convention (objdump 0x445c01 / 0x445c34, and the only caller 0x445e51..0x445e55):
//   local player index in AX (movsx eax,ax), force flag in BL (cmp bl,cl with cl = 0). No
//   stack arguments.
//   // blam-cc: AX -> local_player_index, BL -> force
// The early out compares the saved seat state only when not forced; a non forced call switches
// only away from the other gameplay camera (so it never overrides a dead, scripted or flying
// camera), and the transition time is set to 1.0 only when not forced.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern director directors[1];                              // 0x006ac560

// blam-cc: ECX -> unit, stack -> out_state; result in AX
extern int16_t camera_get_seat_camera_state(datum_index unit, int16_t *out_state); // 0x445b20, this module
extern void camera_first_person_compute_pov(director_camera_data *data, camera_input *input,
    observer_command *command); // 0x446d60, this module
extern void camera_third_person_compute_pov(director_camera_data *data, camera_input *input,
    observer_command *command); // 0x447370, this module

// blam-cc: AX -> local_player_index, BL -> force
void director_update_seat_camera(int16_t local_player_index, uint8_t force)
{
    director *director = &directors[local_player_index];
    int16_t seat_camera_state;
    int16_t third_person = camera_get_seat_camera_state(
        player_control_globals_ptr->local_players[local_player_index].unit, &seat_camera_state);

    if (!force && director->seat_camera_state == seat_camera_state) {
        return;
    }

    if (third_person == 1) {
        if (force || director->pov_proc == camera_first_person_compute_pov) {
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
            director->pov_proc = camera_third_person_compute_pov;
            goto switched;
        }
    } else {
        if (force || director->pov_proc == camera_third_person_compute_pov) {
            director->data.first_person.field_of_view = 0.0f;
            director->pov_proc = camera_first_person_compute_pov;
            goto switched;
        }
    }
    director->seat_camera_state = seat_camera_state;
    return;

switched:
    director->look_scale = 1.0f;
    director->unknown_c0 = 0;
    if (!force) {
        director->transition_time = 1.0f;
    }
    director->seat_camera_state = seat_camera_state;
}

#if 0
Original Ghidra decompilation (0x445c00):

void FUN_00445c00(void)

{
  short in_AX;
  short sVar1;
  int iVar2;
  int iVar3;
  char unaff_BL;
  short local_4 [2];

  iVar2 = (int)in_AX;
  iVar3 = iVar2 * 0xf8;
  sVar1 = FUN_00445b20(local_4);
  if ((unaff_BL == '\0') && (*(short *)(&DAT_006ac5b4 + iVar3) == local_4[0])) {
    return;
  }
  if (sVar1 == 1) {
    if ((unaff_BL == '\0') &&
       ((code *)(&DAT_006ac568)[iVar2 * 0x3e] != camera_first_person_compute_pov))
    goto LAB_00445cb1;
    *(undefined4 *)(&DAT_006ac574 + iVar3) = 0xffffffff;
    *(undefined2 *)(&DAT_006ac578 + iVar3) = 0xffff;
    (&DAT_006ac56c)[iVar3] = 0;
    (&DAT_006ac56d)[iVar3] = 0;
    (&DAT_006ac56e)[iVar3] = 0;
    (&DAT_006ac56f)[iVar3] = 0;
    *(undefined2 *)(&DAT_006ac570 + iVar3) = 0;
    *(undefined4 *)(&DAT_006ac580 + iVar3) = 0;
    *(undefined4 *)(&DAT_006ac57c + iVar3) = 0;
    *(undefined4 *)(&DAT_006ac584 + iVar3) = 0x3f800000;
    (&DAT_006ac568)[iVar2 * 0x3e] = camera_third_person_compute_pov;
  }
  else {
    if ((unaff_BL == '\0') &&
       ((code *)(&DAT_006ac568)[iVar2 * 0x3e] != camera_third_person_compute_pov))
    goto LAB_00445cb1;
    *(undefined4 *)(&DAT_006ac56c + iVar3) = 0;
    (&DAT_006ac568)[iVar2 * 0x3e] = camera_first_person_compute_pov;
  }
  *(undefined4 *)(&DAT_006ac624 + iVar3) = 0x3f800000;
  (&DAT_006ac620)[iVar3] = 0;
  if (unaff_BL == '\0') {
    (&DAT_006ac564)[iVar2 * 0x3e] = 0x3f800000;
  }
LAB_00445cb1:
  *(short *)(&DAT_006ac5b4 + iVar3) = local_4[0];
  return;
}

objdump (the unit argument Ghidra dropped):
  445c13: mov ecx,DWORD PTR ds:0x6b145c      ; player_control_globals_ptr
  445c1d: shl eax,0x6                         ; local_player_index * 0x40
  445c20: mov ecx,DWORD PTR [eax+ecx*1+0x10]  ; local_players[i].unit
  445c25: call 0x445b20
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
