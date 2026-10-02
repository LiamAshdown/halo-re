// actor_evaluate_combat_state_transition  (Ghidra: already named)
// address 0x40c620, size 1608 bytes (0x40c620..0x40cc67)
// name confidence: 0.5   rewrite confidence: 0.9
// REWRITTEN from objdump 0x40c620..0x40cc67 (the old header's 1443 bytes stopped short). Stack: actor. Returns
//   whether the actor changed mode. With a target prop (+0x270):
//   - in mode 10 / state 1, a target seen (or recently engaged) within the actor definition's +0xa0 range raises
//     the combat alert (else 0x40dd50 may take over);
//   - otherwise, unless busy (threat weapon, mode 10 states 2..3, +0x6, riding, +0x5f2 == 2), a target inside the
//     variant's engage range (+0x160, +0x170 when the Actor lacks 0x20000 or no threat weapon) -- and inside
//     +0x37c + 0.8 when +0x1cb -- enters combat (consider 2, mode 10) at most every 10 ticks and after the
//     difficulty-scaled delay since +0x380;
//   - riding in a vehicle (+0x15e == 4) beyond the definition's +0x160 with an unengaged prop, consider 4.
//   Then the retreat / hold logic: mode 10 states 2..5, the vehicle's +0x394 range and facing (dot > 0.5), the
//   Actor flag 0x1000000 and +0x375 either consider 0 (mode 10) or fall back to mode 3 (guard).
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;       // 0x00880360
extern data_array *object_data;      // 0x008603b0
extern data_array *prop_data;        // 0x008802c0
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern game_main_globals *main_game_globals; // 0x006b0b80

extern void *actor_get_actor_definition(datum_index actor_index); // 0x40fa70, EAX
extern uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3); // 0x40dd50
extern void actor_set_combat_alert_flag(datum_index actor_index, uint8_t new_flag); // 0x421a40, EAX, BL
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index); // 0x428370, EAX
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification); // 0x46fe10, stack, CX (difficulty value)
extern uint8_t actor_consider_combat_mode(uint32_t actor_index, int16_t consideration_mode,
    actor_combat_consideration *out); // 0x401a60
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0


#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

