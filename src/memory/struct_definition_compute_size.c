// struct_definition_compute_size
// address 0x4d0d50, size 249 bytes
// name confidence: 0.85 (cea-pdb / callee-name evidence, and reused by name at every one of its
// six call sites across the module)
// rewrite confidence: 0.8
// evidence: struct_definition/struct_definition_field layout in types/memory.h; the struct_array
// (type 7) recursion is cross-checked against the identical pattern in struct_definition_encode
// and struct_definition_decode, all three of which reuse the nested list's terminator record's
// computed_size slot to hold the outer field's total size (see the file header note in
// struct_definition_encode.c for the full derivation).
// register convention: cdecl, all four arguments on the stack; no in_EAX/in_ECX/unaff_* register
// arguments observed.

#include "tags.h"
#include "memory.h"

// FIXED (objdump 0x4d0d6c..0x4d0e0e): the per-field size lives in EAX across iterations and is only assigned
// inside the type switch. A field skipped by the version test, or whose type is out of range, therefore records
// (and adds to the total) the PREVIOUS field's size -- for the very first field, the low 16 bits of the `fields`
// pointer that EAX was loaded with at 0x4d0d63. Only the low 16 bits of the total are ever stored.
void struct_definition_compute_size(struct_definition *definition, int16_t *out_size,
    struct_definition_field *fields, int16_t *out_field_count)
{
    struct_definition_field *field = fields;
    uint16_t total_size = 0;
    uint16_t field_size = (uint16_t)(uint32_t)fields;

    while (field->type != _struct_field_terminator) {
        if (field->minimum_version <= definition->version &&
            (definition->version <= field->maximum_version || field->maximum_version == 0)) {
            switch (field->type) {
            case _struct_field_unused:
            case _struct_field_data:
            case _struct_field_block:
                field_size = (uint16_t)field->count;
                break;
            case _struct_field_int16_array:
                field_size = (uint16_t)(field->count << 1);
                break;
            case _struct_field_int32_array:
                field_size = (uint16_t)(field->count << 2);
                break;
            case _struct_field_int64_array:
                field_size = (uint16_t)(field->count << 3);
                break;
            case _struct_field_string:
                field_size = (uint16_t)(field->count + 1);
                break;
            case _struct_field_variable_data:
                field_size = (uint16_t)(field->count + 2);
                break;
            case _struct_field_struct_array: {
                int16_t element_count = field->count;
                int16_t nested_size = 0;
                int16_t nested_field_count = 0;
                struct_definition_compute_size(definition, &nested_size, field + 1, &nested_field_count);
                field = field + nested_field_count; // lands on the nested list's terminator record
                field_size = (uint16_t)(element_count * nested_size + 2);
                break;
            }
            case _struct_field_terminator:
                field_size = 0;
                break;
            default:
                break;                          // 0x4d0d93 ja: keeps the previous size
            }
        }
        field->computed_size = (int16_t)field_size;
        field = field + 1;
        total_size = (uint16_t)(total_size + field_size);
    }

    if (out_field_count != 0) {
        int32_t distance = (int32_t)((uint8_t *)field - (uint8_t *)fields);
        *out_field_count = (int16_t)(distance / 10) + 1;
    }
    if (out_size != 0) {
        *out_size = (int16_t)total_size;
    }
}

#if 0
Original Ghidra decompilation (0x4d0d50):

void struct_definition_compute_size(int param_1,undefined2 *param_2,short *param_3,short *param_4)

{
  short sVar1;
  short *psVar2;
  short *psVar3;
  undefined4 in_ECX;
  undefined2 uVar4;
  short *psVar6;
  undefined4 local_4;
  int iVar5;

  psVar2 = param_3;
  iVar5 = 0;
  uVar4 = 0;
  sVar1 = *param_3;
  local_4 = in_ECX;
  psVar6 = param_3;
  psVar3 = param_3;
  while (sVar1 != 9) {
    if ((psVar6[2] <= *(short *)(param_1 + 10)) &&
       ((*(short *)(param_1 + 10) <= psVar6[3] || (psVar6[3] == 0)))) {
      switch(*psVar6) {
      case 0:
      case 1:
      case 8:
        psVar3 = (short *)(uint)(ushort)psVar6[1];
        break;
      case 2:
        psVar3 = (short *)(uint)(ushort)(psVar6[1] << 1);
        break;
      case 3:
        psVar3 = (short *)(uint)(ushort)(psVar6[1] << 2);
        break;
      case 4:
        psVar3 = (short *)(uint)(ushort)(psVar6[1] << 3);
        break;
      case 5:
        psVar3 = (short *)(uint)(ushort)(psVar6[1] + 1);
        break;
      case 6:
        psVar3 = (short *)(uint)(ushort)(psVar6[1] + 2);
        break;
      case 7:
        struct_definition_compute_size(param_1,&param_3,psVar6 + 5,&local_4);
        psVar3 = psVar6 + 1;
        psVar6 = psVar6 + (short)local_4 * 5;
        psVar3 = (short *)((ushort)(*psVar3 * (short)param_3) + 2);
        break;
      case 9:
        psVar3 = (short *)0x0;
      }
    }
    psVar6[4] = (short)psVar3;
    psVar6 = psVar6 + 5;
    iVar5 = iVar5 + (int)psVar3;
    uVar4 = (undefined2)iVar5;
    sVar1 = *psVar6;
  }
  if (param_4 != (short *)0x0) {
    iVar5 = (int)psVar6 - (int)psVar2;
    *param_4 = ((short)(iVar5 / 10) + (short)(iVar5 >> 0x1f) + 1) -
               (short)((longlong)iVar5 * 0x66666667 >> 0x3f);
  }
  if (param_2 != (undefined2 *)0x0) {
    *param_2 = uVar4;
  }
  return;
}
#endif
