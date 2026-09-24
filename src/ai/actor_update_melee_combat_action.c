// actor_update_melee_combat_action  (Ghidra: actor_update_melee_combat_action, renamed)
// address 0x40cdf0, size 1742 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: phase-4 summary "chooses and commits the actor's next melee/close-combat
// action (charge, retreat, wait, or reposition) based on its behavior type and target
// state"; calls actor_process_order_request / actor_get_target_state_flags / actor_set_mode.
// register convention: actor_index in EAX (Ghidra's param_1, a datum_index into actor_data).
//
// This function is left very close to the Ghidra decompilation on purpose: it reuses one
// scratch value (uVar12/puVar13, cVar7, cStack_107f9 etc.) for several different purposes
// across branches, which is exactly the kind of thing that is easy to break by renaming.
// Only the pointer-offset arithmetic is replaced with named field accesses; the original
// variable and label names are kept so this file can be diffed line-for-line against the
// #if 0 block below.
//
// UNSURE: several callees in the 0x404xxx-0x409xxx range (order builders, owned by the
// other half of this session's split) are invoked by Ghidra with fewer visible arguments
// than their one-line summaries suggest ("builds a ... order for the actor"), which almost
// always means Ghidra failed to show an actor_index argument already sitting in a register.
// Declared here exactly as the call sites show them; flagged individually below.
// UNSURE: self->target_unit_index (0x270) is indexed here into prop_data (stride 0x138),
// but types/ai.h calls that field a raw unit datum_index on the strength of a different
// function (actor_choose_best_target). One of the two readings is wrong; kept as Ghidra
// has it (a prop_data index) since that is what this function actually does.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;             // 0x00880360
extern data_array *encounter_data;         // 0x008802c8
extern data_array *prop_data;              // 0x008802c0
extern tag_instance *tag_instances;        // 0x0087bc14
extern void *actor_type_procs[16];         // 0x006853b8
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

