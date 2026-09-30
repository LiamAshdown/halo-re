// scenario_objects_place  (Ghidra: FUN_004f3ba0; formerly named objects_update_control_bindings)
// address 0x4f3ba0, size 525 bytes (ends with a tail jump to 0x4f4880)
// name confidence: 0.75  rewrite confidence: 0.85
// evidence: game_start_new_map 0x45b351 calls it with the scenario once the map's pools are reset.
// REWRITTEN (first-boot track, objdump 0x4f3ba0..0x4f3da9):
//   - a joining network game (session +8, else client +0xb14, whose +0x134 is 5) skips the binding refresh and
//     every vehicle placement; otherwise the vehicle control-binding table is rebuilt: in a multiplayer map
//     (0x00719720 == 2) each vehicle placement registers (EDX palette tag, EAX placement byte +0x58, EDI its index,
//     EBX placement word +0x5a), then with a game engine the table is updated (variant b when 0x008607a1 is set)
//     and 0x008607a0 is set
//   - then every object type except scenery and light fixtures (mask 0x240; those depend on the structure BSP) and
//     vehicles in a single-player map (0x00719720 == 1) places each placement of its scenario block (type
//     definition +0x0a placement block, +0x0c palette, +0x0e placement size): vehicles only when the binding table
//     wants that placement (control_binding_table_query), created vehicles record their placement index at +0x5b0,
//     and objects_garbage_collection runs after every placement
//   - finally scenario_objects_place_for_structure_bsp(1) (the tail jump) places the BSP-dependent types.
// blam-cc: stack -> scenario
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "fn_objects.h"

extern int32_t network_server;  // 0x0071c2d4, pointer; +8 is its connection state
extern int32_t network_client;   // 0x0071c2d8, pointer; +0xb14 is its connection state
extern int16_t network_game_mode; // 0x00719720: 1 single player, 2 multiplayer
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern game_engine_definition *current_game_engine;
extern uint8_t g_control_binding_secondary_active; // 0x008607a1
extern uint8_t g_control_binding_state; // 0x008607a0
extern data_array *object_data; // 0x008603b0

extern void control_binding_table_initialize(void); // 0x4f3700
extern void control_binding_table_register_single(int32_t target, int32_t selector, int32_t raw_id,
    uint32_t raw_value); // 0x4f37d0, blam-cc: EDX -> target, EAX -> selector, EDI -> raw_id, EBX -> raw_value
extern void control_binding_table_update_a(void); // 0x4f3890
extern void control_binding_table_update_b(void); // 0x4f39d0
extern uint8_t control_binding_table_query(int32_t target, int32_t raw_id); // 0x4f3ad0, EDX target, stack raw_id


static datum_index palette_tag(TagReflexive *palette, int16_t type)
{
    return *(datum_index *)((uint8_t *)palette->pointer + type * 0x30 + 0xc);
}

void scenario_objects_place(uint8_t *scenario)
{
    uint8_t *connection = 0;
    uint8_t joining = 0;
    int16_t type;

    if (network_server != 0) {
        connection = (uint8_t *)network_server + 8;
    } else if (network_client != 0) {
        connection = (uint8_t *)network_client + 0xb14;
    }
    if (connection != 0 && *(int32_t *)(connection + 0x134) == 5) {
        joining = 1;
    } else {
        control_binding_table_initialize();
        if (network_game_mode == 2) {
            object_type_definition *vehicle = object_type_definitions[_object_type_vehicle];
            int32_t size = vehicle->scenario_placement_size;
            TagReflexive *placements = (TagReflexive *)(scenario + vehicle->scenario_placement_offset);
            TagReflexive *palette = (TagReflexive *)(scenario + vehicle->scenario_palette_offset);
            int16_t i;

            for (i = 0; i < (int32_t)placements->count; i++) {
                uint8_t *placement = (uint8_t *)placements->pointer + i * size;
                int16_t kind = *(int16_t *)placement;
                if (kind != -1) {
                    control_binding_table_register_single(palette_tag(palette, kind), placement[0x58], i,
                                                          (uint32_t)*(int16_t *)(placement + 0x5a));
                }
            }
        }
        if (current_game_engine != 0) {
            if (g_control_binding_secondary_active) {
                control_binding_table_update_b();
            } else {
                control_binding_table_update_a();
            }
        }
        g_control_binding_state = 1;
    }

    for (type = 0; type < k_maximum_object_types; type++) {
        object_type_definition *definition;
        TagReflexive *placements;
        TagReflexive *palette;
        int32_t size;
        int16_t i;

        if (network_game_mode == 1 && type == _object_type_vehicle) {
            continue;
        }
        if (((1 << type) & 0x240) != 0) {   // scenery and light fixtures: scenario_objects_place_for_structure_bsp
            continue;
        }
        definition = object_type_definitions[type];
        if (definition->scenario_placement_offset == -1 || definition->scenario_palette_offset == -1) {
            continue;
        }
        size = definition->scenario_placement_size;
        placements = (TagReflexive *)(scenario + definition->scenario_placement_offset);
        palette = (TagReflexive *)(scenario + definition->scenario_palette_offset);
        for (i = 0; i < (int32_t)placements->count; i++) {
            uint8_t *placement = (uint8_t *)placements->pointer + i * size;
            datum_index object;

            if (type == _object_type_vehicle) {
                if (joining || !control_binding_table_query(palette_tag(palette, *(int16_t *)placement), i)) {
                    continue;
                }
            }
            if (placement == 0) {
                continue;
            }
            object = object_new_from_scenario_placement(placement, palette);
            if (object != k_datum_index_none && type == _object_type_vehicle) {
                uint8_t *vehicle = *(uint8_t **)((uint8_t *)object_data->data + (object & 0xffff) * 0xc + 8);
                ((vehicle_object *)vehicle)->vehicle.cinematic_facing_index = i;
            }
            objects_garbage_collection();
        }
    }
    scenario_objects_place_for_structure_bsp(1);
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
