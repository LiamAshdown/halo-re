// input_guid_parse_ansi  (Ghidra: already named)
// address 0x491670, size 104 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: out/phase4/input_functions.md summary "Converts an ANSI device-id string to UTF-16
// and parses it into a CLSID via CLSIDFromString, returning whether the parse succeeded."; the
// only caller (test_input_device_defaults_find, 0x490090) confirms the ansi string is passed in
// ESI and the output GUID pointer on the stack (`push eax ; lea eax,[esp+4] ; mov esi,ecx ; push
// eax ; call input_guid_parse_ansi`).
// register convention: ansi string in ESI (unaff_ESI); output input_guid pointer on the stack.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"


// blam-cc: ansi string in ESI
// Widens ansi (up to 0x26 characters; longer strings are truncated to 0x26, matching the
// original bounds check) into a stack buffer and parses it with CLSIDFromString. Returns
// nonzero on success.
uint8_t input_guid_parse_ansi(input_guid *out_guid, char *ansi)
{
    int32_t length;
    int32_t i;
    uint16_t wide[40];
    int32_t hresult;

    length = 0;
    while (ansi[length] != '\0') {
        length = length + 1;
    }
    if (0x4e < (uint32_t)(length * 2 + 2)) {
        length = 0x26;
    }
    if ((uint32_t)(length * 2 + 2) < 0x4f) {
        wide[length] = 0;
        for (i = length - 1; i >= 0; i--) {
            wide[i] = (uint8_t)ansi[i];
        }
    }
    hresult = CLSIDFromString(wide, (LPCLSID)out_guid);
    return hresult >= 0;
}

#if 0
Original Ghidra decompilation (0x491670):

bool input_guid_parse_ansi(LPCLSID param_1)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  HRESULT HVar4;
  char *unaff_ESI;
  OLECHAR local_50 [40];

  pcVar2 = unaff_ESI;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  iVar3 = (int)pcVar2 - (int)(unaff_ESI + 1);
  if (0x4e < iVar3 * 2 + 2U) {
    iVar3 = 0x26;
  }
  if (iVar3 * 2 + 2U < 0x4f) {
    local_50[iVar3] = L'\0';
    while (iVar3 = iVar3 + -1, -1 < iVar3) {
      local_50[iVar3] = (ushort)(byte)unaff_ESI[iVar3];
    }
  }
  HVar4 = CLSIDFromString(local_50,param_1);
  return -1 < HVar4;
}
#endif
