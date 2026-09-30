// control_binding_table_query  (orphan pass 4: FUN_004f3ad0, no Ghidra name)
// address 0x4f3ad0, size 197 bytes
// name confidence: 0.35 (behaviour: finds a device-slot row by a target value the same way
//   control_binding_table_register_single.c does, then scans that row's 3 id-slot pairs for a
//   matching raw id and returns its stored flag byte -- a lookup/query counterpart to the
//   register/update functions in this file)
// rewrite confidence: 0.35 (control flow transcribed literally from the decompilation; the
//   table layout is the same one used by every other control_binding_table_*.c file in this
//   pass, cross-checked for consistency)
// evidence: out/phase4/objects_types_notes.md / src/objects/README.md: these six functions
//   "read the packed control words at 0x006f1cec/0x006f1ce8 and belong to the input or game
//   module." Same row-search table at 0x008603ec (stride 0xa0, bound 0x008607ac) as
//   control_binding_table_register_single.c, and the same {count@0x8603e0 (idx*0x14 elements =
//   idx*0x50 bytes), id-array@0x8603f0, flag-array@0x8603f4} layout as
//   control_binding_table_update_a/_b.c.
// register convention: raw id to search for in the stack parameter (param_1, recognized by
//   Ghidra); search target in EDX (in_EDX, unrecognized register parameter). Returns a byte
//   (1 by default, the stored flag byte if a matching id is found, 0 if the row lookup fails
//   its range check).
// blam-cc: control_binding_table_query(int32_t target /*EDX*/, int32_t raw_id /*stack*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"

extern uint32_t current_game_engine; // 0x006f1d20, same global as control_binding_table_initialize.c

extern uint8_t g_control_binding_region_ec[0x3c0]; // base 0x008603ec, same region as control_binding_table_register_single.c
extern uint8_t g_control_binding_region_e0[0x3c0]; // base 0x008603e0, same region as control_binding_table_update_a/_b.c

uint8_t control_binding_table_query(int32_t target, int32_t raw_id)
{
    uint8_t result = 1;
    uint8_t *cursor;
    uint8_t *region_end;
    int32_t row;
    int32_t row_index;

    if (current_game_engine == 0) {
        return result;
    }

    cursor = g_control_binding_region_ec;
    region_end = g_control_binding_region_ec + sizeof(g_control_binding_region_ec);
    row_index = -1;
    row = 0;
    while (cursor < region_end) {
        if (*(int32_t *)cursor == target) {
            row_index = row;
            break;
        }
        cursor += 0xa0;
        row++;
    }
    // (the original's row-search loop simplifies to exactly this: the matching row's index, or
    // -1 if the target value is never found before the region bound)

    result = 0;
    if (row_index < 0 || row_index > 5) {
        return 0;
    }

    {
        int32_t pair;
        for (pair = 0; pair <= 2; pair++) {
            int32_t idx = pair + row_index * 2;
            int32_t count = *(int32_t *)(g_control_binding_region_e0 + idx * 0x50);
            if (count != 0) {
                uint8_t *id_cursor = g_control_binding_region_e0 + 0x10 + idx * 0x50; // DAT_008603f0 offset
                uint32_t slot;
                for (slot = 0; slot < (uint32_t)count; slot++) {
                    if (*(int32_t *)id_cursor == raw_id) {
                        result = *(uint8_t *)(g_control_binding_region_e0 + 0x14 + (slot + idx * 10) * 8); // DAT_008603f4 offset
                        return result;
                    }
                    id_cursor += 8;
                }
            }
        }
    }

    return result;
}

#if 0
Original Ghidra decompilation (0x4f3ad0):

undefined1 FUN_004f3ad0(int param_1)

{
  int iVar1;
  bool bVar2;
  int *piVar3;
  int iVar4;
  int in_EDX;
  uint uVar5;
  undefined1 local_5;
  int local_4;

  local_5 = 1;
  if (DAT_006f1d20 != 0) {
    local_4 = -1;
    iVar4 = 0;
    piVar3 = &DAT_008603ec;
    do {
      iVar1 = iVar4;
      if (*piVar3 == in_EDX) break;
      piVar3 = piVar3 + 0x28;
      iVar4 = iVar4 + 1;
      iVar1 = local_4;
    } while ((int)piVar3 < 0x8607ac);
    local_4 = iVar1;
    local_5 = 0;
    if ((local_4 < 0) || (5 < local_4)) {
      return 0;
    }
    bVar2 = false;
    iVar4 = 0;
    do {
      if (2 < iVar4) {
        return local_5;
      }
      iVar1 = iVar4 + local_4 * 2;
      uVar5 = 0;
      if ((&DAT_008603e0)[iVar1 * 0x14] != 0) {
        piVar3 = &DAT_008603f0 + iVar1 * 0x14;
        do {
          if (*piVar3 == param_1) {
            local_5 = (&DAT_008603f4)[(uVar5 + iVar1 * 10) * 8];
            bVar2 = true;
            break;
          }
          uVar5 = uVar5 + 1;
          piVar3 = piVar3 + 2;
        } while (uVar5 < (uint)(&DAT_008603e0)[iVar1 * 0x14]);
      }
      iVar4 = iVar4 + 1;
    } while (!bVar2);
  }
  return local_5;
}
#endif
