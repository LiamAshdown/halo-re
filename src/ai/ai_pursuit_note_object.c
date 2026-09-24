// ai_pursuit_note_object  (Ghidra: ai_pursuit_note_object; named for this rewrite)
// address 0x436b10, size 120 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: calls squad_recent_object_get_or_create (0x436c60, this batch) with all of its
// own parameters passed straight through (Ghidra's decompile shows the call with zero
// visible arguments), then records object_index into that ai_pursuit's 6-wide ring buffer
// (types/ai.h ai_pursuit.object_index[6], k_ai_pursuit_object_count) unless it is already
// present, and stamps last_tick from the current game tick either way. Matches the phase-4
// summary.
// register convention: Ghidra resolved none of its parameters; the object index is in EDX.
//   // blam-cc: EAX -> min_last_tick, ECX -> type, EDX -> object_index, stack -> encounter_index
// FIXED (register inputs, objdump): EAX carries min_last_tick (pushed at 0x436b14, before being
// reloaded from the true stack slot) and ECX carries type (pushed at 0x436b19), both forwarded
// straight through to squad_recent_object_get_or_create. The old notes guessed these were stack
// arguments and invented a "create_if_missing" stack parameter; the binary only reads one stack
// slot (encounter_index) and always passes a hardcoded 1 (push 0x1 at 0x436b12) for
// create_if_missing, so that parameter is dropped here and hardcoded in the call below.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"

extern data_array *ai_pursuit_data;      // 0x008802d0
extern game_time_globals *game_time;     // 0x006f1d6c

extern datum_index squad_recent_object_get_or_create(datum_index encounter_index, int16_t type,
    int32_t min_last_tick, char create_if_missing); // 0x436c60, this batch

// blam-cc: EAX -> min_last_tick, ECX -> type, EDX -> object_index, stack -> encounter_index
uint8_t ai_pursuit_note_object(datum_index object_index, datum_index encounter_index, int16_t type,
                                int32_t min_last_tick)
{
    uint8_t added = 0;
    datum_index handle = squad_recent_object_get_or_create(encounter_index, type, min_last_tick, 1);

    if (handle != (datum_index)k_datum_index_none) {
        ai_pursuit *pursuit = &((ai_pursuit *)ai_pursuit_data->data)[handle & 0xffff];
        int16_t i;

        for (i = 0; i < k_ai_pursuit_object_count; i++) {
            if (pursuit->object_index[i] == object_index) {
                goto stamp;
            }
        }

        pursuit->object_index[pursuit->cursor] = object_index;
        pursuit->count = pursuit->count + 1;
        pursuit->cursor = (int16_t)((pursuit->cursor + 1) % k_ai_pursuit_object_count);
        added = 1;

    stamp:
        pursuit->last_tick = game_time->game_time;
    }

    return added;
}

#if 0
Original Ghidra decompilation (0x436b10):

undefined1 FUN_00436b10(void)

{
  int iVar1;
  short sVar2;
  uint uVar3;
  int in_EDX;
  undefined1 uVar4;

  uVar4 = 0;
  uVar3 = squad_recent_object_get_or_create();
  if (uVar3 != 0xffffffff) {
    iVar1 = *(int *)(DAT_008802d0 + 0x34) + (uVar3 & 0xffff) * 0x28;
    sVar2 = 0;
    do {
      if (*(int *)(iVar1 + 0xc + sVar2 * 4) == in_EDX) {
        uVar4 = 0;
        if (sVar2 < 6) goto LAB_00436b77;
        break;
      }
      sVar2 = sVar2 + 1;
    } while (sVar2 < 6);
    *(int *)(iVar1 + 0xc + *(short *)(iVar1 + 10) * 4) = in_EDX;
    *(short *)(iVar1 + 8) = *(short *)(iVar1 + 8) + 1;
    *(short *)(iVar1 + 10) = (short)((*(short *)(iVar1 + 10) + 1) % 6);
    uVar4 = 1;
LAB_00436b77:
    *(undefined4 *)(iVar1 + 4) = *(undefined4 *)(DAT_006f1d6c + 0xc);
  }
  return uVar4;
}
#endif
