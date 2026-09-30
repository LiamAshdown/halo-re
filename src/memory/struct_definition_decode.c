// struct_definition_decode
// address 0x4d13c0, size 869 bytes
// name confidence: 0.85 (paired 1:1 with struct_definition_encode; parameter roles cross-checked
// against it -- same definition/stream/version/instance/out-size/fields/out-field-count order,
// mirrored for reading instead of writing)
// rewrite confidence: 0.6 -- same elided-callee-argument caveat as struct_definition_encode for
// the two calls shown as "FUN_004d08a0();" (byte_stream_read_ranged_integer) and
// "byte_stream_read_string();"; both are reconstructed with high confidence from those callees'
// own documented register conventions (out/phase4/memory_types_notes.md) and the values computed
// immediately before each elided call.
// evidence: struct_definition/struct_definition_field layout in types/memory.h. Unlike
// struct_definition_encode, this function recomputes its destination-write cursor as
// `field_start_dest + field->computed_size` at the bottom of every iteration regardless of what
// any given case did to the destination pointer internally (Ghidra: `puVar11 = local_8 +
// psVar7[4]`), so no case needs an explicit "restore" -- confirmed by comparing every case's
// tail against that unconditional reset.
// register convention: cdecl, all seven arguments on the stack; no in_EAX/in_ECX/unaff_* register
// arguments observed for this function's own parameters.

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"
#include <string.h>

// blam-cc: struct_definition_compute_size(definition, out_size, fields, out_field_count)

// blam-cc (0x4d08a0, below this batch's assigned range): maximum in EAX, stream in ECX. Reads a
// 1/2/4-byte big-endian value chosen by `maximum`, the read counterpart of
// byte_stream_write_ranged_integer.

// blam-cc (0x4d0930, below this batch's assigned range): stream in ECX. Returns a pointer to a
// NUL-terminated string inside the stream's own buffer, or NULL (with stream->overflow set) if
// unterminated.

// blam-cc (0x4cfd90, below this batch's assigned range): element size in EAX (2, 4 or 8), base
// pointer in ECX, element count in EDX.
extern void byte_swap_array(int32_t element_size, uint32_t *base, int32_t element_count);

