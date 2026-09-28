// player_add_kill_streak  (Ghidra: FUN_00479ba0; renamed per out/phase4/game_types_notes.md's
// own description of this trio: "0x68 and 0x6a are a two-entry kill-streak countdown; slot 0
// also sets object flag 0x10 and stamps the streak method into unit+0x422")
// address 0x479ba0, size 255 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// FIXED 2026-09-28: unit_data begins at object +0x1f4 (k_unit_data_offset) and its field offsets are absolute; the draft cast the object pointer itself, so unit fields landed 0x1f4 bytes low (e.g. flags at object +0x10).
// evidence: out/phase4/game_types_notes.md (player struct notes); types/game.h
//   player::kill_streak[2] (0x68); types/units.h unit_flags::_unit_flag_unknown_10 (0x10),
//   unit::unknown_422; types/memory.h data_array (maximum_count 0x20, size 0x22, data 0x34);
//   the leading bounds/salt check is a manually inlined datum-validity test (index in range,
//   slot identifier nonzero, and either no salt was supplied or it matches) -- the same shape
//   datum_get itself performs, just without the call.
// register convention: a player handle in EBX (unaff_EBX); `slot` and `amount` are this
//   function's own two stack parameters.
//   // blam-cc: EBX -> player_handle, stack -> slot, amount
// UNSURE: the object_try_and_get(3) call's object argument is elided by Ghidra; modeled here as
//   the player's own `unit`, consistent with every other reading of this idiom in this batch.
//   The final network_role check reads object_headers[player::unit].data directly (not through
//   object_try_and_get), matching types/objects.h object::network_role (0x004).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;      // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern int16_t network_game_mode;    // 0x00719720
extern data_array *object_headers;   // 0x008603b0

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void player_kill_streak_begin(int32_t slot, uint32_t player_handle); // this batch, 0x479d90;
    // blam-cc: EAX -> player_handle, stack -> slot
extern void player_kill_streak_continue(int32_t slot, uint32_t player_handle); // this batch, 0x479de0;
    // blam-cc: EAX -> player_handle, stack -> slot
extern void player_notify_kill_streak_update(int32_t slot, int16_t amount, uint32_t player_handle); // this batch,
    // 0x479aa0; blam-cc: ECX -> player_handle, stack -> slot, amount

// blam-cc: EBX -> player_handle, stack -> slot, amount
// Validates `player_handle` (index in range, slot identifier nonzero and matching the handle's
// own salt, if any) and `slot` (0 or 1; slot 0 additionally requires the player to have a live
// unit without unit_flags bit 0x10 set). On success, fires the streak-start/streak-continue
// notifications the first time this streak is touched, adds `amount` to
// player::kill_streak[slot], and (dedicated server only, and only for the player's own
// controlling unit) forwards the update via FUN_00479aa0. Returns 1 on success, 0 if any check
// failed.
uint8_t player_add_kill_streak(int32_t slot, int16_t amount, uint32_t player_handle)
{
    int16_t index;
    player *p;

    if (player_handle == 0xffffffff) {
        return 0;
    }
    index = (int16_t)player_handle;
    if (index < 0 || player_data->maximum_count <= index) {
        return 0;
    }

    p = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)index * player_data->size);
    if (p->identifier == 0) {
        return 0;
    }
    {
        int16_t salt = (int16_t)(player_handle >> 0x10);
        if (salt != 0 && p->identifier != salt) {
            return 0;
        }
    }

    if (slot < 0 || 1 < slot) {
        return 0;
    }
    if (slot == 0) {
        object *unit = object_try_and_get(p->unit, 3);
        if (unit == 0 || (((unit_data *)((uint8_t *)unit + k_unit_data_offset))->flags & _unit_flag_unknown_10) != 0) {
            return 0;
        }
    }

    {
        int16_t *streak = &p->kill_streak[slot];
        if (*streak == 0) {
            player_kill_streak_begin(slot, player_handle);
        } else if (current_game_engine == 0) {
            player_kill_streak_continue(slot, player_handle);
        }
        *streak = *streak + amount;
    }

    if (network_game_mode == 2) {
        object *owner_unit = (object *)((object_header *)object_headers->data)[p->unit & 0xffff].data;
        if (owner_unit->network_role == 0) {
            player_notify_kill_streak_update(slot, amount, player_handle);
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x479ba0), from tools/pack.py 0x479ba0:

undefined4 FUN_00479ba0(undefined4 param_1,undefined4 param_2)

{
  short *psVar1;
  int iVar2;
  short sVar3;
  short sVar4;
  int unaff_EBX;
  int iVar5;

  if (((unaff_EBX != -1) && (sVar4 = (short)unaff_EBX, -1 < sVar4)) &&
     (sVar4 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar5 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar4;
    sVar4 = *(short *)(iVar5 + *(int *)(DAT_0087a480 + 0x34));
    iVar5 = iVar5 + *(int *)(DAT_0087a480 + 0x34);
    if ((((sVar4 != 0) &&
         ((sVar3 = (short)((uint)unaff_EBX >> 0x10), sVar3 == 0 || (sVar4 == sVar3)))) &&
        (sVar4 = (short)param_1, -1 < sVar4)) &&
       ((sVar4 < 2 &&
        ((sVar4 != 0 ||
         ((iVar2 = object_try_and_get(3), iVar2 != 0 && ((*(byte *)(iVar2 + 0x204) & 0x10) == 0)))))
        ))) {
      psVar1 = (short *)(iVar5 + 0x68 + sVar4 * 2);
      if (*(short *)(iVar5 + 0x68 + sVar4 * 2) == 0) {
        FUN_00479d90(param_1);
      }
      else if (DAT_006f1d20 == 0) {
        FUN_00479de0(param_1);
      }
      *psVar1 = *psVar1 + (short)param_2;
      if ((DAT_00719720 == 2) &&
         (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                           (*(uint *)(iVar5 + 0x34) & 0xffff) * 0xc) + 4) == 0)) {
        FUN_00479aa0(param_1,param_2);
      }
      return 1;
    }
  }
  return 0;
}
#endif