extern void actor_build_order_guard(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_build_order_guard at 0x404510
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int32_t actor_build_order_search_wait(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_build_order_search_wait at 0x4045a0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int32_t actor_build_order_flee(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_build_order_flee at 0x4077d0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint32_t actor_build_order_face_seat_marker_committed(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_build_order_face_seat_marker_committed at 0x407820
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int32_t actor_build_order_minimal_stop(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_build_order_minimal_stop at 0x4078f0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int32_t actor_build_order_wait_byte(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_build_order_wait_byte at 0x4080c0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint32_t actor_build_order_face_seat_marker(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_build_order_face_seat_marker at 0x408110
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int32_t actor_build_order_random_wait(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_build_order_random_wait at 0x409a90
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint8_t actor_process_order_request(uint32_t actor_index, uint16_t order_code);
extern void actor_get_target_state_flags(int16_t ax_mode, int16_t cx_mode, uint8_t shared_flag, uint32_t actor_index, int16_t mode_b, char force_c, char force_d, uint8_t *out_a, char *out_in_e, uint8_t *out_f, uint8_t *out_g, uint8_t *out_h, uint8_t *out_i); // 0x40cc70
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0, this module
extern uint32_t actor_get_firing_position_group_mask(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_get_firing_position_group_mask at 0x412880
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint32_t actor_find_best_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_find_best_firing_position at 0x412ba0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern void actor_set_target_alert_stage1(void); // UNSURE: no visible args
extern void actor_set_target_alert_stage2(void); // UNSURE: no visible args
extern void actor_set_target_alert_stage3(void); // UNSURE: no visible args
extern uint32_t actor_get_target_prop_object_index(uint32_t a, uint32_t b, uint32_t c, uint32_t d);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index,
                                       datum_index object_a, int32_t param_d,
                                       datum_index object_b, datum_index object_c,
                                       uint32_t param_g);
// 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call
// site in the binary cleans up 0x1c bytes, so the shorter forms Ghidra recovers at some
// sites are artefacts, not a reduced-arity overload.
extern uint8_t ai_pursuit_note_object(datum_index encounter_index);  // SIGNATURE-CONFLICT: this call site disagrees with the form the rest of
  // src/ai uses for this address; kept local. See src/ai/README.md.
// src/ai/ai_release_inactive_encounters.c declares 0x436b10 as returning void; this call
// site tests its AL, so the return type is not settled.
extern void ai_starting_location_derive_placement_flags(int16_t squad_index, void *out_a, void *out_b, void *out_c, void *out_d);
extern void encounter_evaluate_support_needs(datum_index actor_index, uint32_t param_2, uint32_t param_3, void *out_a, void *out_b,
                          void *out_c, void *out_d, void *out_e, void *out_f, void *out_g);

// TYPES (folded into types/ai.h by the review pass): the per-ActorType records actor_type_procs points at are only partially
// resolved in types/ai.h (vtable slots +0x10/+0x18/+0x1c, the swarm byte at +0x0d). This
// function additionally reads three int16s and a byte at +0x06/+0x08/+0x0a/+0x0c.

// blam-cc: actor_index in EAX
uint8_t actor_update_melee_combat_action(datum_index actor_index)
{
    actor *self;
    Actor *actor_def;
    encounter *enc;
    datum_index encounter_index_val;
    prop *target_prop; // iStack_107e8, 0 when there is none
    uint8_t cVar7, cVar8;
    uint8_t bVar4;
    uint8_t cStack_107fb, cStack_107fa, cStack_107f9;
    uint8_t uStack_107f8, cStack_107f7;
    uint8_t acStack_107f6[2];
    uint32_t uStack_107f4, uStack_107f0, uStack_107ec;
    int16_t auStack_107d8[2];
    int32_t uStack_107d4, uStack_107d0, uStack_107e0;
    int32_t uVar12;
    // UNSURE: on the LAB_0040d4a6 paths the original leaves the mode-data pointer in whatever
    // register the preceding order builder returned (extraout_ECX / extraout_EDX_02); modelled
    // as a null pointer here rather than inventing a source.
    void *puVar13 = (void *)0;
    int16_t sVar9;
    actor_type_table_entry *type_entry;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    encounter_index_val = self->encounter_index;
    actor_def = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    enc = (encounter_index_val == (datum_index)k_datum_index_none)
              ? (encounter *)0
              : (encounter *)((uint8_t *)encounter_data->data + (encounter_index_val & 0xffff) * sizeof(encounter));

    cVar7 = 0;
    bVar4 = 0;
    cStack_107fb = 0;
    cStack_107fa = 0;

    if (enc != (encounter *)0 && enc->unknown_42 != 0 && self->unknown_6e < 3 &&
        self->unknown_72 == 0 && self->unknown_74 == 0) {
        cVar7 = 1;
        cStack_107fa = 1;
    }
    if (0 < self->unknown_1e4 && self->unknown_6e < 3 && self->unknown_74 == 0) {
        bVar4 = 1;
    }
    if (self->awareness_level < 3 && actor_mode_definitions[self->mode].combat_grade == 0) {
        return 1;
    }

    if (self->order_committed == 0) {
        if (bVar4) goto LAB_0040d3fb;
        if (cVar7 != 0) goto LAB_0040d432;
        if (1 < self->unknown_6e) {
            target_prop = (self->target_unit_index == (datum_index)k_datum_index_none)
                              ? (prop *)0
                              : (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & 0xffff) * sizeof(prop));
            uStack_107ec &= 0xffffff00;
            cStack_107f7 = 0;
            uStack_107f8 = 0;
            uStack_107f4 &= 0xffffff00;
            uStack_107f0 &= 0xffffff00;
            cStack_107f9 = 0;
            acStack_107f6[0] = 0;
            if (target_prop == (prop *)0 || target_prop->noticed_c == 0) {
                uStack_107f8 = (target_prop != (prop *)0);
                type_entry = (actor_type_table_entry *)actor_type_procs[self->type];
                auStack_107d8[0] = type_entry->unknown_06;
                auStack_107d8[1] = type_entry->unknown_08; // uStack_107dc, adjacent to auStack_107d8[0]
                uStack_107d4 = type_entry->unknown_0a;
                uStack_107e0 = type_entry->unknown_0c;
                uStack_107d0 = 0;
                cStack_107fa = 0;
                acStack_107f6[1] = 0;
                uStack_107f0 = 1;
                uStack_107f4 = 1;
                cStack_107f9 = 1;
                cStack_107f7 = uStack_107f8;
                if (encounter_index_val != (datum_index)k_datum_index_none) {
                    ai_starting_location_derive_placement_flags(self->squad_index, &uStack_107ec, &uStack_107d0, &uStack_107e0, auStack_107d8);
                    if (self->swarm == 0) {
                        encounter_evaluate_support_needs(actor_index, uStack_107d0, uStack_107e0, &uStack_107f8, &uStack_107f4,
                                     &uStack_107f0, &cStack_107f9, &cStack_107fa, acStack_107f6 + 1, acStack_107f6);
                    } else {
                        uStack_107f4 = 1;
                        uStack_107f8 = 1;
                        cStack_107f7 = 1;
                        cStack_107f9 = 1;
                        uStack_107f0 = 1;
                    }
                }
                // UNSURE: the original shows ten stack arguments. actor_get_target_state_flags
                // also takes two mode selectors in EAX / ECX and a flag byte in EDX, which
                // Ghidra drops at this call site; zero is passed for all three.
                actor_get_target_state_flags(0, 0, 0,
                                              actor_index, uStack_107d4, 0, self->unknown_375,
                                              &cStack_107f7, (char *)&uStack_107f8, (uint8_t *)&uStack_107f4, (uint8_t *)&uStack_107f0,
                                              &cStack_107f9, acStack_107f6);
            }
            if (self->unknown_3c0 != self->target_unit_index) {
                *(int16_t *)&self->unknown_3c4 = 0;
                self->unknown_3c0 = self->target_unit_index;
                self->unknown_3bc = 0;
                self->unknown_3bd[0] = 0;
            }
            if (cStack_107f7 != 0) {
                cVar7 = actor_build_order_wait_byte((uint8_t)uStack_107f4);
                if (cVar7 != 0) {
                    uVar12 = 5;
                    goto LAB_0040d4a6;
                }
            }
            actor_set_target_alert_stage1();
            if ((uint8_t)uStack_107f4 != 0) {
                cVar7 = actor_build_order_flee(self->unknown_375);
                if (cVar7 != 0) {
                    uVar12 = 7;
                    goto LAB_0040d4a6;
                }
            }
            actor_set_target_alert_stage2();
            if (self->unknown_3bc != 0 && self->unknown_3bd[0] == 0) {
                uVar12 = actor_get_target_prop_object_index(-1, -1, -1, 0);
                // The call site at 0x40d13b pushes seven arguments; Ghidra only recovered three.
                ai_communication_broadcast(0xd, self->unit_index, uVar12, -1, -1, -1, 0);
                self->unknown_3bd[0] = 1;
            }
            if ((uint8_t)uStack_107f0 != 0) {
                cStack_107fa = 0;
                self->unknown_98 = 1;
                if (self->swarm == 0) {
                    if (self->mode == 5 && cStack_107f9 != 0 && target_prop->engaged == 1) {
                        uVar12 = (int32_t)(uint16_t)target_prop->unknown_a6 | (int32_t)0xffff0000u;
                        cStack_107fa = 1;
                        if (target_prop->unknown_a6 == -1) goto LAB_0040d1e3;
LAB_0040d2fe:
                        if (cStack_107f9 != 0) {
                            cVar7 = actor_build_order_face_seat_marker_committed((int16_t)uVar12, (uint8_t)uStack_107ec);
                            if (cVar7 != 0) {
                                uVar12 = 7;
                                goto LAB_0040d32a;
                            }
                        }
                    } else {
LAB_0040d1e3:
                        if (self->unknown_1d0 == (datum_index)k_datum_index_none) {
                            sVar9 = actor_def->num_positions__normal_;
                        } else {
                            sVar9 = actor_def->num_positions__coord_;
                        }
                        if ((uint8_t)uStack_107ec != 0 || self->unknown_3c0 != self->target_unit_index ||
                            *(int16_t *)&self->unknown_3c4 < sVar9) {
                            uint32_t local_context[0x199];      // uStack_10700.. block, zeroed as 0x199 dwords
                            uint8_t local_scratch[60];          // auStack_1073c
                            for (uint32_t i = 0; i < 0x199; i++) local_context[i] = 0;
                            *(int32_t *)((uint8_t *)local_context + 0x08) = self->target_unit_index; // iStack_106f8
                            *(int16_t *)((uint8_t *)local_context + 0x04) = 5;                        // uStack_106fc
                            if (target_prop == (prop *)0) {
                                *(int32_t *)((uint8_t *)local_context + 0x0c) = -1; // uStack_106f4
                            } else {
                                *(int32_t *)((uint8_t *)local_context + 0x0c) = target_prop->unknown_7c;
                            }
                            *((uint8_t *)local_context + 0x21) = (self->target_unit_index != (datum_index)k_datum_index_none); // uStack_106bd
                            *((uint8_t *)local_context + 0x18) = (uint8_t)uStack_107ec; // cStack_106f0
                            *(int32_t *)((uint8_t *)local_context + 0x00) = actor_get_firing_position_group_mask(0); // uStack_10700
                            *(uint32_t *)((uint8_t *)local_context + 0x1c) = 0x41a00000; // uStack_106e4
                            uVar12 = actor_find_best_firing_position(actor_index, local_context, local_scratch,
                                                   &actor_def, (void *)0, acStack_107f6 + 1);
                            // UNSURE: actor_find_best_firing_position is called here with the same argument shape
                            // as the Ghidra decompilation (actor_index, &uStack_10700,
                            // auStack_1073c, &iStack_107e4, auStack_10098, acStack_107f6+1); the
                            // huge auStack_10098[65684] scratch buffer is passed through as NULL
                            // here because its true size makes it implausible as a real stack
                            // buffer -- almost certainly Ghidra mis-sized an out-parameter or a
                            // pointer into caller-owned memory. Needs the disassembly review pass.
                            if ((int16_t)uVar12 != -1) {
                                if (cStack_107fa == 0) {
                                    cVar7 = actor_build_order_face_seat_marker(uVar12);
                                    if (cVar7 != 0) {
                                        uVar12 = 5;
                                        goto LAB_0040d32a;
                                    }
                                }
                                goto LAB_0040d2fe;
                            }
                        }
                    }
                } else if (cStack_107f9 != 0) {
                    cVar7 = actor_build_order_minimal_stop();
                    if (cVar7 != 0) {
                        uVar12 = 7;
                        goto LAB_0040d32a;
                    }
                }
            }
            if (0 < *(int16_t *)&self->unknown_3c4 && self->unit_index != (datum_index)k_datum_index_none) {
                ai_communication_broadcast(0x13, self->unit_index, -1, -1, -1, -1, 0);
            }
            if (self->swarm == 0 && acStack_107f6[0] != 0) {
                cVar7 = actor_build_order_random_wait((uint8_t)uStack_107f0);
                if (cVar7 != 0) {
                    uVar12 = 8;
                    goto LAB_0040d4a6;
                }
            }
        }
    } else {
        if (bVar4) goto LAB_0040d3fb;
        goto LAB_0040d42b;
LAB_0040d3fb:
        if (self->mode == 6 && self->mode_data[5] != 0) {
            return 1;
        }
        cVar8 = actor_build_order_search_wait(actor_index);
        cVar7 = cStack_107fa;
        if (cVar8 != 0) {
            uVar12 = 6;
            goto LAB_0040d4a6;
        }
LAB_0040d42b:
        if (cVar7 != 0) {
LAB_0040d432:
            cStack_107fb = actor_process_order_request(actor_index, -1);
            if (cStack_107fb != 0) {
                return cStack_107fb;
            }
        }
    }

    if (actor_mode_definitions[self->mode].combat_grade == 1) {
        return cStack_107fb;
    }
    actor_set_target_alert_stage3();
    actor_build_order_guard(actor_index);
    uVar12 = 6;
    puVar13 = (void *)0;
    goto LAB_0040d4a6;

    // Two distinct shared tails in the original, NOT one: only LAB_0040d32a (the
    // face-seat-marker / minimal-stop paths) goes on to poke the encounter; LAB_0040d4a6
    // (the search-wait, random-wait and guard paths) sets the mode and returns immediately.
LAB_0040d32a:
    puVar13 = (void *)0;
    actor_set_mode(actor_index, uVar12, puVar13);
    if (self->encounter_index == (datum_index)k_datum_index_none) {
        return 1;
    }
    if (ai_pursuit_note_object(self->encounter_index) != 0) {
        if (*(int16_t *)&self->unknown_3c4 == 0) {
            ai_communication_broadcast(0x10, self->unit_index, -1, -1, -1, -1, 0);
        }
        *(int16_t *)&self->unknown_3c4 = *(int16_t *)&self->unknown_3c4 + 1;
    }
    return 1;

LAB_0040d4a6:
    actor_set_mode(actor_index, uVar12, puVar13);
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
