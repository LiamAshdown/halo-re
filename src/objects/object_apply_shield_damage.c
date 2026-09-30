// object_apply_shield_damage
// address 0x4ef820, size 976 bytes
// name confidence: 0.6 (Ghidra-recovered name)
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x4ef820..0x4efbee and its one caller, object_apply_damage (0x4eef29). EBX is the
//   caller's damage record {object, shield damage dealt (+4), depleted this call (+8)}; the stack is (target,
//   collision geometry tag, material, damage effect block (tag +0x1c4), &notify flags, &shield damage out,
//   &damage in/out, is_local, apply_state). With no shield left the whole amount passes to the body. Otherwise
//   the maximum shield (+0xdc) is scaled by the difficulty table (0x46fe70, ECX 2, AX team) unless a
//   single-player category 1 hit lands on team 1; the damage reaching the shield is the amount less the
//   material leak (+0x28) -- all of it for friendly damage the geometry always shields (notify 0x10 and
//   geometry flag 4) -- raised towards the failing leak fraction (+0xf4) through the geometry's transition
//   function (CX = +0xec) below the failure threshold (+0xf0). A recharging shield (+0x106 bit 0x10) takes it
//   all. The scaled damage (material +0x2c, effect block +0x3c[shield material +0xd2], divided by the difficulty
//   multiplier for notify 0x10|0x20) either depletes the shield (more than is left, or side effect 3: the
//   overflow passes on, the shield is zeroed when local and the depleted flag / notify 8 / record +8 are set
//   when apply_state) or is subtracted (local, +0x106 bit 0x800 clear), dispatching the shield-low effect
//   (0x4efff0, EAX target, ECX geometry +0x194) once below +0x184. Unless negligible (< 0.0001), apply_state
//   restarts the recent-damage bookkeeping (+0xe8, +0xf4, +0xfc) and records the dealt fraction. Locally, a hit
//   of at least the minimum stun damage (+0x108) or an empty shield sets the stun ticks (+0x104) to
//   (int)(stun time +0x10c * 30).
// VERIFIED against disassembly 0x4ef820..0x4efbee (2026-09-30): the one __ftol (0x4efbae) converts
//   geometry +0x10c * 30.0 into the stun ticks word, as written below; every branch, callee register/stack argument
//   and store was compared.
// blam-cc: EBX -> record, stack=(target_index, geometry, material, effect_block, notify_flags,
//   shield_damage_out, remaining_damage, is_local, apply_state)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_main_globals *main_game_globals; // 0x006b0b80

extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification); // 0x46fe10, stack, CX
extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70, ECX, AX
extern real transition_function_evaluate(transition_function_t type, real phase); // 0x4ccac0, CX, stack
extern void object_set_shield_depleted_flag(uint32_t object_index); // 0x4edb10, EDI
extern void object_dispatch_effect_notify(uint32_t forwarded_eax, uint32_t forwarded_ecx); // 0x4efff0, EAX, ECX

