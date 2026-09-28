// hs_report_expected_enum_values  (Ghidra: hs_report_expected_enum_values, already named)
// address 0x486dc0, size 460 bytes
// name confidence: 0.55 (out/phase4/hs_functions.md)
// rewrite confidence: 0.7
// evidence: out/phase4/hs_types_notes.md's hs_enum_definition section and its
//   hs_parse_primitive_procedures entry (0x20..0x24, the five enum-ish value types, all point
//   here); hs.h's hs_syntax_node (type at 0x04, source_offset at 0x0c, data at 0x10) and globals
//   list (hs_compiled_source 0x006b14c0, hs_compile_error 0x006b14d4, hs_compile_error_offset
//   0x006b14d8, hs_compile_error_buffer 0x006b14dc).
// register convention: none (void); node index is the recognized stack parameter (param_1).
// UNSURE: despite the name, this is the actual parser for the enum value types (matches the
//   token text against the enum's name list and stores the matched index), not merely an error
//   formatter; the "must be ... or ..." message is only the failure path. Kept the given name
//   since it is not a FUN_ stub and out/phase4/hs_types_notes.md does not list it as
//   misattributed, but this should be reconsidered as hs_parse_enum.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"


extern data_array *hs_syntax_data;                 // 0x0087a474
extern char *hs_compiled_source;                    // 0x006b14c0
extern char *hs_compile_error;                      // 0x006b14d4
extern int32_t hs_compile_error_offset;             // 0x006b14d8
extern char hs_compile_error_buffer[k_hs_error_buffer_size]; // 0x006b14dc
extern char *hs_type_names[k_hs_type_count];        // 0x00688a78
extern hs_enum_definition hs_enum_definitions[5];   // 0x0065b638 (FIXED: was 0x0065b634, 4 bytes into the previous entry; the original indexes 0x65b538 + type*8), indexed by (type - 0x20),
                                                     // see out/phase4/hs_types_notes.md

// Parses an enum-typed primitive token: looks the token's text up (case-insensitively) in the
// name list of its expected enum type (hs_type 0x20..0x24: game_difficulty, team,
// ai_default_state, actor_type, hud_corner). On a match, stores the matched index in the node
// and returns 1. On no match, formats a `"%s must be "a", "b" or "c"."` compiler error into
// hs_compile_error_buffer, points hs_compile_error at it, records the node's source offset as
// hs_compile_error_offset, and returns 0.
char hs_report_expected_enum_values(datum_index node_index)
{
    hs_syntax_node *node;
    hs_enum_definition *def;
    int16_t match_index;
    int16_t last_index;
    int16_t i;

    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);
    def = &hs_enum_definitions[node->type - 0x20];

    match_index = 0;
    if (0 < def->count) {
        do {
            if (_stricmp(hs_compiled_source + node->source_offset, def->names[match_index]) == 0)
                break;
            match_index = match_index + 1;
        } while (match_index < def->count);
    }

    if (match_index == def->count) {
        sprintf(hs_compile_error_buffer, "%s must be ", hs_type_names[node->type]);
        last_index = 0;
        if (0 < def->count - 1) {
            for (i = 0; i < def->count - 1; i++) {
                strcat(hs_compile_error_buffer, "\"");
                strcat(hs_compile_error_buffer, def->names[i]);
                strcat(hs_compile_error_buffer, "\", ");
                last_index = i + 1;
            }
        }
        if (1 < def->count) {
            strcat(hs_compile_error_buffer, "or ");
        }
        strcat(hs_compile_error_buffer, "\"");
        strcat(hs_compile_error_buffer, def->names[last_index]);
        strcat(hs_compile_error_buffer, "\".");

        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = node->source_offset;
        node->data.short_value = last_index;
        return 0;
    }

    node->data.short_value = match_index;
    return 1;
}

#if 0
Original Ghidra decompilation (0x486dc0):

undefined4 hs_report_expected_enum_values(uint param_1)

