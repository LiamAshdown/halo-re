// unit_set_or_test_seat_and_weapon_label  (Ghidra: unit_set_or_test_seat_and_weapon_label,
// already named)
// address 0x5651e0, size 564 bytes
// name confidence: 0.75 (already carries this name; cea-pdb hint agrees)   rewrite
//   confidence: 0.85 (REWRITTEN from objdump 0x5651e0..0x565413: the draft skipped weapon slot 0 and stopped at
//   the first match; the binary keeps searching every seat/weapon so the last match wins, and param 3 means
//   apply -- 0 only tests)
// evidence: types/tags.h ModelAnimationsAnimationGraphUnitSeat (label TagString at +0x0,
//   animations TagReflexive at 0x40, weapons TagReflexive at 0x58),
//   ModelAnimationsAnimationGraphWeapon (weapon_types TagReflexive at 0xb0),
//   ModelAnimationsAnimationGraphWeaponType (label TagString at +0x0); types/units.h
//   unit_data.animation_state (0x2a3), .animation_definition_index/.animation_weapon_index/
//   .animation_weapon_type_index (0x2a0/0x2a1/0x2a2), .base_animation_state (0x2a7),
//   .animation_state_flags (0x298, _unit_animation_flag_aiming_enabled). The six-string
//   base_animation_state name table at 0x0069fde4.
// register convention: unit index in EAX, seat label / weapon label / test-only flag as the
//   three parameters.
//   // blam-cc: in_EAX -> unit_index, param_1 -> seat_label, param_2 -> weapon_label,
//   //   param_3 -> test_only
// UNSURE: this function's control flow (a two-level search over unit-seat labels and their
//   weapon slots, with a `goto` into what Ghidra placed after the early `return`) is
//   reproduced with explicit `goto`s that mirror the original labels rather than restructured,
//   since the loop re-entry pattern (retrying the next weapon slot within the same seat record
//   before moving to the next seat record) is easy to get subtly wrong with a clean rewrite.
//   The upper 24 bits of the early-return value are undefined register garbage in the original
//   (CONCAT31); only the low byte is meaningful, matching the house simplification used
//   throughout this codebase (see src/memory/bit_stream_write_bit.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern char *unit_base_animation_state_names[6]; // 0x0069fde4, PTR_DAT_0069fde4

extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b

uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label,
                                                uint8_t apply) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t found = 0;
    int16_t seat_i;

    for (seat_i = 0; seat_i < *(int32_t *)(graph + 0xc); seat_i++) {
        ModelAnimationsAnimationGraphUnitSeat *seat =
            (ModelAnimationsAnimationGraphUnitSeat *)(*(uint8_t **)(graph + 0x10) + seat_i * 0x64);
        int16_t weapon_slot;

        if (seat_label != 0 && __stricmp(seat_label, seat->label.string) != 0) {
            continue;
        }
        for (weapon_slot = 0; weapon_slot < (int32_t)seat->weapons.count; weapon_slot++) {
            ModelAnimationsAnimationGraphWeapon *weapon_anim =
                (ModelAnimationsAnimationGraphWeapon *)((uint8_t *)seat->weapons.pointer + weapon_slot * 0xbc);
            int16_t weapon_type_i;

            for (weapon_type_i = 0; weapon_type_i < (int32_t)weapon_anim->weapon_types.count; weapon_type_i++) {
                ModelAnimationsAnimationGraphWeaponType *weapon_type =
                    (ModelAnimationsAnimationGraphWeaponType *)((uint8_t *)weapon_anim->weapon_types.pointer +
                                                                 weapon_type_i * 0x3c);

                if (weapon_label == 0) {
                    break;
                }
                // 0x5652c2: repz cmpsb against "unarmed" (8 bytes, case-sensitive, NUL included)
                if (strcmp(weapon_label, "unarmed") == 0 && weapon_type->label.string[0] == '\0') {
                    break;
                }
                if (__stricmp(weapon_label, weapon_type->label.string) == 0) {
                    break;
                }
            }
            if (weapon_type_i >= (int32_t)weapon_anim->weapon_types.count) {
                continue; // 0x5653d6: no weapon type matched (or the weapon has none)
            }
            if (apply) {
                int32_t animation_count = (int32_t)seat->animations.count;
                int16_t *seat_animations = (int16_t *)seat->animations.pointer;
                uint8_t aiming = (uint8_t)((animation_count > 2 && seat_animations[2] != -1) ||
                                           (animation_count > 3 && seat_animations[3] != -1) ||
                                           (animation_count > 4 && seat_animations[4] != -1));
                int8_t base_state = -1;
                int16_t i;

                if ((uint8_t)unit->animation_state != 0x1c) {
                    unit->animation_state = -1;
                }
                unit->animation_definition_index = (int8_t)seat_i;
                for (i = 0; i < 6; i++) {
                    if (__stricmp(seat_label, unit_base_animation_state_names[i]) == 0) {
                        base_state = (int8_t)i;
                        break;
                    }
                }
                unit->animation_weapon_type_index = (int8_t)weapon_type_i;
                unit->base_animation_state = base_state;
                unit->animation_weapon_index = (int8_t)weapon_slot;
                if (aiming) {
                    *((uint8_t *)&unit->animation_state_flags) |= 2;
                } else {
                    *((uint8_t *)&unit->animation_state_flags) &= 0xfd;
                }
            }
            // 0x5653d1: a match never ends the search; the last matching seat/weapon wins
            found = 1;
        }
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x5651e0):

