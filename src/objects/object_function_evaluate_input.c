// object_function_evaluate_input  (Ghidra: object_types_place_objects_mod_processed_bsps__read;
// renamed per out/phase4/objects_types_notes.md: "0x4f8207
// object_types_place_objects_mod_processed_bsps__read: this is the object function input
// evaluator, and it is the function that fixed the 0x124..0x143 value array")
// address 0x4f8207, size 347 bytes
// name confidence: 0.4 (the types notes identify its role but do not supply a clean name; this
//   one follows the object_function_input enum it evaluates)
// rewrite confidence: 1.0 (FRAGMENT: 0x4f8207 lies inside object_update_export_functions (0x4f80d0..0x4f827b, the loop re-enters at 0x4f8120); no callers; that function is the real rewrite) (Ghidra itself could not fully recover this function's entry point
//   or parameters -- every input arrives as an "unaff_" register and the body is a single
//   goto-threaded loop with an internal re-entry label (code_r0x004f8207) partway through,
//   which usually means the real function starts somewhere earlier that this batch's pack did
//   not capture, or the whole thing is a shared tail folded in by the compiler. Zero recorded
//   callers. This rewrite preserves the goto structure and raw offsets as literally as possible
//   rather than guessing a clean control-flow shape that might not match the compiled code.)
// evidence: types/objects.h object_function_input enum (values 1..8 select
//   object.function_in_values / function_out_values, matching the case values 1/2/3/4 here
//   reading Object-tag-relative floats and the ESI+0x106 bit-4 test near case 0x12); global
//   0x008603b0 object_data; callees random_real (0x4019f0), angle_delta_wrapped (0x470d10).
// register convention: UNRESOLVED. Ghidra reports five live inputs at the point this fragment
//   begins -- EDX (float), EBX (short*), EBP (float*), ESI (int, likely an Object tag data
//   pointer), EDI (int, likely a pre-scaled object index*0xc) and a stack value (loop count) --
//   none of which this rewrite could verify against a caller, since none is recorded.
//   // blam-cc: UNRESOLVED, see above; parameters kept in Ghidra's own unaff_* order

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

extern real random_real(void); // math module, 0x4019f0
extern float angle_delta_wrapped(float a, float b); // 0x470d10, UNSURE: argument order guessed from the call shape
extern uint8_t *player_globals_table; // 0x00746f8c, same global as objects_set_ambient_cluster_override.c
extern double atan2(double y, double x); // x87 FPATAN
extern float fabsf(float x); // x87 FABS

// UNSURE: this is a best-effort structural translation of a fragment Ghidra could not fully
// resolve; see the file header before trusting it for anything beyond documentation.
void object_function_evaluate_input(float initial_angle_input, float initial_st0, int16_t *selectors,
    float *out_values, uint8_t *object_tag_data, int32_t object_index_scaled, int32_t remaining_count)
{
    float value;
    int16_t selector;

    value = angle_delta_wrapped(initial_angle_input, initial_st0);
    value = value * 0.15915494f + 0.5f;
    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        goto clamp_to_one;
    }

