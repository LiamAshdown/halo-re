// hs_format_function_signature  (Ghidra: hs_format_function_signature, already named)
// address 0x484300, size 251 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: builds "(name [param_info | <type> <type> ...])" using the "(%s" / " %s" literals
// at 0x660f50/0x660f4c; uses hs_function_definition::param_info (offset 0x14) verbatim when
// non-NULL, otherwise formats each parameters[] entry as "<typename>" via hs_type_names,
// matching types/hs.h's field evidence exactly. The hand-rolled byte-shuffling append loops are
// a compiled-out sprintf/strcat; rewritten as such (proven equivalent: each one finds the
// current end of the buffer, splices in " <", copies the type name, then "> " -- exactly what
// `sprintf(end, " <%s>", name)` produces).
// register convention: the function index is unrecognized by Ghidra (in_AX) and the output
// buffer is unrecognized (in_ECX); by the blam-cc convention these are the first and second
// register slots, EAX (AX) and ECX.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include <stdio.h>
#include <string.h>

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_type_names[k_hs_type_count]; // 0x00688a78

// blam-cc: function index in AX (EAX), output buffer in ECX
// Formats a human-readable "(name arg<type> ...)" prototype string for hs_function_definitions
// entry `function_index` into `out`.
void hs_format_function_signature(int16_t function_index, char *out)
{
    hs_function_definition *def;
    char *end;
    int16_t i;

    def = hs_function_definitions[function_index];
    sprintf(out, "(%s", def->name);
    if (def->param_info != 0) {
        end = out + strlen(out);
        sprintf(end, " %s", def->param_info);
    } else {
        for (i = 0; i < def->parameter_count; i = i + 1) {
            end = out + strlen(out);
            sprintf(end, " <%s>", hs_type_names[def->parameters[i]]);
        }
    }
    end = out + strlen(out);
    end[0] = ')';
    end[1] = '\0';
}

#if 0
Original Ghidra decompilation (0x484300):

void hs_format_function_signature(void)

{
  char cVar1;
  undefined *puVar2;
  short in_AX;
  short sVar3;
  char *pcVar4;
  char *in_ECX;
  uint uVar5;
  char *pcVar6;
  char *pcVar7;
  char *pcVar8;

  puVar2 = (&PTR_DAT_00688b58)[in_AX];
  _sprintf(in_ECX,(char *)&PTR_DAT_00660f50,*(undefined4 *)(puVar2 + 4));
  pcVar6 = in_ECX;
  if (*(int *)(puVar2 + 0x14) == 0) {
    sVar3 = 0;
    if (0 < *(short *)(puVar2 + 0x1a)) {
      do {
        pcVar6 = in_ECX + -1;
        do {
          pcVar4 = pcVar6;
          pcVar6 = pcVar4 + 1;
        } while (pcVar4[1] != '\0');
        pcVar6[0] = ' ';
        pcVar6[1] = '<';
        pcVar4[3] = '\0';
        pcVar6 = (&PTR_s_unparsed_00688a78)[*(short *)(puVar2 + sVar3 * 2 + 0x1c)];
        pcVar4 = pcVar6;
        do {
          cVar1 = *pcVar4;
          pcVar4 = pcVar4 + 1;
        } while (cVar1 != '\0');
        pcVar8 = in_ECX + -1;
        do {
          pcVar7 = pcVar8 + 1;
          pcVar8 = pcVar8 + 1;
        } while (*pcVar7 != '\0');
        pcVar7 = pcVar6;
        for (uVar5 = (uint)((int)pcVar4 - (int)pcVar6) >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
          *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
          pcVar7 = pcVar7 + 4;
          pcVar8 = pcVar8 + 4;
        }
        for (uVar5 = (int)pcVar4 - (int)pcVar6 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
          *pcVar8 = *pcVar7;
          pcVar7 = pcVar7 + 1;
          pcVar8 = pcVar8 + 1;
        }
        pcVar6 = in_ECX + -1;
        do {
          pcVar4 = pcVar6 + 1;
          pcVar6 = pcVar6 + 1;
        } while (*pcVar4 != '\0');
        pcVar6[0] = '>';
        pcVar6[1] = '\0';
        sVar3 = sVar3 + 1;
      } while (sVar3 < *(short *)(puVar2 + 0x1a));
    }
  }
  else {
    do {
      pcVar4 = pcVar6;
      pcVar6 = pcVar4 + 1;
    } while (*pcVar4 != '\0');
    _sprintf(pcVar4,(char *)&PTR_DAT_00660f4c,*(int *)(puVar2 + 0x14));
  }
  pcVar6 = in_ECX + -1;
  do {
    pcVar4 = pcVar6 + 1;
    pcVar6 = pcVar6 + 1;
  } while (*pcVar4 != '\0');
  pcVar6[0] = ')';
  pcVar6[1] = '\0';
  return;
}
#endif
