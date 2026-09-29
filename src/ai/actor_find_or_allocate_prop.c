// actor_find_or_allocate_prop  (Ghidra: FUN_0043e270)
// address 0x43e270, size 973 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// REWRITTEN from objdump 0x43e270..0x43e63c (the draft ignored the object argument, swapped the two reuse
//   candidates, judged the prop's owner instead of the actor, and called datum_new/actor_unlink_prop/
//   actor_init_prop_from_object without their arguments). Makes room for a new prop (0x138 bytes, prop_data) for
//   object_index in the actor's prop list (+0x50, chained through +0x08), skipping kinds 4..5 (+0x24) and paired
//   props (+0x0c). Each prop is judged with the same rules actor_target_evaluate_squad_link uses to admit one
//   (distance +0x11c, owner actor +0x1c, danger radius +0x20, firing +0x127, fired ticks +0x76, +0x63/+0x6a
//   pinned, +0x12e player-controlled, +0x60 enemy):
//   - no longer admissible: the closest such prop is the first reuse choice;
//   - admissible with the same enemy flag as `kind`: counted, and the closest one that would now sit in a far list
//     is the second choice, used only once there are at least 4 (6 for enemies) such props.
//   A reused prop is detached (actor_replace_object_reference with -1, actor_unlink_prop) and cleared (keeping its
//   salt word); otherwise a new datum is allocated. Either way actor_init_prop_from_object fills it for the object.
// blam-cc: stack -> actor_index, object_index, kind

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include <string.h>
#include "units.h"

extern data_array *actor_data;     // 0x00880360
extern data_array *prop_data;      // 0x008802c0
extern data_array *object_data;    // 0x008603b0
extern data_array *encounter_data; // 0x008802c8

extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference); // 0x428470, stack, ESI, EDI
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove); // 0x43ea20, EAX, EDI
extern void actor_init_prop_from_object(datum_index object_index, datum_index actor_index, datum_index prop_index); // 0x43e640, EAX, EDX, stack
extern datum_index datum_new(data_array *array); // 0x4d0480, EDX
extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index); // 0x40e760, EAX

enum {
    k_prop_admit_drop,
    k_prop_admit_keep,
};

// 0x43e308..0x43e56d: whether the prop would still be admitted; *far_out as the squad link's far flag.
static int actor_prop_still_admitted(datum_index actor_index, uint8_t *self, uint8_t *p, float distance_squared,
    uint8_t *far_out)
{
    datum_index owner_index = ((prop *)p)->owner_actor_index;
    float radius = ((struct prop *)p)->danger_radius;
    int16_t pinned_ticks = ((struct prop *)p)->retain_timer;
    int16_t since_fired = ((struct prop *)p)->dead_ticks;
    uint8_t *owner = 0;

    *far_out = 0;
    if (p[0x12e] != 0) {
        return k_prop_admit_keep;
    }
    if (owner_index != k_datum_index_none) {
        owner = (uint8_t *)actor_data->data + (owner_index & 0xffff) * 0x724;
    }
    if (owner != 0 && (owner[8] == 0 || owner[0x13] != 0)) {
        return k_prop_admit_drop;
    }
    if (p[0x63] != 0 || pinned_ticks > 0) {
        return k_prop_admit_keep;
    }
    if (distance_squared > 1600.0f) {
        return k_prop_admit_drop;
    }
    if (p[0x127] != 0) {
        datum_index encounter_index = ((actor *)self)->encounter_index;

        if (encounter_index != k_datum_index_none) {
            uint8_t *encounter = (uint8_t *)encounter_data->data + (encounter_index & 0xffff) * 0x6c;
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[((prop *)p)->object_index & 0xffff].data;
            int32_t reference = ((struct encounter *)encounter)->unknown_58;
            uint8_t counts = 1;
            uint8_t calm;

            if (!(reference > *(int32_t *)&((struct actor *)self)->unknown_3a0)) {
                reference = *(int32_t *)&((struct actor *)self)->unknown_3a0;
            }
            if (reference != -1) {
                int32_t fired = ((struct unit_object *)unit)->unit.death_time;

                if (fired == -1 || fired < reference) {
                    counts = 0;
                }
            }
            calm = encounter[0x45] == 0 && encounter[0x44] == 0 && encounter[0x42] == 0;
            if (!counts) {
                return k_prop_admit_drop;
            }
            if (calm) {
                return distance_squared < 225.0f ? k_prop_admit_keep : k_prop_admit_drop;
            }
        }
        // 0x43e48d
        if (radius > 0.0f) {
            return k_prop_admit_keep;
        }
        {
            uint8_t enemy = p[0x60];
            float limit;

            if (enemy && since_fired > 0x96) {
                return k_prop_admit_drop;
            }
            if (actor_get_current_mode_combat_grade(actor_index) > 1) {
                return k_prop_admit_drop;
            }
            limit = 16.0f;
            if (!enemy && ((actor *)self)->awareness_level < 3) {
                limit = 64.0f;
            }
            return distance_squared < limit ? k_prop_admit_keep : k_prop_admit_drop;
        }
    }
    // 0x43e4f5
    if (p[0x60] != 0) {
        *far_out = distance_squared > 36.0f;
        return k_prop_admit_keep;
    }
    if (((struct actor *)self)->combat_status >= 4) {
        *far_out = 1;
    } else if (self[0x1cc] == 0) {
        *far_out = distance_squared > 16.0f;
    } else {
        *far_out = 0;
    }
    return distance_squared < 225.0f ? k_prop_admit_keep : k_prop_admit_drop;
}

