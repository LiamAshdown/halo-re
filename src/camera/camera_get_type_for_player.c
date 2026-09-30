// camera_get_type_for_player  (Ghidra: camera_get_type_for_player, already named)
// address 0x445ac0, size 94 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: matches types/camera.h director_camera_type exactly, including the documented quirk
//   ("A first person procedure with a transition still running keeps the previous value"): when
//   the active pov is first person but its cached field_of_view is still 0 (a transition just
//   started and hasn't sampled a real fov yet), the type is NOT set to first_person and the
//   previously cached director.camera_type is returned instead.
// register convention: local player index in CX (in_CX); no stack parameters. Ghidra's
//   "undefined4" return with a garbage 0x44 high word is a decompiler artifact -- the real
//   function returns the int16_t director.camera_type in AX.
//   // blam-cc: CX -> local_player_index

// review fix (phase 4 gate, objdump 0x445ad9..0x445ae9): the first person test reads
//   director +0x04 (transition_time), not the first person mode data at +0x0c; while a
//   transition is still running the cached camera_type is returned unchanged.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"
#include "fn_camera.h"

extern director directors[1]; // 0x006ac560


// blam-cc: CX -> local_player_index
// Recomputes (and caches) director.camera_type from the active pov procedure.
int16_t camera_get_type_for_player(int16_t local_player_index)
{
    director *d = &directors[local_player_index];

    if (d->pov_proc == camera_first_person_compute_pov) {
        if (d->transition_time == 0.0f) { // 0x445adf fld [ecx+0x4]
            d->camera_type = _director_camera_type_first_person;
        }
    } else if (d->pov_proc == camera_third_person_compute_pov) {
        d->camera_type = _director_camera_type_third_person;
    } else {
        d->camera_type = (d->pov_proc != camera_debug_compute_pov) ? _director_camera_type_other
                                                                    : _director_camera_type_scripted;
    }

    return d->camera_type;
}

#if 0
Original Ghidra decompilation (0x445ac0):

undefined4 camera_get_type_for_player(void)

{
  code *pcVar1;
  short in_CX;
  int iVar2;
  int iVar3;

  iVar2 = (int)in_CX;
  iVar3 = iVar2 * 0xf8;
  pcVar1 = (code *)(&DAT_006ac568)[iVar2 * 0x3e];
  if (pcVar1 == camera_first_person_compute_pov) {
    pcVar1 = (code *)0x440000;
    if ((float)(&DAT_006ac564)[iVar2 * 0x3e] == 0.0) {
      *(undefined2 *)(&DAT_006ac5b6 + iVar3) = 0;
      return CONCAT22(0x44,*(undefined2 *)(&DAT_006ac5b6 + iVar3));
    }
  }
  else {
    if (pcVar1 == camera_third_person_compute_pov) {
      *(undefined2 *)(&DAT_006ac5b6 + iVar3) = 1;
      return CONCAT22(0x44,*(undefined2 *)(&DAT_006ac5b6 + iVar3));
    }
    *(ushort *)(&DAT_006ac5b6 + iVar3) = (pcVar1 != camera_debug_compute_pov) + 2;
  }
  return CONCAT22((short)((uint)pcVar1 >> 0x10),*(undefined2 *)(&DAT_006ac5b6 + iVar3));
}
#endif