char actor_evaluate_combat_state_transition(uint32_t actor_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;     // esi
    uint8_t *actor_tag = TAG_DATA(((actor *)a)->actor_definition_tag);                     // [esp+0x1c]
    uint8_t *variant = TAG_DATA(((actor *)a)->actor_variant_tag);                       // [esp+0x20]
    uint8_t *definition = (uint8_t *)actor_get_actor_definition(actor_index);        // [esp+0x2c]
    uint8_t changed = 0;                                                             // [esp+0x12]
    uint8_t fallback = 0;                                                            // [esp+0x13]
    uint8_t *p = 0;                                                                  // [esp+0x28]
    float distance = 3.4028235e+38f;                                                 // [esp+0x14]
    actor_combat_consideration consideration;                                        // [esp+0x30]
    int16_t mode;
    uint8_t hold;

    if (((actor *)a)->target_unit_index != k_datum_index_none) {
        p = (uint8_t *)prop_data->data + (((actor *)a)->target_unit_index & 0xffff) * 0x138;
        distance = *(float *)(p + 0x11c);

        // 0x40c6bd: a searching actor that finds its target raises the alert
        if (((actor *)a)->mode == 0xa && *(int16_t *)(a + 0xa0) == 1) {
            uint8_t engaged = p[0x74] || (p[0x12f] && (int8_t)p[0x121] <= 1);

            if (!engaged && ((Actor *)actor_tag)->stalking_discovery_time > 0.0f &&
                !(*(int16_t *)(a + 0xc2) < (int16_t)(int32_t)(((Actor *)actor_tag)->stalking_discovery_time * 30.0f) /* __ftol 0x40c71b */)) {
                engaged = 1;
            }
            if (engaged) {
                if (!(distance <= *(float *)(definition + 0xa0))) {
                    changed = actor_handle_death(actor_index, 0, 0);
                    if (!changed) {
                        actor_set_combat_alert_flag(actor_index, 1);
                    }
                } else {
                    actor_set_combat_alert_flag(actor_index, 1);
                }
            }
        }

        // 0x40c75c: enter combat when the target is inside the engage range
        if (!(actor_has_unshielded_threat_weapon(actor_index) &&
              (*(datum_index *)(p + 0x110) != k_datum_index_none || p[0x14])) &&
            !(((actor *)a)->mode == 0xa && (*(int16_t *)(a + 0xa0) == 2 || *(int16_t *)(a + 0xa0) == 3)) &&
            !changed && !a[0x6] && ((actor *)a)->active_unit_index == k_datum_index_none &&
            ((struct actor *)a)->firing_state != 2) {
            int32_t now = game_time->game_time;                                      // [esp+0x24]
            uint8_t wide = a[0x378];                                                 // bl
            float base_delay;
            float delay;
            float range;
            int16_t difficulty = (int16_t)(uint16_t)main_game_globals->difficulty;

            if (!actor_has_unshielded_threat_weapon(actor_index) && !(*(uint32_t *)actor_tag & 0x20000)) {
                wide = 1;
            }
            base_delay = a[0x378] ? 0.0f : ((Actor *)actor_tag)->melee_attack_delay;
            delay = weapon_get_zoom_fov(0x14, difficulty) + weapon_get_zoom_fov(0x15, difficulty) * base_delay;
            range = wide ? ((ActorVariant *)variant)->berserk_melee_range : ((ActorVariant *)variant)->melee_range;
            if (!(*(int32_t *)&((actor *)a)->search_wait_time != -1 && *(int32_t *)&((actor *)a)->search_wait_time + 0xa >= now) &&
                distance <= range) {
                uint8_t near_enough = 1;

                if (a[0x1cb]) {
                    float extra = ((Actor *)actor_tag)->melee_fudge_factor;

                    if (!(0.0f <= extra)) {
                        extra = 0.0f;
                    }
                    near_enough = distance <= 0.8f + extra;
                }
                if (near_enough &&
                    (*(int32_t *)&((struct actor *)a)->last_melee_time == -1 || (float)now > delay * 30.0f + (float)*(int32_t *)&((struct actor *)a)->last_melee_time)) {
                    actor_has_unshielded_threat_weapon(actor_index);
                    *(int32_t *)&((actor *)a)->search_wait_time = now;
                    if (actor_consider_combat_mode(actor_index, 2, &consideration)) {
                        actor_set_mode(actor_index, 0xa, &consideration);
                        changed = 1;
                    }
                }
            }
        }

        // 0x40c946: a vehicle gunner beyond the definition's +0x160 range with an unengaged prop
        if (((actor *)a)->mode != 0xa && !a[0x1cb]) {
            int16_t seat_kind = ((struct actor *)a)->vehicle_driving_type;

            if (changed) {
                return changed;
            }
            if (seat_kind > 0) {
                uint8_t ready = 1;

                if (*(int32_t *)&((struct actor *)a)->last_vehicle_charge_time != -1) {
                    uint8_t *vehicle_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(((actor *)a)->active_unit_index));

                    ready = (float)game_time->game_time >
                        *(float *)(vehicle_tag + 0x390) * 30.0f + (float)*(int32_t *)&((struct actor *)a)->last_vehicle_charge_time;
                }
                if (ready && seat_kind == 4 && distance > *(float *)(definition + 0x160) &&
                    *(int16_t *)(p + 0x38) == 0 &&
                    actor_consider_combat_mode(actor_index, 4, &consideration)) {
                    actor_set_mode(actor_index, 0xa, &consideration);
                    return 1;
                }
            }
        } else if (changed) {
            return changed;
        }
    }

    // 0x40ca44: hold or fall back
    fallback = (a[0x375] && !a[0x1cb]) ? 1 : 0;
    hold = 0;                                                                        // bl
    if (!a[0x1cb] && !actor_has_unshielded_threat_weapon(actor_index) && (*(uint32_t *)actor_tag & 0x1000000)) {
        fallback = 1;
    }
    mode = ((actor *)a)->mode;                                                   // dx
    if (mode == 0xa) {
        int16_t state = *(int16_t *)(a + 0xa0);

        if (state == 2 || state == 3) {
            if (!a[0xa3] && !a[0xa4] && !a[0xc5]) {
                fallback = 1;
                goto consider_zero;                                                  // 0x40cbee
            }
            hold = 1;
            goto decide;
        }
        if (a[0x1cb]) {
            goto guard;
        }
        if (state == 4 || state == 5) {
            if (a[0xc5] || ((struct actor *)a)->vehicle_driving_type <= 1) {
                hold = 1;
                goto decide;
            }
            fallback = 1;
            if (state != 4) {
                goto consider_zero;
            }
            {
                uint8_t *vehicle_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(((actor *)a)->active_unit_index));
                float vehicle_range = *(float *)(vehicle_tag + 0x394);

                if (a[0x484] && ((actor *)a)->active_movement.type == 5 &&
                    *(datum_index *)&((actor *)a)->active_movement.destination.x == ((actor *)a)->target_unit_index) {
                    goto guard;
                }
                if (distance < vehicle_range) {
                    goto guard;
                }
                if (!(vehicle_range + vehicle_range > distance)) {
                    goto consider_zero;
                }
                if (((actor *)a)->facing.k * *(float *)(p + 0xe8) + ((actor *)a)->facing.j * *(float *)(p + 0xe4) +
                        *(float *)(p + 0xe0) * ((actor *)a)->facing.i >= 0.5f) {
                    goto consider_zero;
                }
                goto guard;
            }
        }
    }
