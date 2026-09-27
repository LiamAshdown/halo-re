// actor_evaluate_search_node  (Ghidra: FUN_004091d0; it evaluates a vehicle SEAT for an actor about to board)
// address 0x4091d0, size 1003 bytes
// name confidence: 0.2   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4091d0..0x4095ba. Stack: (actor, vehicle, seat, out_entry, out_direction, out_hint,
//   out_score, out_close, out_facing, out_in_front); returns 1 when the seat can be approached. A seat that is
//   occupied (0x56cc10), refused by the seat permission test for actors whose tag has flag 8 (0x56cdd0), or
//   without entry points for the actor's unit (0x5640a0: entry point, seat marker, "enter-hint" marker) is out.
//   The approach direction is seat - entry in xy (the actor's facing when degenerate); the target is whichever
//   of the entry point / seat marker is nearer in xy. Another actor already boarding this seat (mode 9 at +0x6c,
//   +0x9c vehicle, +0xa0 seat) that is nearer to its own target rules the seat out. Flags: close = xy distance
//   below 0.7, facing = (seat - actor) . facing above 0.6, in_front = distance below 1.1 and that dot positive;
//   score = 10 / (distance + 1), plus 3.5 when the seat flag 3 test (0x56cd70) disagrees with the actor
//   variant's flag bit 7.
// blam-cc: stack -> (actor_index, vehicle_index, seat_index, out_entry, out_direction, out_hint, out_score,
//   out_close, out_facing, out_in_front)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;       // 0x00880360
extern data_array *prop_data;        // 0x008802c0
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern double sqrt(double x);
extern uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index); // 0x56cc10, EDI, SI
extern uint8_t unit_seat_flag_bit10(uint32_t unit_index, int16_t seat_index); // 0x56cdd0, EAX, CX
extern uint8_t unit_seat_flag_bit3(uint32_t unit_index, int16_t seat_index); // 0x56cd70, EAX, CX
extern uint8_t unit_find_weapon_marker_transform(uint32_t unit_index, uint32_t vehicle_index, int16_t seat_index,
    real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint); // 0x5640a0, EAX unit, stack
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900, EAX, ECX
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

static uint8_t *actor_try_get(datum_index handle)
{
    int16_t index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    uint8_t *record;

    if (index < 0 || index >= actor_data->maximum_count) {
        return 0;
    }
    record = (uint8_t *)actor_data->data + actor_data->size * index;
    if (*(int16_t *)record == 0 || (salt != 0 && *(int16_t *)record != salt)) {
        return 0;
    }
    return record;
}

