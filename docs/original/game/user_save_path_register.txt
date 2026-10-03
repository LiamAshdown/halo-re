// user_save_path_register  (Ghidra: user_save_path_register, already named)
// address 0x551650, size 73 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/game_types_notes.md ("save games" section): "user_save_path_table comes
//   from user_save_path_register / _lookup / _remove; 0x00721f30 + 8 * 0x105 == 0x00722758
//   closes the pair of arrays" -- an 8-entry table pairing a user id (0x00722758, stride 4) with
//   a 0x105-byte path buffer (0x00721f30, stride 0x105).
// register convention: none -- both are genuine stack parameters (Ghidra's own param_1/param_2).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t user_save_path_keys[k_maximum_user_save_paths]; // 0x00722758
extern char user_save_paths[k_maximum_user_save_paths][k_user_save_path_slot_stride]; // 0x00721f30


// Finds the first free (user id 0) slot, copies `path` into it (up to 0x104 chars, per
// strncpy's own count), stores `user_id`, and returns the slot index; returns -1 if all 8
// slots are taken.
int32_t user_save_path_register(uint32_t user_id, char *path)
{
    int32_t i = 0;
    do {
        if (user_save_path_keys[i] == 0) {
            strncpy(user_save_paths[i], path, 0x104);
            user_save_path_keys[i] = user_id;
            return i;
        }
        i = i + 1;
    } while (i < k_maximum_user_save_paths);
    return -1;
}

#if 0
Original Ghidra decompilation (0x551650), from tools/pack.py 0x551650:

int user_save_path_register(undefined4 param_1,char *param_2)

{
  int iVar1;

  iVar1 = 0;
  do {
    if ((&DAT_00722758)[iVar1] == 0) {
      _strncpy(&DAT_00721f30 + iVar1 * 0x105,param_2,0x104);
      (&DAT_00722758)[iVar1] = param_1;
      return iVar1;
    }
    iVar1 = iVar1 + 1;
  } while (iVar1 < 8);
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
