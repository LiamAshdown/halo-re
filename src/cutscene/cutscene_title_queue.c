// cutscene_title_queue  (Ghidra: cutscene_title_queue, already named; hs cinematic_set_title
// 0x47f910 / cinematic_set_title_delayed 0x47f960 call it)
// address 0x449960, size 95 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md cinematic_globals +0x0c titles[4]: scans for the
// first empty slot (title_index == -1) and fills it with the title index and a negative tick
// count that counts up to zero as the start delay runs out.
// register convention: __cdecl, both parameters recognized by Ghidra on the stack (int16 title
// index, float delay in seconds); no custom registers.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

extern int32_t ROUND(float x); // MSVC round-to-nearest helper

extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c

// Queues a cutscene title for display in the first free slot (of up to
// k_cinematic_title_slot_count concurrent slots), scheduling it to start fading in after
// delay_seconds. Does nothing if every slot is already occupied.
void cutscene_title_queue(int16_t title_index, float delay_seconds)
{
    int16_t i;

    for (i = 0; i < k_cinematic_title_slot_count; i += 1) {
        if (cinematic_globals_ptr->titles[i].title_index == k_cinematic_title_none) {
            cinematic_globals_ptr->titles[i].title_index = title_index;
            cinematic_globals_ptr->titles[i].ticks =
                -(int16_t)ROUND(delay_seconds * k_cinematic_ticks_per_second);
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x449960):

void cutscene_title_queue(undefined2 param_1,float param_2)

{
  int iVar1;
  short sVar2;

  iVar1 = DAT_006f187c;
  sVar2 = 0;
  do {
    if (*(short *)(DAT_006f187c + 0xc + sVar2 * 4) == -1) {
      if (sVar2 < 4) {
        *(undefined2 *)(DAT_006f187c + 0xc + sVar2 * 4) = param_1;
        *(short *)(iVar1 + 0xe + sVar2 * 4) = -(short)(int)ROUND(param_2 * 30.0);
      }
      return;
    }
    sVar2 = sVar2 + 1;
  } while (sVar2 < 4);
  return;
}
#endif