void object_apply_shield_damage(uint32_t target_index, uint8_t *geometry, uint8_t *material, uint8_t *effect_block,
    uint32_t *notify_flags, float *shield_damage_out, float *remaining_damage, uint8_t is_local,
    uint8_t apply_state, object_shield_impulse_result *record)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[target_index & 0xffff].data;
    float *shield = (float *)(obj + 0xe4);
    uint16_t *vitality_flags = (uint16_t *)(obj + 0x106);
    float passthrough = *remaining_damage;   // [esp+0x14]
    float to_shield = *remaining_damage;     // [esp+0x10]
    float maximum;                           // [esp+0x18]
    float inverse_maximum;                   // [esp+0x1c]
    uint8_t negligible = 0;
    uint8_t unscaled = 0;

    record->shield_damage_dealt = 0.0f;
    record->depleted_this_call = 0;
    if (current_game_engine == 0 && *(int16_t *)(effect_block + 0x2) == 1 && ((object *)obj)->owner_team == 1) {
        unscaled = 1;
    }
    if (!(*shield > 0.0f)) {
        to_shield = 0.0f;
        if (is_local != 1) {
            goto done;
        }
        *shield = 0.0f;
        goto stun;
    }
    maximum = ((object *)obj)->maximum_shield_vitality;
    if (!unscaled) {
        maximum = weapon_get_zoom_fov_resolved(2, ((object *)obj)->owner_team) * maximum;
    }
    inverse_maximum = (maximum > 0.0f) ? 1.0f / maximum : 0.0f;
    if ((*notify_flags & 0x10) == 0 || (*geometry & 4) == 0) {
        float threshold = *(float *)(geometry + 0xf0);

        to_shield = (1.0f - *(float *)(material + 0x28)) * passthrough;
        if (*shield <= threshold && threshold > 0.0f) {
            real t = transition_function_evaluate(*(transition_function_t *)(geometry + 0xec), *shield / threshold);
            float leak = *(float *)(geometry + 0xf4);

            to_shield = ((1.0f - leak) * t + leak) * to_shield;
        }
    }
    if (*vitality_flags & 0x10) {
        to_shield = passthrough;
        passthrough = 0.0f;
    } else {
        float scaled;
        float dealt;

        if (to_shield < 0.0f) {
            to_shield = 0.0f;
        }
        passthrough = passthrough - to_shield;
        if ((*notify_flags & 0x10) && (*notify_flags & 0x20)) {
            real multiplier = weapon_get_zoom_fov(0, main_game_globals->difficulty);

            if (multiplier > 0.0f) {
                to_shield = to_shield / multiplier;
            }
        }
        scaled = to_shield * *(float *)(material + 0x2c) *
            *(float *)(effect_block + 0x3c + *(int16_t *)(geometry + 0xd2) * 4);
        if (scaled < 0.0001f) {
            negligible = 1;
        }
        dealt = inverse_maximum * scaled;
        if (dealt > *shield || *(int16_t *)effect_block == 3) {
            float overflow = scaled - maximum * *shield;

            if (overflow > 0.0f) {
                passthrough = overflow + passthrough;
            }
            if (is_local == 1) {
                *shield = 0.0f;
            }
            if ((*vitality_flags & 8) == 0 && apply_state == 1) {
                object_set_shield_depleted_flag(target_index);
                *notify_flags |= 8;
                record->depleted_this_call = 1;
            }
        } else {
            if (is_local == 1 && (*vitality_flags & 0x800) == 0) {
                *shield = *shield - dealt;
            }
            if ((*vitality_flags & 2) == 0 && *shield < *(float *)(geometry + 0x184)) {
                object_dispatch_effect_notify(target_index, *(uint32_t *)(geometry + 0x194));
                *vitality_flags |= 2;
            }
        }
        if (negligible) {
            goto local_stun;
        }
    }
    if (apply_state == 1) {
        float fraction = (*remaining_damage - passthrough) * inverse_maximum;
        float recent;

        ((object *)obj)->shield_damage_ticks = 0;
        if ((*vitality_flags & 8) == 0) {
            ((object *)obj)->current_shield_damage = 1.0f;
        }
        recent = fraction + ((object *)obj)->recent_shield_damage;
        ((object *)obj)->recent_shield_damage = recent;
        if (((object *)obj)->current_shield_damage > 1.0f) {
            ((object *)obj)->current_shield_damage = 1.0f;
        }
        if (recent > 1.0f) {
            ((object *)obj)->recent_shield_damage = 1.0f;
        }
        record->shield_damage_dealt = fraction;
    }
local_stun:
    if (is_local != 1) {
        goto done;
    }
stun:
    if (!(to_shield < *(float *)(geometry + 0x108)) || *shield == 0.0f) {
        ((object *)obj)->shield_stun_ticks = (int16_t)(int32_t)(*(float *)(geometry + 0x10c) * 30.0f);
    }
done:
    *shield_damage_out = to_shield;
    *remaining_damage = passthrough;
}

#if 0
Original Ghidra decompilation (0x4ef820):

void object_apply_shield_damage
               (uint param_1,byte *param_2,int param_3,short *param_4,uint *param_5,float *param_6,
               float *param_7,char param_8,char param_9)

