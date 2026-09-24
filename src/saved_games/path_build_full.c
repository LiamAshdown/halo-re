// path_build_full  (Ghidra: FUN_005560d0, renamed)
// address 0x5560d0, size 152 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md summary "Builds a fully-qualified path into a
// destination buffer from a source path and a path-kind selector, prefixing a base directory or
// a relative-path marker as needed."; out/phase4/saved_games_types_notes.md register-conventions
// list: "path_build_full EAX source, EDX destination, CX location"; every file_reference_*
// caller in this module loads EAX = ref->path, EDX = &local_buffer, CX = ref->location before
// calling it (confirmed by objdump at each of those call sites). types/saved_games.h
// file_reference_location: 2 = absolute (copied as-is), 1 = root-relative (prefixed with the
// drive-root template file_root_template), anything < 1 = plain relative (prefixed with ".\\"
// unless source[1] is already ':' or '\\').
// register convention: source path in EAX, destination buffer in EDX, file_reference_location
// value in CX (confirmed by objdump at every call site in this module).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern char file_root_template[4]; // 0x0069fa50

extern void _strncat(char *dest, const char *source, uint32_t count); // CRT

// blam-cc: source in EAX, destination in EDX, location in CX
// Builds a fully-qualified path into destination from source, according to location:
//   _file_location_absolute (2): destination is source, copied byte for byte.
//   _file_location_root (1, or anything > 2): destination is file_root_template followed by
//     source, strncat-bounded to 0xfb characters after the template.
//   anything else (_file_location_relative and below): destination is ".\\" followed by
//     source, UNLESS source already begins with a drive letter or backslash at index 1 (i.e.
//     source[1] == '\\' or ':'), in which case destination is source with no prefix.
void path_build_full(char *source, char *destination, int16_t location)
{
    char *cursor;
    char c;
    int32_t offset;
    uint32_t length;
    uint32_t i;

    *destination = '\0';
    if (location == 2) {
        offset = (int32_t)(destination - source);
        do {
            c = *source;
            source[offset] = c;
            source++;
        } while (c != '\0');
        return;
    }
    if (location < 1) {
        cursor = source;
        if (source[1] != '\\' && source[1] != ':') {
            destination[0] = '.';
            destination[1] = '\\';
            destination[2] = '\0';
        }
        do {
            c = *cursor;
            cursor++;
        } while (c != '\0');
        length = (uint32_t)(cursor - source);
        cursor = destination - 1;
        do {
            cursor++;
        } while (*cursor != '\0');
        for (i = length >> 2; i != 0; i--) {
            *(uint32_t *)cursor = *(uint32_t *)source;
            source += 4;
            cursor += 4;
        }
        for (i = length & 3; i != 0; i--) {
            *cursor = *source;
            source++;
            cursor++;
        }
        return;
    }
    {
        char *dst;
        char *src;

        src = file_root_template;
        dst = destination;
        do {
            c = *src;
            *dst = c;
            src++;
            dst++;
        } while (c != '\0');
    }
    _strncat(destination, source, 0xfb);
    return;
}

#if 0
Original Ghidra decompilation (0x5560d0):

void FUN_005560d0(void)

{
  char *pcVar1;
  char cVar2;
  char *in_EAX;
  uint uVar3;
  short in_CX;
  char *pcVar4;
  uint uVar5;
  char *in_EDX;
  int iVar6;

  *in_EDX = '\0';
  if (in_CX == 2) {
    iVar6 = (int)in_EDX - (int)in_EAX;
    do {
      cVar2 = *in_EAX;
      in_EAX[iVar6] = cVar2;
      in_EAX = in_EAX + 1;
    } while (cVar2 != '\0');
    return;
  }
  if (in_CX < 1) {
    pcVar4 = in_EAX;
    if ((in_EAX[1] != '\\') && (in_EAX[1] != ':')) {
      in_EDX[0] = '.';
      in_EDX[1] = '\\';
      in_EDX[2] = '\0';
    }
    do {
      cVar2 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar2 != '\0');
    uVar3 = (int)pcVar4 - (int)in_EAX;
    pcVar4 = in_EDX + -1;
    do {
      pcVar1 = pcVar4 + 1;
      pcVar4 = pcVar4 + 1;
    } while (*pcVar1 != '\0');
    for (uVar5 = uVar3 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
      *(undefined4 *)pcVar4 = *(undefined4 *)in_EAX;
      in_EAX = in_EAX + 4;
      pcVar4 = pcVar4 + 4;
    }
    for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
      *pcVar4 = *in_EAX;
      in_EAX = in_EAX + 1;
      pcVar4 = pcVar4 + 1;
    }
    return;
  }
  pcVar4 = &DAT_0069fa50;
  do {
    cVar2 = *pcVar4;
    (in_EDX + -0x69fa50)[(int)pcVar4] = cVar2;
    pcVar4 = pcVar4 + 1;
  } while (cVar2 != '\0');
  _strncat(in_EDX,in_EAX,0xfb);
  return;
}
#endif
