// player_spawn_starting_profile_weapon  (Ghidra: FUN_00477810; named per
// out/phase4/game_functions.md: "Creates a new object attached to an owning unit, selecting
// its datum role based on the current game engine mode and team state.")
// address 0x477810, size 169 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x477810
//   --stop-address=0x4778c0): the only caller, unit_apply_starting_profile (0x473c50, already
//   rewritten -- src/game/unit_apply_starting_profile.c), calls this twice with ESI pointing at
//   scenario's ScenarioPlayerStartingProfile::primary_weapon and ::secondary_weapon (both
//   TagDependency, types/tags.h) -- so `unaff_ESI+0xc` is that TagDependency's own `tag_id`
//   field and `unaff_ESI+0x10/+0x12` are the rounds_loaded/rounds_reserved int16 pair that
//   follow the TagDependency in ScenarioPlayerStartingProfile. object_placement_data_initialize
//   (0x4f53a0) and object_new_with_datum_role_control (0x4f54b0) are already-rewritten sibling
//   files in this batch. The team/role gate reads the new weapon tag's Object::object_type
//   (tag_instance data at +0x14 per types/cache.h) to index object_type_definitions
//   (types/objects.h, 0x0069bfdc) and tests its still-unnamed +0x10 field. The final two writes
//   land on weapon_data::magazines[0].rounds_unloaded/.rounds_loaded (types/items.h
//   weapon_magazine_state, object+0x2b6/+0x2b8). NOTE: unit_apply_starting_profile.c's own
//   header calls this function's second (stack) parameter "owner_unit_handle" and passes its
//   own `unit_handle` for it; this function's body never reads that value for anything besides
//   forwarding it verbatim into object_placement_data_initialize's `role` argument, so both
//   descriptions agree on behavior even though they disagree on what the value conceptually
//   represents. Named `role` here to match object_placement_data_initialize's own parameter.
// register convention: the owning struct pointer (a TagDependency inside the caller's
//   ScenarioPlayerStartingProfile) in ESI (unaff_ESI); the datum role value ("param_2" to
//   object_placement_data_initialize) is this function's own single stack parameter.
//   // blam-cc: ESI -> weapon_dependency, stack -> role
// UNSURE: object_type_definition::unknown_10's real meaning (why a non-multiplayer-dedicated
//   weapon type with that field set forces datum role 0 instead of 3).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"

extern int16_t network_game_mode;    // 0x00719720
extern tag_instance *tag_instances;  // 0x0087bc14
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern data_array *object_headers;   // 0x008603b0

extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0
extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role); // 0x4f54b0

// blam-cc: ESI -> weapon_dependency, stack -> role
// Spawns the weapon tag referenced by `weapon_dependency` (a ScenarioPlayerStartingProfile
// TagDependency) as a fresh object with the given datum role -- downgraded from 3 to 0 when
// running as a dedicated server and the weapon's object type carries a set unknown_10 field --
// then stamps the caller's starting rounds_loaded/rounds_reserved onto the new object's first
// weapon magazine. Returns the new object's datum index, or -1 if the dependency has no tag.
datum_index player_spawn_starting_profile_weapon(TagDependency *weapon_dependency, uint32_t role)
{
    object_placement_data placement;
    datum_index new_object;

    new_object = (datum_index)0xffffffff;
    if (*(datum_index *)&weapon_dependency->tag_id != (datum_index)0xffffffff) {
        object_placement_data_initialize(&placement, *(datum_index *)&weapon_dependency->tag_id, (datum_index)role);

        {
            uint32_t datum_role = 3;
            if (network_game_mode == 2) {
                tag_instance *inst = &tag_instances[weapon_dependency->tag_id.index];
                Object *tag_data = (Object *)inst->data;
                object_type_definition *def = object_type_definitions[tag_data->object_type];
                if (def->unknown_10 != 0xffffffff) {
                    datum_role = 0;
                }
            }
            new_object = object_new_with_datum_role_control(&placement, datum_role);
        }

        if (new_object != (datum_index)0xffffffff) {
            weapon_data *weapon = (weapon_data *)((object_header *)object_headers->data)[new_object & 0xffff].data;
            weapon->magazines[0].rounds_unloaded = *(int16_t *)((uint8_t *)weapon_dependency + 0x12);
            weapon->magazines[0].rounds_loaded = *(int16_t *)((uint8_t *)weapon_dependency + 0x10);
        }
    }
    return new_object;
}

#if 0
Original Ghidra decompilation (0x477810), from tools/pack.py 0x477810:

uint FUN_00477810(undefined4 param_1)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  int unaff_ESI;
  uint local_88 [34];

  uVar2 = 0xffffffff;
  if (*(int *)(unaff_ESI + 0xc) != -1) {
    object_placement_data_initialize(*(int *)(unaff_ESI + 0xc),param_1);
    uVar3 = 3;
    if ((DAT_00719720 == 2) &&
       (*(int *)((&PTR_PTR_0069bfdc)
                 [**(short **)((local_88[0] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)] + 0x10) != -1))
    {
      uVar3 = 0;
    }
    uVar2 = object_new_with_datum_role_control(local_88,uVar3);
    if (uVar2 != 0xffffffff) {
      iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
      *(undefined2 *)(iVar1 + 0x2b6) = *(undefined2 *)(unaff_ESI + 0x12);
      *(undefined2 *)(iVar1 + 0x2b8) = *(undefined2 *)(unaff_ESI + 0x10);
    }
  }
  return uVar2;
}
#endif
