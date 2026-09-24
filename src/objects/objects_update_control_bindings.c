// objects_update_control_bindings
// address 0x4f3ba0, size 521 bytes
// name confidence: 0.3 (still FUN_004f3ba0 in Ghidra; functions.md's summary -- "Per-tick
//   orchestrator that rebuilds and applies the network/local control-binding table across
//   object types, then runs a follow-up transform+garbage-collect pass on affected objects" --
//   matches the shape of the code, conf=0.4 there)
// rewrite confidence: 0.35
// evidence: types/objects.h object_type_definition (subdefinitions/next are NOT used here; this
//   function instead reads three int16 fields at 0x0a/0x0c/0x0e that the header currently folds
//   into unknown_0a and unknown_0c -- new evidence that unknown_0c is really two int16 halves,
//   kept as raw offsets rather than redefining the struct); global 0x0069bfdc
//   object_type_definitions[12] (PTR_PTR_0069bfe0 is exactly &object_type_definitions[0] + 4,
//   i.e. object_type_definitions[_object_type_vehicle], not a separate global); global
//   0x008603b0 object_data; globals 0x006f1d20, 0x008607a0, 0x008607a1 already named in
//   types/objects.h's global list. Everything else here (0x0071c2d4, 0x0071c2d8, 0x00719720,
//   and the callees at 0x4f3700/0x4f37d0/0x4f3890/0x4f39d0/0x4f3ad0) belongs to the input/game
//   or player module per out/phase4/objects_types_notes.md's "Not objects-module code" section
//   and out/phase4/objects_functions.md and is out of scope for this batch -- kept as opaque
//   externs.
// register convention: param_1 in EAX (Ghidra shows a plain, unprefixed parameter; treated as
//   the leading register per the module's convention). param_1 is a base address that combines
//   with per-type int16 offsets read out of object_type_definition to reach some slot_config
//   table; which one is not established here.
// UNSURE: almost every byte-offset and foreign global in this function is unverified beyond
//   "the control flow and arithmetic below matches the decompilation exactly". In particular:
//   the exact meaning of param_1, of object_type_definition+0x0c/+0x0e, of the write to
//   object+0x5b0 (far past the documented 0x1f4-byte object header), and of the two "local
//   player" globals 0x0071c2d4/0x0071c2d8.
// reconciled: R38 object_type_definition +0x0a/+0x0c/+0x0e/+0x10 -> scenario_placement_offset/scenario_palette_offset/scenario_placement_size/network_delta_message_type (int32, -1 = none)
// reconciled: R04 0x006f1d20 uint8_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"

extern int32_t object_control_local_player_a; // 0x0071c2d4, UNSURE: foreign (player) module global
extern int32_t object_control_local_player_b; // 0x0071c2d8, UNSURE: foreign (player) module global
extern int16_t network_game_mode; // 0x00719720, UNSURE: foreign module global
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint8_t object_control_binding_unknown_7a0; // 0x008607a0, per types/objects.h's global list
extern uint8_t object_control_binding_unknown_7a1; // 0x008607a1, per types/objects.h's global list
extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

// Foreign-module (input/game) control-binding helpers; skipped in this batch per
// out/phase4/objects_types_notes.md's "Not objects-module code" section.
extern void FUN_004f3700(void);            // 0x4f3700, rebuilds the packed control word
extern void FUN_004f37d0(int32_t index);   // 0x4f37d0, UNSURE: the index is not visibly passed
                                            //   as an argument in the original call; recorded
                                            //   here only so the call site documents its context.
extern void FUN_004f3890(void);            // 0x4f3890
extern void FUN_004f39d0(void);            // 0x4f39d0
extern char FUN_004f3ad0(int32_t index);   // 0x4f3ad0

extern uint32_t object_get_or_build_render_permutation(); // 0x4f9b70 = object_get_or_build_render_permutation in this
    // module, whose definition is (int16_t *pair, uint8_t *table_owner) with the pair in EDI.
    // No prototype is asserted: Ghidra models fewer or differently-typed arguments here than
    // the other call site(s) of the same address, because the missing operands travel in
    // registers it could not source. The empty parameter list is the convention this module
    // already uses for FUN_00450870 -- one declaration per symbol, no invented signature.

extern void objects_garbage_collection(void); // 0x4f9c60
extern void objects_update_player_visibility_masks(uint8_t prune); // this module, 0x4f4880 // 0x4f4880, this batch

