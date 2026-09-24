// command_line_parse_to_argv  (Ghidra: command_line_parse_to_argv, already named)
// address 0x5425f0, size 352 bytes
// name confidence: 0.65   rewrite confidence: 0.55
// evidence: tokenizes on '-'-prefixed flags and whitespace, converts '"' to a literal space,
// GlobalAlloc's an argv-style array whose [0] is the shared empty string DAT_0065512c.
// register convention: command line in EDI (unaff_EDI in Ghidra), count out-pointer on the
// stack.
// blam-cc: EDI -> command_line (1st parameter), stack arg -> out_count (2nd parameter)
// UNSURE: kept as a literal transliteration of the decompile (including the odd
// per-iteration re-index in the whitespace-skip loops, which Ghidra reconstructed from register
// reuse) rather than a simplified rewrite, to avoid silently changing edge-case behaviour on
// malformed input.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern char empty_string_0065512c; // 0x0065512c, the shared MSVC empty std::string/char literal
extern int32_t _isspace(int32_t c);
extern void *GlobalAlloc(uint32_t flags, uint32_t size);

// Tokenizes the raw Halo command line into a GlobalAlloc'd argv-style array of substring
// pointers, writing the token count out through out_count. argv[0] is always the shared empty
// string; the real tokens start at argv[1].
char **command_line_parse_to_argv(char *command_line, int32_t *out_count)
{
    char *p;
    int32_t length;
    int32_t i;
    int32_t j;
    int32_t token_count;
    uint8_t in_flag;
    char **argv;
    char **out;
    char c;

    *out_count = 0;
    argv = 0;

    if (command_line == 0) {
        return argv;
    }

    in_flag = 0;
    p = command_line;
    do {
        c = *p;
        p++;
    } while (c != '\0');
    length = (int32_t)(p - (command_line + 1));

    // Pass 1: isolate '-flag' tokens at word boundaries and turn '"' into a literal space.
    i = 0;
    if (length > 0) {
        do {
            if (command_line[i] == '-') {
                in_flag = 1;
                if (i != 0 && (command_line[i - 1] == '\0' || _isspace((uint8_t)command_line[i - 1]) != 0)) {
                    command_line[i - 1] = '\0';
                }
            } else if (_isspace((uint8_t)command_line[i]) != 0 && in_flag) {
                command_line[i] = '\0';
                in_flag = 0;
            }
            if (command_line[i] == '"') {
                command_line[i] = ' ';
            }
            i++;
        } while (i < length);
    }

    // Pass 2: count the resulting non-empty tokens (maximal runs of non-null bytes).
    i = 0;
    token_count = 0;
    if (length > 0) {
        do {
            if (command_line[i] == '\0') {
                i++;
            } else {
                token_count++;
                do {
                    j = i + 1;
                    i++;
                } while (command_line[j] != '\0');
            }
        } while (i < length);
    }

    *out_count = token_count + 1;
    argv = (char **)GlobalAlloc(0, (uint32_t)(token_count + 1) * 4);
    argv[0] = &empty_string_0065512c;

    // Pass 3: record each token's start pointer, then trim its trailing whitespace in place.
    i = 0;
    if (length > 0) {
        out = argv + 1;
        do {
            if (command_line[i] == '\0') {
                i++;
            } else {
                while (_isspace((uint8_t)command_line[i]) != 0) {
                    i++;
                }
                *out = command_line + i;
                out++;

                c = command_line[i];
                while (c != '\0') {
                    j = i + 1;
                    i++;
                    c = command_line[j];
                }

                {
                    char *tail = command_line + i - 1;
                    while (_isspace((uint8_t)*tail) != 0) {
                        *tail = '\0';
                        tail--;
                    }
                }
            }
        } while (i < length);
    }

    return argv;
}

#if 0
Original Ghidra decompilation (0x5425f0):


undefined4 * command_line_parse_to_argv(int *param_1)

{
  char *pcVar1;
  int iVar2;
  char cVar3;
  bool bVar4;
  undefined4 *puVar5;
  char *pcVar6;
  int iVar7;
  int iVar8;
  int *piVar9;
  int iVar10;
  char *unaff_EDI;
  
  *param_1 = 0;
  puVar5 = (undefined4 *)0x0;
  if (unaff_EDI != (char *)0x0) {
    bVar4 = false;
    pcVar6 = unaff_EDI;
    do {
      cVar3 = *pcVar6;
      pcVar6 = pcVar6 + 1;
    } while (cVar3 != '\0');
    iVar7 = (int)pcVar6 - (int)(unaff_EDI + 1);
    iVar10 = 0;
    if (0 < iVar7) {
      do {
        if (unaff_EDI[iVar10] == '-') {
          bVar4 = true;
          if ((iVar10 != 0) &&
             ((unaff_EDI[iVar10 + -1] == '\0' ||
              (iVar8 = _isspace((int)unaff_EDI[iVar10 + -1]), iVar8 != 0)))) {
            unaff_EDI[iVar10 + -1] = '\0';
          }
        }
        else {
          iVar8 = _isspace((int)unaff_EDI[iVar10]);
          if ((iVar8 != 0) && (bVar4)) {
            unaff_EDI[iVar10] = '\0';
            bVar4 = false;
          }
        }
        if (unaff_EDI[iVar10] == '\"') {
          unaff_EDI[iVar10] = ' ';
        }
        iVar10 = iVar10 + 1;
      } while (iVar10 < iVar7);
    }
    iVar10 = 0;
    iVar8 = 0;
    if (0 < iVar7) {
      do {
        if (unaff_EDI[iVar10] == '\0') {
          iVar10 = iVar10 + 1;
        }
        else {
          iVar8 = iVar8 + 1;
          do {
            iVar2 = iVar10 + 1;
            iVar10 = iVar10 + 1;
          } while (unaff_EDI[iVar2] != '\0');
        }
      } while (iVar10 < iVar7);
    }
    *param_1 = iVar8 + 1;
    puVar5 = GlobalAlloc(0,(iVar8 + 1) * 4);
    iVar10 = 0;
    *puVar5 = &DAT_0065512c;
    if (0 < iVar7) {
      piVar9 = puVar5 + 1;
      do {
        if (unaff_EDI[iVar10] == '\0') {
          iVar10 = iVar10 + 1;
        }
        else {
          for (iVar8 = _isspace((int)unaff_EDI[iVar10]); iVar8 != 0;
              iVar8 = _isspace((int)unaff_EDI[iVar8])) {
            iVar8 = iVar10 + 1;
            iVar10 = iVar10 + 1;
          }
          *piVar9 = (int)(unaff_EDI + iVar10);
          piVar9 = piVar9 + 1;
          cVar3 = unaff_EDI[iVar10];
          while (cVar3 != '\0') {
            iVar8 = iVar10 + 1;
            iVar10 = iVar10 + 1;
            cVar3 = unaff_EDI[iVar8];
          }
          pcVar6 = unaff_EDI + iVar10 + -1;
          iVar8 = _isspace((int)unaff_EDI[iVar10 + -1]);
          while (iVar8 != 0) {
            *pcVar6 = '\0';
            pcVar1 = pcVar6 + -1;
            pcVar6 = pcVar6 + -1;
            iVar8 = _isspace((int)*pcVar1);
          }
        }
      } while (iVar10 < iVar7);
    }
  }
  return puVar5;
}
#endif
