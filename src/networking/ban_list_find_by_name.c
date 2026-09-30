// ban_list_find_by_name  (Ghidra: ban_list_find_by_name, already named)
// address 0x4e37d0, size 73 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/networking_types_notes.md "ban_list_entry" section: this function
// actually searches the +0x0d field (cd_key_hash), not the display name at +0x00 -- the
// existing name is what the note calls "slightly misleading", kept as-is per the task rules
// (only FUN_xxxxxx placeholders get renamed).
// register convention: __cdecl, one recognized parameter.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern growable_array ban_list; // 0x006b859c, element_size 0x38; see network_banlist_save.c

// Linear-searches the ban list for an entry whose stored CD-key hash (+0x0d) case-insensitively
// matches `key`. Returns the matching entry, or 0 if none matches.
ban_list_entry *ban_list_find_by_name(char *key)
{
    ban_list_entry *entries = (ban_list_entry *)ban_list.data;
    int32_t row = 0;
    int32_t i;

    if (0 < ban_list.count) {
        i = 0;
        do {
            if (_stricmp(entries[i].cd_key_hash, key) == 0) {
                return &entries[i];
            }
            row = row + 1;
            i = i + 1;
        } while (row < ban_list.count);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e37d0), from tools/pack.py 0x4e37d0:

int ban_list_find_by_name(char *param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;

  iVar3 = 0;
  if (0 < DAT_006b85a0) {
    iVar4 = 0;
    do {
      iVar1 = iVar4 + DAT_006b85a4;
      iVar2 = __stricmp((char *)(iVar1 + 0xd),param_1);
      if (iVar2 == 0) {
        return iVar1;
      }
      iVar3 = iVar3 + 1;
      iVar4 = iVar4 + 0x38;
    } while (iVar3 < DAT_006b85a0);
  }
  return 0;
}
#endif
