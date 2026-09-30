// user_save_path_lookup  (Ghidra: user_save_path_lookup, already named)
// address 0x5516a0, size 40 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: shares the table with the sibling user_save_path_register.c / _remove.c (this
//   batch); DAT_00721f28 is a default/fallback path pointer, distinct from the per-user table.
// register convention: a user id in ECX (in_ECX); no stack parameters.
//   // blam-cc: ECX -> user_id
// UNSURE: the `if (iVar1 == -1)` check inside the loop can never be true (iVar1 only ever
//   reaches 0..7 there), so that branch is dead code, preserved literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

extern uint32_t user_save_path_keys[k_maximum_user_save_paths]; // 0x00722758
extern char user_save_paths[k_maximum_user_save_paths][k_user_save_path_slot_stride]; // 0x00721f30
extern char *user_save_path_default; // 0x00721f28

// blam-cc: ECX -> user_id
// Returns the registered path for `user_id`, or the default path if no slot matches.
char *user_save_path_lookup(uint32_t user_id)
{
    int32_t i = 0;
    do {
        if (user_save_path_keys[i] == user_id) {
            if (i == -1) {
                return user_save_path_default; // UNSURE: unreachable, see header note
            }
            return user_save_paths[i];
        }
        i = i + 1;
    } while (i < k_maximum_user_save_paths);
    return user_save_path_default;
}

#if 0
Original Ghidra decompilation (0x5516a0), from tools/pack.py 0x5516a0:

undefined * user_save_path_lookup(void)

{
  int iVar1;
  int in_ECX;

  iVar1 = 0;
  do {
    if ((&DAT_00722758)[iVar1] == in_ECX) {
      if (iVar1 == -1) {
        return DAT_00721f28;
      }
      return &DAT_00721f30 + iVar1 * 0x105;
    }
    iVar1 = iVar1 + 1;
  } while (iVar1 < 8);
  return DAT_00721f28;
}
#endif
