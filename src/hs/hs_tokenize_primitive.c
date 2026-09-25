// hs_tokenize_primitive  (Ghidra: hs_tokenize_primitive, already named)
// address 0x4861b0, size 212 bytes
// name confidence: 0.9   rewrite confidence: 0.7
// evidence: CEA-PDB string match ("this quoted constant is unterminated."); reads either a
// double-quoted string (NUL-terminating it in place at the closing quote) or a bare word up to
// ')', ';', whitespace or a newline (left un-terminated -- hs_tokenize_nonprimitive terminates
// bare words itself, after calling skip_whitespace), then lowercases the token text unless
// hs_preserve_token_case is set.
// register convention: the cursor pointer is unrecognized by Ghidra (in_EAX) and the node
// index is unrecognized (in_ECX); by the blam-cc convention these are the first and second
// register slots, EAX and ECX.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern char * string_to_lowercase(char *string); // 0x004491e0

extern data_array *hs_syntax_data;         // 0x0087a474
extern char *hs_compiled_source;           // 0x006b14c0
extern char *hs_compile_error;             // 0x006b14d4
extern int32_t hs_compile_error_offset;    // 0x006b14d8
extern char hs_space_characters[2];        // 0x0065b660
extern char hs_newline_characters[2];      // 0x0065b664
extern uint8_t hs_preserve_token_case;     // 0x007102fd

// blam-cc: cursor pointer in EAX, node index in ECX
// Tokenizes a single primitive value (quoted string or bare word) starting at *cursor into the
// syntax node `node_index`, recording its source_offset and advancing *cursor past it.
void hs_tokenize_primitive(char **cursor, datum_index node_index)
{
    hs_syntax_node *node;
    char *start;
    char *p;
    char c;
    int16_t i;

    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * hs_syntax_data->size);
    start = *cursor;
    if (*start == '"') {
        *cursor = start + 1;
        node->source_offset = (int32_t)(start + 1 - hs_compiled_source);
        c = **cursor;
        while ((c != '\0') && (**cursor != '"')) {
            p = *cursor + 1;
            *cursor = p;
            c = *p;
        }
        if (**cursor == '\0') {
            hs_compile_error = "this quoted constant is unterminated.";
            hs_compile_error_offset = node->source_offset - 1;
        }
        **cursor = '\0';
        *cursor = *cursor + 1;
    } else {
        node->source_offset = (int32_t)(start - hs_compiled_source);
        if (**cursor != '\0') {
            for (;;) {
                c = **cursor;
                if ((c == ')') || (c == ';')) {
                    break;
                }
                i = 0;
                do {
                    if (c == hs_space_characters[i]) {
                        goto done;
                    }
                    i = i + 1;
                } while (i < 2);
                i = 0;
                do {
                    if (c == hs_newline_characters[i]) {
                        goto done;
                    }
                    i = i + 1;
                } while (i < 2);
                p = *cursor + 1;
                *cursor = p;
                if (*p == '\0') {
                    break;
                }
            }
        }
    }
done:
    if (hs_preserve_token_case == 0) {
        string_to_lowercase(hs_compiled_source + node->source_offset);
    }
}

#if 0
Original Ghidra decompilation (0x4861b0):

void hs_tokenize_primitive(void)

{
  int iVar1;
  char cVar2;
  int *in_EAX;
  short sVar3;
  uint in_ECX;
  char *pcVar4;

  iVar1 = *(int *)(DAT_0087a474 + 0x34) + (in_ECX & 0xffff) * 0x14;
  pcVar4 = (char *)*in_EAX;
  if (*pcVar4 == '\"') {
    *in_EAX = (int)(pcVar4 + 1);
    *(int *)(iVar1 + 0xc) = (int)(pcVar4 + 1) - DAT_006b14c0;
    cVar2 = *(char *)*in_EAX;
    while ((cVar2 != '\0' && (*(char *)*in_EAX != '\"'))) {
      pcVar4 = (char *)*in_EAX + 1;
      *in_EAX = (int)pcVar4;
      cVar2 = *pcVar4;
    }
    if (*(char *)*in_EAX == '\0') {
      DAT_006b14d4 = "this quoted constant is unterminated.";
      DAT_006b14d8 = *(int *)(iVar1 + 0xc) + -1;
    }
    *(undefined1 *)*in_EAX = 0;
    *in_EAX = *in_EAX + 1;
  }
  else {
    *(int *)(iVar1 + 0xc) = (int)pcVar4 - DAT_006b14c0;
    if (*(char *)*in_EAX != '\0') {
      while( true ) {
        cVar2 = *(char *)*in_EAX;
        if ((cVar2 == ')') || (cVar2 == ';')) break;
        sVar3 = 0;
        do {
          if (cVar2 == (&DAT_0065b660)[sVar3]) goto LAB_00486271;
          sVar3 = sVar3 + 1;
        } while (sVar3 < 2);
        sVar3 = 0;
        do {
          if (cVar2 == (&DAT_0065b664)[sVar3]) goto LAB_00486271;
          sVar3 = sVar3 + 1;
        } while (sVar3 < 2);
        pcVar4 = (char *)*in_EAX + 1;
        *in_EAX = (int)pcVar4;
        if (*pcVar4 == '\0') break;
      }
    }
  }
LAB_00486271:
  if (DAT_007102fd == '\0') {
    string_to_lowercase();
  }
  return;
}
#endif
