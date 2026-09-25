// biped_update_target_lock_timer  (Ghidra: biped_update_target_lock_timer, renamed)
// address 0x55e0a0, size 237 bytes
// name confidence: 0.35   rewrite confidence: 0.35
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

extern void actor_squad_react_to_grenade_for_vehicle_occupants(void);                              // UNSURE module
extern char recorded_animation_object_is_playing(void);                               // UNSURE module
extern int16_t unit_get_local_player_weapon_index(void);      // 0x4726b0
extern void local_player_set_controlled_unit(void);           // 0x474fc0, UNSURE args

// Maintains a per-biped target-lock counter (biped_data.unknown_500): once the same target has
// been tracked for more than 3 ticks, saturates the counter and, if the target is a local-player
// biped currently holding a weapon, forces its own counter to saturate too and hands it control
// via local_player_set_controlled_unit.
void biped_update_target_lock_timer(datum_index target, uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if ((int8_t)biped->unknown_500 < 0) {
        if (target != k_datum_index_none) {
            biped->unknown_500 = 0xf1;
            return;
        }
        biped->unknown_500 = biped->unknown_500 + 1;
        return;
    }

    if (target != k_datum_index_none) {
        object *target_obj = ((object_header *)object_data->data)[target & 0xffff].data;
        unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

        actor_squad_react_to_grenade_for_vehicle_occupants();
        if (unit->controlling_player != k_datum_index_none || recorded_animation_object_is_playing()) {
            if (biped->unknown_4fc != target) {
                biped->unknown_4fc = target;
                biped->unknown_500 = 0;
                return;
            }
            biped->unknown_500 = biped->unknown_500 + 1;
            if ((int8_t)biped->unknown_500 > 3) {
                if (target_obj->type == 0 && DAT_0087abc3 != 0 &&
                    unit_get_local_player_weapon_index() != -1) {
                    biped_data *target_biped = (biped_data *)((uint8_t *)target_obj + k_unit_object_size);
                    target_biped->unknown_500 = 0xf1;
                    local_player_set_controlled_unit();
                }
                biped->unknown_500 = 0xf1;
            }
        }
    }
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
