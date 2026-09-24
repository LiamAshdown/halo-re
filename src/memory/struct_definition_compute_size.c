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

void struct_definition_compute_size(struct_definition *definition, int16_t *out_size,
    struct_definition_field *fields, int16_t *out_field_count)
{
    struct_definition_field *field = fields;
    int32_t total_size = 0;
    uint16_t last_size = 0; // Ghidra: uVar4, truncated (short) running total, i.e. total_size at
                             // the point the terminator is reached -- used only for *out_size.

    while (field->type != _struct_field_terminator) {
        int32_t field_size = 0; // Ghidra: psVar3, reused both as an integer and (case 7 only) as
                                 // a genuine struct_definition_field* -- split into separate
                                 // variables here (nested_fields) for clarity; no behavior change.

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
                struct_definition_field *nested_fields = field + 1;
                int16_t element_count = field->count; // Ghidra: psVar3 = psVar6 + 1, i.e. this
                    // is read from the struct_array field's OWN record, captured before `field`
                    // gets reassigned below.
                int16_t nested_size = 0;
                int16_t nested_field_count = 0;
                struct_definition_compute_size(definition, &nested_size, nested_fields,
                    &nested_field_count);
                field = field + nested_field_count; // struct_definition_field* stride already
                    // matches the original "psVar6 + (short)local_4 * 5" short-pointer math;
                    // lands on the nested list's terminator record (see struct_definition_encode.c
                    // for the full derivation of why).
                field_size = (uint16_t)(element_count * nested_size) + 2;
                break;
            }
            case _struct_field_terminator:
                field_size = 0;
                break;
            }
        }
        field->computed_size = (int16_t)field_size; // written through the (possibly reassigned,
            // struct_array case) `field` pointer, exactly like the original's `psVar6[4] = ...`
            // executing after the switch.
        field = field + 1;
        total_size = total_size + field_size;
        last_size = (uint16_t)total_size;
    }

    if (out_field_count != 0) {
        int32_t distance = (int32_t)((uint8_t *)field - (uint8_t *)fields);
        *out_field_count = (int16_t)(distance / 10) + 1; // UNSURE: literal port of Ghidra's
            // "(short)(iVar5/10) + (short)(iVar5>>0x1f) + 1 - (short)((longlong)iVar5*0x66666667
            // >> 0x3f)" -- that is a compiler-generated signed-division-by-10 idiom; for the
            // always-non-negative distances this function sees it reduces to distance/10 + 1,
            // which is what is written here.
    }
    if (out_size != 0) {
        *out_size = (int16_t)last_size;
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
