// player_check_vehicle_interaction  (Ghidra: FUN_00478600; renamed -- dispatched for object_type
// vehicle candidates by the sibling scanners FUN_00478400/FUN_00478500, this batch)
// address 0x478600, size 366 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against 0x478600; fixed the flip action's object (EBX = the vehicle))
// evidence: types/objects.h object::vitality_flags (0x106), object::up (0x080, hence up.z at
//   0x088), object::velocity (0x068), object::angular_velocity (0x08c); types/tags.h
//   GlobalsPlayerControl::minimum_angle_for_vehicle_flipping (0x70, confirmed by exact offset
//   arithmetic from the struct's own field list) reached through Globals::player_control
//   (TagReflexive, pointer at absolute +0x114) on global_globals (0x00746fa0); player::unit
//   (types/game.h, +0x34); player_set_pending_interaction_action (0x478e00, this batch, whose
//   priority-type constant 0xb already means "clear").
// register convention: none -- both are genuine stack parameters (Ghidra's own param_1/param_2).
// UNSURE: unit_current_weapon_type_is_2_or_3's exact effect (a boolean gate on the examining player's own unit,
//   elided argument); object+0x4cc's bit 0x10 and object+0x324 (both read on the candidate
//   vehicle, presumably "is occupied" style fields) are not named anywhere in types/units.h or
//   types/objects.h for this module, kept as raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern Globals *global_globals;    // 0x00746fa0

extern double cos(double x); // x87 FCOS
extern uint8_t unit_current_weapon_type_is_2_or_3(uint32_t unit_index); // 0x56bd60, not in this batch; UNSURE exact signature
extern int16_t unit_find_best_seat_to_enter(uint32_t unit_index, uint32_t vehicle_index,
    uint32_t *out_seat); // 0x566560, UNSURE exact signature


// Examines whether `player_index`'s unit can interact with the vehicle `candidate_object`.
// Unless it is vitality-frozen: if the vehicle is tipped past minimum_angle_for_vehicle_flipping
// (its up.z at or below cos(pi/2 - that angle)), clears the player's pending interaction (when
// nothing already occupies it); otherwise, if both the player's own unit and the vehicle are
// nearly stationary, looks for a seat to enter and sets the pending interaction to "enter as
// driver" (9) or "enter as passenger" (8) accordingly.
void player_check_vehicle_interaction(uint32_t player_index, uint32_t candidate_object)
{
    object *vehicle = (object *)((object_header *)object_data->data)[candidate_object & 0xffff].data;

    if ((*((uint8_t *)&vehicle->vitality_flags) & 4) == 0) {
        GlobalsPlayerControl *player_control = (GlobalsPlayerControl *)global_globals->player_control.pointer;
        double flip_threshold = cos(1.5707963705062866 - (double)player_control[0].minimum_angle_for_vehicle_flipping);

        if ((double)vehicle->up.k <= flip_threshold) {
            if ((*(uint8_t *)&((vehicle_object *)vehicle)->vehicle.flags & 0x10) == 0 &&
                *(int32_t *)&((vehicle_object *)vehicle)->unit.driver_unit_index == -1) {
                // FIXED (0x47875f): EBX = the vehicle (ebp), the flip target; the draft passed -1
                player_set_pending_interaction_action(0xb, (int16_t)0xffff, player_index, candidate_object);
            }
        } else {
            player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
            uint32_t unit_index = (uint32_t)p->unit;

            if (unit_current_weapon_type_is_2_or_3(unit_index) == 0) {
                object *unit_obj = (object *)((object_header *)object_data->data)[unit_index & 0xffff].data;
                float unit_speed_sq = unit_obj->velocity.k * unit_obj->velocity.k +
                    unit_obj->velocity.j * unit_obj->velocity.j + unit_obj->velocity.i * unit_obj->velocity.i;
                float vehicle_spin_sq = vehicle->angular_velocity.k * vehicle->angular_velocity.k +
                    vehicle->angular_velocity.j * vehicle->angular_velocity.j +
                    vehicle->angular_velocity.i * vehicle->angular_velocity.i;

                if (unit_speed_sq < 0.01f && vehicle_spin_sq < 0.01f) {
                    uint32_t seat = 0xffffffff;
                    int16_t result = unit_find_best_seat_to_enter(unit_index, candidate_object, &seat);
                    if (result == 1) {
                        player_set_pending_interaction_action(9, (int16_t)seat, player_index, candidate_object);
                        return;
                    }
                    if (result == 2) {
                        player_set_pending_interaction_action(8, (int16_t)seat, player_index, candidate_object);
                        return;
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x478600), from tools/pack.py 0x478600:

void FUN_00478600(uint param_1,uint param_2)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  char cVar5;
  short sVar6;
  float10 fVar7;

  uVar4 = param_2;
  iVar1 = *(int *)(DAT_008603b0 + 0x34);
  iVar2 = *(int *)(iVar1 + 8 + (param_2 & 0xffff) * 0xc);
  if ((*(byte *)(iVar2 + 0x106) & 4) == 0) {
    fVar7 = (float10)fcos((float10)1.5707964 -
                          (float10)*(float *)(*(int *)(DAT_00746fa0 + 0x114) + 0x70));
    if ((float10)*(float *)(iVar2 + 0x88) <= fVar7) {
      if (((*(byte *)(iVar2 + 0x4cc) & 0x10) == 0) && (*(int *)(iVar2 + 0x324) == -1)) {
        player_set_pending_interaction_action(0xb,0xffffffff);
      }
    }
    else {
      uVar3 = *(uint *)((param_1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34);
      cVar5 = FUN_0056bd60();
      if (((cVar5 == '\0') &&
          (iVar1 = *(int *)(iVar1 + 8 + (uVar3 & 0xffff) * 0xc),
          *(float *)(iVar1 + 0x70) * *(float *)(iVar1 + 0x70) +
          *(float *)(iVar1 + 0x6c) * *(float *)(iVar1 + 0x6c) +
          *(float *)(iVar1 + 0x68) * *(float *)(iVar1 + 0x68) < 0.01)) &&
         (*(float *)(iVar2 + 0x94) * *(float *)(iVar2 + 0x94) +
          *(float *)(iVar2 + 0x90) * *(float *)(iVar2 + 0x90) +
          *(float *)(iVar2 + 0x8c) * *(float *)(iVar2 + 0x8c) < 0.01)) {
        param_2 = 0xffffffff;
        sVar6 = unit_find_best_seat_to_enter(uVar3,uVar4,&param_2);
        if (sVar6 == 1) {
          player_set_pending_interaction_action(9,param_2);
          return;
        }
        if (sVar6 == 2) {
          player_set_pending_interaction_action(8,param_2);
          return;
        }
      }
    }
  }
  return;
}
#endif
