// struct_definition_encode
// address 0x4d0e80, size 1258 bytes
// name confidence: 0.85 (matches cea-pdb hint set and the well-evidenced struct_definition family)
// rewrite confidence: 0.55 -- this is the most register-heavy function in the module; several
// callee arguments are elided by Ghidra (it shows e.g. "FUN_004d0700();" with no argument list)
// because those callees expect values already resident in specific registers across this
// function's whole body rather than freshly loaded at the call site. Every such call is marked
// UNSURE below with the reconstruction reasoning; the reconstructed values are the only ones
// consistent with (a) the callee's own documented register convention (see
// out/phase4/memory_types_notes.md) and (b) the values Ghidra shows being computed immediately
// before the elided call and never used again except through it.
// evidence: struct field layout from struct_definition/struct_definition_field in types/memory.h;
// parameter roles cross-checked against the sibling function struct_definition_compute_size
// (0x4d0d50, identical fields/version-filter logic) and the recursive self-call at the bottom of
// case 7, which pins down param_1..param_7 unambiguously (definition, output stream, version,
// source data, out-source-size, fields, out-field-count).
// register convention: struct_definition_encode itself takes all seven arguments on the stack
// (Ghidra shows no in_EAX/in_ECX/unaff_* for its own parameters). The callees it invokes with
// elided argument lists (byte_stream_write_ranged_integer at 0x4d0700, byte_stream_write_string
// at 0x4d07e0, byte_swap_array at 0x4cfd90) all expect the byte_stream in ESI; this function
// evidently keeps `output` resident in ESI for its entire body (a whole-function register
// allocation), which is why Ghidra shows those calls with the stream argument simply missing.

#include "tags.h"
#include "memory.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: struct_definition_compute_size(definition, out_size, fields, out_field_count)
extern void struct_definition_compute_size(struct_definition *definition, int16_t *out_size,
    struct_definition_field *fields, int16_t *out_field_count);

// blam-cc (0x4d0700, below this batch's assigned range): maximum in EAX, value in EDX, stream in
// ESI. Chooses a 1/2/4-byte big-endian encoding wide enough for `maximum` and writes `value`.
extern uint32_t byte_stream_write_ranged_integer(int32_t maximum, uint32_t value, byte_stream *stream);

// blam-cc (0x4d07e0, below this batch's assigned range): max_length in CX, stream in ESI, string
// on the stack.
extern uint32_t byte_stream_write_string(char *string, int16_t max_length, byte_stream *stream);

// blam-cc (0x4cfd90, below this batch's assigned range): element size in EAX (2, 4 or 8), base
// pointer in ECX, element count in EDX.
extern void byte_swap_array(int32_t element_size, uint32_t *base, int32_t element_count);

