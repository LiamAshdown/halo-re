// motion_sensor_reset  (Ghidra: already named)
// address 0x4b3660, size 57 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// phase-4 review: checked against objdump 0x4b3660..0x4b369b (0x15c dwords cleared, blip type 6 in 10 x 16 slots).
// evidence: types/interface.h motion_sensor_globals (0x570 bytes, zeroed then the empty
// blip marker written into 10 groups of 16 records, stepping the record by 4 bytes and the
// group by 0x84) -- this function is the evidence the header comment cites.
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern motion_sensor_globals *motion_sensor; // 0x00719438

void __cdecl motion_sensor_reset(void)
{
    int32_t *clear = (int32_t *)motion_sensor;
    int i;
    uint8_t *type_byte;
    int group, slot;

    for (i = 0; i < 0x15c; i++) {
        clear[i] = 0;
    }

    type_byte = (uint8_t *)motion_sensor + 2; // motion_sensor_blip::type of players[0].history[0].blips[0]
    for (group = 0; group < 10; group++) {
        uint8_t *slot_type = type_byte;
        for (slot = 0; slot < 0x10; slot++) {
            *slot_type = _blip_type_empty;
            slot_type += 4;
        }
        type_byte += 0x84;
    }
}

#if 0
Original Ghidra decompilation (0x4b3660):

void __cdecl motion_sensor_reset(void)

{
  undefined4 *puVar1;
  undefined1 *puVar2;
  int iVar3;
  int iVar4;
  undefined1 *puVar5;
  undefined4 *puVar6;

  puVar1 = DAT_00719438;
  puVar6 = DAT_00719438;
  for (iVar3 = 0x15c; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  puVar5 = (undefined1 *)((int)puVar1 + 2);
  iVar3 = 10;
  do {
    iVar4 = 0x10;
    puVar2 = puVar5;
    do {
      *puVar2 = 6;
      puVar2 = puVar2 + 4;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
    puVar5 = puVar5 + 0x84;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
