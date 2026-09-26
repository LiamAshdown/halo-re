// biped_update_target_lock_timer  (Ghidra: biped_update_target_lock_timer, renamed)
// address 0x55e0a0, size 237 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (step 1: rewritten from objdump -d 0x55e0a0..0x55e18c)
// evidence: biped_data.unknown_500/unknown_4fc match types/units.h exactly ("how many ticks
//   that target has been held" / "the target 0x55e0a0 is tracking").

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern uint8_t DAT_0087abc3;        // UNSURE global (cheat/debug toggle)

extern void actor_squad_react_to_grenade_for_vehicle_occupants(datum_index vehicle_object_index,
    datum_index other_object_index); // 0x42bd70, blam-cc: EAX, EBX
extern uint8_t recorded_animation_object_is_playing(datum_index unit_index); // 0x44acc0, blam-cc: ESI
extern int32_t unit_get_local_player_weapon_index(datum_index unit);        // 0x4726b0, blam-cc: EAX
extern void local_player_set_controlled_unit(datum_index new_unit, int16_t local_player_index);
    // 0x474fc0, blam-cc: ESI -> new_unit, DI -> local_player_index

// blam-cc: EAX -> target, ECX -> object_index
// FIXED (step 1, objdump -d 0x55e0a0..0x55e18c): target arrives in EAX and the biped in ECX, and every
// callee takes register arguments the draft left out. The one pushed stack slot at the call site is
// never read.
void biped_update_target_lock_timer(datum_index target, uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    object *target_obj;

    if ((int8_t)biped->unknown_500 < 0) {
        if (target != k_datum_index_none) {
            biped->unknown_500 = 0xf1;
        } else {
            biped->unknown_500 = biped->unknown_500 + 1;
        }
        return;
    }
    if (target == k_datum_index_none) {
        return;
    }
    target_obj = ((object_header *)object_data->data)[target & 0xffff].data;
    actor_squad_react_to_grenade_for_vehicle_occupants(target, object_index);
    if (unit->controlling_player == k_datum_index_none && !recorded_animation_object_is_playing(object_index)) {
        return;
    }
    if (biped->unknown_4fc != target) {
        biped->unknown_4fc = target;
        biped->unknown_500 = 0;
        return;
    }
    biped->unknown_500 = biped->unknown_500 + 1;
    if ((int8_t)biped->unknown_500 <= 3) {
        return;
    }
    if (target_obj->type == 0 && DAT_0087abc3 != 0) {
        int32_t local_player = unit_get_local_player_weapon_index(object_index);
        if ((int16_t)local_player != -1) {
            biped_data *target_biped = (biped_data *)((uint8_t *)target_obj + k_unit_object_size);
            target_biped->unknown_500 = 0xf1;
            local_player_set_controlled_unit(target, (int16_t)local_player);
        }
    }
    biped->unknown_500 = 0xf1;
}

#if 0
Original Ghidra decompilation (0x55e0a0):

void FUN_0055e0a0(void)

{
  int iVar1;
  int iVar2;
  char cVar3;
  short sVar4;
  uint in_EAX;
  uint in_ECX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if (*(char *)(iVar1 + 0x500) < '\0') {
    if (in_EAX != 0xffffffff) {
      *(undefined1 *)(iVar1 + 0x500) = 0xf1;
      return;
    }
    *(char *)(iVar1 + 0x500) = *(char *)(iVar1 + 0x500) + '\x01';
    return;
  }
  if (in_EAX != 0xffffffff) {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    FUN_0042bd70();
    if ((*(int *)(iVar1 + 0x218) != -1) || (cVar3 = FUN_0044acc0(), cVar3 != '\0')) {
      if (*(uint *)(iVar1 + 0x4fc) != in_EAX) {
        *(uint *)(iVar1 + 0x4fc) = in_EAX;
        *(undefined1 *)(iVar1 + 0x500) = 0;
        return;
      }
      cVar3 = *(char *)(iVar1 + 0x500) + '\x01';
      *(char *)(iVar1 + 0x500) = cVar3;
      if ('\x03' < cVar3) {
        if (((*(short *)(iVar2 + 0xb4) == 0) && (DAT_0087abc3 != '\0')) &&
           (sVar4 = unit_get_local_player_weapon_index(), sVar4 != -1)) {
          *(undefined1 *)(iVar2 + 0x500) = 0xf1;
          local_player_set_controlled_unit();
        }
        *(undefined1 *)(iVar1 + 0x500) = 0xf1;
      }
    }
  }
  return;
}
#endif
