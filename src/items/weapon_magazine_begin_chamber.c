// weapon_magazine_begin_chamber  (Ghidra: FUN_004c3b00; named per types/items.h
// weapon_magazine_state_enum comment block: "weapon_magazine_begin_chamber (0x4c3b00)")
// address 0x4c3b00, size 181 bytes
// VERIFIED against disassembly 0x4c3b00..0x4c3bb5 (2026-09-30)
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against objdump 0x4c3b00..0x4c3bb4)
// evidence: types/items.h weapon_magazine_state_enum (_weapon_magazine_chambering = 3),
//   weapon_state (_weapon_state_chamber_primary=3/_secondary=4); types/tags.h
//   WeaponMagazine.chamber_time (0x1c).
// register convention: item index is a Ghidra-recognized parameter; magazine index in EAX.
// blam-cc: stack -> item_index, EAX -> magazine_index
// The x87 operand Ghidra dropped is `fld [magazine_tag+0x1c]; fmul [0x672ac8]` (= 30.0f), then __ftol
// into state_ticks: chamber_time * 30.0f, confirmed in the disassembly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force); // 0x4c5670
extern uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, real scale_a,
    real scale_b); // 0x4c47d0, ECX item, EDI tag, stack (scale_a, scale_b)

// Starts chambering a round: only when the magazine is idle or chamber-pending and every
// trigger/weapon state is idle. Sets the magazine to "chambering" for chamber_time ticks.
void weapon_magazine_begin_chamber(datum_index item_index, int16_t magazine_index)
{
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponMagazine *magazine_tag;
    weapon_magazine_state *magazine;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;
    magazine = &wd->magazines[magazine_index];

    if ((magazine->state == 0 || magazine->state == _weapon_magazine_chamber_pending) &&
        wd->triggers[0].effect_state == 0 && wd->triggers[1].effect_state == 0 && wd->state == 0) {
        weapon_set_state(item_index, magazine_index + 3, 0);
        weapon_play_trigger_tag_effect(item_index, *(datum_index *)&magazine_tag->chambering_effect.tag_id, 0, 0); // ECX item, EDI tag +0x54 (chambering_effect.tag_id)
        magazine->state = _weapon_magazine_chambering;
        magazine->state_ticks = (int16_t)(magazine_tag->chamber_time * 30.0f);
    }
}

#if 0
Original Ghidra decompilation (0x4c3b00):

void FUN_004c3b00(uint param_1)

{
  short *psVar1;
  int iVar2;
  short sVar3;
  int in_EAX;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  psVar1 = (short *)(iVar2 + 0x2b0 + (short)in_EAX * 0xc);
  if ((((*psVar1 == 0) || (*psVar1 == 2)) && (*(char *)(iVar2 + 0x261) == '\0')) &&
     ((*(char *)(iVar2 + 0x289) == '\0' && (*(char *)(iVar2 + 0x238) == '\0')))) {
    weapon_set_state(in_EAX + 3,0);
    weapon_play_trigger_tag_effect(0,0);
    *psVar1 = 3;
    sVar3 = __ftol();
    psVar1[1] = sVar3;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