void struct_definition_decode(struct_definition *definition, byte_stream *input, int16_t version,
    void *dest_instance, int16_t *out_dest_size, struct_definition_field *fields,
    int16_t *out_field_count)
{
    struct_definition_field *field = fields;
    uint8_t *dest_cursor = (uint8_t *)dest_instance;

    for (;;) {
        uint8_t *dest; // this field's destination start (Ghidra: local_8, == dest_cursor here)

        if (field->type == _struct_field_terminator) {
            if (out_field_count != 0) {
                int32_t distance = (int32_t)((uint8_t *)field - (uint8_t *)fields);
                *out_field_count = (int16_t)(distance / 10) + 1;
            }
            if (out_dest_size != 0) {
                *out_dest_size = (int16_t)((int16_t)(int32_t)dest_cursor -
                    (int16_t)(int32_t)dest_instance); // UNSURE: same 16-bit pointer truncation
                    // as struct_definition_encode's out_source_size; preserved as decompiled.
            }
            return;
        }

        dest = dest_cursor;

        if (version < field->minimum_version ||
            (field->maximum_version < version && field->maximum_version != 0)) {
            // Field does not apply to this wire version: zero the destination's reserved bytes
            // and read nothing from the input stream.
            memset(dest, 0, (uint32_t)(int32_t)field->computed_size);
        } else {
            switch (field->type) {
            case _struct_field_data:
            case _struct_field_block: {
                uint32_t byte_count = (uint32_t)(int32_t)field->count;
                int32_t new_cursor = input->cursor + (int32_t)byte_count;
                if (input->size < new_cursor || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    input->cursor = new_cursor;
                    if (src != 0) {
                        memcpy(dest, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_int16_array: {
                uint32_t byte_count = (uint32_t)field->count * 2;
                if (input->size < input->cursor + (int32_t)byte_count || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    uint16_t i;
                    for (i = 0; i < field->count; i = i + 1) { // byte-swap in place in the INPUT
                        // buffer, matching the original exactly -- this mutates the wire buffer.
                        uint16_t *slot = (uint16_t *)src + i;
                        *slot = (uint16_t)(((*slot & 0xff) << 8) | (*slot >> 8));
                    }
                    input->cursor += byte_count;
                    if (src != 0) {
                        memcpy(dest, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_int32_array: {
                uint32_t byte_count = (uint32_t)field->count * 4;
                if (input->size < input->cursor + (int32_t)byte_count || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    byte_swap_array(4, (uint32_t *)src, field->count);
                    input->cursor += byte_count;
                    if (src != 0) {
                        memcpy(dest, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_int64_array: {
                uint32_t byte_count = (uint32_t)field->count * 8;
                if (input->size < input->cursor + (int32_t)byte_count || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    byte_swap_array(8, (uint32_t *)src, field->count);
                    input->cursor += byte_count;
                    if (src != 0) {
                        memcpy(dest, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_string: {
                char *str = byte_stream_read_string(input);
                if (str != 0) {
                    strcpy((char *)dest, str);
                }
                break;
            }

            case _struct_field_variable_data: {
                uint16_t count = (uint16_t)byte_stream_read_ranged_integer(field->count, input);
                // UNSURE-worthy but literal: Ghidra sign-extends the wire count here
                // (uVar6 = (uint)(short)uVar3) and, unlike _struct_field_struct_array below,
                // never clamps it against field->count. A wire value >= 0x8000 therefore makes
                // byte_count huge and new_cursor smaller than input->cursor, so the bounds test
                // passes and the copy runs away. Preserved exactly; see src/memory/README.md.
                uint32_t byte_count = (uint32_t)(int32_t)(int16_t)count;
                int32_t new_cursor;
                *(uint16_t *)dest = count;
                new_cursor = input->cursor + (int32_t)byte_count;
                if (input->size < new_cursor || input->overflow != 0) {
                    input->overflow = 1;
                } else {
                    uint8_t *src = input->data + input->cursor;
                    input->cursor = new_cursor;
                    if (src != 0) {
                        memcpy(dest + 2, src, byte_count);
                    }
                }
                break;
            }

            case _struct_field_struct_array: {
                int16_t count = (int16_t)byte_stream_read_ranged_integer(field->count, input);
                int16_t nested_field_count = 0;
                uint8_t *elem;
                struct_definition_compute_size(definition, 0, field + 1, &nested_field_count);
                if (count < 0 || field->count < count) {
                    count = 0;
                }
                *(uint16_t *)dest = (uint16_t)count;
                elem = dest + 2;
                if (0 < count) {
                    do {
                        int16_t nested_size = 0;
                        struct_definition_decode(definition, input, version, elem, &nested_size,
                            field + 1, 0);
                        elem = elem + nested_size;
                        count = count - 1;
                    } while (count != 0);
                }
                field = field + nested_field_count; // lands on the nested list's terminator
                    // record, matching struct_definition_compute_size / _encode.
                break;
            }
            }
        }

        {
            int16_t field_size = field->computed_size;
            field = field + 1;
            dest_cursor = dest + field_size;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d13c0):

void struct_definition_decode
               (undefined4 param_1,int *param_2,undefined4 param_3,ushort *param_4,short *param_5,
               short *param_6,short *param_7)

{
  char cVar1;
  short sVar2;
  ushort uVar3;
  char *pcVar4;
  uint uVar5;
  uint uVar6;
  short *psVar7;
  int iVar8;
  undefined4 *puVar9;
  undefined4 *puVar10;
  ushort *puVar11;
  short local_10 [2];
  short local_c [2];
  ushort *local_8;
  int local_4;

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
    local_8 = puVar11;
    if (((short)param_3 < psVar7[2]) || ((psVar7[3] < (short)param_3 && (psVar7[3] != 0)))) {
      sVar2 = psVar7[4];
      for (uVar6 = (uint)(int)sVar2 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        puVar11[0] = 0;
        puVar11[1] = 0;
        puVar11 = puVar11 + 2;
      }
      for (uVar6 = (int)sVar2 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined1 *)puVar11 = 0;
        puVar11 = (ushort *)((int)puVar11 + 1);
      }
    }
    else {
      switch(*psVar7) {
      case 1:
        iVar8 = (int)psVar7[1] + param_2[1];
        if ((param_2[2] < iVar8) || ((char)param_2[3] != '\0')) {
LAB_004d163f:
          *(undefined1 *)(param_2 + 3) = 1;
        }
        else {
          puVar9 = (undefined4 *)(*param_2 + param_2[1]);
          param_2[1] = iVar8;
          if (puVar9 != (undefined4 *)0x0) {
            sVar2 = psVar7[1];
            for (uVar6 = (uint)(int)sVar2 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
              *(undefined4 *)puVar11 = *puVar9;
              puVar9 = puVar9 + 1;
              puVar11 = puVar11 + 2;
            }
            for (uVar6 = (int)sVar2 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
              *(undefined1 *)puVar11 = *(undefined1 *)puVar9;
              puVar9 = (undefined4 *)((int)puVar9 + 1);
              puVar11 = (ushort *)((int)puVar11 + 1);
            }
          }
        }
        break;
      case 2:
        iVar8 = (int)psVar7[1];
        local_4 = iVar8 * 2;
        if ((param_2[2] < param_2[1] + local_4) || ((char)param_2[3] != '\0')) goto LAB_004d163f;
        puVar9 = (undefined4 *)(*param_2 + param_2[1]);
        puVar10 = puVar9;
        if (0 < iVar8) {
          do {
            iVar8 = iVar8 + -1;
            *(ushort *)puVar10 =
                 CONCAT11((char)*(undefined2 *)puVar10,(char)((ushort)*(undefined2 *)puVar10 >> 8));
            puVar10 = (undefined4 *)((int)puVar10 + 2);
          } while (iVar8 != 0);
        }
        param_2[1] = param_2[1] + local_4;
        if (puVar9 != (undefined4 *)0x0) {
          sVar2 = psVar7[1];
          for (uVar6 = ((int)sVar2 & 0x7fffffffU) >> 1; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined4 *)puVar11 = *puVar9;
            puVar9 = puVar9 + 1;
            puVar11 = puVar11 + 2;
          }
          for (iVar8 = ((int)sVar2 & 1U) << 1; iVar8 != 0; iVar8 = iVar8 + -1) {
            *(undefined1 *)puVar11 = *(undefined1 *)puVar9;
            puVar9 = (undefined4 *)((int)puVar9 + 1);
            puVar11 = (ushort *)((int)puVar11 + 1);
          }
        }
        break;
      case 3:
        sVar2 = psVar7[1];
        if ((param_2[2] < param_2[1] + sVar2 * 4) || ((char)param_2[3] != '\0')) goto LAB_004d163f;
        puVar9 = (undefined4 *)(*param_2 + param_2[1]);
        byte_swap_array();
        param_2[1] = param_2[1] + sVar2 * 4;
        if (puVar9 != (undefined4 *)0x0) {
          uVar6 = (int)psVar7[1] << 2;
LAB_004d1628:
          puVar11 = local_8;
          for (uVar5 = uVar6 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
            *(undefined4 *)puVar11 = *puVar9;
            puVar9 = puVar9 + 1;
            puVar11 = puVar11 + 2;
          }
          for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined1 *)puVar11 = *(undefined1 *)puVar9;
            puVar9 = (undefined4 *)((int)puVar9 + 1);
            puVar11 = (ushort *)((int)puVar11 + 1);
          }
        }
        break;
      case 4:
        sVar2 = psVar7[1];
        if ((param_2[2] < param_2[1] + sVar2 * 8) || ((char)param_2[3] != '\0')) goto LAB_004d163f;
        puVar9 = (undefined4 *)(*param_2 + param_2[1]);
        byte_swap_array();
        param_2[1] = param_2[1] + sVar2 * 8;
        if (puVar9 != (undefined4 *)0x0) {
          uVar6 = (int)psVar7[1] << 3;
          goto LAB_004d1628;
        }
        break;
      case 5:
        pcVar4 = (char *)byte_stream_read_string();
        if (pcVar4 != (char *)0x0) {
          iVar8 = (int)puVar11 - (int)pcVar4;
          do {
            cVar1 = *pcVar4;
            pcVar4[iVar8] = cVar1;
            pcVar4 = pcVar4 + 1;
          } while (cVar1 != '\0');
        }
        break;
      case 6:
        uVar3 = FUN_004d08a0();
        *puVar11 = uVar3;
        uVar6 = (uint)(short)uVar3;
        iVar8 = param_2[1] + uVar6;
        if ((param_2[2] < iVar8) || ((char)param_2[3] != '\0')) goto LAB_004d163f;
        puVar9 = (undefined4 *)(*param_2 + param_2[1]);
        param_2[1] = iVar8;
        if (puVar9 != (undefined4 *)0x0) {
          puVar11 = puVar11 + 1;
          for (uVar5 = uVar6 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
            *(undefined4 *)puVar11 = *puVar9;
            puVar9 = puVar9 + 1;
            puVar11 = puVar11 + 2;
          }
          for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined1 *)puVar11 = *(undefined1 *)puVar9;
            puVar9 = (undefined4 *)((int)puVar9 + 1);
            puVar11 = (ushort *)((int)puVar11 + 1);
          }
        }
        break;
      case 7:
        uVar3 = FUN_004d08a0();
        struct_definition_compute_size(param_1,0,psVar7 + 5,local_c);
        if (((short)uVar3 < 0) || (psVar7[1] < (short)uVar3)) {
          uVar3 = 0;
        }
        *local_8 = uVar3;
        puVar11 = local_8 + 1;
        if (0 < (short)uVar3) {
          uVar6 = (uint)uVar3;
          do {
            struct_definition_decode(param_1,param_2,param_3,puVar11,local_10,psVar7 + 5,0);
            puVar11 = (ushort *)((int)puVar11 + (int)local_10[0]);
            uVar6 = uVar6 - 1;
          } while (uVar6 != 0);
        }
        psVar7 = psVar7 + local_c[0] * 5;
        break;
      case 8:
        iVar8 = (int)psVar7[1] + param_2[1];
        if ((param_2[2] < iVar8) || ((char)param_2[3] != '\0')) goto LAB_004d163f;
        puVar9 = (undefined4 *)(*param_2 + param_2[1]);
        param_2[1] = iVar8;
        if (puVar9 != (undefined4 *)0x0) {
          uVar6 = (uint)psVar7[1];
          goto LAB_004d1628;
        }
      }
    }
    puVar11 = (ushort *)((int)local_8 + (int)psVar7[4]);
    psVar7 = psVar7 + 5;
    sVar2 = *psVar7;
  } while( true );
}
#endif
