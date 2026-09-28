// ban_list_get_or_add_entry  (Ghidra: ban_list_get_or_add_entry, already named)
// address 0x4e3890, size 117 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/networking_types_notes.md "ban_list_entry" section: "0x4e35c0 passes the
// 12-character name from FUN_00557950 as the first argument and the 32-character identifier
// from gcd_getkeyhash as the second"; ban_list_entry (name[13]/cd_key_hash[33]).
// register convention: __cdecl (Ghidra recognized both parameters).
// UNSURE: growable_array_add_element (src/memory/growable_array_add_element.c) takes its array
// pointer in ESI; ban_list_get_or_add_entry's own disassembly was not independently re-checked
// for how it sets ESI up before this call (ban_list.data/.count already work correctly for the
// find and manual-index paths, which is the part this batch's other ban-list files depend on).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern growable_array ban_list; // 0x006b859c, element_size 0x38; see network_banlist_save.c

extern ban_list_entry *ban_list_find_by_name(char *key); // this batch, 0x4e37d0
extern uint32_t growable_array_add_element(growable_array *array); // 0x4cf810, ESI -> array

// Finds the existing ban-list entry for `cd_key_hash`, or appends and initializes a new one
// storing `name` (up to 12 characters) and `cd_key_hash` (up to 32 characters). Returns 0 if the
// list could not grow.
ban_list_entry *ban_list_get_or_add_entry(char *name, char *cd_key_hash)
{
    ban_list_entry *entry;
    int32_t index;

    entry = ban_list_find_by_name(cd_key_hash);
    if (entry != 0) {
        return entry;
    }
    index = growable_array_add_element(&ban_list);
    if (index != -1) {
        entry = &((ban_list_entry *)ban_list.data)[index];
        strncpy(entry->name, name, 0xc);
        entry->name[0xc] = 0;
        strncpy(entry->cd_key_hash, cd_key_hash, 0x20);
        entry->cd_key_hash[0x20] = 0;
        entry->indefinite = 0;
        entry->ban_count = 0;
        entry->expiry_time = 0;
        return entry;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e3890), from tools/pack.py 0x4e3890:

char * __cdecl ban_list_get_or_add_entry(char *param_1,char *param_2)

{
  char *pcVar1;
  int iVar2;

  pcVar1 = (char *)ban_list_find_by_name(param_2);
  if (pcVar1 != (char *)0x0) {
    return pcVar1;
  }
  iVar2 = growable_array_add_element();
  if (iVar2 != -1) {
    pcVar1 = (char *)(iVar2 * 0x38 + DAT_006b85a4);
    _strncpy(pcVar1,param_1,0xc);
    pcVar1[0xc] = '\0';
    _strncpy(pcVar1 + 0xd,param_2,0x20);
    pcVar1[0x2d] = '\0';
    pcVar1[0x30] = '\0';
    pcVar1[0x2e] = '\0';
    pcVar1[0x2f] = '\0';
    pcVar1[0x34] = '\0';
    pcVar1[0x35] = '\0';
    pcVar1[0x36] = '\0';
    pcVar1[0x37] = '\0';
    return pcVar1;
  }
  return (char *)0x0;
}
#endif