void objects_update_control_bindings(int32_t param_1) // blam-cc: EAX -> param_1
{
    int already_active = 0;
    int32_t context = 0;

    if (object_control_local_player_a == 0) {
        if (object_control_local_player_b != 0) {
            context = object_control_local_player_b + 0xb14;
            goto have_context;
        }
    } else {
        context = object_control_local_player_a + 8;
have_context:
        if (context != 0 && *(int32_t *)(context + 0x134) == 5) {
            already_active = 1;
            goto after_rebuild;
        }
    }

    FUN_004f3700();

    if (network_game_mode == 2) {
        object_type_definition *vehicle_def = object_type_definitions[_object_type_vehicle];
        int16_t stride = vehicle_def->scenario_placement_size;
        int32_t *slot = (int32_t *)(vehicle_def->scenario_placement_offset + param_1);
        int16_t i = 0;
        if (*slot > 0) {
            int32_t index = 0;
            do {
                if (*(int16_t *)(index * stride + slot[1]) != -1) {
                    FUN_004f37d0(index);
                }
                i = i + 1;
                index = i;
            } while (index < *slot);
        }
    }

    if (current_game_engine != 0) {
        if (object_control_binding_unknown_7a1 == 0) {
            FUN_004f3890();
        } else {
            FUN_004f39d0();
        }
    }
    object_control_binding_unknown_7a0 = 1;

after_rebuild:
    {
        int16_t type_index = 0;
        do {
            if ((network_game_mode != 1 || type_index != 1) &&
                ((1 << (type_index & 0x1f)) & _object_mask_scenery_and_light_fixture) == 0) {
                object_type_definition *def = object_type_definitions[type_index];
                if (def->scenario_placement_offset != -1 && def->scenario_palette_offset != -1) {
                    int16_t stride = def->scenario_placement_size;
                    int16_t base_off = def->scenario_palette_offset;
                    int32_t *slot = (int32_t *)(def->scenario_placement_offset + param_1);
                    int16_t j = 0;
                    if (*slot > 0) {
                        int32_t index = 0;
                        do {
                            int32_t entry = slot[1];
                            int enter_block = 1;
                            if (type_index == 1) {
                                enter_block = 0;
                                if (!already_active) {
                                    enter_block = (FUN_004f3ad0(index) != 0);
                                }
                            }
                            if (enter_block) {
                                if (index * stride + entry != 0) {
                                    uint32_t object_index = object_get_or_build_render_permutation(base_off + param_1);
                                    if (object_index != 0xffffffff && type_index == 1) {
                                        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
                                        *(int16_t *)((uint8_t *)obj + 0x5b0) = j; // UNSURE: see file header
                                    }
                                    objects_garbage_collection();
                                }
                            }
                            j = j + 1;
                            index = j;
                        } while (index < *slot);
                    }
                }
            }
            type_index = type_index + 1;
            if (type_index > 0xb) {
                objects_update_player_visibility_masks(param_1);
                return;
            }
        } while (1);
    }
}

#if 0
Original Ghidra decompilation (0x4f3ba0):

void FUN_004f3ba0(int param_1)

{
  short sVar1;
  undefined *puVar2;
  int iVar3;
  bool bVar4;
  char cVar5;
  short sVar6;
  int iVar7;
  uint uVar8;
  short sVar9;
  short sVar10;
  int *piVar11;

  bVar4 = false;
  if (DAT_0071c2d4 == 0) {
    if (DAT_0071c2d8 != 0) {
      iVar7 = DAT_0071c2d8 + 0xb14;
      goto LAB_004f3bc8;
    }
  }
  else {
    iVar7 = DAT_0071c2d4 + 8;
LAB_004f3bc8:
    if ((iVar7 != 0) && (*(int *)(iVar7 + 0x134) == 5)) {
      bVar4 = true;
      goto LAB_004f3c90;
    }
  }
  FUN_004f3700();
  if (DAT_00719720 == 2) {
    sVar9 = *(short *)(PTR_PTR_0069bfe0 + 0xe);
    piVar11 = (int *)(*(short *)(PTR_PTR_0069bfe0 + 10) + param_1);
    sVar6 = 0;
    if (0 < *(int *)(*(short *)(PTR_PTR_0069bfe0 + 10) + param_1)) {
      iVar7 = 0;
      do {
        if (*(short *)(iVar7 * sVar9 + piVar11[1]) != -1) {
          FUN_004f37d0();
        }
        sVar6 = sVar6 + 1;
        iVar7 = (int)sVar6;
      } while (iVar7 < *piVar11);
    }
  }
  if (DAT_006f1d20 != 0) {
    if (DAT_008607a1 == '\0') {
      FUN_004f3890();
    }
    else {
      FUN_004f39d0();
    }
  }
  DAT_008607a0 = 1;
LAB_004f3c90:
  sVar9 = 0;
  do {
    if ((((DAT_00719720 != 1) || (sVar9 != 1)) && ((1 << ((byte)sVar9 & 0x1f) & 0x240U) == 0)) &&
       ((puVar2 = (&PTR_PTR_0069bfdc)[sVar9], *(short *)(puVar2 + 10) != -1 &&
        (*(short *)(puVar2 + 0xc) != -1)))) {
      sVar6 = *(short *)(puVar2 + 0xe);
      sVar1 = *(short *)(puVar2 + 0xc);
      piVar11 = (int *)(*(short *)(puVar2 + 10) + param_1);
      sVar10 = 0;
      if (0 < *(int *)(*(short *)(puVar2 + 10) + param_1)) {
        iVar7 = 0;
        do {
          iVar3 = piVar11[1];
          if (sVar9 == 1) {
            if (!bVar4) {
              cVar5 = FUN_004f3ad0(iVar7);
              if (cVar5 != '\0') goto LAB_004f3d3d;
            }
          }
          else {
LAB_004f3d3d:
            if (iVar7 * sVar6 + iVar3 != 0) {
              uVar8 = FUN_004f9b70(sVar1 + param_1);
              if ((uVar8 != 0xffffffff) && (sVar9 == 1)) {
                *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar8 & 0xffff) * 0xc) +
                          0x5b0) = sVar10;
              }
              objects_garbage_collection();
            }
          }
          sVar10 = sVar10 + 1;
          iVar7 = (int)sVar10;
        } while (iVar7 < *piVar11);
      }
    }
    sVar9 = sVar9 + 1;
    if (0xb < sVar9) {
      FUN_004f4880();
      return;
    }
  } while( true );
}
#endif
