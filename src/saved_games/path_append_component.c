// path_append_component  (Ghidra: path_append_component, already named)
// address 0x555ec0, size 84 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: already named by Ghidra/CEA; out/phase4/saved_games_types_notes.md register
// conventions list: "path_append_component ESI destination, EBX component". If component is
// non-empty, appends a '\\' separator (unless destination is currently empty) and then as much
// of component as fits in a 0xff-character destination buffer, always NUL-terminating at
// destination[0xff].
// register convention: destination buffer in ESI, component string in EBX (confirmed by
// objdump 0x555ec0..0x555f13: `cmp byte[ebx],0` gates the whole body, `mov eax,esi` walks the
// destination to find its end).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"


// blam-cc: destination in ESI, component in EBX
void path_append_component(char *destination, const char *component)
{
    char *end;

    if (*component != '\0') {
        end = destination;
        while (*end != '\0') {
            end++;
        }
        if (end != destination) {
            *end = '\\';
            end++;
            *end = '\0';
        }
        strncpy(end, component, 0xff - (uint32_t)(end - destination));
        destination[0xff] = '\0';
    }
    return;
}

#if 0
Original Ghidra decompilation (0x555ec0):

void path_append_component(void)

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
      *_Dest = '\\';
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
