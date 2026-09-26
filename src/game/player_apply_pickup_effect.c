// player_apply_pickup_effect  (Ghidra: FUN_00479930; renamed -- applies the effect of an
// "equipment" pickup (health, shield, kill-streak boost) and deletes the picked-up object)
// address 0x479930, size 361 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/functions.json callee list (object_restore_full_body_vitality,
//   object_shield_recharge_start, object_delete, all already established); types/game.h
//   player_globals::respawn_stagger (0x0e); this batch's player_add_kill_streak (cases 3/4 map
//   to kill-streak slots 0/1). The tag field at +0x308 (a small discriminator: 1=respawn-stagger
//   boost, 2=shield recharge, 3/4=kill-streak slot, 5=full health restore) is the same
//   undocumented offset already flagged UNSURE in player_check_vehicle_boarding_interaction.c
//   and its lightweight sibling (this batch); the __ftol source float and DAT_006b0b80 are not
//   named anywhere in this batch's evidence.
// register convention: none -- both are genuine stack parameters (Ghidra's own
//   param_1/param_2).
// UNSURE: __ftol's source float (elided register argument, presumably a duration/amount field
//   on the picked-up object's tag data); DAT_006b0b80's identity (a byte flag global, offset
//   +2 set to 1 on the respawn-stagger case); the three player_trigger_*_effect siblings (this
//   batch) are called here with no visible argument in Ghidra's own decompilation -- EDX must
//   still hold `player_index` from this function's own top-of-body computation, matched here by
//   passing it explicitly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern data_array *player_data;              // 0x0087a480
extern data_array *object_headers;           // 0x008603b0
extern tag_instance *tag_instances;          // 0x0087bc14
extern player_globals *local_player_globals; // 0x0087a478
extern uint8_t *global_006b0b80;             // 0x006b0b80, UNSURE identity, see header note
extern int16_t network_game_mode;            // 0x00719720

extern uint8_t object_shield_recharge_start(uint32_t object_index); // 0x4edba0, UNSURE exact signature
extern void player_trigger_shield_recharge_effect(uint32_t player_index); // this batch, 0x479710
extern uint8_t object_restore_full_body_vitality(uint32_t object_index); // 0x4ed9d0, UNSURE exact signature
extern void player_trigger_full_health_effect(uint32_t player_index); // this batch, 0x479890
extern uint8_t player_add_kill_streak(int32_t slot, int16_t amount, uint32_t player_handle); // this batch, 0x479ba0
extern void player_trigger_kill_streak_effect(uint32_t player_index); // this batch, 0x4797d0
extern void hud_post_item_message(int16_t count, int32_t source, uint8_t kind, int16_t local_player_index,
    int8_t machine_id); // 0x4ae350, EAX count, ECX source, DL kind, stack (local player, machine)
extern void equipment_pickup_play_sound(uint32_t object_index); // 0x4bbb50, EAX object
extern void object_delete(uint32_t object_index); // 0x4f5bd0

// Applies the pickup effect of `pickup_object`'s tag (discriminated by its +0x308 field) to
// `player_index`, then always notifies via hud_post_item_message/equipment_pickup_play_sound and deletes the pickup.
// No-op (does not delete the pickup) if the source amount (__ftol'd from an elided float) is
// not positive, or if the specific effect's own gate (shield recharge / full vitality restore /
// kill-streak) reports failure.
// REWRITTEN from objdump 0x479930..0x479a98: the amount is __ftol(powerup tag +0x30c * 30) tested as a 16-bit value;
// the shield / health calls take the player's UNIT (player+0x34) in EAX; the kill-streak effect fires when the low 16
// bits of the slot are zero (test bp,bp); the HUD message gets count 0, the pickup's definition tag as its source,
// kind 0, the local player index and the byte at player+0x64; the sound gets the pickup object; object_delete tail.
void player_apply_pickup_effect(uint32_t player_index, uint32_t pickup_object)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    object *pickup = (object *)((object_header *)object_headers->data)[pickup_object & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[pickup->definition_tag & 0xffff].data;
    int16_t amount = (int16_t)(int32_t)(*(float *)(tag + 0x30c) * 30.0f); // fld / fmul 30 / __ftol, then test ax
    int16_t discriminator;

    if (amount < 1) {
        return;
    }

    discriminator = *(int16_t *)(tag + 0x308);
    if (discriminator == 1) {
        local_player_globals->respawn_stagger = local_player_globals->respawn_stagger + amount;
        global_006b0b80[2] = 1;
    } else if (discriminator == 2) {
        if (object_shield_recharge_start(p->unit) == 0) {
            return;
        }
        if (network_game_mode == 0) {
            player_trigger_shield_recharge_effect(player_index);
        }
    } else if (discriminator == 5) {
        if (object_restore_full_body_vitality(p->unit) == 0) {
            return;
        }
        if (network_game_mode == 0) {
            player_trigger_full_health_effect(player_index);
        }
    } else {
        int32_t slot = (int32_t)player_index;
        if (discriminator == 3) {
            slot = 0;
        } else if (discriminator == 4) {
            slot = 1;
        }
        if (player_add_kill_streak(slot, amount, player_index) == 0) {
            return;
        }
        if ((int16_t)slot == 0 && network_game_mode == 0) {
            player_trigger_kill_streak_effect(player_index);
        }
    }

    hud_post_item_message(0, (int32_t)pickup->definition_tag, 0, p->local_player_index,
                          (int8_t)*((uint8_t *)p + 0x64));
    if (p->local_player_index != -1) {
        equipment_pickup_play_sound(pickup_object);
    }
    object_delete(pickup_object);
}

#if 0
Original Ghidra decompilation (0x479930), from tools/pack.py 0x479930:

void FUN_00479930(uint param_1,uint param_2)

{
  short sVar1;
  int iVar2;
  char cVar3;
  undefined4 uVar4;
  int iVar5;

  iVar5 = (param_1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  iVar2 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar4 = __ftol();
  if ((short)uVar4 < 1) {
    return;
  }
  sVar1 = *(short *)(iVar2 + 0x308);
  if (sVar1 == 1) {
    *(short *)(DAT_0087a478 + 0xe) = *(short *)(DAT_0087a478 + 0xe) + (short)uVar4;
    *(undefined1 *)(DAT_006b0b80 + 2) = 1;
  }
  else if (sVar1 == 2) {
    cVar3 = object_shield_recharge_start();
    if (cVar3 == '\0') {
      return;
    }
    if (DAT_00719720 == 0) {
      FUN_00479710();
    }
  }
  else if (sVar1 == 5) {
    cVar3 = object_restore_full_body_vitality();
    if (cVar3 == '\0') {
      return;
    }
    if (DAT_00719720 == 0) {
      FUN_00479890();
    }
  }
  else {
    if (sVar1 == 3) {
      param_1 = 0;
    }
    else if (sVar1 == 4) {
      param_1 = 1;
    }
    cVar3 = FUN_00479ba0(param_1,uVar4);
    if (cVar3 == '\0') {
      return;
    }
    if (((short)param_1 == 0) && (DAT_00719720 == 0)) {
      FUN_004797d0();
    }
  }
  FUN_004ae350(*(undefined2 *)(iVar5 + 2),*(undefined1 *)(iVar5 + 100));
  if (*(short *)(iVar5 + 2) != -1) {
    FUN_004bbb50();
  }
  object_delete();
  return;
}
#endif
