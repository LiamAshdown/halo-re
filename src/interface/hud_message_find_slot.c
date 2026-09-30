// hud_message_find_slot  (Ghidra: FUN_004ae480, renamed in the phase-4 review)
// address 0x4ae480, size 128 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4ae480..0x4ae4ff. Walks the four slots of a player record. A slot is a
// candidate when it matches {source, kind} (source not -1) or is inactive; the walk stops at
// the first candidate when source is -1 or equals the slot source (so an inactive slot is
// kept only while an exact match may still follow). Without a candidate the active slot with
// the smallest timestamp is returned. The first rewrite was equivalent; this one names the
// candidate logic.
// register convention: ESI source; two stack arguments.
//   // blam-cc: source -> ESI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

// blam-cc: source -> ESI
hud_message_slot *hud_message_find_slot(int32_t source, hud_player_messaging_state *record, uint8_t source_kind)
{
    hud_message_slot *candidate = 0;
    int32_t oldest_time = 0x7fffffff;
    int16_t oldest = 0;
    uint16_t i;

    for (i = 0; i < 4; i++) {
        hud_message_slot *slot = &record->messages[(int16_t)i];

        if ((source != -1 && source == slot->source && source_kind == slot->source_kind) || slot->active == 0) {
            candidate = slot;
            if (source == -1 || source == slot->source) {
                break;
            }
        } else if (slot->timestamp < oldest_time) {
            oldest_time = slot->timestamp;
            oldest = (int16_t)i;
        }
    }
    if (candidate != 0) {
        return candidate;
    }
    return &record->messages[oldest];
}

#if 0
Original Ghidra decompilation (0x4ae480):

int * FUN_004ae480(int param_1,char param_2)

{
  int iVar1;
  ushort uVar2;
  int *piVar3;
  ushort uVar4;
  int *piVar5;
  int unaff_ESI;
  int iVar6;

  uVar2 = 0;
  uVar4 = 0;
  iVar6 = 0x7fffffff;
  piVar5 = (int *)0x0;
  do {
    piVar3 = (int *)((short)uVar4 * 0x8c + param_1);
    if ((((unaff_ESI == -1) || (unaff_ESI != piVar3[0x21])) ||
        (param_2 != *(char *)((int)piVar3 + 0x8a))) && (*(char *)((int)piVar3 + 0x82) != '\0')) {
      iVar1 = *piVar3;
      piVar3 = piVar5;
      if (iVar1 < iVar6) {
        iVar6 = iVar1;
        uVar2 = uVar4;
      }
    }
    else if ((unaff_ESI == -1) || (unaff_ESI == piVar3[0x21])) break;
    uVar4 = uVar4 + 1;
    piVar5 = piVar3;
  } while (uVar4 < 4);
  if (piVar3 != (int *)0x0) {
    return piVar3;
  }
  return (int *)((short)uVar2 * 0x8c + param_1);
}
#endif
