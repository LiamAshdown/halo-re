// player_effect_mark_damage_direction_dispatch  (Ghidra: FUN_00456ad0, still unnamed there)
// address 0x456ad0, size 237 bytes
// name confidence: 0.3   rewrite confidence: 0.2 (low confidence: param_1's shape, the
//   message_delta_decode_compound_field/message_delta_decode_compound_field_staged gate and the exact fields of the local descriptor forwarded to
//   player_effect_mark_damage_direction are none of them established elsewhere in this batch)
// evidence: this module's player_effect_mark_damage_direction (0x456cf0), which this function's
//   tail call feeds. out/phase4/effects_types_notes.md's misattribution table places this
//   address in the player_effect group, not the "contrail" framing functions.md guessed at
//   phase 2.
// register convention: a small descriptor pointer in EAX (in_EAX, first dword tested against
//   zero to pick between two foreign gate functions).
//   // blam-cc: in_EAX -> descriptor
// UNSURE (extensive): message_delta_decode_compound_field/message_delta_decode_compound_field_staged are foreign gate functions this batch does not
//   otherwise examine; the on-stack player_data-derived value built at local_64/local_58 with
//   the same XOR-0x69746572 pattern seen in player_weapon_locality_for_object.c is modeled the
//   same way there -- as an ordinary data_iterator over player_data -- rather than
//   reverse-engineered. The loop only ever uses the FIRST live player record and only reaches
//   the tail call when the player has a valid local_player_index, matching the decompile's
//   early `return` inside the loop body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480

extern uint8_t message_delta_decode_compound_field(int32_t a1, int32_t a2, int32_t a3); // 0x4ec590, UNSURE: out of range
extern void message_delta_decode_compound_field_staged(void); // 0x4ec670, UNSURE: out of range
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void player_effect_mark_damage_direction(uint32_t object_index, uint32_t *descriptor,
    void *direction_block, void *rotation_block, float falloff); // 0x456cf0, this module

void player_effect_mark_damage_direction_dispatch(uint32_t *descriptor)
{
    if (*(int32_t *)*descriptor != 0) {
        message_delta_decode_compound_field_staged();
        return;
    }

    if (message_delta_decode_compound_field(0, 0, 0) != 0) {
        data_iterator iterator;
        player *record;
        uint32_t local_descriptor[7] = {0, 0, 0, 0, 0, 0, 0};

        iterator.data = player_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)0xffffffff;

        record = (player *)data_iterator_next(&iterator);
        while (record != (player *)0) {
            if (record->local_player_index != -1) {
                local_descriptor[3] = 0xffffffff; // UNSURE: only ever the "no fourth object"
                                    // case, since the counter this branches on is always 0 here
                player_effect_mark_damage_direction((uint32_t)iterator.index, local_descriptor,
                    &local_descriptor[3], &local_descriptor[4], 0.0f); // UNSURE: field bindings
                                    // guessed, see file header
                return;
            }
            record = (player *)data_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x456ad0):

void FUN_00456ad0(void)

{
  char cVar1;
  undefined4 *in_EAX;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  undefined4 uVar5;
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  uint local_64;
  undefined2 local_60;
  undefined4 local_5c;
  uint local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_48;

  if (*(int *)*in_EAX != 0) {
    FUN_004ec670();
    return;
  }
  iVar4 = 0;
  uVar5 = 0;
  local_78 = 0;
  local_74 = 0;
  local_70 = 0;
  local_6c = 0;
  uVar3 = 0;
  local_68 = 0;
  cVar1 = FUN_004ec590(0,0,0);
  if (cVar1 != '\0') {
    local_64 = DAT_0087a480;
    local_58 = DAT_0087a480 ^ 0x69746572;
    local_60 = 0;
    local_5c = 0xffffffff;
    iVar2 = data_iterator_next();
    local_54 = uVar3;
    local_50 = uVar5;
    while (iVar2 != 0) {
      if (*(short *)(iVar2 + 2) != -1) {
        local_48 = 0xffffffff;
        if (iVar4 != 0) {
          local_48 = *(undefined4 *)(*(int *)(PTR_DAT_00687130 + 0x28) + iVar4 * 4);
        }
        FUN_00456cf0(&local_54,&local_78,local_6c,local_68);
        return;
      }
      iVar2 = data_iterator_next();
    }
  }
  return;
}
#endif