datum_index actor_find_or_allocate_prop(datum_index actor_index, uint32_t object_index, char kind)
{
    uint8_t *self = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    datum_index cursor = ((actor *)self)->first_prop;
    datum_index drop_choice = k_datum_index_none;
    datum_index far_choice = k_datum_index_none;
    float drop_distance = 3.4028234663852886e+38f;
    float far_distance = 3.4028234663852886e+38f; // 0x672be0
    int16_t same_kind_count = 0;
    datum_index result;

    while (cursor != k_datum_index_none) {
        datum_index current = cursor;
        uint8_t *p = (uint8_t *)prop_data->data + (cursor & 0xffff) * 0x138;
        int16_t prop_kind = ((prop *)p)->state;
        float distance = ((prop *)p)->distance;
        uint8_t far_flag;

        cursor = ((prop *)p)->next_in_actor;
        if ((prop_kind >= 4 && prop_kind <= 5) || ((prop *)p)->pair_index != k_datum_index_none) {
            continue;
        }
        if (actor_prop_still_admitted(actor_index, self, p, distance * distance, &far_flag) == k_prop_admit_keep) {
            if ((char)p[0x60] != kind) {
                continue;
            }
            same_kind_count++;
            if (far_flag && far_distance > distance) {
                far_choice = current;
                far_distance = distance;
            }
        } else if (distance < drop_distance) {
            drop_choice = current;
            drop_distance = distance;
        }
    }

    result = drop_choice;
    if (result == k_datum_index_none) {
        result = far_choice;
        if (result != k_datum_index_none && same_kind_count < (kind != 0 ? 6 : 4)) {
            result = k_datum_index_none;
        }
    }
    if (result == k_datum_index_none) {
        result = datum_new(prop_data);
    } else {
        uint8_t *p = (uint8_t *)prop_data->data + (result & 0xffff) * 0x138;
        int16_t salt = *(int16_t *)p;

        actor_replace_object_reference(actor_index, 0xffffffff, result);
        actor_unlink_prop(actor_index, result);
        memset(p, 0, 0x138);
        *(int16_t *)p = salt;
    }
    actor_init_prop_from_object(object_index, actor_index, result);
    return result;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043e270 @ 0x43e270) ----
uint FUN_0043e270(uint param_1,undefined4 param_2,char param_3)

