// biped_apply_idle_fidget  (Ghidra: biped_apply_idle_fidget, renamed)
// address 0x55e940, size 191 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (checked against objdump 0x55e940..0x55ea97)
// evidence: object.up/angular_velocity (0x080/0x08c, objects.h); Biped.biped_flags bit 0x100;
//   unit_data.animation_state 0x2a3.
// register convention: object index in EDI, a 2-byte animation-state output array in param_1.
//   // blam-cc: EDI -> object_index, stack -> state_out
// UNSURE: the vector3d_cross_product call's operands (guessed here as forward x up); which axis
//   unit_rotate_basis_about_axis is given.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern real vector3d_normalize_with_length(real_vector3d *v);
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand);
extern real_vector3d *global_up3d_pointer; // 0x00696720 (0, 0, 1)
extern real random_real_range(real min, real max); // 0x401050
extern double fcos(double x);
extern double fsin(double x);
extern uint32_t biped_is_idle_eligible(uint32_t object_index); // 0x55e8e0, this batch
extern void unit_rotate_basis_about_axis(uint32_t object_index); // 0x55e6b0, this batch

// Nudges an idle-eligible unit with a small randomized angular impulse (perpendicular to its
// up-vector when reasonably upright, otherwise a random direction in the horizontal plane) to
// produce idle fidget motion, unless it's already in one of the special "greeting" animation
// states, then refreshes its orientation basis and reports a follow-up animation-state code.
void biped_apply_idle_fidget(uint32_t object_index, uint8_t *state_out)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint32_t already_idle = 0;

    if (biped_is_idle_eligible(object_index)) {
        already_idle = 1;
        if ((tag->biped_flags & 0x100) == 0) {
            goto tail;
        }
        if (unit->animation_state != 0x1f && unit->animation_state != 0x29) {
            float magnitude = (float)random_real_range(0.05235988, 0.08726646);
            real_vector3d impulse_dir;

            if (!(obj->up.k <= 0.8f)) { // 0x55e9c8: above 0.8 (or NaN) picks a random horizontal direction
                double angle = random_real_range(0.0, 6.2831855);
                impulse_dir.i = (float)fcos(angle);
                impulse_dir.j = (float)fsin(angle);
                impulse_dir.k = 0.0f;
            } else {
                vector3d_cross_product(&impulse_dir, global_up3d_pointer, &obj->up); // 0x55e9d5: EAX out, ECX *0x696720, stack up
                if (!(vector3d_normalize_with_length(&impulse_dir) > 0.0f)) {
                    double angle = random_real_range(0.0, 6.2831855);
                    impulse_dir.i = (float)fcos(angle);
                    impulse_dir.j = (float)fsin(angle);
                    impulse_dir.k = 0.0f;
                }
            }
            obj->angular_velocity.i += impulse_dir.i * magnitude;
            obj->angular_velocity.j += impulse_dir.j * magnitude;
            obj->angular_velocity.k += impulse_dir.k * magnitude;
        }
        unit_rotate_basis_about_axis(object_index); // 0x55ea65: EAX = object
    }

tail:
    {
        int8_t state = unit->animation_state;
        if (state == 0x27 || state == 0x28) {
            state_out[0] = 0x28;
        } else if (state == 0x14 || already_idle) {
            state_out[0] = 0x14;
        }
    }
}

#if 0
Original Ghidra decompilation (0x55e940):

void FUN_0055e940(undefined1 *param_1)

{
  uint *puVar1;
  bool bVar2;
  char cVar3;
  uint unaff_EDI;
  float10 fVar4;
  float fVar5;
  float fVar6;
  float local_c;
  float fStack_8;
  float fStack_4;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  bVar2 = false;
  cVar3 = FUN_0055e8e0();
  if ((cVar3 == '\0') ||
     (bVar2 = true,
     (*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 0x100) == 0))
  goto LAB_0055ea6c;
  if ((*(char *)((int)puVar1 + 0x2a3) != '\x1f') && (*(char *)((int)puVar1 + 0x2a3) != ')')) {
    fVar5 = random_real_range(0.05235988,0.08726646);
    if (0.8 <= (float)puVar1[0x22]) {
LAB_0055ea04:
      fVar6 = random_real_range(0.0,6.2831855);
      fVar4 = (float10)fcos((float10)fVar6);
      local_c = (float)fVar4;
      fVar4 = (float10)fsin((float10)fVar6);
      fStack_8 = (float)fVar4;
      fStack_4 = 0.0;
    }
    else {
      vector3d_cross_product(puVar1 + 0x20);
      fVar4 = (float10)vector3d_normalize_with_length();
      if (fVar4 <= (float10)0.0) goto LAB_0055ea04;
    }
    puVar1[0x23] = (uint)(local_c * fVar5 + (float)puVar1[0x23]);
    puVar1[0x24] = (uint)(fStack_8 * fVar5 + (float)puVar1[0x24]);
    puVar1[0x25] = (uint)(fStack_4 * fVar5 + (float)puVar1[0x25]);
  }
  FUN_0055e6b0();
LAB_0055ea6c:
  cVar3 = *(char *)((int)puVar1 + 0x2a3);
  if ((cVar3 == '\'') || (cVar3 == '(')) {
    *param_1 = 0x28;
  }
  else if ((cVar3 == '\x14') || (bVar2)) {
    *param_1 = 0x14;
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
