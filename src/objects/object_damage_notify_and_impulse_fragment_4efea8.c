#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
// object_damage_notify_and_impulse_fragment_4efea8   (renamed by the phase-4 review pass; Ghidra/PDB name was `caseD_0`,
//    which this file's own header shows does not describe the code)
// address 0x4efea8, size 104 bytes
// name confidence: 0.1 (inherited switch-case label; out/phase4/objects_types_notes.md:
// "Outlined switch-case block (kind 0/1 reaction) belonging to object_damage_notify_and_impulse's
// dispatch, followed by the shared HUD/network-notify epilogue")
// rewrite confidence: n/a — not independently rewritten
//
// This is not a real function: it has zero callers. It is Ghidra's decompilation of the
// biped/vehicle reaction case (kind 0 or 1) of object_damage_notify_and_impulse's (0x4efcf0)
// switch statement, plus that function's shared player-death-notify and unit-type-mask dispatch
// tail — all of it already present, correctly parameterized, in object_damage_notify_and_impulse.c.
// Nothing here is translated independently to avoid a second, inconsistent copy of the same code.
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
