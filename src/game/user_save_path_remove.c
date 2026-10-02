// user_save_path_remove  (Ghidra: user_save_path_remove, already named)
// address 0x5516d0, size 64 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: shares the table with the sibling user_save_path_register.c / _lookup.c (this
//   batch); the 0x41-dword-plus-one-byte clear (0x104 + 1 = 0x105 bytes) matches the table's
//   own per-entry path length exactly.
// register convention: a user id in EAX (in_EAX); no stack parameters.
//   // blam-cc: EAX -> user_id
// UNSURE: the `if (iVar2 != -1) ... break` shape makes the `break` path (iVar2 == -1)
//   unreachable, same dead-code shape as user_save_path_lookup.c; preserved literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint32_t user_save_path_keys[k_maximum_user_save_paths]; // 0x00722758
extern char user_save_paths[k_maximum_user_save_paths][k_user_save_path_slot_stride]; // 0x00721f30

// blam-cc: EAX -> user_id
// Finds the slot registered for `user_id`, clears its user id and zeroes its path buffer, and
// returns 1. Returns 0 if no slot matches.
uint8_t user_save_path_remove(uint32_t user_id)
{
    int32_t i = 0;
    do {
        if (user_save_path_keys[i] == user_id) {
            if (i != -1) {
                uint32_t *raw;
                int32_t j;

                user_save_path_keys[i] = 0;
                raw = (uint32_t *)user_save_paths[i];
                for (j = 0; j < 0x41; j++) {
                    raw[j] = 0;
                }
                ((uint8_t *)raw)[0x104] = 0;
                return 1;
            }
            break; // UNSURE: unreachable, see header note
        }
        i = i + 1;
    } while (i < k_maximum_user_save_paths);
    return 0;
}

#if 0
Original Ghidra decompilation (0x5516d0), from tools/pack.py 0x5516d0:

uint user_save_path_remove(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  undefined4 *puVar3;

  iVar2 = 0;
  do {
    if ((&DAT_00722758)[iVar2] == in_EAX) {
      if (iVar2 != -1) {
        (&DAT_00722758)[iVar2] = 0;
        puVar3 = (undefined4 *)(&DAT_00721f30 + iVar2 * 0x105);
        for (iVar1 = 0x41; iVar1 != 0; iVar1 = iVar1 + -1) {
          *puVar3 = 0;
          puVar3 = puVar3 + 1;
        }
        *(undefined1 *)puVar3 = 0;
        return 1;
      }
      break;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 8);
  return in_EAX & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
