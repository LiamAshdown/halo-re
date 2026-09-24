// hud_message_compare  (Ghidra: FUN_004ae500, renamed in the phase-4 review)
// address 0x4ae500, size 45 bytes
// name confidence: 0.65 (chosen)   rewrite confidence: 0.95
// evidence: objdump 0x4ae500..0x4ae52c. qsort comparator (0x623410) handed to qsort by
// hud_messaging_update over the four hud_message_slot records: newest timestamp first, then
// the larger source, then the larger sequence (b - a in each step).
// register convention: plain cdecl.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

int32_t hud_message_compare(const void *a, const void *b)
{
    const hud_message_slot *left = (const hud_message_slot *)a;
    const hud_message_slot *right = (const hud_message_slot *)b;
    int32_t difference;

    difference = right->timestamp - left->timestamp;
    if (difference == 0) {
        difference = right->source - left->source;
        if (difference == 0) {
            difference = (int32_t)right->sequence - (int32_t)left->sequence;
        }
    }
    return difference;
}

#if 0
Original Ghidra decompilation (0x4ae500):

int FUN_004ae500(int *param_1,int *param_2)

{
  int iVar1;

  iVar1 = *param_2 - *param_1;
  if ((iVar1 == 0) && (iVar1 = param_2[0x21] - param_1[0x21], iVar1 == 0)) {
    iVar1 = (uint)*(byte *)((int)param_2 + 0x83) - (uint)*(byte *)((int)param_1 + 0x83);
  }
  return iVar1;
}
#endif
