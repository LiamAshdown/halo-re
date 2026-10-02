// string_trim_whitespace  (Ghidra: string_trim_whitespace, already named)
// address 0x4e4040, size 91 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md ("In-place trims leading and trailing
// whitespace/newline characters from a string pointer, updating the pointer to the first
// non-whitespace character").
// register convention: EDI -> string_ptr (a pointer to the caller's own string pointer; both
// the trailing-trim and the leading-trim/advance operate through *string_ptr, and the final
// write updates *string_ptr itself, matching a `char **` in/out parameter).
//   // blam-cc: EDI -> string_ptr
// UNSURE: the leading-whitespace loop does not merely advance the pointer -- it overwrites each
// skipped whitespace/newline byte with NUL in place before advancing, which this rewrite
// preserves exactly even though it destroys those bytes in the original buffer.

#include "tags.h"
#include "memory.h"
#include <ctype.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Trims *string_ptr in place: walks back from the end, replacing trailing
// whitespace/'\n'/'\r' with NUL, then walks forward from the start, replacing each leading
// whitespace/'\n'/'\r' with NUL and advancing *string_ptr to the first character that is none of
// those.
void string_trim_whitespace(char **string_ptr) // blam-cc: EDI -> string_ptr
{
    char *end = *string_ptr;
    char *p;
    char *start;

    while (*end != 0) {
        end = end + 1;
    }
    while (1) {
        end = end - 1;
        if (!isspace((uint8_t)*end) && *end != '\n' && *end != '\r') {
            break;
        }
        *end = 0;
    }
    for (start = *string_ptr; isspace((uint8_t)*start) || *start == '\n' || *start == '\r'; start = start + 1) {
        *start = 0;
    }
    *string_ptr = start;
}

#if 0
Original Ghidra decompilation (0x4e4040), from tools/pack.py 0x4e4040:

void string_trim_whitespace(void)

{
  int iVar1;
  char *pcVar2;
  char *pcVar3;
  undefined4 *unaff_EDI;

  pcVar3 = (char *)*unaff_EDI;
  do {
    pcVar2 = pcVar3;
    pcVar3 = pcVar2 + 1;
  } while (*pcVar2 != '\0');
  while( true ) {
    pcVar2 = pcVar2 + -1;
    iVar1 = _isspace((int)*pcVar2);
    if (((iVar1 == 0) && (*pcVar2 != '\n')) && (*pcVar2 != '\r')) break;
    *pcVar2 = '\0';
  }
  for (pcVar3 = (char *)*unaff_EDI;
      ((iVar1 = _isspace((int)*pcVar3), iVar1 != 0 || (*pcVar3 == '\n')) || (*pcVar3 == '\r'));
      pcVar3 = pcVar3 + 1) {
    *pcVar3 = '\0';
  }
  *unaff_EDI = pcVar3;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
