// object_damage_notify_and_impulse_fragment_4eff10   (renamed by the phase-4 review pass; Ghidra/PDB name was `mdp_decode_incremental_iterated`,
//    which this file's own header shows does not describe the code)
// address 0x4eff10, size 199 bytes
// name confidence: 0.1 (inherited PDB name; out/phase4/objects_types_notes.md: "Outlined
// default-case block of object_damage_notify_and_impulse's kind-based dispatch, followed by the
// same shared HUD/network-notify epilogue; the inherited name does not match the code")
// rewrite confidence: n/a — not independently rewritten
//
// This is not a real function: it has zero callers. It is Ghidra's decompilation of the tail of
// the vehicle reaction case plus the shared player-death-notify and unit-type-mask dispatch of
// object_damage_notify_and_impulse (0x4efcf0) — all of it already present, correctly
// parameterized, in object_damage_notify_and_impulse.c. Nothing here is translated independently
// to avoid a second, inconsistent copy of the same code.


#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
