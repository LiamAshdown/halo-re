// ui_string_has_button_prompt_token  (Ghidra: ui_string_has_button_prompt_token, already named)
// address 0x49ada0, size 50 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: matches the given name; scans for each '%' in the string and asks
// ui_button_prompt_index_from_string whether what follows it is a recognised button token,
// stopping at the first match. Ghidra's decompile passes the text on to that callee with no
// visible argument; ui_button_prompt_index_from_string's own register convention (EBX -> text)
// is assumed to be fed by a compiler-generated EAX->EBX move at the call site.
// register convention: text in EAX (in_EAX), unresolved register read.
// blam-cc: EAX -> text

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

extern int16_t ui_button_prompt_index_from_string(uint16_t *text); // 0x49ac30, blam-cc: EBX -> text; -1 when no token matches

// blam-cc: EAX -> text
// Returns true if `text` contains at least one recognised "%buttonname" prompt-substitution
// token (a '%' immediately followed by a known button caption prefix).
uint8_t ui_string_has_button_prompt_token(uint16_t *text)
{
    uint16_t *percent;

    while (text != (uint16_t *)0 && (percent = (uint16_t *)wcschr((const wchar_t *)text, L'%')) != (uint16_t *)0) {
        text = percent + 1;
        if (ui_button_prompt_index_from_string(text) != 0xffff) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x49ada0):

undefined4 ui_string_has_button_prompt_token(void)

{
  short sVar1;
  wchar_t *in_EAX;
  wchar_t *pwVar2;

  while ((in_EAX != (wchar_t *)0x0 && (pwVar2 = _wcschr(in_EAX,L'%'), pwVar2 != (wchar_t *)0x0))) {
    in_EAX = pwVar2 + 1;
    sVar1 = ui_button_prompt_index_from_string();
    if (sVar1 != -1) {
      return 1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
