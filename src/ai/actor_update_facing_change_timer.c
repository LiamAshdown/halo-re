// actor_update_facing_change_timer  (Ghidra: actor_update_facing_change_timer, renamed)
// address 0x423670, size 208 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump 0x423670..0x42373f; stored constant FIXED)
// evidence: types/tags.h Actor.change_facing_stand_time (byte-counted to offset 0x334, the
//   same field family as Actor.surprise_distance at 0x2b0 and Actor.event_look_time_modifier
//   at 0xd4/0xd8 -- all confirmed the same way by hand-counting the struct). Actor tag pointer
//   via actor.actor_definition_tag (0x58), matching every sibling function in this range.
//   objdump (bin/halo.exe 0x423670..0x42373f) resolves the otherwise-argument-less __ftol
//   call: its hidden FPU operand is change_facing_stand_time * 30.0 (seconds -> ticks, the
//   same 30.0 constant every dialogue-timing function in this range uses), and the four
//   .rdata float constants compare against 0.0, 30.0, 4.0 and 1.8 respectively.
//   UNSURE: the code re-purposes actor.target_unit_index (0x270, a *unit* handle everywhere
//   else per types/ai.h) as a *prop* index here (scaled by the prop stride 0x138). Preserved
//   literally rather than assumed to be a decompiler artefact -- types/ai.h's own "apparent
//   contradictions" note says some of these are genuine retail behaviour.
// register convention: EAX -> actor_index; no other register or stack operands are read.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t __ftol(double x); // FISTP-based float-to-int truncation

// blam-cc: EAX -> actor_index
// Once the actor's "facing change pending" flag is set and its Actor tag allows a nonzero
// stand-facing-change time, clears the flag and converts that time to a tick count. If the
// actor's current target (read here as a prop index, see UNSURE above) is within 4 world
// units, clamps a smoothing field (unknown_354) to no more than 1.8, snapping it down to 0.9
// when it currently exceeds that.
void actor_update_facing_change_timer(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);
    // 0x354/0x358/0x35a all fall inside types/ai.h's opaque actor.unknown_350[0x1c] run, which
    // has no individual field names; addressed here as raw offsets from that array's base.
    uint8_t *pending_flag = &self->unknown_358;
    int16_t *ticks_field = &self->unknown_35a;
    float *smoothing_field = &self->danger_meter;

    if (*pending_flag != 0 && actor_tag->change_facing_stand_time > 0.0f) {
        *pending_flag = 0;
        *ticks_field = (int16_t)__ftol((double)(actor_tag->change_facing_stand_time * 30.0f));

        if (self->target_unit_index != (datum_index)k_datum_index_none) {
            prop *target = &((prop *)prop_data->data)[self->target_unit_index & 0xffff];
            if (target->distance < 4.0f) {
                if (*smoothing_field <= 1.8f) {
                    *smoothing_field = 1.8f; // FIXED (0x423732): the same constant 0x672c9c is stored (was 0.9)
                }
                // else: *smoothing_field already <= 1.8 is false, i.e. it exceeds 1.8; the
                // original leaves it unchanged here (a self-assignment in the decompile).
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x423670):

void FUN_00423670(void)

{
  undefined2 uVar1;
  uint in_EAX;
  int iVar2;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((*(char *)(iVar2 + 0x358) != '\0') &&
     (0.0 < *(float *)(*(int *)((*(uint *)(iVar2 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                      0x334))) {
    *(undefined1 *)(iVar2 + 0x358) = 0;
    uVar1 = __ftol();
    *(undefined2 *)(iVar2 + 0x35a) = uVar1;
    if ((*(uint *)(iVar2 + 0x270) != 0xffffffff) &&
       (*(float *)((*(uint *)(iVar2 + 0x270) & 0xffff) * 0x138 + 0x11c +
                  *(int *)(DAT_008802c0 + 0x34)) < 4.0)) {
      if (1.8 < *(float *)(iVar2 + 0x354)) {
        *(undefined4 *)(iVar2 + 0x354) = *(undefined4 *)(iVar2 + 0x354);
        return;
      }
      *(undefined4 *)(iVar2 + 0x354) = 0x3fe66666;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
