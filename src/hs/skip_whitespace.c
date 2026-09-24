// skip_whitespace  (Ghidra: skip_whitespace, already named)
// address 0x486350, size 193 bytes
// name confidence: 0.9   rewrite confidence: 0.75
// evidence: CEA-PDB string match ("unterminated comment."); a 3-state scanner (plain text,
// line comment started with ';', block comment started with ";*") advancing *cursor past
// whitespace (hs_space_characters/hs_newline_characters, types/hs.h) and both HS comment
// forms, ending a block comment on "*;".
// register convention: the cursor-pointer argument is unrecognized by Ghidra (in_EDX); by the
// blam-cc convention this is the third register slot, EDX.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern char hs_space_characters[2];   // 0x0065b660
extern char hs_newline_characters[2]; // 0x0065b664
extern char *hs_compile_error;        // 0x006b14d4

// blam-cc: cursor pointer in EDX
// Advances *cursor past whitespace and both line (";...") and block (";*...*;") HS comments.
void skip_whitespace(char **cursor)
{
    int16_t state; // 0 = plain text, 1 = line comment, 2 = block comment
    char *p;
    char c;
    int16_t i;

    state = 0;
top:
    if (state == 0) {
        p = *cursor;
        c = *p;
        if (c == ';') {
            *cursor = p + 1;
            state = 1;
            if (p[1] == '*') {
                state = 2;
                *cursor = p + 2;
            }
            goto top;
        }
        i = 0;
        for (;;) {
            if (c == hs_space_characters[i]) {
                goto found;
            }
            i = i + 1;
            if (!(i < 2)) {
                break;
            }
        }
        i = 0;
        while (c != hs_newline_characters[i]) {
            i = i + 1;
            if (1 < i) {
                return;
            }
        }
    found:
        *cursor = p + 1;
    } else if (state == 1) {
        p = *cursor;
        if (*p == '\0') {
            return;
        }
        i = 0;
        do {
            if (*p == hs_newline_characters[i]) {
                state = 0;
                break;
            }
            i = i + 1;
        } while (i < 2);
        *cursor = p + 1;
    } else {
        p = *cursor;
        if (*p == '\0') {
            hs_compile_error = "unterminated comment.";
            return;
        }
        if ((*p == '*') && (p[1] == ';')) {
            state = 0;
            *cursor = p + 1;
        }
        *cursor = *cursor + 1;
    }
    if (state == 3) {
        return; // dead: state is never assigned 3 anywhere above; kept for fidelity
    }
    goto top;
}

#if 0
Original Ghidra decompilation (0x486350):

void skip_whitespace(void)

{
  char cVar1;
  short sVar2;
  int *in_EDX;
  char *pcVar3;
  short sVar4;

  sVar4 = 0;
LAB_00486355:
  do {
    if (sVar4 == 0) {
      pcVar3 = (char *)*in_EDX;
      cVar1 = *pcVar3;
      if (cVar1 == ';') {
        *in_EDX = (int)(pcVar3 + 1);
        sVar4 = 1;
        if (pcVar3[1] == '*') {
          sVar4 = 2;
          *in_EDX = (int)(pcVar3 + 2);
        }
        goto LAB_00486355;
      }
      sVar2 = 0;
      do {
        if (cVar1 == (&DAT_0065b660)[sVar2]) goto LAB_004863a2;
        sVar2 = sVar2 + 1;
      } while (sVar2 < 2);
      sVar2 = 0;
      while (cVar1 != (&DAT_0065b664)[sVar2]) {
        sVar2 = sVar2 + 1;
        if (1 < sVar2) {
          return;
        }
      }
LAB_004863a2:
      *in_EDX = (int)(pcVar3 + 1);
    }
    else {
      if (sVar4 == 1) {
        pcVar3 = (char *)*in_EDX;
        if (*pcVar3 == '\0') {
          return;
        }
        sVar2 = 0;
        do {
          if (*pcVar3 == (&DAT_0065b664)[sVar2]) {
            sVar4 = 0;
            break;
          }
          sVar2 = sVar2 + 1;
        } while (sVar2 < 2);
        goto LAB_004863a2;
      }
      pcVar3 = (char *)*in_EDX;
      if (*pcVar3 == '\0') {
        DAT_006b14d4 = "unterminated comment.";
        return;
      }
      if ((*pcVar3 == '*') && (pcVar3[1] == ';')) {
        sVar4 = 0;
        *in_EDX = (int)(pcVar3 + 1);
      }
      *in_EDX = *in_EDX + 1;
    }
    if (sVar4 == 3) {
      return;
    }
  } while( true );
}
#endif
