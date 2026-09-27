// actor_update_melee_combat_action  (Ghidra: actor_update_melee_combat_action; really "choose the next combat mode")
// address 0x40cdf0, size 1742 bytes
// name confidence: 0.2   rewrite confidence: 0.85
// REWRITTEN from objdump 0x40cdf0..0x40d4bd. The draft called the target-alert stages, the pursuit note, the placement
//   flags and support evaluation with missing operands. Stack: actor. Returns 1 when a mode was set. Searching
//   actors (+0x1e4) take a search-wait (mode 6), regrouping ones process their order; otherwise, in combat grade
//   2+, the type (+0x6..+0xc), starting location (0x436d40), encounter support (0x436dc0) and target state (0x40cc70)
//   decide between advancing (a wait order, mode 5), retreating (mode 7), taking a firing position (0x412ba0 query,
//   mode 5 / committed mode 7, counted in +0x3c4 and noted for pursuit) or a random wait (mode 8); the fallback is
//   guard (mode 6, at the current position for search / wait modes).
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include <string.h>

extern data_array *actor_data;       // 0x00880360
extern data_array *encounter_data;   // 0x008802c8
extern data_array *prop_data;        // 0x008802c0
extern tag_instance *tag_instances;  // 0x0087bc14
extern uint8_t *actor_type_definitions[]; // 0x006853b8
// FIXED 2026-09-27 (static loop, scratchpad/equcheck.py): this file declared the table at 0x0065524c with the
// combat grade at +0xc, but the linker binds actor_mode_definitions to 0x00655254 (16 other files), so the
// read landed on process_proc's low word. Binary: [mode * 0x38 + 0x655258] == definitions[mode].combat_grade.
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

extern void ai_starting_location_derive_placement_flags(datum_index encounter_index, int16_t starting_location_index,
    uint8_t *out_a, int16_t *out_b, uint8_t *out_c, int16_t *out_d, int16_t *out_edx, int16_t *out_esi); // 0x436d40, EAX, stack, EDX, ESI
extern void encounter_evaluate_support_needs(datum_index encounter_index, datum_index self_actor_index, int16_t mode,
    uint8_t phase, uint8_t *out_crowded, uint8_t *out_flanked, uint8_t *out_a, uint8_t *out_b,
    uint8_t *out_reachable_a, uint8_t *out_reachable_b, uint8_t *out_any); // 0x436dc0, EAX, stack
extern void actor_get_target_state_flags(int16_t ax_mode, int16_t cx_mode, uint8_t shared_flag, uint32_t actor_index,
    int16_t mode_b, char force_c, char force_d, uint8_t *out_a, char *out_in_e, uint8_t *out_f, uint8_t *out_g,
    uint8_t *out_h, uint8_t *out_i); // 0x40cc70, EAX, ECX, EDX, stack
extern int32_t actor_build_order_wait_byte(uint32_t actor_index, uint8_t byte_a, uint32_t *order); // 0x4080c0, EAX, stack, ESI
extern void actor_set_target_alert_stage1(datum_index target_prop_index, datum_index actor_index); // 0x41fb00, ECX, ESI
extern int32_t actor_build_order_flee(uint32_t actor_index, uint8_t byte_a, uint32_t *order); // 0x4077d0, EAX, stack, EDX
extern void actor_set_target_alert_stage2(datum_index target_prop_index, datum_index actor_index); // 0x41fb60, ECX, ESI
extern datum_index actor_get_target_prop_object_index(datum_index actor_index); // 0x4283d0, EAX
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340
extern int32_t actor_build_order_minimal_stop(uint32_t actor_index, uint32_t *order); // 0x4078f0, EAX, ESI
extern uint32_t actor_get_firing_position_group_mask(datum_index actor_index, int16_t kind, int16_t search_override); // 0x412880, EAX, SI, stack
extern uint32_t actor_find_best_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok); // 0x412ba0
extern uint32_t actor_build_order_face_seat_marker(uint32_t actor_index, int16_t firing_position_index, uint32_t *order); // 0x408110, EAX, stack, EDX
extern uint32_t actor_build_order_face_seat_marker_committed(uint32_t actor_index, int16_t firing_position_index,
    uint8_t byte_a, uint32_t *order); // 0x407820, EAX, stack, EDX
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0
extern uint8_t ai_pursuit_note_object(datum_index object_index, datum_index encounter_index, int16_t type,
    int32_t min_last_tick); // 0x436b10, EDX, stack, ECX, EAX
