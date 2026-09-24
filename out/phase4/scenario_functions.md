# scenario module: 17 functions (address, current Ghidra name, size, agent confidence, summary)

- 0x53e660 FUN_0053e660 size=27 conf=0.4 :: Calls each of 10 registered function pointers in sequence, used as a pre-switch (deactivate) callback fan-out when leaving the current structure bsp.
- 0x53e680 FUN_0053e680 size=27 conf=0.4 :: Calls each of 13 registered function pointers in sequence, used as a post-switch (activate) callback fan-out after a new structure bsp becomes current.
- 0x53e6a0 scenario_load size=215 conf=0.85 :: Loads the map's cache file, resolves the scenario and matg globals tag data pointers, and switches in the initial structure bsp.
- 0x53e780 FUN_0053e780 size=57 conf=0.3 :: Resolves an index via FUN_005013a0 and, if found, fetches an associated 16-bit field from a stride-0x10 table, writing both values through a caller-supplied struct pointer in ESI.
- 0x53e7c0 FUN_0053e7c0 size=71 conf=0.4 :: Returns a pointer to the globals-tag block element for a given index (0x374-byte stride), or a static default element if the index is out of range.
- 0x53e810 FUN_0053e810 size=87 conf=0.35 :: Given an implicit object/leaf reference, resolves its cluster's referenced tag and returns whether that tag's low bit flag is set.
- 0x53e870 FUN_0053e870 size=71 conf=0.3 :: Retries an allocation/lookup helper up to 150 times, aging a floating-point field by 0.05 per failed attempt, and reports whether the first attempt already succeeded.
- 0x53e8c0 FUN_0053e8c0 size=665 conf=0.4 :: Smoothly blends a per-object ambient/background sound environment record toward a new value as the listener moves, snapping instead when the jump exceeds 15 world units.
- 0x53eb60 FUN_0053eb60 size=65 conf=0.4 :: Tests a single bit in the structure bsp's cluster-by-cluster visibility bit matrix for a given (row, column) cluster pair.
- 0x53ebb0 FUN_0053ebb0 size=109 conf=0.45 :: Searches a 36-byte-stride name/record array for an entry whose text matches the given string and returns its index, or 0xffff if not found.
- 0x53ec30 FUN_0053ec30 size=209 conf=0.4 :: Given an object's leaf reference, locates the boundary portal (if any) facing outward and tests whether a supplied point is on its far side.
- 0x53ed10 FUN_0053ed10 size=66 conf=0.4 :: Resolves a leaf index through two chained indirection tables down to a final surface/material reference, or -1 if any link is unset.
- 0x53ed60 FUN_0053ed60 size=147 conf=0.4 :: Combines the portal-search and surface-reference helpers to return a packed material flag for the current leaf and optionally outputs a related cluster field.
- 0x53ee00 FUN_0053ee00 size=163 conf=0.4 :: Computes the signed distance from an implicit point/vector to the current leaf's boundary plane, biased by a referenced tag's flag-gated offset.
- 0x53eeb0 scenario_structure_bsp_switch size=259 conf=0.65 :: Switches the currently active structure bsp to the requested index, running deactivate/activate callback tables and refreshing the cached bsp tag-data pointers.
- 0x53efc0 FUN_0053efc0 size=96 conf=0.45 :: Checks whether a new structure bsp has been requested and, if so, unloads the previous one and performs the switch.
- 0x53f020 scenario_trigger_volume_contains_point size=296 conf=0.6 :: Tests whether a world-space point lies inside a scenario trigger volume, supporting both axis-aligned box and oriented (matrix-transformed) box variants.
