// unit_local_player_weapon_flag_check  (Ghidra: unit_local_player_weapon_flag_check)
// address 0x565b00, size 87 bytes
// name confidence: 0.25 (phase2 candidate)   rewrite confidence: 0.95 (VERIFIED against objdump)
// evidence: units.h globals note "0x0087a478 the local player globals; count at +0x0c, handles
//   from +0x04"; players module data_array at 0x0087a480, stride 0x200, unit handle at +0x34
//   (per units.h globals note); unit_current_weapon_has_flag (0x565b60).
// register convention: no parameters (reads the local-player globals directly).
// UNSURE: unit_current_weapon_has_flag is called with zero visible arguments in the original;
//   modelled here as passing the resolved local player's unit handle explicitly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t *local_player_globals; // 0x0087a478; count at +0xc, handles from +0x4
extern data_array *player_data;        // 0x0087a480, stride 0x200, unit handle at +0x34

extern uint8_t unit_current_weapon_has_flag(uint32_t unit_index); // 0x565b60

uint8_t unit_local_player_weapon_flag_check(void)
{
    if (*(int16_t *)(local_player_globals + 0xc) == 1) {
        int32_t slot = -1;
        if (*(int32_t *)(local_player_globals + 4) != -1) {
            slot = 0;
        }
        if (slot != -1 && slot < 1) {
            uint32_t player_handle = *(uint32_t *)(local_player_globals + 4 + slot * 4);
            if (player_handle != (uint32_t)-1) {
                datum_index unit_handle = *(datum_index *)((uint8_t *)player_data->data +
                                                             (player_handle & 0xffff) * 0x200 + 0x34);
                if (unit_handle != (datum_index)-1) {
                    return unit_current_weapon_has_flag(unit_handle);
                }
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x565b00):

undefined1 FUN_00565b00(void)

{
  uint uVar1;
  undefined1 uVar2;
  short sVar3;

  if (*(short *)(DAT_0087a478 + 0xc) == 1) {
    sVar3 = -1;
    if (*(int *)(DAT_0087a478 + 4) != -1) {
      sVar3 = 0;
    }
    if ((((sVar3 != -1) && (sVar3 < 1)) &&
        (uVar1 = *(uint *)(DAT_0087a478 + 4 + sVar3 * 4), uVar1 != 0xffffffff)) &&
       (*(int *)((uVar1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) != -1)) {
      uVar2 = FUN_00565b60();
      return uVar2;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
