// item_detonation_timer_start  (Ghidra: FUN_004bd450; renamed per types/items.h:
// "FUN_004bd450 seeds it exactly once (if (*(short *)&((struct item_object *)item)->item.detonation_countdown == 0))")
// address 0x4bd450, size 169 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: types/items.h item_data.detonation_countdown (0x1f8); types/tags.h Item
//   (detonation_delay[2], float bounds at 0x2e0/0x2e4); src/math/random_real_range.c (the
//   exact same LCG + scale-by-1/65535 + lerp sequence, already named and proven against this
//   binary); global 0x008603b0 object_data, 0x0087bc14 tag_instances.
// register convention: object index in EAX (in_EAX), matching every other bare "in_EAX" object
//   accessor in this module.
// resolved from disassembly (objdump -d -M intel bin/halo.exe, 0x4bd450..0x4bd4f8): Ghidra's
//   decompile shows `__ftol()` with zero visible operands because the whole
//   random_real_range-shaped computation happens on the x87 stack between the LCG update and
//   the truncation call; the disassembly makes the operation exactly
//   `(int16)(random_real_range(tag->detonation_delay[0], tag->detonation_delay[1]) * 30.0)`,
//   confirmed field-for-field against random_real_range's own body and against the float32
//   constant at 0x00672ac8 (read from the binary: 30.0f, ticks per second).
// UNSURE: effect_new_on_object is called with several pushed arguments in the disassembly
//   (0, 0, 0, 0, 0xffffffff, object_index) that Ghidra's own decompile of this call site does
//   not recognise as parameters (it shows a bare `effect_new_on_object();`); kept as the zero-argument
//   opaque call other rewrites in this codebase already use for the same address (e.g.
//   src/objects/object_dispatch_effect_notify.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "effects.h"
#include "units.h"
#include "fn_math.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source);
    // 0x4507a0, blam-cc: EAX -> creator_object_index, ECX -> definition_index, stack -> the other six


// Lazily seeds an item's detonation_countdown, exactly once, from the Item tag's
// detonation_delay range converted to ticks.
void item_detonation_timer_start(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    item_data *item = (item_data *)((uint8_t *)obj + k_item_data_offset);

    if (item->detonation_countdown == 0) {
        Item *tag = (Item *)tag_instances[obj->definition_tag & 0xffff].data;

        // 0x4bd48a..0x4bd49b: EAX = the item, ECX = Item tag +0x2f4 (detonating effect), stack: the item, -1, 0..
        effect_new_on_object(object_index, *(datum_index *)&((struct Item *)tag)->detonating_effect.tag_id, object_index, -1, 0.0f, 0.0f,
            0, 0);

        item->detonation_countdown =
            (int16_t)(random_real_range(tag->detonation_delay[0], tag->detonation_delay[1]) * 30.0f);
    }
}

#if 0
Original Ghidra decompilation (0x4bd450):

void FUN_004bd450(void)

{
  int iVar1;
  undefined2 uVar2;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (*(short *)(iVar1 + 0x1f8) == 0) {
    FUN_004507a0();
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    uVar2 = __ftol();
    *(undefined2 *)(iVar1 + 0x1f8) = uVar2;
  }
  return;
}
#endif
