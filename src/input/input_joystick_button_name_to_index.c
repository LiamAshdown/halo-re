// input_joystick_button_name_to_index  (Ghidra: FUN_004912e0)
// address 0x4912e0, size 82 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/input_types_notes.md: "0x4912e0 parses 'buttonN' (prefix 0x0065b8f0)."
// strstr is strstr(see src/interface/console_printf_verbose.c's precedent); objdump of
// 0x4912e0..0x491331 confirms the name string arrives in EAX (`push eax` as strstr's first
// arg), the needle is joystick_button_prefix "button" (0x0065b8f0), and the trailing digits are
// matched against decimal_suffixes[0x20][3] ("0".."31", stride 3, ending at 0x0065b9e8).
// register convention: name string in EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char joystick_button_prefix[0x18]; // 0x0065b8f0, "button"
extern char decimal_suffixes[0x20][3];    // 0x0065b988, "0" .. "31"

extern char *strstr(const char *haystack, const char *needle); // CRT strstr (0x625430: the MSVC asm strstr, haystack then needle; case-sensitive)
extern int32_t _stricmp(const char *a, const char *b);    // 0x628d8b, libc

// blam-cc: name in EAX
// Resolves a "buttonN" style joystick input name string (e.g. from a 'bind' command) back to
// its numeric button index (0..31), or -1 if name doesn't contain the "button" prefix or its
// suffix isn't one of the decimal strings "0".."31".
int16_t input_joystick_button_name_to_index(char *name)
{
    char *suffix;
    char *table_entry;
    int16_t index;

    suffix = strstr(name, joystick_button_prefix);
    if (suffix == (char *)0) {
        return -1;
    }
    suffix = suffix + 6; // strlen("button")
    index = 0;
    table_entry = decimal_suffixes[0];
    while (index < 0x20) { // table_entry < 0x0065b9e8 in the binary
        if (_stricmp(suffix, table_entry) == 0) {
            return index;
        }
        table_entry = table_entry + 3;
        index = index + 1;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4912e0):

short FUN_004912e0(void)

{
  int iVar1;
  int iVar2;
  char *_Str2;
  short sVar3;

  iVar1 = FUN_00625430();
  if (iVar1 == 0) {
    return -1;
  }
  sVar3 = 0;
  _Str2 = "0";
  do {
    iVar2 = __stricmp((char *)(iVar1 + 6),_Str2);
    if (iVar2 == 0) {
      return sVar3;
    }
    _Str2 = _Str2 + 3;
    sVar3 = sVar3 + 1;
  } while ((int)_Str2 < 0x65b9e8);
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