extern int32_t actor_build_order_random_wait(uint32_t actor_index, uint8_t byte_a, uint32_t *order); // 0x409a90, EAX, stack, ECX
extern int32_t actor_build_order_search_wait(uint32_t actor_index, actor_order *order); // 0x4045a0, EAX, ESI
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code); // 0x409ea0
extern void actor_set_target_alert_stage3(datum_index target_prop_index, datum_index actor_index); // 0x41fbc0, EDX, ESI
extern int32_t actor_build_order_guard(uint32_t actor_index, actor_order *order, int16_t guard_at_current_position); // 0x404510, EAX, EDX, EBX

#define W(p, o) (*(int16_t *)((p) + (o)))
#define D(p, o) (*(datum_index *)((p) + (o)))

// 0x40d32a: a firing position was taken; note it for the encounter's pursuit and count the moves
static uint8_t actor_combat_commit_position(datum_index actor_index, uint8_t *a, uint8_t *target, int16_t position)
{
    datum_index object = target != 0 ? D(target, 0x7c) : k_datum_index_none;

    if (D(a, 0x34) != k_datum_index_none &&
        ai_pursuit_note_object(actor_index, D(a, 0x34), position, (int32_t)object)) {
        if (W(a, 0x3c4) == 0) {
            ai_communication_broadcast(0x10, D(a, 0x18), k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
        }
        W(a, 0x3c4) += 1;
    }
    return 1;
}

uint8_t actor_update_melee_combat_action(datum_index actor_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;   // ebx
    uint8_t *actor_tag = (uint8_t *)tag_instances[D(a, 0x58) & 0xffff].data;     // [esp+0x24]
    datum_index encounter_index = D(a, 0x34);
    uint8_t *encounter = encounter_index != k_datum_index_none
        ? (uint8_t *)encounter_data->data + (encounter_index & 0xffff) * 0x6c : 0;
    uint8_t result = 0;                 // [esp+0xd]
    uint8_t regroup = 0;                // [esp+0xe] (dl)
    uint8_t searching = 0;              // cl
    uint8_t order[0x8c];                // [esp+0x40]

    memset(order, 0, sizeof(order));
    if (encounter != 0 && encounter[0x42] && W(a, 0x6e) <= 2 && W(a, 0x72) == 0 && W(a, 0x74) == 0) {
        regroup = 1;
    }
    if (W(a, 0x1e4) > 0 && W(a, 0x6e) <= 2 && W(a, 0x74) == 0) {
        searching = 1;
    }
    if (W(a, 0x6a) < 3 && actor_mode_definitions[W(a, 0x6c)].combat_grade == 0 /* 0x40ceb7 */) {
        return 1;
    }
    if (a[0x160] || searching || regroup) {
        // 0x40d3f7
        if (searching) {
            if (W(a, 0x6c) == 6 && a[0xa1]) {
                return 1;
            }
            if (actor_build_order_search_wait(actor_index, (actor_order *)order)) {
                actor_set_mode(actor_index, 6, order);
                return 1;
            }
        }
        if (regroup) {
            result = actor_process_order_request(actor_index, 0xffff);
            if (result) {
                return result;
            }
        }
        goto guard;
    }
    if (W(a, 0x6e) < 2) {
        goto guard;
    }

    {
        uint8_t *target = D(a, 0x270) != k_datum_index_none
            ? (uint8_t *)prop_data->data + (D(a, 0x270) & 0xffff) * 0x138 : 0;   // [esp+0x20]
        uint8_t hold = 0;           // [esp+0x1c]
        uint8_t advance = 0;        // [esp+0x11]
        uint8_t pressed = 0;        // [esp+0x10]
        uint8_t retreat = 0;        // [esp+0x14]
        uint8_t reposition = 0;     // [esp+0x18]
        uint8_t move_ok = 0;        // [esp+0xf]
        uint8_t wait_ok = 0;        // [esp+0x12]

        if (target == 0 || !target[0xbb]) {
            // 0x40cf4e: what the type, the encounter and the target say
            uint8_t *type = actor_type_definitions[W(a, 0x4)];
            int16_t ax_mode = W(type, 0x6);         // [esp+0x30]
            int16_t cx_mode = W(type, 0x8);         // [esp+0x2c]
            int16_t mode_b = W(type, 0xa);          // [esp+0x34]
            uint8_t phase = type[0xc];              // [esp+0x28]
            int16_t support_mode = 0;               // [esp+0x38]
            uint8_t reachable_b = 0;                // [esp+0x13]

            regroup = 0;
            if (target != 0) {
                pressed = 1;
                advance = 1;
            }
            reposition = 1;
            retreat = 1;
            move_ok = 1;
            if (encounter_index != k_datum_index_none) {
                ai_starting_location_derive_placement_flags(encounter_index, (int16_t)*(uint16_t *)(a + 0x3a), &hold,
                                                            &support_mode, &phase, &ax_mode, &mode_b, &cx_mode);
                if (a[0x6]) {
                    retreat = 1;
                    pressed = 1;
                    advance = 1;
                    move_ok = 1;
                    reposition = 1;
                } else {
                    encounter_evaluate_support_needs(encounter_index, actor_index, support_mode, phase, &pressed,
                                                     &retreat, &reposition, &move_ok, &regroup, &reachable_b, &wait_ok);
                }
            }
            actor_get_target_state_flags(ax_mode, cx_mode, regroup, actor_index, mode_b, 0, (char)a[0x375], &advance,
                                         (char *)&pressed, &retreat, &reposition, &move_ok, &wait_ok);
        }

        // 0x40d08c: a new target resets the move count
        if (D(a, 0x3c0) != D(a, 0x270)) {
            W(a, 0x3c4) = 0;
            D(a, 0x3c0) = D(a, 0x270);
            a[0x3bc] = 0;
            a[0x3bd] = 0;
        }
        if (advance && actor_build_order_wait_byte(actor_index, retreat, (uint32_t *)order)) {
            actor_set_mode(actor_index, 5, order);
            return 1;
        }
        actor_set_target_alert_stage1(D(a, 0x270), actor_index);
        if (retreat && actor_build_order_flee(actor_index, a[0x375], (uint32_t *)order)) {
            actor_set_mode(actor_index, 7, order);
            return 1;
        }
        actor_set_target_alert_stage2(D(a, 0x270), actor_index);
        if (a[0x3bc] && !a[0x3bd]) {
            ai_communication_broadcast(0xd, D(a, 0x18), actor_get_target_prop_object_index(actor_index), -1,
                                       k_datum_index_none, k_datum_index_none, 0);
            a[0x3bd] = 1;
        }

        if (reposition) {
            // 0x40d16c: take a firing position
            int16_t position = -1;
            uint8_t have_position = 0;  // [esp+0xe]

            a[0x98] = 1;
            if (a[0x6]) {
                if (move_ok && actor_build_order_minimal_stop(actor_index, (uint32_t *)order)) {
                    actor_set_mode(actor_index, 7, order);
                    return actor_combat_commit_position(actor_index, a, target, position);
                }
            } else {
                int16_t limit;

                if (W(a, 0x6c) == 5 && move_ok && W(a, 0xa4) == 1) {
                    position = W(a, 0xa6);
                    have_position = 1;
                }
                if (!(have_position && position != -1)) {
                    limit = D(a, 0x1d0) == k_datum_index_none ? W(actor_tag, 0x356) : W(actor_tag, 0x354);
                    if (!hold && D(a, 0x3c0) == D(a, 0x270) && W(a, 0x3c4) >= limit) {
                        goto no_position;
                    }
                    {
                        static uint8_t query[0x664];            // [esp+0x108]
                        static uint8_t candidate[0x3c];         // [esp+0xcc]
                        static path_find_context path_context;   // [esp+0x770]
                        uint32_t previous_owner = 0;            // [esp+0x24]
                        uint8_t path_ok = 0;                    // [esp+0x13]

                        memset(query, 0, sizeof(query));
                        W(query, 0x4) = 5;
                        D(query, 0x8) = D(a, 0x270);
                        D(query, 0xc) = target != 0 ? D(target, 0x7c) : k_datum_index_none;
                        query[0x43] = (uint8_t)(D(a, 0x270) != k_datum_index_none);
                        query[0x14] = hold;
                        *(uint32_t *)query = actor_get_firing_position_group_mask(actor_index, 5, 0);
                        *(float *)(query + 0x1c) = 20.0f;
                        position = (int16_t)actor_find_best_firing_position(actor_index, (actor_firing_position_query *)query,
                            (actor_firing_position_candidate *)candidate, &previous_owner,
                            &path_context, &path_ok);
                    }
                    if (position == -1) {
                        goto no_position;
                    }
                    if (!have_position &&
                        actor_build_order_face_seat_marker(actor_index, position, (uint32_t *)order)) {
                        actor_set_mode(actor_index, 5, order);
                        return actor_combat_commit_position(actor_index, a, target, position);
                    }
                }
                if (move_ok &&
                    actor_build_order_face_seat_marker_committed(actor_index, position, hold, (uint32_t *)order)) {
                    actor_set_mode(actor_index, 7, order);
                    return actor_combat_commit_position(actor_index, a, target, position);
                }
            }
        }

    no_position:
        // 0x40d39d
        if (W(a, 0x3c4) > 0 && D(a, 0x18) != k_datum_index_none) {
            ai_communication_broadcast(0x13, D(a, 0x18), k_datum_index_none, -1, k_datum_index_none,
                                       k_datum_index_none, 0);
        }
        if (!a[0x6] && wait_ok && actor_build_order_random_wait(actor_index, reposition, (uint32_t *)order)) {
            actor_set_mode(actor_index, 8, order);
            return 1;
        }
    }

guard:
    // 0x40d445: otherwise guard
    if (actor_mode_definitions[W(a, 0x6c)].combat_grade == 1 /* 0x40d45a */) {
        return result;
    }
    {
        int16_t mode = W(a, 0x6c);
        int16_t guard_at = ((mode == 7 && !a[0x9d]) || mode == 8) ? 0 : 0x5a;

        actor_set_target_alert_stage3(D(a, 0x270), actor_index);
        actor_build_order_guard(actor_index, (actor_order *)order, guard_at);
        actor_set_mode(actor_index, 6, order);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x40cdf0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

char FUN_0040cdf0(uint param_1)

{
  int iVar1;
  uint uVar2;
  undefined *puVar3;
  bool bVar4;
  uint uVar5;
  uint uVar6;
  char cVar7;
  char cVar8;
  short sVar9;
  int iVar10;
  undefined1 *extraout_ECX;
  undefined1 *extraout_EDX;
  undefined1 *extraout_EDX_00;
  undefined1 *extraout_EDX_01;
  undefined1 *extraout_EDX_02;
  undefined4 *puVar11;
  undefined4 uVar12;
  undefined1 *puVar13;
  char cStack_107fb;
  char cStack_107fa;
  char cStack_107f9;
  undefined1 uStack_107f8;
  char cStack_107f7;
  char acStack_107f6 [2];
  uint uStack_107f4;
  uint uStack_107f0;
  uint uStack_107ec;
  int iStack_107e8;
  int iStack_107e4;
  undefined4 uStack_107e0;
  undefined2 uStack_107dc;
  undefined2 auStack_107d8 [2];
  undefined4 uStack_107d4;
  undefined4 uStack_107d0;
  int iStack_107cc;
  undefined1 auStack_107c8 [140];
  undefined1 auStack_1073c [60];
  undefined4 uStack_10700;
  undefined2 uStack_106fc;
  int iStack_106f8;
  undefined4 uStack_106f4;
  char cStack_106f0;
  undefined4 uStack_106e4;
  undefined1 uStack_106bd;
  undefined1 auStack_10098 [65684];

  iStack_107cc = (param_1 & 0xffff) * 0x724;
  uVar2 = *(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x34 + iStack_107cc);
  iVar1 = *(int *)(DAT_00880360 + 0x34) + iStack_107cc;
  iStack_107e4 = *(int *)((*(uint *)(iVar1 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (uVar2 == 0xffffffff) {
    iVar10 = 0;
  }
  else {
    iVar10 = (uVar2 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  }
  cVar7 = '\0';
  bVar4 = false;
  cStack_107fb = '\0';
  cStack_107fa = '\0';
  if ((((iVar10 != 0) && (*(char *)(iVar10 + 0x42) != '\0')) && (*(short *)(iVar1 + 0x6e) < 3)) &&
     ((*(short *)(iVar1 + 0x72) == 0 && (*(short *)(iVar1 + 0x74) == 0)))) {
    cVar7 = '\x01';
    cStack_107fa = '\x01';
  }
  if (((0 < *(short *)(iVar1 + 0x1e4)) && (*(short *)(iVar1 + 0x6e) < 3)) &&
     (*(short *)(iVar1 + 0x74) == 0)) {
    bVar4 = true;
  }
  if ((*(short *)(iVar1 + 0x6a) < 3) &&
     (*(short *)(&DAT_00655258 + *(short *)(iVar1 + 0x6c) * 0x38) == 0)) {
    return '\x01';
  }
  if (*(char *)(iVar1 + 0x160) == '\0') {
    if (bVar4) {
LAB_0040d3fb:
      if ((*(short *)(iVar1 + 0x6c) == 6) && (*(char *)(iVar1 + 0xa1) != '\0')) {
        return '\x01';
      }
      puVar13 = auStack_107c8;
      cVar8 = FUN_004045a0();
      cVar7 = cStack_107fa;
      if (cVar8 != '\0') {
        uVar12 = 6;
        goto LAB_0040d4a6;
      }
      goto LAB_0040d42b;
    }
    if (cVar7 != '\0') goto LAB_0040d432;
    if (1 < *(short *)(iVar1 + 0x6e)) {
      if (*(uint *)(iVar1 + 0x270) == 0xffffffff) {
        iStack_107e8 = 0;
      }
      else {
        iStack_107e8 = (*(uint *)(iVar1 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
      }
      uStack_107ec = uStack_107ec & 0xffffff00;
      cStack_107f7 = '\0';
      uStack_107f8 = 0;
      uVar6 = uStack_107f4 >> 8;
      uStack_107f4 = uStack_107f4 & 0xffffff00;
      uVar5 = uStack_107f0 >> 8;
      uStack_107f0 = uStack_107f0 & 0xffffff00;
      cStack_107f9 = '\0';
      acStack_107f6[0] = '\0';
      if ((iStack_107e8 == 0) || (*(char *)(iStack_107e8 + 0xbb) == '\0')) {
        uStack_107f8 = iStack_107e8 != 0;
        puVar3 = (&PTR_PTR_006853b8)[*(short *)(iVar1 + 4)];
        auStack_107d8[0] = *(undefined2 *)(puVar3 + 6);
        uStack_107dc = *(undefined2 *)(puVar3 + 8);
        uStack_107d4 = CONCAT22(uStack_107d4._2_2_,*(undefined2 *)(puVar3 + 10));
        uStack_107e0 = CONCAT31(uStack_107e0._1_3_,puVar3[0xc]);
        uStack_107d0 = 0;
        cStack_107fa = '\0';
        acStack_107f6[1] = 0;
        uStack_107f0 = CONCAT31((int3)uVar5,1);
        uStack_107f4 = CONCAT31((int3)uVar6,1);
        cStack_107f9 = '\x01';
        cStack_107f7 = uStack_107f8;
        if (uVar2 != 0xffffffff) {
          FUN_00436d40(*(undefined2 *)(iVar1 + 0x3a),&uStack_107ec,&uStack_107d0,&uStack_107e0,
                       auStack_107d8);
          if (*(char *)(iVar1 + 6) == '\0') {
            FUN_00436dc0(param_1,uStack_107d0,uStack_107e0,&uStack_107f8,&uStack_107f4,&uStack_107f0
                         ,&cStack_107f9,&cStack_107fa,acStack_107f6 + 1,acStack_107f6);
          }
          else {
            uStack_107f4 = CONCAT31(uStack_107f4._1_3_,1);
            uStack_107f8 = 1;
            cStack_107f7 = '\x01';
            cStack_107f9 = '\x01';
            uStack_107f0 = CONCAT31(uStack_107f0._1_3_,1);
          }
        }
        actor_get_target_state_flags
                  (param_1,uStack_107d4,0,*(undefined1 *)(iVar1 + 0x375),&cStack_107f7,&uStack_107f8
                   ,&uStack_107f4,&uStack_107f0,&cStack_107f9,acStack_107f6);
      }
      if (*(int *)(iVar1 + 0x3c0) != *(int *)(iVar1 + 0x270)) {
        *(undefined2 *)(iVar1 + 0x3c4) = 0;
        *(int *)(iVar1 + 0x3c0) = *(int *)(iVar1 + 0x270);
        *(undefined1 *)(iVar1 + 0x3bc) = 0;
        *(undefined1 *)(iVar1 + 0x3bd) = 0;
      }
      if (cStack_107f7 != '\0') {
        puVar13 = auStack_107c8;
        cVar7 = FUN_004080c0(uStack_107f4);
        if (cVar7 != '\0') {
          uVar12 = 5;
          goto LAB_0040d4a6;
        }
      }
      FUN_0041fb00();
      if (((char)uStack_107f4 != '\0') &&
         (cVar7 = FUN_004077d0(*(undefined1 *)(iVar1 + 0x375)), cVar7 != '\0')) {
        uVar12 = 7;
        puVar13 = extraout_EDX;
        goto LAB_0040d4a6;
      }
      FUN_0041fb60();
      if ((*(char *)(iVar1 + 0x3bc) != '\0') && (*(char *)(iVar1 + 0x3bd) == '\0')) {
        uVar12 = FUN_004283d0(0xffffffff,0xffffffff,0xffffffff,0);
        ai_communication_broadcast(0xd,*(undefined4 *)(iVar1 + 0x18),uVar12);
        *(undefined1 *)(iVar1 + 0x3bd) = 1;
      }
      if ((char)uStack_107f0 != '\0') {
        cStack_107fa = '\0';
        *(undefined1 *)(iVar1 + 0x98) = 1;
        if (*(char *)(iVar1 + 6) == '\0') {
          if (((*(short *)(iVar1 + 0x6c) == 5) && (cStack_107f9 != '\0')) &&
             (*(short *)(iVar1 + 0xa4) == 1)) {
            uVar12 = CONCAT22(0xffff,*(short *)(iVar1 + 0xa6));
            cStack_107fa = '\x01';
            if (*(short *)(iVar1 + 0xa6) == -1) goto LAB_0040d1e3;
LAB_0040d2fe:
            if ((cStack_107f9 != '\0') && (cVar7 = FUN_00407820(uVar12,uStack_107ec), cVar7 != '\0')
               ) {
              uVar12 = 7;
              puVar13 = extraout_EDX_01;
LAB_0040d32a:
              actor_set_mode(param_1,uVar12,puVar13);
              if (*(int *)(iVar1 + 0x34) == -1) {
                return '\x01';
              }
              cVar7 = FUN_00436b10(*(int *)(iVar1 + 0x34));
              if (cVar7 != '\0') {
                if (*(short *)(iVar1 + 0x3c4) == 0) {
                  ai_communication_broadcast
                            (0x10,*(undefined4 *)(iVar1 + 0x18),0xffffffff,0xffffffff,0xffffffff,
                             0xffffffff,0);
                }
                *(short *)(iVar1 + 0x3c4) = *(short *)(iVar1 + 0x3c4) + 1;
                return '\x01';
              }
              return '\x01';
            }
          }
          else {
LAB_0040d1e3:
            if (*(int *)(iVar1 + 0x1d0) == -1) {
              sVar9 = *(short *)(iStack_107e4 + 0x356);
            }
            else {
              sVar9 = *(short *)(iStack_107e4 + 0x354);
            }
            if ((((char)uStack_107ec != '\0') ||
                (*(int *)(iVar1 + 0x3c0) != *(int *)(iVar1 + 0x270))) ||
               (*(short *)(iVar1 + 0x3c4) < sVar9)) {
              puVar11 = &uStack_10700;
              for (iVar10 = 0x199; iVar10 != 0; iVar10 = iVar10 + -1) {
                *puVar11 = 0;
                puVar11 = puVar11 + 1;
              }
              iStack_106f8 = *(int *)(iVar1 + 0x270);
              uStack_106fc = 5;
              if (iStack_107e8 == 0) {
                uStack_106f4 = 0xffffffff;
              }
              else {
                uStack_106f4 = *(undefined4 *)(iStack_107e8 + 0x7c);
              }
              uStack_106bd = iStack_106f8 != -1;
              cStack_106f0 = (char)uStack_107ec;
              uStack_10700 = FUN_00412880(0);
              uStack_106e4 = 0x41a00000;
              uVar12 = FUN_00412ba0(param_1,&uStack_10700,auStack_1073c,&iStack_107e4,auStack_10098,
                                    acStack_107f6 + 1);
              if ((short)uVar12 != -1) {
                if ((cStack_107fa == '\0') && (cVar7 = FUN_00408110(uVar12), cVar7 != '\0')) {
                  uVar12 = 5;
                  puVar13 = extraout_EDX_00;
                  goto LAB_0040d32a;
                }
                goto LAB_0040d2fe;
              }
            }
          }
        }
        else if (cStack_107f9 != '\0') {
          puVar13 = auStack_107c8;
          cVar7 = FUN_004078f0();
          if (cVar7 != '\0') {
            uVar12 = 7;
            goto LAB_0040d32a;
          }
        }
      }
      if ((0 < *(short *)(iVar1 + 0x3c4)) && (*(int *)(iVar1 + 0x18) != -1)) {
        ai_communication_broadcast
                  (0x13,*(int *)(iVar1 + 0x18),0xffffffff,0xffffffff,0xffffffff,0xffffffff,0);
      }
      if (((*(char *)(iVar1 + 6) == '\0') && (acStack_107f6[0] != '\0')) &&
         (cVar7 = FUN_00409a90(uStack_107f0), cVar7 != '\0')) {
        uVar12 = 8;
        puVar13 = extraout_ECX;
        goto LAB_0040d4a6;
      }
    }
  }
  else {
    if (bVar4) goto LAB_0040d3fb;
LAB_0040d42b:
    if (cVar7 != '\0') {
LAB_0040d432:
      cStack_107fb = actor_process_order_request(param_1,0xffffffff);
      if (cStack_107fb != '\0') {
        return cStack_107fb;
      }
    }
  }
  if (*(short *)(&DAT_00655258 +
                *(short *)(*(int *)(DAT_00880360 + 0x34) + 0x6c + iStack_107cc) * 0x38) == 1) {
    return cStack_107fb;
  }
  FUN_0041fbc0();
  FUN_00404510();
  uVar12 = 6;
  puVar13 = extraout_EDX_02;
LAB_0040d4a6:
  actor_set_mode(param_1,uVar12,puVar13);
  return '\x01';
}
#endif