void struct_definition_encode(struct_definition *definition, byte_stream *output, int16_t version,
    void *source, int16_t *out_source_size, struct_definition_field *fields,
    int16_t *out_field_count)
{
    struct_definition_field *field = fields;
    uint8_t *src_cursor = (uint8_t *)source;
    struct_definition_field *field_start;
    uint8_t *src_start;
    uint8_t *dest;
    uint32_t byte_count;
    uint16_t count;
    int16_t nested_field_count;
    int16_t nested_source_size;

    for (;;) {
        if (field->type == _struct_field_terminator) {
            if (out_field_count != 0) {
                int32_t distance = (int32_t)((uint8_t *)field - (uint8_t *)fields);
                *out_field_count = (int16_t)(distance / 10) + 1; // UNSURE: literal port of the
                    // same rounding-toward-zero-via-magic-constant expression compute_size uses;
                    // see struct_definition_compute_size.c for the identical pattern.
            }
            if (out_source_size != 0) {
                *out_source_size = (int16_t)((int16_t)(int32_t)src_cursor -
                    (int16_t)(int32_t)source); // UNSURE: Ghidra truncates both pointers to 16
                    // bits before subtracting (matches the int16_t out-param type); preserved
                    // exactly rather than widened to a true pointer difference.
            }
            return;
        }

        field_start = field;
        src_start = src_cursor;

        if (version < field->minimum_version ||
            (field->maximum_version < version && field->maximum_version != 0)) {
            // Field does not apply to this wire version: still reserve/zero its wire bytes so
            // later fields land at the expected offset, but never read from the source struct.
            switch (field->type) {
            case _struct_field_data:
            case _struct_field_int16_array:
            case _struct_field_int32_array:
            case _struct_field_int64_array:
            case _struct_field_block:
                byte_count = (uint32_t)(int32_t)field->count; // note: NOT scaled by element width, unlike
                    // the matching cases below -- a skipped field only reserves `count` bytes
                    // regardless of element size. Preserved as decompiled.
                if (output->size < (int32_t)(byte_count + output->cursor) || output->overflow != 0) {
                    goto overflow;
                }
                dest = output->data + output->cursor;
                memset(dest, 0, byte_count);
                output->cursor += byte_count;
                break;
            case _struct_field_string:
                if (output->size < output->cursor + 1 || output->overflow != 0) {
                    goto overflow;
                }
                output->data[output->cursor] = 0;
                output->cursor += 1;
                break;
            case _struct_field_variable_data:
            case _struct_field_struct_array:
                // UNSURE: Ghidra elides this call's arguments entirely (no locals are computed
                // first, unlike the matching case below). The only value that makes sense for a
                // skipped variable-length field is "zero elements", so this is reconstructed as
                // writing a zero count using the field's own count as the encoding's maximum.
                byte_stream_write_ranged_integer(field->count, 0, output);
                break;
            }
        } else {
            switch (field->type) {
            case _struct_field_data:
            case _struct_field_block:
                byte_count = (uint32_t)(int32_t)field->count;
                if (output->size < (int32_t)(byte_count + output->cursor) || output->overflow != 0) {
                    goto overflow;
                }
                dest = output->data + output->cursor;
                if (src_cursor == 0) {
                    memset(dest, 0, byte_count);
                } else {
                    memcpy(dest, src_cursor, byte_count);
                }
                output->cursor += byte_count;
                break;

            case _struct_field_int16_array: {
                uint16_t i;
                byte_count = (uint32_t)field->count * 2;
                if (output->size < (int32_t)(byte_count + output->cursor) || output->overflow != 0) {
                    goto overflow;
                }
                dest = output->data + output->cursor;
                if (src_cursor == 0) {
                    memset(dest, 0, byte_count);
                } else {
                    memcpy(dest, src_cursor, byte_count);
                }
                for (i = 0; i < field->count; i = i + 1) {
                    uint16_t *slot = (uint16_t *)dest + i;
                    *slot = (uint16_t)(((*slot & 0xff) << 8) | (*slot >> 8));
                }
                output->cursor += byte_count;
                break;
            }

            case _struct_field_int32_array:
                byte_count = (uint32_t)field->count * 4;
                if (output->size < (int32_t)(output->cursor + byte_count) || output->overflow != 0) {
                    goto overflow;
                }
                dest = output->data + output->cursor;
                if (src_cursor == 0) {
                    memset(dest, 0, byte_count);
                } else {
                    memcpy(dest, src_cursor, byte_count);
                }
                byte_swap_array(4, (uint32_t *)dest, field->count);
                output->cursor += byte_count;
                field = field_start;
                src_cursor = src_start;
                break;

            case _struct_field_int64_array:
                byte_count = (uint32_t)field->count * 8;
                if ((int32_t)(byte_count + output->cursor) <= output->size && output->overflow == 0) {
                    dest = output->data + output->cursor;
                    if (src_cursor == 0) {
                        memset(dest, 0, byte_count);
                    } else {
                        memcpy(dest, src_cursor, byte_count);
                    }
                    byte_swap_array(8, (uint32_t *)dest, field->count);
                    output->cursor += byte_count;
                    field = field_start;
                    src_cursor = src_start;
                    break;
                }
                goto overflow;

            case _struct_field_string:
                // UNSURE: max_length (CX) and stream (ESI) are elided; reconstructed as
                // field->count and `output` per byte_stream_write_string's own register
                // convention and this function's whole-body ESI == output allocation.
                byte_stream_write_string((char *)src_cursor, field->count, output);
                break;

            case _struct_field_variable_data: {
                uint8_t *elements;
                count = *(uint16_t *)src_cursor;
                elements = src_cursor + 2;
                if ((int16_t)count < 0 || field->count < (int16_t)count) {
                    count = 0;
                }
                byte_count = count;
                // UNSURE: EAX/EDX elided; reconstructed as (field->count, count) matching the
                // "maximum, value" convention documented for byte_stream_write_ranged_integer.
                byte_stream_write_ranged_integer(field->count, count, output);
                field = field_start;
                if (output->size < (int32_t)(byte_count + output->cursor) || output->overflow != 0) {
                    output->overflow = 1;
                } else {
                    dest = output->data + output->cursor;
                    if (elements == 0) { // UNSURE: Ghidra's own condition, ported literally; if
                        // src_cursor is NULL this reads elements=(uint8_t*)2, never NULL, so
                        // this branch is effectively unreachable as decompiled -- see the note in
                        // the file header about not "fixing" apparent source quirks.
                        memset(dest, 0, byte_count);
                    } else {
                        memcpy(dest, elements, byte_count);
                    }
                    output->cursor += byte_count;
                }
                break;
            }

            case _struct_field_struct_array: {
                int16_t requested_count = *(int16_t *)src_cursor;
                src_cursor = src_cursor + 2;
                struct_definition_compute_size(definition, 0, field + 1, &nested_field_count);
                if (requested_count < 0 || field->count < requested_count) {
                    requested_count = 0;
                }
                // UNSURE: EAX/EDX elided; same reconstruction as the variable_data case.
                byte_stream_write_ranged_integer(field->count, requested_count, output);
                if (0 < requested_count) {
                    int16_t remaining = requested_count;
                    do {
                        struct_definition_encode(definition, output, version, src_cursor,
                            &nested_source_size, field + 1, 0);
                        src_cursor = src_cursor + nested_source_size;
                        remaining = remaining - 1;
                    } while (remaining != 0);
                }
                field = field_start + nested_field_count; // lands on the nested list's
                    // terminator record, exactly like struct_definition_compute_size; the shared
                    // per-field advance below then moves past it using field_start's own
                    // computed_size (== requested_count * nested_size + 2, already cached by
                    // struct_definition_compute_size).
                src_cursor = src_start;
                break;
            }
            }
        }

        goto advance;

    overflow:
        // Ghidra LAB_004d11ee: set the flag, then `break` out of the switch -- control still
        // reaches the shared per-field advance below, so the walk continues to the terminator
        // with every remaining field short-circuited by the overflow test.
        output->overflow = 1;

    advance:
        {
            int16_t field_size = field->computed_size;
            field = field + 1;
            src_cursor = src_cursor + field_size;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d0e80):

void struct_definition_encode
               (undefined4 param_1,int *param_2,undefined4 param_3,ushort *param_4,short *param_5,
               short *param_6,short *param_7)

{
  short *psVar1;
  short sVar2;
  ushort uVar3;
  uint uVar4;
  uint uVar5;
  undefined4 *puVar6;
  short *psVar7;
  int iVar8;
  ushort *puVar9;
  undefined4 *puVar10;
  ushort *puVar11;
  short local_14 [2];
  short local_10 [2];
  short *local_c;
  ushort *local_8;
  ushort *local_4;

  sVar2 = *param_6;
  psVar7 = param_6;
  puVar11 = param_4;
  do {
    if (sVar2 == 9) {
      if (param_7 != (short *)0x0) {
        iVar8 = (int)psVar7 - (int)param_6;
        *param_7 = ((short)(iVar8 / 10) + (short)(iVar8 >> 0x1f) + 1) -
                   (short)((longlong)iVar8 * 0x66666667 >> 0x3f);
      }
      if (param_5 != (short *)0x0) {
        *param_5 = (short)puVar11 - (short)param_4;
      }
      return;
    }
    local_c = psVar7;
    local_8 = puVar11;
    if (((short)param_3 < psVar7[2]) || ((psVar7[3] < (short)param_3 && (psVar7[3] != 0)))) {
      switch(*psVar7) {
      case 1:
      case 2:
      case 3:
      case 4:
      case 8:
        uVar5 = (uint)psVar7[1];
        if ((param_2[2] < (int)(uVar5 + param_2[1])) || ((char)param_2[3] != '\0'))
        goto LAB_004d11ee;
        puVar6 = (undefined4 *)(*param_2 + param_2[1]);
        for (uVar4 = uVar5 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
          *puVar6 = 0;
          puVar6 = puVar6 + 1;
        }
        for (uVar4 = uVar5 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
          *(undefined1 *)puVar6 = 0;
          puVar6 = (undefined4 *)((int)puVar6 + 1);
        }
        param_2[1] = param_2[1] + uVar5;
        break;
      case 5:
        if ((param_2[2] < param_2[1] + 1) || ((char)param_2[3] != '\0')) goto LAB_004d11ee;
        *(undefined1 *)(*param_2 + param_2[1]) = 0;
        param_2[1] = param_2[1] + 1;
        break;
      case 6:
      case 7:
        FUN_004d0700();
      }
    }
    else {
      switch(*psVar7) {
      case 1:
        uVar5 = (uint)psVar7[1];
        if ((param_2[2] < (int)(param_2[1] + uVar5)) || ((char)param_2[3] != '\0'))
        goto LAB_004d11ee;
        puVar6 = (undefined4 *)(*param_2 + param_2[1]);
        if (puVar11 == (ushort *)0x0) {
          for (uVar4 = uVar5 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
            *puVar6 = 0;
            puVar6 = puVar6 + 1;
          }
          for (uVar4 = uVar5 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
            *(undefined1 *)puVar6 = 0;
            puVar6 = (undefined4 *)((int)puVar6 + 1);
          }
          param_2[1] = param_2[1] + uVar5;
        }
        else {
          puVar9 = puVar11;
          for (uVar4 = uVar5 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
            *puVar6 = *(undefined4 *)puVar9;
            puVar9 = puVar9 + 2;
            puVar6 = puVar6 + 1;
          }
          for (uVar4 = uVar5 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
            *(char *)puVar6 = (char)*puVar9;
            puVar9 = (ushort *)((int)puVar9 + 1);
            puVar6 = (undefined4 *)((int)puVar6 + 1);
          }
          param_2[1] = param_2[1] + uVar5;
        }
        break;
      case 2:
        uVar5 = (uint)psVar7[1];
        local_4 = (ushort *)(uVar5 * 2);
        if ((param_2[2] < (int)(param_2[1] + (int)local_4)) || ((char)param_2[3] != '\0'))
        goto LAB_004d11ee;
        puVar6 = (undefined4 *)(*param_2 + param_2[1]);
        if (puVar11 == (ushort *)0x0) {
          puVar10 = puVar6;
          for (uVar4 = (uVar5 & 0x7fffffff) >> 1; uVar4 != 0; uVar4 = uVar4 - 1) {
            *puVar10 = 0;
            puVar10 = puVar10 + 1;
          }
          for (uVar4 = (uint)local_4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
            *(undefined1 *)puVar10 = 0;
            puVar10 = (undefined4 *)((int)puVar10 + 1);
          }
        }
        else {
          puVar9 = puVar11;
          puVar10 = puVar6;
          for (uVar4 = (uVar5 & 0x7fffffff) >> 1; uVar4 != 0; uVar4 = uVar4 - 1) {
            *puVar10 = *(undefined4 *)puVar9;
            puVar9 = puVar9 + 2;
            puVar10 = puVar10 + 1;
          }
          for (uVar4 = (uint)local_4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
            *(char *)puVar10 = (char)*puVar9;
            puVar9 = (ushort *)((int)puVar9 + 1);
            puVar10 = (undefined4 *)((int)puVar10 + 1);
          }
        }
        if (0 < (int)uVar5) {
          do {
            uVar5 = uVar5 - 1;
            *(ushort *)puVar6 =
                 CONCAT11((char)*(undefined2 *)puVar6,(char)((ushort)*(undefined2 *)puVar6 >> 8));
            puVar6 = (undefined4 *)((int)puVar6 + 2);
          } while (uVar5 != 0);
        }
        param_2[1] = (int)(param_2[1] + (int)local_4);
        break;
      case 3:
        uVar5 = (uint)psVar7[1];
        local_4 = (ushort *)(uVar5 * 4);
        if ((param_2[2] < (int)(param_2[1] + (int)local_4)) || ((char)param_2[3] != '\0'))
        goto LAB_004d11ee;
        puVar6 = (undefined4 *)(*param_2 + param_2[1]);
        if (puVar11 == (ushort *)0x0) {
          for (uVar5 = uVar5 & 0x3fffffff; uVar5 != 0; uVar5 = uVar5 - 1) {
            *puVar6 = 0;
            puVar6 = puVar6 + 1;
          }
          for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
            *(undefined1 *)puVar6 = 0;
            puVar6 = (undefined4 *)((int)puVar6 + 1);
          }
        }
        else {
          for (uVar5 = uVar5 & 0x3fffffff; uVar5 != 0; uVar5 = uVar5 - 1) {
            *puVar6 = *(undefined4 *)puVar11;
            puVar11 = puVar11 + 2;
            puVar6 = puVar6 + 1;
          }
          for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
            *(char *)puVar6 = (char)*puVar11;
            puVar11 = (ushort *)((int)puVar11 + 1);
            puVar6 = (undefined4 *)((int)puVar6 + 1);
          }
        }
LAB_004d1045:
        byte_swap_array();
        param_2[1] = (int)(param_2[1] + (int)local_4);
        psVar7 = local_c;
        puVar11 = local_8;
        break;
      case 4:
        uVar5 = (uint)psVar7[1];
        local_4 = (ushort *)(uVar5 * 8);
        if (((int)((int)local_4 + param_2[1]) <= param_2[2]) && ((char)param_2[3] == '\0')) {
          puVar6 = (undefined4 *)(*param_2 + param_2[1]);
          if (puVar11 == (ushort *)0x0) {
            for (iVar8 = (uVar5 & 0x1fffffff) << 1; iVar8 != 0; iVar8 = iVar8 + -1) {
              *puVar6 = 0;
              puVar6 = puVar6 + 1;
            }
            for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
              *(undefined1 *)puVar6 = 0;
              puVar6 = (undefined4 *)((int)puVar6 + 1);
            }
          }
          else {
            for (iVar8 = (uVar5 & 0x1fffffff) << 1; iVar8 != 0; iVar8 = iVar8 + -1) {
              *puVar6 = *(undefined4 *)puVar11;
              puVar11 = puVar11 + 2;
              puVar6 = puVar6 + 1;
            }
            for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
              *(char *)puVar6 = (char)*puVar11;
              puVar11 = (ushort *)((int)puVar11 + 1);
              puVar6 = (undefined4 *)((int)puVar6 + 1);
            }
          }
          goto LAB_004d1045;
        }
LAB_004d11ee:
        *(undefined1 *)(param_2 + 3) = 1;
        break;
      case 5:
        byte_stream_write_string(puVar11);
        break;
      case 6:
        uVar3 = *puVar11;
        local_4 = puVar11 + 1;
        if (((short)uVar3 < 0) || (psVar7[1] < (short)uVar3)) {
          uVar3 = 0;
        }
        uVar5 = (uint)(short)uVar3;
        FUN_004d0700();
        psVar7 = local_c;
        if ((param_2[2] < (int)(uVar5 + param_2[1])) || ((char)param_2[3] != '\0')) {
          *(undefined1 *)(param_2 + 3) = 1;
        }
        else {
          puVar6 = (undefined4 *)(*param_2 + param_2[1]);
          uVar4 = uVar5 >> 2;
          puVar9 = local_4;
          puVar11 = local_8;
          if (local_4 == (ushort *)0x0) {
            for (; uVar4 != 0; uVar4 = uVar4 - 1) {
              *puVar6 = 0;
              puVar6 = puVar6 + 1;
            }
            for (uVar4 = uVar5 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
              *(undefined1 *)puVar6 = 0;
              puVar6 = (undefined4 *)((int)puVar6 + 1);
            }
            param_2[1] = param_2[1] + uVar5;
          }
          else {
            for (; uVar4 != 0; uVar4 = uVar4 - 1) {
              *puVar6 = *(undefined4 *)puVar9;
              puVar9 = puVar9 + 2;
              puVar6 = puVar6 + 1;
            }
            for (uVar4 = uVar5 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
              *(char *)puVar6 = (char)*puVar9;
              puVar9 = (ushort *)((int)puVar9 + 1);
              puVar6 = (undefined4 *)((int)puVar6 + 1);
            }
            param_2[1] = param_2[1] + uVar5;
          }
        }
        break;
      case 7:
        uVar3 = *puVar11;
        puVar11 = puVar11 + 1;
        struct_definition_compute_size(param_1,0,psVar7 + 5,local_10);
        if (((short)uVar3 < 0) || (psVar7[1] < (short)uVar3)) {
          uVar3 = 0;
        }
        FUN_004d0700();
        if (0 < (short)uVar3) {
          uVar5 = (uint)uVar3;
          do {
            struct_definition_encode(param_1,param_2,param_3,puVar11,local_14,local_c + 5,0);
            puVar11 = (ushort *)((int)puVar11 + (int)local_14[0]);
            uVar5 = uVar5 - 1;
          } while (uVar5 != 0);
        }
        psVar7 = local_c + local_10[0] * 5;
        puVar11 = local_8;
        break;
      case 8:
        local_4 = (ushort *)(int)psVar7[1];
        if ((param_2[2] < (int)((int)local_4 + param_2[1])) || ((char)param_2[3] != '\0'))
        goto LAB_004d11ee;
        puVar6 = (undefined4 *)(*param_2 + param_2[1]);
        if (puVar11 == (ushort *)0x0) {
          for (uVar5 = (uint)local_4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
            *puVar6 = 0;
            puVar6 = puVar6 + 1;
          }
          for (uVar5 = (uint)local_4 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
            *(undefined1 *)puVar6 = 0;
            puVar6 = (undefined4 *)((int)puVar6 + 1);
          }
          param_2[1] = (int)(param_2[1] + (int)local_4);
        }
        else {
          puVar9 = puVar11;
          for (uVar5 = (uint)local_4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
            *puVar6 = *(undefined4 *)puVar9;
            puVar9 = puVar9 + 2;
            puVar6 = puVar6 + 1;
          }
          for (uVar5 = (uint)local_4 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
            *(char *)puVar6 = (char)*puVar9;
            puVar9 = (ushort *)((int)puVar9 + 1);
            puVar6 = (undefined4 *)((int)puVar6 + 1);
          }
          param_2[1] = (int)(param_2[1] + (int)local_4);
        }
      }
    }
    psVar1 = psVar7 + 4;
    psVar7 = psVar7 + 5;
    puVar11 = (ushort *)((int)puVar11 + (int)*psVar1);
    sVar2 = *psVar7;
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
