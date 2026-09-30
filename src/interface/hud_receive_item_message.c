// hud_receive_item_message  (Ghidra: FUN_004ae200, renamed in the phase-4 review)
// address 0x4ae200, size 332 bytes
// name confidence: 0.5 (chosen)   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4ae200..0x4ae34b in the phase-4 review (the first rewrite
// was a 0.15 literal copy of a decompile that had lost every register argument). EAX is the
// incoming network message; a message whose first dword of its decode state is nonzero is
// dropped through 0x4ec670. Otherwise 0x4ec590 (EAX message, ECX out) decodes the 8 byte
// hud_item_message payload that hud_post_item_message (0x4ae350) encodes as message type 6.
// The payload goes to the first local player (players data_array walked with an inline
// data_iterator, signature data ^ 'reti') through hud_add_item_message (0x4ae400). Then, by
// the object type of the item tag: equipment (3) runs the powerup effect of its powerup_type
// (2 over shield 0x479710, 3 active camouflage 0x4797d0, 5 health 0x479890, EDX the player
// datum; over shield on a client also sets bit 4 of the unit object +0x106) and plays its
// pickup_sound (+0x31c); a weapon (2) plays its pickup_sound (+0x49c) through 0x543dd0 (EDX
// sound, gain 1.0). Only the first local player is handled.
// register convention: EAX message.
//   // blam-cc: message -> EAX
// reconciled: R16 the local iterator shadow struct (int32 next_index) is now types/memory.h data_iterator; the binary stores next_index as a WORD

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"
#include "fn_game.h"
#include "fn_interface.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480
extern int16_t network_game_mode;   // 0x00719720, 0 local, 1 client, 2 host

extern int8_t message_delta_decode_compound_field(void *message, hud_item_message *out_payload); // 0x4ec590, message decode, blam-cc: EAX message, ECX out
extern int32_t message_delta_decode_compound_field_staged(void *message); // 0x4ec670, networking; drops the message, blam-cc: EAX message
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator


extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern void sound_start_unspatialized(datum_index sound, float gain); // 0x543dd0, plays a 2D sound, blam-cc: EDX sound

// blam-cc: message -> EAX
// Client side of the HUD item message: posts the pickup text and plays the pickup feedback.
void hud_receive_item_message(void **message)
{
    hud_item_message payload;
    data_iterator iterator;
    player *p;
    int16_t *item_tag;
    datum_index sound;

    if (*(int32_t *)*message != 0) {
        message_delta_decode_compound_field_staged(message);
        return;
    }
    if (message_delta_decode_compound_field(message, &payload) == 0) {
        return;
    }
    iterator.data = player_data;
    iterator.next_index = 0; // the binary writes only the low word
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    for (p = (player *)data_iterator_next(&iterator); p != 0;
         p = (player *)data_iterator_next(&iterator)) {
        if (p->local_player_index != -1) {
            break;
        }
    }
    if (p == 0) {
        return;
    }

    hud_add_item_message(p->local_player_index, payload.item_definition, payload.kind, payload.count);
    item_tag = (int16_t *)tag_instances[payload.item_definition & 0xffff].data;
    if (item_tag[0] == 3) { // equipment
        switch (*(int16_t *)((uint8_t *)item_tag + 0x308)) { // Equipment powerup_type
        case 2: // over shield
            player_trigger_shield_recharge_effect(iterator.index);
            if (network_game_mode == 1) {
                object *unit = object_try_and_get(p->unit, 1);
                if (unit != 0) {
                    *((uint8_t *)unit + 0x106) |= 0x10;
                }
            }
            break;
        case 3: // active camouflage
            player_trigger_kill_streak_effect(iterator.index);
            break;
        case 5: // health
            player_trigger_full_health_effect(iterator.index);
            break;
        }
        sound = *(datum_index *)((uint8_t *)item_tag + 0x31c); // Equipment pickup_sound tag id
    } else if (item_tag[0] == 2 && item_tag != 0) { // weapon
        sound = *(datum_index *)((uint8_t *)item_tag + 0x49c); // Weapon pickup_sound tag id
    } else {
        return;
    }
    if (sound != (datum_index)-1) {
        sound_start_unspatialized(sound, 1.0f);
    }
}

#if 0
Original Ghidra decompilation (0x4ae200):

void FUN_004ae200(void)

{
  short sVar1;
  short *psVar2;
  char cVar3;
  undefined4 *in_EAX;
  int iVar4;
  uint local_1c;
  undefined4 local_16;

  if (*(int *)*in_EAX != 0) {
    FUN_004ec670();
    return;
  }
  cVar3 = FUN_004ec590();
  if (cVar3 != '\0') {
    iVar4 = data_iterator_next();
    while (iVar4 != 0) {
      if (*(short *)(iVar4 + 2) != -1) {
        FUN_004ae400(local_16);
        psVar2 = *(short **)((local_1c & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if (*psVar2 != 3) {
          if (*psVar2 != 2) {
            return;
          }
          if (psVar2 == (short *)0x0) {
            return;
          }
          iVar4 = *(int *)(psVar2 + 0x24e);
          goto LAB_004ae327;
        }
        sVar1 = psVar2[0x184];
        if (sVar1 == 2) {
          FUN_00479710();
          if ((DAT_00719720 == 1) && (iVar4 = object_try_and_get(1), iVar4 != 0)) {
            *(byte *)(iVar4 + 0x106) = *(byte *)(iVar4 + 0x106) | 0x10;
          }
        }
        else {
          if (sVar1 == 3) {
            FUN_004797d0();
            iVar4 = *(int *)(psVar2 + 0x18e);
            goto LAB_004ae327;
          }
          if (sVar1 == 5) {
            FUN_00479890();
            iVar4 = *(int *)(psVar2 + 0x18e);
            goto LAB_004ae327;
          }
        }
        iVar4 = *(int *)(psVar2 + 0x18e);
LAB_004ae327:
        if (iVar4 == -1) {
          return;
        }
        FUN_00543dd0(0x3f800000);
        return;
      }
      iVar4 = data_iterator_next();
    }
  }
  return;
}
#endif