{
  char *pcVar1;
  int iVar2;
  undefined **ppuVar3;
  char cVar4;
  undefined *puVar5;
  short sVar6;
  int iVar7;
  char *pcVar8;
  uint uVar9;
  int iVar10;
  short sVar11;
  char *pcVar12;
  undefined2 *puVar13;
  char *pcVar14;
  undefined4 *puVar15;
  undefined2 *puVar16;

  iVar10 = (int)*(short *)(*(int *)(DAT_0087a474 + 0x34) + 4 + (param_1 & 0xffff) * 0x14);
  iVar2 = *(int *)(DAT_0087a474 + 0x34) + (param_1 & 0xffff) * 0x14;
  ppuVar3 = &PTR_LAB_0065b538 + iVar10 * 2;
  sVar11 = 0;
  if (0 < *(short *)ppuVar3) {
    do {
      iVar7 = __stricmp((char *)(*(int *)(iVar2 + 0xc) + DAT_006b14c0),
                        *(char **)((&PTR_LAB_0065b53c)[iVar10 * 2] + sVar11 * 4));
      if (iVar7 == 0) break;
      sVar11 = sVar11 + 1;
    } while (sVar11 < *(short *)ppuVar3);
  }
  if (sVar11 == *(short *)ppuVar3) {
    _sprintf(&DAT_006b14dc,"%s must be ",(&PTR_s_unparsed_00688a78)[*(short *)(iVar2 + 4)]);
    sVar11 = *(short *)ppuVar3;
    iVar7 = 0;
    sVar6 = 0;
    if (0 < sVar11 + -1) {
      do {
        puVar13 = (undefined2 *)((int)&DAT_006b14d8 + 3);
        do {
          pcVar1 = (char *)((int)puVar13 + 1);
          puVar13 = (undefined2 *)((int)puVar13 + 1);
        } while (*pcVar1 != '\0');
        *puVar13 = 0x22;
        pcVar1 = *(char **)((&PTR_LAB_0065b53c)[iVar10 * 2] + iVar7 * 4);
        pcVar8 = pcVar1;
        do {
          cVar4 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar4 != '\0');
        pcVar14 = (char *)((int)&DAT_006b14d8 + 3);
        do {
          pcVar12 = pcVar14 + 1;
          pcVar14 = pcVar14 + 1;
        } while (*pcVar12 != '\0');
        pcVar12 = pcVar1;
        for (uVar9 = (uint)((int)pcVar8 - (int)pcVar1) >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
          *(undefined4 *)pcVar14 = *(undefined4 *)pcVar12;
          pcVar12 = pcVar12 + 4;
          pcVar14 = pcVar14 + 4;
        }
        for (uVar9 = (int)pcVar8 - (int)pcVar1 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
          *pcVar14 = *pcVar12;
          pcVar12 = pcVar12 + 1;
          pcVar14 = pcVar14 + 1;
        }
        puVar15 = (undefined4 *)((int)&DAT_006b14d8 + 3);
        do {
          pcVar1 = (char *)((int)puVar15 + 1);
          puVar15 = (undefined4 *)((int)puVar15 + 1);
        } while (*pcVar1 != '\0');
        sVar6 = sVar6 + 1;
        iVar7 = (int)sVar6;
        *puVar15 = 0x202c22;
      } while (iVar7 < sVar11 + -1);
    }
    if (1 < *(short *)ppuVar3) {
      puVar15 = (undefined4 *)((int)&DAT_006b14d8 + 3);
      do {
        pcVar1 = (char *)((int)puVar15 + 1);
        puVar15 = (undefined4 *)((int)puVar15 + 1);
      } while (*pcVar1 != '\0');
      *puVar15 = 0x20726f;
    }
    puVar13 = (undefined2 *)((int)&DAT_006b14d8 + 3);
    do {
      pcVar1 = (char *)((int)puVar13 + 1);
      puVar13 = (undefined2 *)((int)puVar13 + 1);
    } while (*pcVar1 != '\0');
    puVar5 = (&PTR_LAB_0065b53c)[iVar10 * 2];
    *puVar13 = 0x22;
    pcVar1 = *(char **)(puVar5 + sVar6 * 4);
    pcVar8 = pcVar1;
    do {
      cVar4 = *pcVar8;
      pcVar8 = pcVar8 + 1;
    } while (cVar4 != '\0');
    pcVar14 = (char *)((int)&DAT_006b14d8 + 3);
    do {
      pcVar12 = pcVar14 + 1;
      pcVar14 = pcVar14 + 1;
    } while (*pcVar12 != '\0');
    pcVar12 = pcVar1;
    for (uVar9 = (uint)((int)pcVar8 - (int)pcVar1) >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
      *(undefined4 *)pcVar14 = *(undefined4 *)pcVar12;
      pcVar12 = pcVar12 + 4;
      pcVar14 = pcVar14 + 4;
    }
    for (uVar9 = (int)pcVar8 - (int)pcVar1 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
      *pcVar14 = *pcVar12;
      pcVar12 = pcVar12 + 1;
      pcVar14 = pcVar14 + 1;
    }
    puVar13 = (undefined2 *)((int)&DAT_006b14d8 + 3);
    do {
      puVar16 = puVar13;
      puVar13 = (undefined2 *)((int)puVar16 + 1);
    } while (*(char *)((int)puVar16 + 1) != '\0');
    *(undefined2 *)((int)puVar16 + 1) = 0x2e22;
    *(undefined1 *)((int)puVar16 + 3) = 0;
    DAT_006b14d4 = &DAT_006b14dc;
    DAT_006b14d8 = *(undefined4 *)(iVar2 + 0xc);
    *(short *)(iVar2 + 0x10) = sVar6;
    return 0;
  }
  *(short *)(iVar2 + 0x10) = sVar11;
  return 1;
}
#endif
