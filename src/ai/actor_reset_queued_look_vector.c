// actor_reset_queued_look_vector  (Ghidra: actor_reset_queued_look_vector, renamed)
// address 0x417ae0, size 139 bytes
// name confidence: 0.35  rewrite confidence: 0.9 (VERIFIED against objdump 0x417ae0..0x417b6a)
// evidence: only touches actor.secondary_action (0x418), actor.queued_look_vector (0x6e0,
// which actor_snapshot_orientation also seeds from the same global_origin3d_pointer) and
// actor.unknown_6ec (which actor_snapshot_orientation also sets to 0xffff); gated on there
// being no secondary action queued and (no controlled unit, or unit_is_in_busy_animation_state failing) and
// actor_wants_reload_or_swap (already named actor_wants_to_reload_or_swap_weapon in
// out/phase4/ai_functions.md, not yet rewritten) also failing.
// register convention: actor_index is a genuine stack parameter (confirmed by objdump -d
// -M intel: `mov ebp,[esp+0xc]` after two register pushes).
// blam-cc: stack -> actor_index
// UNSURE: the return value's low byte is 1 only on the reset path; every other path returns
// whatever unit_is_in_busy_animation_state or the cached data_array pointer happened to leave in EAX, and the
// one caller (0x415480) only tests it as a plain boolean, so this rewrite returns 0 for
// every non-reset path rather than propagating that leftover value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern const real_vector3d *global_origin3d_pointer; // 0x00696714

extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX = the unit (actor +0x18)
extern uint8_t actor_wants_reload_or_swap(datum_index actor_index);   // 0x40ab80, phase-4 name actor_wants_to_reload_or_swap_weapon, not yet rewritten

// blam-cc: stack -> actor_index
uint8_t actor_reset_queued_look_vector(datum_index actor_index)
{
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->secondary_action != (int16_t)-1) {
        return 0;
    }
    if (self->unit_index != (datum_index)k_datum_index_none) {
        if (unit_is_in_busy_animation_state(self->unit_index)) {
            return 0;
        }
    }
    if (actor_wants_reload_or_swap(actor_index)) {
        return 0;
    }

    self->unknown_504 = 0;
    self->queued_look_vector = *global_origin3d_pointer;
    self->unknown_6ec = -1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x417ae0):

uint FUN_00417ae0(uint param_1)

{
  int iVar1;
  undefined *puVar2;
  uint uVar3;
  int iVar4;

  iVar4 = (param_1 & 0xffff) * 0x724;
  iVar1 = *(int *)(DAT_00880360 + 0x34) + iVar4;
  uVar3 = DAT_00880360;
  if (*(short *)(iVar1 + 0x418) == -1) {
    if (*(int *)(iVar1 + 0x18) != -1) {
      uVar3 = FUN_00569c90();
      if ((char)uVar3 != '\0') goto LAB_00417b64;
    }
    uVar3 = FUN_0040ab80();
    puVar2 = PTR_DAT_00696714;
    if ((char)uVar3 == '\0') {
      *(undefined1 *)(iVar1 + 0x504) = 0;
      *(undefined4 *)(iVar1 + 0x6e0) = *(undefined4 *)puVar2;
      uVar3 = DAT_00880360;
      *(undefined4 *)(iVar1 + 0x6e4) = *(undefined4 *)(puVar2 + 4);
      *(undefined4 *)(iVar1 + 0x6e8) = *(undefined4 *)(puVar2 + 8);
      *(undefined2 *)(*(int *)(uVar3 + 0x34) + 0x6ec + iVar4) = 0xffff;
      return CONCAT31((int3)(uVar3 >> 8),1);
    }
  }
LAB_00417b64:
  return uVar3 & 0xffffff00;
}
#endif