decide:                                                                              // 0x40cbe2
    if (!fallback) {
        goto guard;
    }
    if (hold) {
        goto consider;                                                               // 0x40cbf4
    }
consider_zero:                                                                       // 0x40cbee
    if (mode == 0xa) {
        goto settle;
    }
consider:
    if (actor_consider_combat_mode(actor_index, 0, &consideration)) {
        actor_set_mode(actor_index, 0xa, &consideration);
        changed = 1;
    } else {
        goto guard;
    }
settle:                                                                              // 0x40cc1d
    if (fallback || changed) {
        return changed;
    }
guard:                                                                               // 0x40cc2d
    if (((actor *)a)->mode == 3) {
        return changed;
    }
    *(uint32_t *)&consideration = 0;
    actor_set_mode(actor_index, 3, &consideration);
    return 1;
}

#if 0
Original Ghidra decompilation (0x40c620):

/* WARNING: Removing unreachable block (ram,0x0040cc29) */

char actor_evaluate_combat_state_transition(uint param_1)

{
  float fVar1;
  int iVar2;
  uint *puVar3;
  int iVar4;
  bool bVar5;
  bool bVar6;
  char cVar7;
  char cVar8;
  short sVar9;
  int iVar10;
  int iVar11;
  float10 fVar12;
  float10 fVar13;
  char local_a2;
  float local_a0;
  float local_9c;
  int local_8c;
  undefined4 local_84 [33];
  
  iVar2 = *(int *)(DAT_00880360 + 0x34);
  iVar10 = (param_1 & 0xffff) * 0x724;
  iVar11 = iVar10 + iVar2;
  puVar3 = *(uint **)((*(uint *)(iVar10 + 0x58 + iVar2) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = *(int *)((*(uint *)(iVar10 + 0x5c + iVar2) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar10 = actor_get_actor_definition();
  local_a2 = '\0';
  local_8c = 0;
  local_a0 = 3.4028235e+38;
  if (*(uint *)(iVar11 + 0x270) != 0xffffffff) {
    local_8c = (*(uint *)(iVar11 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
    local_a0 = *(float *)(local_8c + 0x11c);
    if ((((*(short *)(iVar11 + 0x6c) == 10) && (*(short *)(iVar11 + 0xa0) == 1)) &&
        (((*(char *)(local_8c + 0x74) != '\0' ||
          ((*(char *)(local_8c + 0x12f) != '\0' && (*(char *)(local_8c + 0x121) < '\x02')))) ||
         ((0.0 < (float)puVar3[0xca] && (sVar9 = __ftol(), sVar9 <= *(short *)(iVar11 + 0xc2)))))))
       && ((local_a0 <= *(float *)(iVar10 + 0xa0) ||
           (local_a2 = FUN_0040dd50(param_1,0,0), local_a2 == '\0')))) {
      actor_set_combat_alert_flag();
    }
    cVar7 = FUN_00428370();
    if (((cVar7 == '\0') ||
        ((*(int *)(local_8c + 0x110) == -1 && (*(char *)(local_8c + 0x14) == '\0')))) &&
       (((*(short *)(iVar11 + 0x6c) != 10 ||
         ((*(short *)(iVar11 + 0xa0) != 2 && (*(short *)(iVar11 + 0xa0) != 3)))) &&
        ((local_a2 == '\0' &&
         (((*(char *)(iVar11 + 6) == '\0' && (*(int *)(iVar11 + 0x158) == -1)) &&
          (*(short *)(iVar11 + 0x5f2) != 2)))))))) {
      iVar4 = *(int *)(DAT_006f1d6c + 0xc);
      cVar7 = *(char *)(iVar11 + 0x378);
      cVar8 = FUN_00428370();
      if ((cVar8 == '\0') && ((*puVar3 & 0x20000) == 0)) {
        cVar7 = '\x01';
      }
      if (*(char *)(iVar11 + 0x378) == '\0') {
        local_9c = (float)puVar3[0xde];
      }
      else {
        local_9c = 0.0;
      }
      fVar12 = (float10)FUN_0046fe10(0x15);
      fVar13 = (float10)FUN_0046fe10(0x14);
      if (cVar7 == '\0') {
        fVar1 = *(float *)(iVar2 + 0x160);
      }
      else {
        fVar1 = *(float *)(iVar2 + 0x170);
      }
      if (((*(int *)(iVar11 + 0x37c) == -1) || (*(int *)(iVar11 + 0x37c) + 10 < iVar4)) &&
         (local_a0 <= fVar1)) {
        if (*(char *)(iVar11 + 0x1cb) != '\0') {
          if (0.0 <= (float)puVar3[0xdf]) {
            fVar1 = (float)puVar3[0xdf];
          }
          else {
            fVar1 = 0.0;
          }
          if (fVar1 + 0.8 < local_a0) goto LAB_0040c946;
        }
        if ((*(int *)(iVar11 + 0x380) == -1) ||
           (fVar1 = (float)(fVar13 + (float10)(float)(fVar12 * (float10)local_9c)) * 30.0 +
                    (float)*(int *)(iVar11 + 0x380), (float)iVar4 < fVar1 == ((float)iVar4 == fVar1)
           )) {
          FUN_00428370();
          *(int *)(iVar11 + 0x37c) = iVar4;
          cVar7 = actor_consider_combat_mode(param_1,2,local_84);
          if (cVar7 != '\0') {
            actor_set_mode(param_1,10,local_84);
            local_a2 = '\x01';
          }
        }
      }
    }
LAB_0040c946:
    if ((*(short *)(iVar11 + 0x6c) == 10) || (*(char *)(iVar11 + 0x1cb) != '\0')) {
      if (local_a2 != '\0') {
        return local_a2;
      }
    }
    else {
      if (local_a2 != '\0') {
        return local_a2;
      }
      if ((0 < *(short *)(iVar11 + 0x15e)) &&
         (((((*(int *)(iVar11 + 0x388) == -1 ||
             (*(float *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                              (*(uint *)(iVar11 + 0x158) & 0xffff) * 0xc) & 0xffff)
                                  * 0x20 + 0x14 + DAT_0087bc14) + 0x390) * 30.0 +
              (float)*(int *)(iVar11 + 0x388) < (float)*(int *)(DAT_006f1d6c + 0xc))) &&
            (*(short *)(iVar11 + 0x15e) == 4)) &&
           ((*(float *)(iVar10 + 0x160) < local_a0 && (*(short *)(local_8c + 0x38) == 0)))) &&
          (cVar7 = actor_consider_combat_mode(param_1,4,local_84), cVar7 != '\0')))) {
        actor_set_mode(param_1,10,local_84);
        return '\x01';
      }
    }
  }
  if ((*(char *)(iVar11 + 0x375) == '\0') || (bVar6 = true, *(char *)(iVar11 + 0x1cb) != '\0')) {
    bVar6 = false;
  }
  bVar5 = false;
  if (((*(char *)(iVar11 + 0x1cb) == '\0') && (cVar7 = FUN_00428370(), cVar7 == '\0')) &&
     ((*puVar3 & 0x1000000) != 0)) {
    bVar6 = true;
  }
  if (*(short *)(iVar11 + 0x6c) == 10) {
    sVar9 = *(short *)(iVar11 + 0xa0);
    if ((sVar9 == 2) || (sVar9 == 3)) {
      if ((*(char *)(iVar11 + 0xa3) != '\0') ||
         ((*(char *)(iVar11 + 0xa4) != '\0' || (*(char *)(iVar11 + 0xc5) != '\0')))) {
LAB_0040cbe0:
        bVar5 = true;
        goto LAB_0040cbe2;
      }
    }
    else {
      if (*(char *)(iVar11 + 0x1cb) != '\0') goto LAB_0040cc2d;
      if ((sVar9 != 4) && (sVar9 != 5)) goto LAB_0040cbe2;
      if ((*(char *)(iVar11 + 0xc5) != '\0') || (*(short *)(iVar11 + 0x15e) < 2)) goto LAB_0040cbe0;
      if ((sVar9 == 4) &&
         ((((iVar2 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                          (*(uint *)(iVar11 + 0x158) & 0xffff) * 0xc) & 0xffff) *
                              0x20 + 0x14 + DAT_0087bc14), *(char *)(iVar11 + 0x484) != '\0' &&
            (*(short *)(iVar11 + 0x46c) == 5)) &&
           (*(int *)(iVar11 + 0x470) == *(int *)(iVar11 + 0x270))) ||
          ((local_a0 < *(float *)(iVar2 + 0x394) ||
           ((fVar1 = *(float *)(iVar2 + 0x394), local_a0 < fVar1 + fVar1 &&
            (*(float *)(local_8c + 0xe0) * *(float *)(iVar11 + 0x174) +
             *(float *)(iVar11 + 0x178) * *(float *)(local_8c + 0xe4) +
             *(float *)(iVar11 + 0x17c) * *(float *)(local_8c + 0xe8) < 0.5))))))))
      goto LAB_0040cc2d;
    }
LAB_0040cbee:
    if (*(short *)(iVar11 + 0x6c) == 10) {
      return local_a2;
    }
  }
  else {
LAB_0040cbe2:
    if (!bVar6) goto LAB_0040cc2d;
    if (!bVar5) goto LAB_0040cbee;
  }
  cVar7 = actor_consider_combat_mode(param_1,0,local_84);
  if (cVar7 != '\0') {
    actor_set_mode(param_1,10,local_84);
    return 1;
  }
LAB_0040cc2d:
  if (*(short *)(iVar11 + 0x6c) == 3) {
    return local_a2;
  }
  local_84[0] = 0;
  actor_set_mode(param_1,3,local_84);
  return '\x01';
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
