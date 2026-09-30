// network_game_search_results_add_or_update  (Ghidra: network_game_search_results_add_or_update,
// already named)
// address 0x4da7d0, size 582 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_types_notes.md "network_game_search_entry (0x130)" fully
// documents this function's field writes; every offset below is taken directly from that
// section and from types/networking.h's network_game_search_entry struct.
// register convention: the search table (9-entry array) is the sole cdecl stack parameter
// (`param_1`); the incoming announcement record arrives in EBX (unaff_EBX).
// blam-cc: EBX -> announcement, stack -> results
// UNSURE: `announcement`'s own layout has no declared type (types/networking.h explicitly
// leaves it undeclared -- see that header's note on the message-delta-decoded announcement
// record); accessed via raw offsets on a `uint8_t *` view, matching the type notes' own
// convention.
// Return: only AL is defined (0 or 1), so a plain 0/1 return is equivalent.

// VERIFIED against disassembly 0x4da7d0..0x4daa16 (2026-09-30): FIXED: eviction scans for the first entry with joinable == 0 (+0x12c, cl at 0x4da8a0), not unknown_12f (+0x12f); everything else (expiry, identity match on dword 0, free slot, field copies/offsets, name fallback L"???", flags) matches
#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include <wchar.h>
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc

// blam-cc: EBX -> announcement, stack -> results
int32_t network_game_search_results_add_or_update(network_game_search_entry *results,
                                                    const uint8_t *announcement)
{
    large_integer counter;
    int32_t now_ms;
    int32_t i;
    int32_t slot;
    char joinable;
    network_game_search_entry *entry;
    const wchar_t *name_source;

    joinable = 1;
    if ((*(announcement + 0x15e) & 2) == 0 || *(const int16_t *)(announcement + 0x156) > 0xf) {
        joinable = 0;
    }

    // Expire stale entries.
    for (i = 0; i < 9; i = i + 1) {
        entry = &results[i];
        if (entry->in_use == 0) {
            memset(entry, 0, sizeof(network_game_search_entry));
            continue;
        }
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        if (6000 < now_ms - entry->received_ms) {
            memset(entry, 0, sizeof(network_game_search_entry));
        }
    }

    // Find an existing entry with the same identity.
    slot = -1;
    for (i = 0; i < 9; i = i + 1) {
        if (*(const uint32_t *)announcement == results[i].identity[0]) {
            slot = i;
            break;
        }
    }
    // Otherwise, take the first free slot.
    if (slot == -1) {
        for (i = 0; i < 9; i = i + 1) {
            if (results[i].in_use == 0) {
                slot = i;
                break;
            }
        }
    }
    // Otherwise, if the announced game is joinable, evict the first entry that is not joinable.
    if (slot == -1) {
        if (!joinable) {
            return 0;
        }
        for (i = 0; i < 9; i = i + 1) {
            if (results[i].joinable == 0) {
                memset(&results[i], 0, sizeof(network_game_search_entry));
                slot = i;
                break;
            }
        }
        if (slot == -1) {
            return 0;
        }
    }

    entry = &results[slot];
    entry->in_use = 1;
    entry->identity[0] = *(const uint32_t *)(announcement + 0x00);
    entry->identity[1] = *(const uint32_t *)(announcement + 0x04);
    entry->identity[2] = *(const uint32_t *)(announcement + 0x08);
    entry->identity[3] = *(const uint32_t *)(announcement + 0x0c);
    entry->identity[4] = *(const uint32_t *)(announcement + 0x10);
    entry->identity[5] = *(const uint32_t *)(announcement + 0x14);

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    entry->received_ms = now_ms;

    entry->unknown_12a = *(const int16_t *)(announcement + 0x1c);
    name_source = (const wchar_t *)(announcement + 0x1e);
    if (*name_source == L'\0') {
        name_source = L"???";
    }
    wcsncpy((wchar_t *)entry->name, name_source, 0x3f);
    entry->name[63] = 0;

    entry->game_engine_index = *(const int16_t *)(announcement + 0x154);
    for (i = 0; i < 0x21; i = i + 1) {
        entry->info[i] = *(const uint32_t *)(announcement + 0xd0 + i * 4);
    }
    entry->player_count = *(const int16_t *)(announcement + 0x156);
    entry->unknown_124 = *(const int16_t *)(announcement + 0x158);
    entry->unknown_126 = *(const int16_t *)(announcement + 0x15a);
    entry->unknown_128 = *(const int16_t *)(announcement + 0x15c);
    entry->joinable = joinable;
    entry->stats_logging = (*(announcement + 0x15e) >> 2) & 1;

    if (entry->game_engine_index == 3 && (*(announcement + 0x15e) & 8) != 0) {
        entry->unknown_12f = 1;
        return 1;
    }
    entry->unknown_12f = 0;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4da7d0):

uint network_game_search_results_add_or_update(int *param_1)

