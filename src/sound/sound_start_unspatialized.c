// sound_start_unspatialized  (Ghidra: FUN_00543dd0; earlier draft name sound_start_trampoline)
// address 0x543dd0, size 51 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: builds a _sound_location_none location (scale from the stack, gain 1.0) and
//   starts the sound with no owner and no location proc. Callers are UI-ish one-shots:
//   game_engine_koth_update_hill_occupancy_state, game_engine_ctf_initialize_flags, the hud
//   message code, item_transfer_ammunition, unit_update.
// Phase-4 review: rewritten from the disassembly below (the earlier draft treated it as a pure
//   seven-argument trampoline).
// register convention: EDX -> definition_index, stack -> scale.
// Only type, scale and gain are written; the rest of the location is stack garbage in the
//   binary and is never read for an unspatialized sound. Zeroed here.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint); // 0x549af0

// blam-cc: EDX -> definition_index, stack -> scale
datum_index sound_start_unspatialized(datum_index definition_index, float scale)
{
    sound_location location = { 0 };

    location.type = _sound_location_none;
    location.scale = scale;
    location.gain = 1.0f;

    return sound_play_new(definition_index, &location, k_datum_index_none, (sound_location_proc)0, (void *)0, 0, 0);
}

#if 0
Original Ghidra decompilation (0x543dd0):

void FUN_00543dd0(void)

{
  FUN_00549af0();
  return;
}

Disassembly (0x543dd0..0x543e03, capstone; phase-4 review):

0x543dd0: sub esp, 0x40
0x543dd3: mov eax, dword ptr [esp + 0x44]
0x543dd7: push 0
0x543dd9: push 0
0x543ddb: push 0
0x543ddd: push 0
0x543ddf: push -1
0x543de1: lea ecx, [esp + 0x14]
0x543de5: push ecx
0x543de6: push edx
0x543de7: mov word ptr [esp + 0x1c], 0
0x543dee: mov dword ptr [esp + 0x20], eax
0x543df2: mov dword ptr [esp + 0x24], 0x3f800000
0x543dfa: call 0x549af0
0x543dff: add esp, 0x5c
0x543e02: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
