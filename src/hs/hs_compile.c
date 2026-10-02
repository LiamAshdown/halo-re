// hs_compile  (Ghidra: hs_compile, already named)
// address 0x485770, size 295 bytes
// name confidence: 0.9   rewrite confidence: 0.75
// evidence: called from hs_compile_source as
// hs_compile(source_file->source.size, source_file->source.pointer, &error_message,
// &error_offset) (ScenarioSourceFile offsets 0x20/0x2c = TagDataOffset.size/.pointer,
// confirmed in out/phase4/hs_types_notes.md); tokenizes and type-checks every top-level
// expression in the source text as an implicit _hs_type_special_form.
// register convention: __cdecl, all four parameters recognized directly by Ghidra.
// UNSURE: `success` is read before being assigned on the very first loop check if the source
// text is immediately empty (Ghidra's local_9 is likewise only assigned inside the loop body),
// so that specific edge case is undefined behavior in the original binary too; preserved as-is
// rather than given a synthetic default.

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char *hs_source_buffer_append(char *text, uint32_t length); // 0x004856f0, this batch
extern void skip_whitespace(char **cursor); // 0x00486350, this batch
extern datum_index hs_tokenize(char **cursor); // 0x00486120, this batch
extern char hs_parse_nonprimitive(datum_index node_index); // 0x00486710, this batch
extern char hs_parse_primitive(datum_index node_index); // 0x00486480, this batch

extern char *hs_compile_error;             // 0x006b14d4
extern int32_t hs_compile_error_offset;    // 0x006b14d8
extern data_array *hs_syntax_data;         // 0x0087a474
extern uint8_t hs_syntax_data_dirty;       // 0x006b14d0
extern int32_t hs_compiled_source_length;  // 0x006b14bc

// Tokenizes and type-checks every top-level expression of `source_text` (source_length bytes,
// the source file's own recorded length) as an implicit special form, appending it to
// hs_compiled_source first. On the first failure, reports the error message/offset relative to
// this source file's own position in the combined buffer.
void hs_compile(int32_t source_length, char *source_text, char **error_message, int32_t *error_offset)
{
    char *cursor;
    char success;
    datum_index node_index;
    hs_syntax_node *node;

    cursor = hs_source_buffer_append(source_text, (uint32_t)source_length);
    if (cursor == 0) {
        *error_message = (char *)"couldn't allocate memory for compiled source.";
        return;
    }
    hs_compile_error = 0;
    *error_message = 0;
    *error_offset = 0;
    hs_compile_error_offset = -1;
    skip_whitespace(&cursor);
    do {
        if (*cursor == '\0') {
            if (success != 0) {
                return;
            }
            break;
        }
        node_index = hs_tokenize(&cursor);
        skip_whitespace(&cursor);
        if (hs_compile_error != 0) {
            break;
        }
        node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * hs_syntax_data->size);
        success = 1;
        if (node->type == 0) {
            node->type = _hs_type_special_form;
            if ((node->flags & _hs_syntax_node_primitive_bit) == 0) {
                success = hs_parse_nonprimitive(node_index);
            } else {
                node->index_union = _hs_type_special_form;
                success = hs_parse_primitive(node_index);
            }
        }
    } while (success != 0);
    *error_message = hs_compile_error;
    hs_syntax_data_dirty = 1;
    if (hs_compile_error_offset != -1) {
        hs_compile_error_offset = hs_compile_error_offset + (source_length - hs_compiled_source_length);
        *error_offset = hs_compile_error_offset + (int32_t)source_text;
    }
}

#if 0
Original Ghidra decompilation (0x485770):

void __cdecl
hs_compile(int source_length_field,int source_ptr_field,int *error_message,int *error_offset)

{
  int iVar1;
  char cVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  char *local_4;

  local_4 = (char *)hs_source_buffer_append(source_ptr_field);
  if (local_4 == (char *)0x0) {
    *error_message = (int)"couldn\'t allocate memory for compiled source.";
    return;
  }
  DAT_006b14d4 = 0;
  *error_message = 0;
  *error_offset = 0;
  cVar2 = '\x01';
  DAT_006b14d8 = -1;
  skip_whitespace();
  do {
    if (*local_4 == '\0') {
      if (cVar2 != '\0') {
        return;
      }
      break;
    }
    uVar3 = hs_tokenize(&local_4);
    skip_whitespace();
    iVar1 = DAT_0087a474;
    if (DAT_006b14d4 != 0) break;
    iVar5 = (uVar3 & 0xffff) * 0x14;
    iVar4 = *(int *)(DAT_0087a474 + 0x34) + iVar5;
    cVar2 = '\x01';
    if (*(short *)(iVar4 + 4) == 0) {
      *(undefined2 *)(iVar4 + 4) = 1;
      if ((*(byte *)(*(int *)(iVar1 + 0x34) + 6 + iVar5) & 1) == 0) {
        cVar2 = hs_parse_nonprimitive(uVar3);
      }
      else {
        *(undefined2 *)(iVar4 + 2) = 1;
        cVar2 = hs_parse_primitive();
      }
    }
  } while (cVar2 != '\0');
  *error_message = DAT_006b14d4;
  DAT_006b14d0 = 1;
  if (DAT_006b14d8 != -1) {
    DAT_006b14d8 = DAT_006b14d8 + (source_length_field - DAT_006b14bc);
    *error_offset = DAT_006b14d8 + source_ptr_field;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