{
  char cVar1;
  undefined2 uVar2;
  uint uVar3;
  int iVar4;
  float fVar5;
  bool bVar6;
  float fVar7;
  bool bVar8;
  bool bVar9;
  short sVar10;
  int iVar11;
  int iVar12;
  short sVar13;
  int iVar14;
  int iVar15;
  undefined4 *puVar16;
  int iVar17;
  undefined4 *puVar18;
  float10 fVar19;
  float10 extraout_ST0;
  float local_1c;
  uint local_18;
  uint local_14;
  uint local_4;

  fVar19 = (float10)3.4028235e+38;
  iVar14 = (param_1 & 0xffff) * 0x724;
  local_18 = 0xffffffff;
  local_14 = 0xffffffff;
  iVar15 = *(int *)(DAT_00880360 + 0x34);
  iVar11 = iVar15 + iVar14;
  local_1c = 3.4028235e+38;
  sVar13 = 0;
  uVar3 = *(uint *)(iVar15 + 0x50 + iVar14);
  do {
    while( true ) {
      do {
        local_4 = uVar3;
        if (local_4 == 0xffffffff) {
          if ((local_18 == 0xffffffff) &&
             (((local_14 == 0xffffffff || (sVar13 < (short)((ushort)(param_3 != '\0') * 2 + 4))) ||
              (local_18 = local_14, local_14 == 0xffffffff)))) {
            local_18 = datum_new();
          }
          else {
            puVar16 = (undefined4 *)((local_18 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34));
            actor_replace_object_reference(param_1);
            FUN_0043ea20();
            uVar2 = *(undefined2 *)puVar16;
            puVar18 = puVar16;
            for (iVar15 = 0x4e; iVar15 != 0; iVar15 = iVar15 + -1) {
              *puVar18 = 0;
              puVar18 = puVar18 + 1;
            }
            *(undefined2 *)puVar16 = uVar2;
          }
          FUN_0043e640(local_18);
          return local_18;
        }
        iVar14 = *(int *)(DAT_008802c0 + 0x34);
        iVar17 = (local_4 & 0xffff) * 0x138;
        sVar10 = *(short *)(iVar17 + 0x24 + iVar14);
        uVar3 = *(uint *)(iVar17 + 8 + iVar14);
        iVar17 = iVar17 + iVar14;
      } while (((3 < sVar10) && (sVar10 < 6)) || (*(int *)(iVar17 + 0xc) != -1));
      fVar5 = *(float *)(iVar17 + 0x11c) * *(float *)(iVar17 + 0x11c);
      if (*(uint *)(iVar17 + 0x1c) == 0xffffffff) {
        iVar14 = 0;
      }
      else {
        iVar14 = (*(uint *)(iVar17 + 0x1c) & 0xffff) * 0x724 + iVar15;
      }
      bVar9 = false;
      if (*(char *)(iVar17 + 0x12e) == '\0') break;
LAB_0043e445:
      if (((*(char *)(iVar17 + 0x60) == param_3) && (sVar13 = sVar13 + 1, bVar9)) &&
         ((float10)*(float *)(iVar17 + 0x11c) < fVar19)) {
        local_14 = local_4;
        fVar19 = (float10)*(float *)(iVar17 + 0x11c);
      }
    }
    if ((iVar14 == 0) || ((*(char *)(iVar14 + 8) != '\0' && (*(char *)(iVar14 + 0x13) == '\0')))) {
      if ((*(char *)(iVar17 + 99) != '\0') || (0 < *(short *)(iVar17 + 0x6a))) goto LAB_0043e445;
      if (fVar5 <= 1600.0) {
        if (*(char *)(iVar17 + 0x127) == '\0') {
          if (*(char *)(iVar17 + 0x60) == '\0') {
            if (*(short *)(iVar11 + 0x6e) < 4) {
              if ((*(char *)(iVar11 + 0x1cc) != '\0') || (bVar9 = true, fVar5 <= 16.0)) {
                bVar9 = false;
              }
            }
            else {
              bVar9 = true;
            }
            if (225.0 <= fVar5) goto LAB_0043e573;
          }
          else if (fVar5 <= 36.0) {
            bVar9 = false;
          }
          else {
            bVar9 = true;
          }
          goto LAB_0043e445;
        }
        bVar6 = true;
        if (*(uint *)(iVar11 + 0x34) == 0xffffffff) {
LAB_0043e48d:
          if (0.0 < *(float *)(iVar17 + 0x20)) goto LAB_0043e445;
          cVar1 = *(char *)(iVar17 + 0x60);
          if (((cVar1 == '\0') || (*(short *)(iVar17 + 0x76) < 0x97)) &&
             (sVar10 = FUN_0040e760(), fVar19 = extraout_ST0, sVar10 < 2)) {
            fVar7 = 16.0;
            if ((cVar1 == '\0') && (*(short *)(iVar11 + 0x6a) < 3)) {
              fVar7 = 64.0;
            }
            if (fVar5 < fVar7) goto LAB_0043e445;
          }
        }
        else {
          iVar12 = (*(uint *)(iVar11 + 0x34) & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
          iVar14 = *(int *)(iVar12 + 0x58);
          if (*(int *)(iVar12 + 0x58) <= *(int *)(iVar11 + 0x3a0)) {
            iVar14 = *(int *)(iVar11 + 0x3a0);
          }
          if ((iVar14 != -1) &&
             ((iVar4 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                        (*(uint *)(iVar17 + 0x18) & 0xffff) * 0xc) + 0x41c),
              iVar4 == -1 || (iVar4 < iVar14)))) {
            bVar6 = false;
          }
          if (((*(char *)(iVar12 + 0x45) == '\0') && (*(char *)(iVar12 + 0x44) == '\0')) &&
             (*(char *)(iVar12 + 0x42) == '\0')) {
            bVar8 = true;
          }
          else {
            bVar8 = false;
          }
          if (bVar6) {
            if (!bVar8) goto LAB_0043e48d;
            if (fVar5 < 225.0) goto LAB_0043e445;
          }
        }
      }
    }
LAB_0043e573:
    if (*(float *)(iVar17 + 0x11c) < local_1c) {
      local_18 = local_4;
      local_1c = *(float *)(iVar17 + 0x11c);
    }
  } while( true );
}
#endif
