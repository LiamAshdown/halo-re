// ui_button_prompt_index_from_string  (Ghidra: ui_button_prompt_index_from_string, already named)
// address 0x49ac30, size 66 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: matches the given name; linear-scans types/interface.h's documented
// ui_button_caption[0x28] table (first entry is the "a-button" token) doing a length-bounded
// case-insensitive wide compare against the input, returning the matching index or 0xffff.
// register convention: input string in EBX (unaff_EBX), unresolved register read.
// blam-cc: EBX -> text

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint16_t *ui_button_caption[0x28]; // 0x00692708, first entry is the a-button token


// blam-cc: EBX -> text
// Looks up which button-prompt token (e.g. "a-button") `text` begins with, returning its index
// into the button caption table or 0xFFFF if no token matches.
int16_t ui_button_prompt_index_from_string(uint16_t *text)
{
    uint16_t index = 0;

    do {
        uint32_t token_length = wcslen((const wchar_t *)ui_button_caption[index]);

        if (_wcsnicmp((const wchar_t *)text, (const wchar_t *)ui_button_caption[index], token_length) == 0) {
            break;
        }
        index = index + 1;
    } while (index < 0x28);

    if (index == 0x28) {
        return 0xffff;
    }
    return index;
}

#if 0
Original Ghidra decompilation (0x49ac30):

ushort ui_button_prompt_index_from_string(void)

{
  size_t _MaxCount;
  int iVar1;
  wchar_t *unaff_EBX;
  ushort uVar2;

  uVar2 = 0;
  do {
    _MaxCount = FUN_00625b7a((&PTR_u_a_button_00692708)[(short)uVar2]);
    iVar1 = __wcsnicmp(unaff_EBX,(wchar_t *)(&PTR_u_a_button_00692708)[(short)uVar2],_MaxCount);
    if (iVar1 == 0) break;
    uVar2 = uVar2 + 1;
  } while (uVar2 < 0x28);
  if (uVar2 == 0x28) {
    return 0xffff;
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
