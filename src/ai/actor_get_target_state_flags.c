// actor_get_target_state_flags  (Ghidra: actor_get_target_state_flags, already named)
// address 0x40cc70, size 375 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/ai.h actor.target_unit_index (0x270)/alert_level/unknown_15e;
//   prop.noticed_a (0xb9)/noticed_b (0xba)/kind (0x24); phase-4 summary "computes a set of
//   target-state flags (shields down, morale-gated, target invalid) used by the combat
//   decision routines".
// register convention: two mode-selector shorts and a flag byte in EAX/ECX/EDX, plus seven
//   recognized stack parameters (actor index, two more mode selectors, a flag, and five
//   output byte pointers).
//   // blam-cc: EAX -> ax_mode, ECX -> cx_mode, EDX -> shared_flag,
//   //          stack -> actor_index/mode_b/force_c/force_d/out_a/out_in_e/out_f/out_g/out_h/out_i

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern void actor_target_get_relationship_object(datum_index target_prop_index); // 0x41f3a0, this module,
                                                 // blam-cc: EAX -> target_prop_index
extern uint8_t actor_firing_position_near_point(datum_index actor_index, real_point3d *point,
    uint32_t start_surface_index, int16_t kind); // 0x412960, EDX, stack

void actor_get_target_state_flags(int16_t ax_mode, int16_t cx_mode, uint8_t shared_flag, uint32_t actor_index, int16_t mode_b, char force_c, char force_d, uint8_t *out_a, char *out_in_e, uint8_t *out_f, uint8_t *out_g, uint8_t *out_h, uint8_t *out_i)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (ax_mode == 2 || (ax_mode == 1 && shared_flag != 0)) {
        *out_f = 0;
    }
    if (cx_mode == 2 || (cx_mode == 1 && shared_flag != 0)) {
        *out_g = 0;
        *out_h = 0;
    } else if (mode_b == 2 || (mode_b == 1 && shared_flag != 0)) {
        *out_h = 0;
    }
    if (force_c != 0) {
        *out_g = 0;
        *out_h = 0;
        *out_i = 0;
    }

    if (a->target_unit_index == (datum_index)k_datum_index_none) {
        *out_a = 0;
        *out_f = 0;
        return;
    }

    {
        prop *p = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];
        uint8_t no_noticed_b = (p->noticed_b == 0);

        *out_a = (p->noticed_a == 0);
        *out_f = no_noticed_b;

        if (force_d == 0) {
            if (no_noticed_b) {
                if (p->kind > 1 && p->kind < 4) {
                    // 0x40cd7a loads EAX from actor.target_unit_index just before this call.
                    actor_target_get_relationship_object(a->target_unit_index);
                }
                *out_f = actor_firing_position_near_point(actor_index, (real_point3d *)((uint8_t *)p + 0xf0), *(uint32_t *)&((struct prop *)p)->path_surface_index, 1);
            }
        } else {
            *out_a = 0;
        }
    }

    if (*out_in_e == 0 && a->alert_level < 3) {
        *out_a = 0;
    }
    if (a->unknown_15e > 0) {
        *out_i = 0;
    }
    if (a->unknown_15e == 4) {
        *out_f = 0;
        *out_h = 0;
    }
}

#if 0
Original Ghidra decompilation (0x40cc70):

void actor_get_target_state_flags
               (uint param_1,short param_2,char param_3,char param_4,undefined1 *param_5,
               char *param_6,undefined1 *param_7,undefined1 *param_8,undefined1 *param_9,
               undefined1 *param_10)

{
  undefined1 uVar1;
  short in_AX;
  int iVar2;
  short in_CX;
  char in_DL;
  int iVar3;
  bool bVar4;

  iVar3 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((in_AX == 2) || ((in_AX == 1 && (in_DL != '\0')))) {
    *param_7 = 0;
  }
  if ((in_CX == 2) || ((in_CX == 1 && (in_DL != '\0')))) {
    *param_8 = 0;
    *param_9 = 0;
  }
  else if ((param_2 == 2) || ((param_2 == 1 && (in_DL != '\0')))) {
    *param_9 = 0;
  }
  if (param_3 != '\0') {
    *param_8 = 0;
    *param_9 = 0;
    *param_10 = 0;
  }
  if (*(uint *)(iVar3 + 0x270) == 0xffffffff) {
    *param_5 = 0;
    *param_7 = 0;
    return;
  }
  iVar2 = (*(uint *)(iVar3 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  *param_5 = *(char *)(iVar2 + 0xb9) == '\0';
  bVar4 = *(char *)(iVar2 + 0xba) == '\0';
  *param_7 = bVar4;
  if (param_4 == '\0') {
    if (bVar4) {
      if ((1 < *(short *)(iVar2 + 0x24)) && (*(short *)(iVar2 + 0x24) < 4)) {
        actor_target_get_relationship_object();
      }
      uVar1 = FUN_00412960(iVar2 + 0xf0,*(undefined4 *)(iVar2 + 0xec),1);
      *param_7 = uVar1;
    }
  }
  else {
    *param_5 = 0;
  }
  if ((*param_6 == '\0') && (*(short *)(iVar3 + 0x6e) < 3)) {
    *param_5 = 0;
  }
  if (0 < *(short *)(iVar3 + 0x15e)) {
    *param_10 = 0;
  }
  if (*(short *)(iVar3 + 0x15e) == 4) {
    *param_7 = 0;
    *param_9 = 0;
  }
  return;
}
#endif