uint unit_set_or_test_seat_and_weapon_label(char *param_1,char *param_2,char param_3)

{
  uint *puVar1;
  int iVar2;
  short sVar3;
  short sVar4;
  uint in_EAX;
  uint uVar5;
  int iVar6;
  char *_Str2;
  int iVar7;
  char *_Str2_00;
  short sVar8;
  short sVar9;
  char *pcVar10;
  char *pcVar11;
  bool bVar12;
  undefined1 local_19;
  undefined1 local_14;
  undefined1 local_10;
  int local_c;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x44) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar5 = DAT_0087bc14 & 0xffffff00;
  local_19 = 0;
  local_c = 0;
  if (0 < *(int *)(iVar2 + 0xc)) {
    iVar6 = 0;
    do {
      _Str2 = (char *)(iVar6 * 100 + *(int *)(iVar2 + 0x10));
      if (((param_1 == (char *)0x0) || (iVar6 = __stricmp(param_1,_Str2), iVar6 == 0)) &&
         (sVar4 = 0, 0 < *(int *)(_Str2 + 0x58))) {
        iVar6 = 0;
LAB_00565280:
        iVar6 = iVar6 * 0xbc + *(int *)(_Str2 + 0x5c);
        iVar7 = 0;
        sVar3 = 0;
        if (0 < *(int *)(iVar6 + 0xb0)) {
          do {
            _Str2_00 = (char *)(iVar7 * 0x3c + *(int *)(iVar6 + 0xb4));
            if (param_2 == (char *)0x0) {
LAB_005652fc:
              if (param_3 == '\0') goto LAB_005653d1;
              iVar6 = *(int *)(_Str2 + 0x40);
              if (((iVar6 < 3) || (*(short *)(*(int *)(_Str2 + 0x44) + 4) == -1)) &&
                 (((iVar6 < 4 || (*(short *)(*(int *)(_Str2 + 0x44) + 6) == -1)) &&
                  ((iVar6 < 5 || (*(short *)(*(int *)(_Str2 + 0x44) + 8) == -1)))))) {
                bVar12 = false;
              }
              else {
                bVar12 = true;
              }
              if (*(char *)((int)puVar1 + 0x2a3) != '\x1c') {
                *(undefined1 *)((int)puVar1 + 0x2a3) = 0xff;
              }
              *(undefined1 *)(puVar1 + 0xa8) = (undefined1)local_c;
              sVar9 = 0;
              goto LAB_00565370;
            }
            iVar7 = 8;
            bVar12 = true;
            pcVar10 = param_2;
            pcVar11 = "unarmed";
            do {
              if (iVar7 == 0) break;
              iVar7 = iVar7 + -1;
              bVar12 = *pcVar10 == *pcVar11;
              pcVar10 = pcVar10 + 1;
              pcVar11 = pcVar11 + 1;
            } while (bVar12);
            if (((bVar12) && (*_Str2_00 == '\0')) ||
               (iVar7 = __stricmp(param_2,_Str2_00), iVar7 == 0)) goto LAB_005652fc;
            sVar3 = sVar3 + 1;
            iVar7 = (int)sVar3;
          } while (iVar7 < *(int *)(iVar6 + 0xb0));
        }
        goto LAB_005653d6;
      }
LAB_005653f1:
      local_c = local_c + 1;
      iVar6 = (int)(short)local_c;
    } while (iVar6 < *(int *)(iVar2 + 0xc));
    uVar5 = CONCAT31((int3)(char)((uint)local_c >> 8),local_19);
  }
  return uVar5;
  while (sVar9 = sVar9 + 1, sVar8 = -1, sVar9 < 6) {
LAB_00565370:
    iVar6 = __stricmp(param_1,(&PTR_DAT_0069fde4)[sVar9]);
    sVar8 = sVar9;
    if (iVar6 == 0) break;
  }
  local_14 = (undefined1)sVar3;
  local_10 = (undefined1)sVar4;
  *(undefined1 *)((int)puVar1 + 0x2a2) = local_14;
  *(char *)((int)puVar1 + 0x2a7) = (char)sVar8;
  *(undefined1 *)((int)puVar1 + 0x2a1) = local_10;
  if (bVar12) {
    *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] | 2;
  }
  else {
    *(byte *)(puVar1 + 0xa6) = (byte)puVar1[0xa6] & 0xfd;
  }
LAB_005653d1:
  local_19 = 1;
LAB_005653d6:
  sVar4 = sVar4 + 1;
  iVar6 = (int)sVar4;
  if (*(int *)(_Str2 + 0x58) <= iVar6) goto LAB_005653f1;
  goto LAB_00565280;
}
#endif
