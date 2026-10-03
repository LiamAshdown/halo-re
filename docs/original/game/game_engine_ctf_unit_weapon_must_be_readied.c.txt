// game_engine_ctf_unit_weapon_must_be_readied  (Ghidra: FUN_00466bc0; named per
// out/phase4/game_functions.md: "Returns whether the current weapon has the must-be-readied
// flag while the active game-variant type equals the CTF value (1).")
// address 0x466bc0, size 81 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: types/game.h game_engine_index (_game_engine_ctf == 1); the object_data ->
//   definition_tag -> tag_instances chain matches every other Weapon-flag lookup in this batch
//   (see unit_current_weapon_prevents_camo_depower.c); WeaponFlags::must_be_readied (bit 3,
//   types/tags.h) matches the ">> 3 & 1" the disassembly performs.
// register convention: unit handle in ECX (in_ECX).
//   // blam-cc: ECX -> unit_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88
extern data_array *object_data;                  // 0x008603b0
extern tag_instance *tag_instances;                 // 0x0087bc14

// blam-cc: ECX -> unit_handle
// Returns whether `unit_handle`'s object has a weapon tag with the must_be_readied flag set,
// while a multiplayer CTF engine is active. False in every other case.
uint8_t game_engine_ctf_unit_weapon_must_be_readied(datum_index unit_handle)
{
    object *unit_obj;
    Weapon *weapon_tag;

    if (current_game_engine == 0 || game_engine_variant.game_engine_index != _game_engine_ctf) {
        return 0;
    }

    unit_obj = ((object_header *)object_data->data)[unit_handle & 0xffff].data;
    weapon_tag = (Weapon *)tag_instances[unit_obj->definition_tag & 0xffff].data;
    return (uint8_t)((weapon_tag->weapon_flags >> 3) & 1);
}

#if 0
Original Ghidra decompilation (0x466bc0), from tools/pack.py 0x466bc0:

undefined1 FUN_00466bc0(void)

{
  undefined1 uVar1;
  uint in_ECX;

  uVar1 = 0;
  if (((DAT_006f1d20 != 0) && (DAT_006f1cb8 == 1)) &&
     ((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc)
                          & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) >> 3 & 1) != 0)) {
    uVar1 = 1;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
