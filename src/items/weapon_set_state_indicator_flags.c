// weapon_set_state_indicator_flags  (Ghidra: FUN_004c5580; named from
// out/phase4/items_functions.md, "Sets one of two indicator flags on the item depending on its
// current weapon state and associated tag threshold fields")
// address 0x4c5580, size 164 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: types/items.h weapon_state (chamber_primary=3/chamber_secondary=4),
//   weapon_trigger_state.ejection_port_recovery (trigger+0x14); types/tags.h
//   WeaponTrigger.ejection_port_recovery_time (0xa4) and flags bit 7 (0x80,
//   ejects_during_chamber). Confirmed against the binary with an offsetof probe.
// register convention: item index in EAX.
// blam-cc: EAX -> item_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "fn_items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// While chambering, primes the corresponding trigger's ejection_port_recovery meter to 1.0 when
// its tag trigger both has a positive ejection_port_recovery_time and the ejects_during_chamber
// flag (bit 7 of the low flags byte).
void weapon_set_state_indicator_flags(datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *triggers;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    triggers = (WeaponTrigger *)weapon_tag->triggers.pointer;

    if (wd->state == _weapon_state_chamber_primary) {
        if (triggers[0].ejection_port_recovery_time > 0.0f && (int8_t)triggers[0].flags < 0) {
            wd->triggers[0].ejection_port_recovery = 1.0f;
        }
    } else if (wd->state == _weapon_state_chamber_secondary) {
        if (triggers[1].ejection_port_recovery_time > 0.0f && (int8_t)triggers[1].flags < 0) {
            wd->triggers[1].ejection_port_recovery = 1.0f;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4c5580):

void FUN_004c5580(void)

{
  uint *puVar1;
  int iVar2;
  char *pcVar3;
  uint in_EAX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((char)puVar1[0x8e] == '\x03') {
    pcVar3 = *(char **)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x500);
    if ((0.0 < *(float *)(pcVar3 + 0xa4)) && (*pcVar3 < '\0')) {
      puVar1[0x9d] = 0x3f800000;
    }
  }
  else if ((((char)puVar1[0x8e] == '\x04') &&
           (iVar2 = *(int *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x500),
           0.0 < *(float *)(iVar2 + 0x1b8))) && (*(char *)(iVar2 + 0x114) < '\0')) {
    puVar1[0xa7] = 0x3f800000;
    return;
  }
  return;
}
#endif