{
  int iVar1;
  uint uVar2;
  int *piVar3;
  int *unaff_EBX;
  int iVar4;
  int *piVar5;
  int *piVar6;
  undefined8 uVar7;
  wchar_t *_Source;
  char local_9;
  LARGE_INTEGER local_8;

  if (((*(byte *)((int)unaff_EBX + 0x15e) & 2) == 0) ||
     (local_9 = '\x01', 0xf < *(short *)((int)unaff_EBX + 0x156))) {
    local_9 = '\0';
  }
  piVar3 = param_1 + 6;
  iVar4 = 9;
  do {
    if (*(char *)((int)piVar3 + 0x115) == '\0') {
LAB_004da849:
      piVar5 = piVar3 + -6;
      for (iVar1 = 0x4c; iVar1 != 0; iVar1 = iVar1 + -1) {
        *piVar5 = 0;
        piVar5 = piVar5 + 1;
      }
    }
    else {
      QueryPerformanceCounter(&local_8);
      uVar7 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
      iVar1 = __alldiv(uVar7,DAT_006ac8f8,DAT_006ac8fc);
      if (6000 < iVar1 - *piVar3) goto LAB_004da849;
    }
    piVar3 = piVar3 + 0x4c;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  iVar4 = 0;
  piVar3 = param_1;
  do {
    if (*unaff_EBX == *piVar3) goto LAB_004da8ce;
    iVar4 = iVar4 + 1;
    piVar3 = piVar3 + 0x4c;
  } while (iVar4 < 9);
  iVar4 = 0;
  piVar3 = param_1;
  do {
    if (*(char *)((int)piVar3 + 0x12d) == '\0') goto LAB_004da8ce;
    iVar4 = iVar4 + 1;
    piVar3 = piVar3 + 0x4c;
  } while (iVar4 < 9);
  uVar2 = CONCAT31((int3)((uint)iVar4 >> 8),local_9);
  if (local_9 != '\0') {
    uVar2 = 0;
    piVar3 = param_1;
    do {
      if ((char)piVar3[0x4b] == '\0') {
        piVar5 = piVar3;
        for (iVar4 = 0x4c; iVar4 != 0; iVar4 = iVar4 + -1) {
          *piVar5 = 0;
          piVar5 = piVar5 + 1;
        }
LAB_004da8ce:
        *(undefined1 *)((int)piVar3 + 0x12d) = 1;
        *piVar3 = *unaff_EBX;
        piVar3[1] = unaff_EBX[1];
        piVar3[2] = unaff_EBX[2];
        piVar3[3] = unaff_EBX[3];
        piVar3[4] = unaff_EBX[4];
        piVar3[5] = unaff_EBX[5];
        QueryPerformanceCounter(&local_8);
        uVar7 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
        iVar4 = __alldiv(uVar7,DAT_006ac8f8,DAT_006ac8fc);
        piVar3[6] = iVar4;
        _Source = (wchar_t *)((int)unaff_EBX + 0x1e);
        *(short *)((int)piVar3 + 0x12a) = (short)unaff_EBX[7];
        if (*_Source == L'\0') {
          _Source = L"???";
        }
        _wcsncpy((wchar_t *)(piVar3 + 7),_Source,0x3f);
        *(undefined2 *)((int)piVar3 + 0x9a) = 0;
        *(short *)(piVar3 + 0x48) = (short)unaff_EBX[0x55];
        piVar5 = unaff_EBX + 0x34;
        piVar6 = piVar3 + 0x27;
        for (iVar4 = 0x21; iVar4 != 0; iVar4 = iVar4 + -1) {
          *piVar6 = *piVar5;
          piVar5 = piVar5 + 1;
          piVar6 = piVar6 + 1;
        }
        *(undefined2 *)((int)piVar3 + 0x122) = *(undefined2 *)((int)unaff_EBX + 0x156);
        *(short *)(piVar3 + 0x49) = (short)unaff_EBX[0x56];
        *(undefined2 *)((int)piVar3 + 0x126) = *(undefined2 *)((int)unaff_EBX + 0x15a);
        *(short *)(piVar3 + 0x4a) = (short)unaff_EBX[0x57];
        *(char *)(piVar3 + 0x4b) = local_9;
        *(byte *)((int)piVar3 + 0x12e) = *(byte *)((int)unaff_EBX + 0x15e) >> 2 & 1;
        if (((short)piVar3[0x48] == 3) && ((*(byte *)((int)unaff_EBX + 0x15e) & 8) != 0)) {
          *(undefined1 *)((int)piVar3 + 0x12f) = 1;
          return 1;
        }
        *(undefined1 *)((int)piVar3 + 0x12f) = 0;
        return 1;
      }
      uVar2 = uVar2 + 1;
      piVar3 = piVar3 + 0x4c;
    } while ((int)uVar2 < 9);
  }
  return uVar2 & 0xffffff00;
}
#endif
