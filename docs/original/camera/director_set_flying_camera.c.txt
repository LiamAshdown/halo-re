// director_set_flying_camera  (Ghidra: FUN_00445f40; renamed)
// address 0x445f40, size 67 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: camera_update calls it for director_globals.mode 2 ("flying" in the name table at
//   0x00686a10). It (re)initialises the flying camera data through flying_camera_initialize
//   (0x446350, EAX = &director.data, stack = local player index) and installs the flying pov
//   0x4464f0 (a real function Ghidra has no entry for).
// register convention (objdump 0x445f41 / 0x445f4a and the caller 0x44576e..0x445776): local
//   player index in AX, force flag in CL (camera_update passes director_globals.mode_changed).
//   // blam-cc: AX -> local_player_index, CL -> force

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern director directors[1]; // 0x006ac560

// blam-cc: EAX -> data, stack -> local_player_index
extern void flying_camera_initialize(editor_camera_data *data, int16_t local_player_index); // 0x446350, this module
// the flying pov procedure 0x4464f0 (no Ghidra function; see README known gaps)
extern void flying_camera_compute_pov(director_camera_data *data, camera_input *input,
    observer_command *command); // 0x4464f0, this module

// blam-cc: AX -> local_player_index, CL -> force
void director_set_flying_camera(int16_t local_player_index, uint8_t force)
{
    director *director = &directors[local_player_index];

    if (force || director->pov_proc != flying_camera_compute_pov) {
        flying_camera_initialize(&director->data.editor, local_player_index);
        director->pov_proc = flying_camera_compute_pov;
        director->look_scale = 1.0f;
        director->unknown_c0 = 0;
    }
}

#if 0
Original Ghidra decompilation (0x445f40):

void FUN_00445f40(void)

{
  short in_AX;
  char in_CL;
  int iVar1;

  iVar1 = (int)in_AX;
  if ((in_CL != '\0') || ((undefined1 *)(&DAT_006ac568)[iVar1 * 0x3e] != &LAB_004464f0)) {
    FUN_00446350();
    (&DAT_006ac568)[iVar1 * 0x3e] = &LAB_004464f0;
    *(undefined4 *)(&DAT_006ac624 + iVar1 * 0xf8) = 0x3f800000;
    (&DAT_006ac620)[iVar1 * 0xf8] = 0;
  }
  return;
}

objdump for the call Ghidra left without arguments:
  445f5d: push eax             ; local_player_index (still the AX argument)
  445f5e: lea eax,[esi+0xc]    ; &directors[i].data
  445f61: call 0x446350
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