{
  int iVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  int iVar5;
  undefined2 uVar6;
  int iVar7;
  int unaff_EBX;
  bool bVar8;
  float10 fVar9;
  float local_14;
  float local_10;
  float local_c;
  float local_8;

  iVar5 = DAT_008603b0;
  iVar7 = (param_1 & 0xffff) * 0xc;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
  fVar2 = *param_7;
  bVar4 = false;
  bVar8 = DAT_006f1d20 == 0;
  *(undefined4 *)(unaff_EBX + 4) = 0;
  *(undefined1 *)(unaff_EBX + 8) = 0;
  if (((bVar8) && (param_4[1] == 1)) && (*(short *)(iVar1 + 0xb8) == 1)) {
    bVar4 = true;
  }
  if (*(float *)(iVar1 + 0xe4) <= 0.0) {
    local_14 = 0.0;
    local_10 = fVar2;
    if (param_8 != '\x01') goto LAB_004efbba;
    *(undefined4 *)(iVar1 + 0xe4) = 0;
  }
  else {
    local_c = *(float *)(*(int *)(*(int *)(iVar5 + 0x34) + 8 + iVar7) + 0xdc);
    if (!bVar4) {
      fVar9 = (float10)FUN_0046fe70();
      local_c = (float)(fVar9 * (float10)local_c);
    }
    if (local_c <= 0.0) {
      local_8 = 0.0;
    }
    else {
      local_8 = 1.0 / local_c;
    }
    if ((((*param_5 & 0x10) == 0) || (local_14 = fVar2, (*param_2 & 4) == 0)) &&
       ((local_14 = (1.0 - *(float *)(param_3 + 0x28)) * fVar2,
        *(float *)(iVar1 + 0xe4) < *(float *)(param_2 + 0xf0) !=
        (*(float *)(iVar1 + 0xe4) == *(float *)(param_2 + 0xf0)) &&
        (0.0 < *(float *)(param_2 + 0xf0))))) {
      fVar9 = (float10)FUN_004ccac0(*(float *)(iVar1 + 0xe4) / *(float *)(param_2 + 0xf0));
      local_14 = (float)((((float10)1.0 - (float10)*(float *)(param_2 + 0xf4)) * fVar9 +
                         (float10)*(float *)(param_2 + 0xf4)) * (float10)local_14);
    }
    if ((*(byte *)(iVar1 + 0x106) & 0x10) == 0) {
      if (local_14 < 0.0) {
        local_14 = 0.0;
      }
      local_10 = fVar2 - local_14;
      if ((((*param_5 & 0x10) != 0) && ((*param_5 & 0x20) != 0)) &&
         (fVar9 = (float10)FUN_0046fe10(0), (float10)0.0 < fVar9)) {
        local_14 = (float)((float10)local_14 / fVar9);
      }
      fVar2 = local_14 * *(float *)(param_3 + 0x2c) *
              *(float *)(param_4 + *(short *)(param_2 + 0xd2) * 2 + 0x1e);
      if ((*(float *)(iVar1 + 0xe4) < local_8 * fVar2) || (*param_4 == 3)) {
        fVar3 = fVar2 - local_c * *(float *)(iVar1 + 0xe4);
        if (0.0 < fVar3) {
          local_10 = fVar3 + local_10;
        }
        if (param_8 == '\x01') {
          *(undefined4 *)(iVar1 + 0xe4) = 0;
        }
        if (((*(byte *)(iVar1 + 0x106) & 8) == 0) && (param_9 == '\x01')) {
          FUN_004edb10();
          *param_5 = *param_5 | 8;
          *(undefined1 *)(unaff_EBX + 8) = 1;
        }
      }
      else {
        if ((param_8 == '\x01') && ((*(ushort *)(iVar1 + 0x106) & 0x800) == 0)) {
          *(float *)(iVar1 + 0xe4) = *(float *)(iVar1 + 0xe4) - local_8 * fVar2;
        }
        if (((*(byte *)(iVar1 + 0x106) & 2) == 0) &&
           (*(float *)(iVar1 + 0xe4) < *(float *)(param_2 + 0x184))) {
          FUN_004efff0();
          *(byte *)(iVar1 + 0x106) = *(byte *)(iVar1 + 0x106) | 2;
        }
      }
      if (0.0001 <= fVar2) goto LAB_004efb04;
    }
    else {
      local_10 = 0.0;
      local_14 = fVar2;
LAB_004efb04:
      if (param_9 == '\x01') {
        fVar2 = *param_7;
        *(undefined4 *)(iVar1 + 0xfc) = 0;
        local_8 = (fVar2 - local_10) * local_8;
        if ((*(byte *)(iVar1 + 0x106) & 8) == 0) {
          *(undefined4 *)(iVar1 + 0xe8) = 0x3f800000;
        }
        fVar2 = local_8 + *(float *)(iVar1 + 0xf4);
        *(float *)(iVar1 + 0xf4) = fVar2;
        if (1.0 < *(float *)(iVar1 + 0xe8)) {
          *(undefined4 *)(iVar1 + 0xe8) = 0x3f800000;
        }
        if (1.0 < fVar2) {
          *(undefined4 *)(iVar1 + 0xf4) = 0x3f800000;
        }
        *(float *)(unaff_EBX + 4) = local_8;
      }
    }
    if (param_8 != '\x01') goto LAB_004efbba;
  }
  if ((*(float *)(param_2 + 0x108) <= local_14) || (*(float *)(iVar1 + 0xe4) == 0.0)) {
    uVar6 = FUN_006391b4();
    *(undefined2 *)(iVar1 + 0x104) = uVar6;
  }
LAB_004efbba:
  *param_6 = local_14;
  *param_7 = local_10;
  return;
}
#endif