store_and_advance:
    *out_values = value;
    for (;;) {
        selectors++;
        out_values++;
        remaining_count--;
        if (remaining_count == 0) {
            return;
        }
        selector = *selectors;
        if (selector != 0) {
            break;
        }
    }

    value = 0.0f;
    switch (selector) {
        case 1:
            value = *(float *)(object_tag_data + 0xe0);
            goto store_and_advance;
        case 2:
            value = *(float *)(object_tag_data + 0xe4);
            if (value <= 1.0f) {
                goto store_and_advance;
            }
            break;
        case 3:
            value = *(float *)(object_tag_data + 0xec);
            goto store_and_advance;
        case 4:
            value = *(float *)(object_tag_data + 0xe8);
            goto store_and_advance;
        case 5:
            if (*out_values == 1.0f) {
                value = random_real();
            }
            goto store_and_advance;
        case 0x12:
            if ((*(uint8_t *)(object_tag_data + 0x106) & 4) != 0) {
                value = 0.0f;
                goto store_and_advance;
            }
            break;
        case 0x13: {
            // UNSURE: object_index_scaled already carries the *0xc header stride per the
            // original's own "DAT_008603b0->data + 8 + unaff_EDI" arithmetic.
            object *obj = *(object **)((uint8_t *)object_data->data + 8 + object_index_scaled);
            real_matrix4x3 *node = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);
            if (0.995f <= fabsf(node->left.i)) { // UNSURE: field mapping guessed, see file header
                value = *out_values;
                goto store_and_advance;
            }
            initial_st0 = (float)atan2((double)node->forward.j, (double)node->forward.k); // UNSURE: mapping guessed
            initial_angle_input = *(float *)(player_globals_table + 0x4c); // UNSURE: see file header
            value = angle_delta_wrapped(initial_angle_input, initial_st0);
            value = value * 0.15915494f + 0.5f;
            if (value >= 0.0f) {
                goto clamp_to_one;
            }
            value = 0.0f;
            goto store_and_advance;
        }
        default:
            value = (float)*(uint8_t *)(object_tag_data + (int16_t)(selector - 10) + 0x178) * 0.003921569f;
            goto store_and_advance;
    }
    value = 1.0f;
    goto store_and_advance;

clamp_to_one:
    goto store_and_advance;
}

#if 0
Original Ghidra decompilation (0x4f8207):

void object_types_place_objects_mod_processed_bsps__read(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  float in_EDX;
  short *unaff_EBX;
  float *unaff_EBP;
  int unaff_ESI;
  int unaff_EDI;
  float10 in_ST0;
  float fVar4;
  int in_stack_0000001c;

code_r0x004f8207:
  fVar4 = angle_delta_wrapped(in_EDX,(float)in_ST0);
  fVar4 = fVar4 * 0.15915494 + 0.5;
  if (0.0 <= fVar4) goto LAB_004f817c;
  fVar4 = 0.0;
LAB_004f825d:
  *unaff_EBP = fVar4;
  do {
    unaff_EBX = unaff_EBX + 1;
    unaff_EBP = unaff_EBP + 1;
    in_stack_0000001c = in_stack_0000001c + -1;
    if (in_stack_0000001c == 0) {
      return;
    }
    sVar1 = *unaff_EBX;
  } while (sVar1 == 0);
  fVar4 = 0.0;
  switch(sVar1) {
  case 1:
    fVar4 = *(float *)(unaff_ESI + 0xe0);
    goto LAB_004f825d;
  case 2:
    fVar4 = *(float *)(unaff_ESI + 0xe4);
LAB_004f817c:
    if (fVar4 <= 1.0) goto LAB_004f825d;
    break;
  case 3:
    fVar4 = *(float *)(unaff_ESI + 0xec);
    goto LAB_004f825d;
  case 4:
    fVar4 = *(float *)(unaff_ESI + 0xe8);
    goto LAB_004f825d;
  case 5:
    if (*unaff_EBP == 1.0) {
      fVar4 = random_real();
    }
    goto LAB_004f825d;
  default:
    fVar4 = (float)*(byte *)((short)(sVar1 + -10) + 0x178 + unaff_ESI) * 0.003921569;
    goto LAB_004f825d;
  case 0x12:
    if ((*(byte *)(unaff_ESI + 0x106) & 4) != 0) {
      fVar4 = 0.0;
      goto LAB_004f825d;
    }
    break;
  case 0x13:
    goto switchD_004f8146_caseD_13;
  }
  fVar4 = 1.0;
  goto LAB_004f825d;
switchD_004f8146_caseD_13:
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + unaff_EDI);
  iVar3 = *(short *)(iVar2 + 0x1f2) + iVar2;
  if (0.995 <= ABS(*(float *)(*(short *)(iVar2 + 0x1f2) + 0xc + iVar2))) goto LAB_004f823a;
  in_ST0 = (float10)fpatan((float10)*(float *)(iVar3 + 4),(float10)*(float *)(iVar3 + 8));
  in_EDX = *(float *)(DAT_00746f8c + 0x4c);
  goto code_r0x004f8207;
LAB_004f823a:
  fVar4 = *unaff_EBP;
  goto LAB_004f825d;
}
#endif
