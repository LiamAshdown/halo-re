// hs_find_function_by_name  (Ghidra: hs_find_function_by_name, already named)
// address 0x483520, size 144 bytes
// name confidence: 0.9   rewrite confidence: 0.8
// evidence: linear scan of hs_function_definitions (0x20a entries, k_hs_hs_function_count) by
// name; the CEA string hints ('player_effect_set_max_ru' + the appended 'mble') and
// 'player_effect_set_max_vibrate' match hs_functions.md's "applying one legacy name-alias
// rewrite first". The inline byte-copy/scan that assembles "player_effect_set_max_rumble" onto
// the stack and finds its own NUL terminator is a compiler-unrolled strcpy; it is rewritten as
// one, which is byte-for-byte the same string the original code builds.
// register convention: the searched name is unrecognized by Ghidra (in_EDX); by the blam-cc
// convention this is the third register slot, EDX.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include <string.h>


extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58

// blam-cc: searched name in EDX
// Looks up a script function's table index by name, first rewriting the legacy alias
// "player_effect_set_max_rumble" to its current name "player_effect_set_max_vibrate".
int16_t hs_find_function_by_name(char *name)
{
    char alias[32];
    int16_t index;

    strcpy(alias, "player_effect_set_max_rumble");
    if (_stricmp(name, alias) == 0) {
        name = "player_effect_set_max_vibrate";
    }
    for (index = 0; index < k_hs_function_count; index++) {
        if (_stricmp(hs_function_definitions[index]->name, name) == 0) {
            return index;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x483520):

short hs_find_function_by_name(void)

{
  undefined4 *puVar1;
  int iVar2;
  char *in_EDX;
  short sVar3;
  char *pcVar4;
  char *pcVar5;
  undefined4 *puVar6;
  char local_80 [4];
  undefined1 auStack_7c [124];

  pcVar4 = "player_effect_set_max_ru";
  pcVar5 = local_80;
  for (iVar2 = 6; iVar2 != 0; iVar2 = iVar2 + -1) {
    *(undefined4 *)pcVar5 = *(undefined4 *)pcVar4;
    pcVar4 = pcVar4 + 4;
    pcVar5 = pcVar5 + 4;
  }
  *pcVar5 = *pcVar4;
  puVar1 = (undefined4 *)&stack0xffffff7f;
  do {
    puVar6 = puVar1;
    puVar1 = (undefined4 *)((int)puVar6 + 1);
  } while (*(char *)((int)puVar6 + 1) != '\0');
  *(undefined4 *)((int)puVar6 + 1) = 0x656c626d;
  *(undefined1 *)((int)puVar6 + 5) = 0;
  iVar2 = __stricmp(in_EDX,local_80);
  if (iVar2 == 0) {
    in_EDX = "player_effect_set_max_vibrate";
  }
  sVar3 = 0;
  do {
    iVar2 = __stricmp(*(char **)((&PTR_DAT_00688b58)[sVar3] + 4),in_EDX);
    if (iVar2 == 0) {
      return sVar3;
    }
    sVar3 = sVar3 + 1;
  } while (sVar3 < 0x20a);
  return -1;
}
#endif