uint8_t actor_evaluate_search_node(datum_index actor_index, datum_index vehicle_index, int16_t seat_index,
    real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score,
    uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *variant = TAG_DATA(*(datum_index *)(act + 0x5c));
    real_point3d entry;
    real_point3d seat;
    real_point3d hint;
    real_vector3d direction;
    real_vector2d to_seat;
    float ax;
    float ay;
    float distance;
    float dot;
    float score;
    uint8_t close;
    uint8_t facing;
    uint8_t in_front;
    datum_index prop_index;

    if (unit_is_seat_occupied((int32_t)vehicle_index, seat_index)) {
        return 0;
    }
    if ((TAG_DATA(*(datum_index *)(act + 0x58))[0x4] & 8) && !unit_seat_flag_bit10(vehicle_index, seat_index)) {
        return 0;
    }
    if (!unit_find_weapon_marker_transform(*(datum_index *)(act + 0x18), vehicle_index, seat_index, &entry, &seat,
                                           &hint)) {
        return 0;
    }
    object_get_position((real_point3d *)&direction, vehicle_index); // 0x40927a: overwritten right away
    direction.i = seat.x - entry.x;
    direction.k = 0.0f;
    direction.j = seat.y - entry.y;
    if (vector2d_normalize_with_length((real_vector2d *)&direction) == 0.0f) {
        direction = *(real_vector3d *)(act + 0x174);
    }
    ax = *(float *)(act + 0x12c);
    ay = *(float *)(act + 0x130);
    if (sqrt((seat.y - ay) * (seat.y - ay) + (seat.x - ax) * (seat.x - ax)) <
        sqrt((entry.y - ay) * (entry.y - ay) + (entry.x - ax) * (entry.x - ax))) {
        distance = (float)sqrt((seat.y - ay) * (seat.y - ay) + (seat.x - ax) * (seat.x - ax));
    } else {
        distance = (float)sqrt((entry.y - ay) * (entry.y - ay) + (entry.x - ax) * (entry.x - ax));
    }
    for (prop_index = *(datum_index *)(act + 0x50); prop_index != k_datum_index_none;) {
        uint8_t *prop = (uint8_t *)prop_data->data + (prop_index & 0xffff) * 0x138;
        datum_index other_index = *(datum_index *)(prop + 0x1c);

        prop_index = *(datum_index *)(prop + 0x8);
        if (prop[0x60] == 0 && other_index != k_datum_index_none) {
            uint8_t *other = actor_try_get(other_index);

            if (other != 0 && *(int16_t *)(other + 0x6c) == 9 && *(datum_index *)(other + 0x9c) == vehicle_index &&
                *(int16_t *)(other + 0xa0) == seat_index) {
                float dx = *(float *)(other + 0xcc) - *(float *)(other + 0x12c);
                float dy = *(float *)(other + 0xd0) - *(float *)(other + 0x130);

                if (distance * distance > dy * dy + dx * dx) {
                    return 0;
                }
            }
        }
    }
    to_seat.i = seat.x - ax;
    to_seat.j = seat.y - ay;
    vector2d_normalize_with_length(&to_seat);
    dot = to_seat.j * *(float *)(act + 0x178) + to_seat.i * *(float *)(act + 0x174);
    close = (uint8_t)(distance < 0.7f);
    facing = (uint8_t)(dot > 0.6f);
    in_front = (uint8_t)(distance < 1.1f && dot > 0.0f);
    score = 10.0f / (distance + 1.0f);
    if ((unit_seat_flag_bit3(vehicle_index, seat_index) != 0) != ((variant[0] & 0x80) != 0)) {
        score = score + 3.5f;
    }
    if (out_entry != 0) {
        *out_entry = entry;
    }
    if (out_direction != 0) {
        *out_direction = direction;
    }
    if (out_hint != 0) {
        *out_hint = hint;
    }
    if (out_score != 0) {
        *out_score = score;
    }
    if (out_close != 0) {
        *out_close = close;
    }
    if (out_facing != 0) {
        *out_facing = facing;
    }
    if (out_in_front != 0) {
        *out_in_front = in_front;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4091d0):

undefined4
FUN_004091d0(uint param_1,int param_2,undefined4 param_3,float *param_4,float *param_5,
            undefined4 *param_6,float *param_7,int param_8,int param_9,undefined1 *param_10)

{
  float fVar1;
  float fVar2;
  char *pcVar3;
  float fVar4;
  float fVar5;
  char cVar6;
  short sVar7;
  int iVar8;
  short *psVar9;
  short sVar10;
  undefined1 uVar11;
  int iVar12;
  int iVar13;
  short *psVar14;
  uint uVar15;
  float10 fVar16;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;

  iVar13 = (param_1 & 0xffff) * 0x724;
  iVar12 = *(int *)(DAT_00880360 + 0x34) + iVar13;
  pcVar3 = *(char **)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x5c + iVar13) & 0xffff) * 0x20 +
                      0x14 + DAT_0087bc14);
  cVar6 = FUN_0056cc10();
  if ((cVar6 == '\0') &&
     ((((*(byte *)(*(int *)((*(uint *)(iVar12 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 4) &
        8) == 0 || (cVar6 = FUN_0056cdd0(), cVar6 != '\0')) &&
      (cVar6 = FUN_005640a0(param_2,param_3,&local_34,&local_1c,&local_10), cVar6 != '\0')))) {
    object_get_position();
    local_28 = local_1c - local_34;
    local_20 = 0.0;
    local_24 = local_18 - local_30;
    fVar16 = (float10)vector2d_normalize_with_length();
    if ((float10)0.0 == fVar16) {
      local_28 = *(float *)(iVar12 + 0x174);
      local_24 = *(float *)(iVar12 + 0x178);
      local_20 = *(float *)(iVar12 + 0x17c);
    }
    fVar4 = local_34 - *(float *)(iVar12 + 300);
    fVar1 = local_30 - *(float *)(iVar12 + 0x130);
    fVar2 = local_1c - *(float *)(iVar12 + 300);
    fVar5 = local_18 - *(float *)(iVar12 + 0x130);
    if (SQRT(fVar4 * fVar4 + fVar1 * fVar1) <= SQRT(fVar2 * fVar2 + fVar5 * fVar5)) {
      fVar4 = local_34 - *(float *)(iVar12 + 300);
      fVar1 = local_30;
    }
    else {
      fVar4 = local_1c - *(float *)(iVar12 + 300);
      fVar1 = local_18;
    }
    fVar1 = fVar1 - *(float *)(iVar12 + 0x130);
    uVar15 = *(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x50 + iVar13);
    fVar4 = SQRT(fVar4 * fVar4 + fVar1 * fVar1);
    do {
      do {
        if (uVar15 == 0xffffffff) {
          fVar1 = *(float *)(iVar12 + 300);
          fVar2 = *(float *)(iVar12 + 0x130);
          vector2d_normalize_with_length();
          fVar1 = (local_1c - fVar1) * *(float *)(iVar12 + 0x174) +
                  (local_18 - fVar2) * *(float *)(iVar12 + 0x178);
          if ((1.1 <= fVar4) || (fVar1 <= 0.0)) {
            uVar11 = 0;
          }
          else {
            uVar11 = 1;
          }
          if (*pcVar3 < '\0') {
            cVar6 = FUN_0056cd70();
            fVar16 = extraout_ST0;
            if (cVar6 != '\0') goto LAB_00409523;
          }
          else {
            cVar6 = FUN_0056cd70();
            fVar16 = extraout_ST0_00;
            if (cVar6 == '\0') goto LAB_00409523;
          }
          fVar16 = fVar16 + (float10)3.5;
LAB_00409523:
          if (param_4 != (float *)0x0) {
            *param_4 = local_34;
            param_4[1] = local_30;
            param_4[2] = local_2c;
          }
          if (param_5 != (float *)0x0) {
            *param_5 = local_28;
            param_5[1] = local_24;
            param_5[2] = local_20;
          }
          if (param_6 != (undefined4 *)0x0) {
            *param_6 = local_10;
            param_6[1] = local_c;
            param_6[2] = local_8;
          }
          if (param_7 != (float *)0x0) {
            *param_7 = (float)fVar16;
          }
          if (param_8 != 0) {
            *(bool *)param_8 = fVar4 < 0.7;
          }
          if (param_9 != 0) {
            *(bool *)param_9 = 0.6 < fVar1;
          }
          if (param_10 != (undefined1 *)0x0) {
            *param_10 = uVar11;
          }
          return 1;
        }
        iVar13 = *(int *)(DAT_008802c0 + 0x34);
        iVar8 = (uVar15 & 0xffff) * 0x138;
        uVar15 = *(uint *)(iVar8 + 8 + iVar13);
      } while ((*(char *)(iVar8 + 0x60 + iVar13) != '\0') ||
              (iVar13 = *(int *)(iVar8 + iVar13 + 0x1c), iVar13 == -1));
      psVar14 = (short *)0x0;
      sVar7 = (short)iVar13;
      if ((-1 < sVar7) && (sVar7 < *(short *)(DAT_00880360 + 0x20))) {
        psVar9 = (short *)((int)*(short *)(DAT_00880360 + 0x22) * (int)sVar7 +
                          *(int *)(DAT_00880360 + 0x34));
        sVar7 = *psVar9;
        if ((sVar7 != 0) &&
           ((sVar10 = (short)((uint)iVar13 >> 0x10), sVar10 == 0 || (sVar7 == sVar10)))) {
          psVar14 = psVar9;
        }
      }
    } while ((((psVar14[0x36] != 9) || (*(int *)(psVar14 + 0x4e) != param_2)) ||
             (psVar14[0x50] != (short)param_3)) ||
            (fVar4 * fVar4 <=
             (*(float *)(psVar14 + 0x66) - *(float *)(psVar14 + 0x96)) *
             (*(float *)(psVar14 + 0x66) - *(float *)(psVar14 + 0x96)) +
             (*(float *)(psVar14 + 0x68) - *(float *)(psVar14 + 0x98)) *
             (*(float *)(psVar14 + 0x68) - *(float *)(psVar14 + 0x98))));
  }
  return 0;
}
#endif
