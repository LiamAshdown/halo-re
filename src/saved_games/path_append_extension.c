// path_append_extension  (Ghidra: FUN_00555f20, renamed)
// address 0x555f20, size 84 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_functions.md summary "Appends a suffix onto a destination
// path buffer using a '.' separator, e.g. to add a file extension." Byte-for-byte identical to
// path_append_component (0x555ec0) except the separator character ('.' instead of '\\').
// register convention: destination buffer in ESI, suffix string in EBX (confirmed by objdump
// 0x555f20..0x555f73, identical shape to path_append_component's prologue).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"


// blam-cc: destination in ESI, suffix in EBX
void path_append_extension(char *destination, const char *suffix)
{
    char *end;

    if (*suffix != '\0') {
        end = destination;
        while (*end != '\0') {
            end++;
        }
        if (end != destination) {
            *end = '.';
            end++;
            *end = '\0';
        }
        strncpy(end, suffix, 0xff - (uint32_t)(end - destination));
        destination[0xff] = '\0';
    }
    return;
}

#if 0
Original Ghidra decompilation (0x555f20):

void FUN_00555f20(void)

{
  char cVar1;
  char *pcVar2;
  char *_Dest;
  char *unaff_EBX;
  char *unaff_ESI;

  pcVar2 = unaff_ESI;
  if (*unaff_EBX != '\0') {
    do {
      _Dest = pcVar2;
      pcVar2 = _Dest + 1;
    } while (*_Dest != '\0');
    if (_Dest != unaff_ESI) {
      *_Dest = '.';
      *pcVar2 = '\0';
      _Dest = pcVar2;
    }
    pcVar2 = unaff_ESI;
    do {
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
    _strncpy(_Dest,unaff_EBX,0xff - ((int)pcVar2 - (int)(unaff_ESI + 1)));
    unaff_ESI[0xff] = '\0';
  }
  return;
}
#endif
