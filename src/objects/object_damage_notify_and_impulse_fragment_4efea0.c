// object_damage_notify_and_impulse_fragment_4efea0   (renamed by the phase-4 review pass; Ghidra/PDB name was `mdp_decode_stateless_iterated`,
//    which this file's own header shows does not describe the code)
// address 0x4efea0, size 8 bytes
// name confidence: 0.1 (inherited PDB name; out/phase4/objects_types_notes.md: "A zero-caller
// stub whose body is identical to the tail of object_damage_notify_and_impulse (HUD/network
// notify then post-damage dispatch); the inherited 'mdp_decode' name does not match the actual
// (no decode of any kind happens)")
// rewrite confidence: n/a — not independently rewritten
//
// This is not a real function: it has zero callers and its 8-byte body (plus the stack-relative
// reads Ghidra shows spilling from a caller frame it never receives, `in_stack_00000040` etc.)
// is Ghidra's decompilation of a code range that is simply the tail end of
// object_damage_notify_and_impulse (0x4efcf0) — the player-death notify block and the final
// unit-type-mask dispatch to FUN_005674a0. See object_damage_notify_and_impulse.c for the full,
// correctly-parameterized rewrite of that logic; nothing here is translated independently to
// avoid a second, inconsistent copy of the same code.
