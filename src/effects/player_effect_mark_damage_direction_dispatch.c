// player_effect_mark_damage_direction_dispatch  (Ghidra: FUN_00456ad0, still unnamed there)
// address 0x456ad0, size 237 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// REWRITTEN from objdump 0x456ad0..0x456bbc: the client side of the type 0xb message that
//   player_effect_send_network_update (0x456bc0) builds. EAX: the decode context. When **context is set the
//   message is staged (0x4ec670); otherwise it is decoded (0x4ec590) into {tag, object network id, flags,
//   direction, blend, amount} and the first player with a local index gets
//   player_effect_mark_damage_direction(player, {tag, flags, object}, &direction, blend, amount), the network id
//   mapped back to an object through object_network_id_table +0x28 (-1 for id 0).
// blam-cc: EAX -> context

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480
extern network_id_table *object_network_id_table; // 0x00687130

extern uint8_t message_delta_decode_compound_field(void **context, void *destination); // 0x4ec590, EAX, ECX
extern uint8_t message_delta_decode_compound_field_staged(void **context); // 0x4ec670, EAX
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, EDI
extern void player_effect_mark_damage_direction(datum_index player_index, const damage_data *dd,
    const real_vector3d *direction, float random_blend, float damage_amount); // 0x456cf0, EAX, stack

void player_effect_mark_damage_direction_dispatch(void **context)
{
    uint32_t fields[8];
    data_iterator iterator;
    player *record;
    damage_data dd;

    if (**(int32_t **)context != 0) {
        message_delta_decode_compound_field_staged(context);
        return;
    }
    memset(fields, 0, sizeof(fields));
    if (!message_delta_decode_compound_field(context, fields)) {
        return;
    }
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)0xffffffff;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    for (record = (player *)data_iterator_next(&iterator); record != 0;
         record = (player *)data_iterator_next(&iterator)) {
        if (record->local_player_index != -1) {
            dd.damage_effect_tag = fields[0];
            dd.responsible_object = (fields[1] != 0) ?
                object_network_id_table->handles[fields[1]] : k_datum_index_none;
            dd.flags = fields[2];
            player_effect_mark_damage_direction(iterator.index, &dd, (const real_vector3d *)&fields[3],
                *(float *)&fields[6], *(float *)&fields[7]);
            return;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
