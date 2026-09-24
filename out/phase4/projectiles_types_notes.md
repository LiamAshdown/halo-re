# projectiles module: type recovery notes

Header: `types/projectiles.h`. Smoke test: `out/phase4/projectiles_smoke.c`, built with

```
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/projectiles_smoke.c
```

It passes. It carries negative-array size assertions for all eight structs plus ~60
`offsetof` assertions, the `projectile_data` ones written as *object* offsets
(`0x1f4 + offsetof(projectile_data, state) == 0x230` and so on) so a field reordering fails
the build instead of passing silently. The other seven phase-4 smoke files still compile
unchanged (`cache`, `hs`, `items`, `math`, `objects`, `units`, `tags` all re-verified).

The module was handed to me as "22 functions, 0x4bda60..0x4c1070". Those 22 are the projectile
slice of the run Ghidra seeded as `items`, split out by `out/phase4/items_types_notes.md`;
the packs are `out/phase2/items/00.md` and `01.md`, and there are no
`out/phase2/results/projectiles_*.json` files.

---

## The binary evidence everything hangs on

### 1. The object_type_definition row at 0x0069b9a0 -> the size of `projectile_data`

Read out of `.data` (PE image base 0x400000, `.data` VA 0x676000 -> file offset 0x276000; the
12-pointer table is at 0x0069bfdc):

| field | value |
|---|---|
| `name` | `"projectile"` |
| `category` (+0x04) | `0x70726f6a` = `proj` — this field is the tag group fourcc, not a category |
| `object_size` (+0x08) | **0x2b0** |
| +0x0a / +0x0c / +0x0e | -1 / -1 / -1 (no scenario placement block) |
| +0x10 | **1** (the network delta message index) |
| `subdefinitions[]` (+0x80) | `{ object, projectile }` |

Two-deep chain, so `projectile_data` starts at object+0x1f4 and is exactly
`0x2b0 - 0x1f4 = 0xbc` bytes. Nothing in the header is inferred from the highest offset seen.

Because it starts at 0x1f4 it **overlaps `item_data`** (also 0x1f4, 0x38 bytes), which is why
`types/items.h` deliberately left these fields out.

### 2. The projectile row function table -> five functions Ghidra never created

Dumping +0x14..0x7c of the same row gives the module its real shape. These are the columns the
broadcast/override helpers at 0x4f3e30..0x4f4760 call:

| column | address | what it does |
|---|---|---|
| +0x14..+0x20, +0x30 | 0x0044ad80 | shared `ret` stub |
| +0x28 | **0x4bd7c0** | `projectile_new` — **missed by Ghidra**; the single best source in the module |
| +0x34 | 0x4bdc00 | `projectile_update` |
| +0x38 | 0x4c0250 | `projectile_update_function_values` |
| +0x3c | **0x4bf0c0** | `projectile_notify_object_deleted` — **missed** |
| +0x44 | **0x4c0ac0** | `projectile_force_detonate` — **missed** |
| +0x60 | 0x00571dd0 | shared `xor al,al; ret` |
| +0x64 | 0x4c0b10 | `projectile_send_creation` |
| +0x68 | **0x4c0ed0** | `projectile_network_baseline_take` — **missed** |
| +0x6c | 0x4c0f30 | `projectile_build_network_update` |
| +0x70 | 0x4c1070 | `projectile_apply_network_update` |
| +0x74 | **0x4c1270** | `projectile_is_old_enough` — **missed** |
| +0x78 | 0x00572a80 | shared `mov al,1; ret` |

I disassembled all five with capstone. They are worth creating in Ghidra before the clean-C
pass: `projectile_new` alone establishes the initial value of `flags`, the type and initial
value of `state`, `material_response_index`, `ignore_object_index`, `tracked_object_index`,
`contrail_attachment_index` and both timer rates, and `projectile_network_baseline_take`
resolves the three network bytes that the decompiled functions only read.

### 3. The static ProjectileMaterialResponse at 0x00695e20 -> the tag block

The fallback record both `projectile_response` (0x4bf390) and `projectile_detonate` (0x4c0670)
substitute for an out-of-range material index is 0xa0 static bytes: all zero except three
`TagDependency` triples `{'effe', &k_empty_string, 0, -1}` at +0x04, +0x3c and +0x68. That is
exactly `default_effect` / `potential_effect` / `detonation_effect` in `types/tags.h`, and the
consumers read the `tag_id` of each at +0x10, +0x48 and +0x74 with stride 0xa0. The binary
therefore confirms `ProjectileMaterialResponse` independently of the tag definitions, and
`projectile_response` additionally touches +0x02, +0x24, +0x26, +0x28, +0x2c, +0x30, +0x34,
+0x38, +0x5c, +0x60, +0x64, +0x90, +0x98 and +0x9c, every one of them a named field.

### 4. The Projectile tag offsets -> the attribution

Every tag offset the 22 functions read lands on a named `Projectile` field (Object base 0x17c):
`0x17c` flags, `0x180` detonation_timer_starts, `0x182` impact_noise, `0x184..0x18a`
projectile_a_in..d_in, `0x198` super_detonation.tag_id, `0x1a0` collision_radius, `0x1a4`
arming_time, `0x1b8` effect.tag_id, `0x1bc/0x1c0` timer[2], `0x1c4` minimum_velocity, `0x1c8`
maximum_range, `0x1cc` air_gravity_scale, `0x1d0/0x1d4` air_damage_range, `0x1d8`
water_gravity_scale, `0x1dc/0x1e0` water_damage_range, `0x1e4` initial_velocity, `0x1e8`
final_velocity, `0x1ec` guided_angular_velocity, `0x200` detonation_started.tag_id, `0x210`
flyby_sound.tag_id, `0x220` attached_detonation_damage.tag_id, `0x230` impact_damage.tag_id,
`0x240/0x244` the material response reflexive.

The flag bits are the clincher: `projectile_new` and `projectile_response` both branch on
tag+0x17c bit 0x04 then bit 0x40 then bit 0x20 to choose between `timer[1]`, `timer[0]` and
`random_real_range(timer[0], timer[1])`, which is `detonation_max_time_if_attached`,
`minimum_unattached_detonation_time` and `random_attached_detonation_time` in exactly the
`ProjectileFlags` bit order. Bit 0x08 (`has_super_combining_explosion`) gates the sibling
sweep in both attach paths and in `projectile_detonate`.

---

## Per-struct: which functions established which fields

### `projectile_data` (object 0x1f4..0x2b0, 0xbc bytes)

| offset | field | established by |
|---|---|---|
| 0x1f4 | `unknown_1f4[0x38]` | **UNRESOLVED**, see below |
| 0x22c | `flags` | `projectile_new` stores the literal dword 2; 0x4c0180 sets/clears 0x01; 0x4bdc00 tests 0x02/0x08/0x10/0x20 and sets 0x04/0x20; 0x4bf390 sets 0x08/0x10/0x14/0x80; 0x4bf1c0 sets 0x08/0x80; 0x4c0670 sets 0x40; 0x4c0ac0 clears 0x08; 0x4c0250 tests 0x02 |
| 0x230 | `state` | `projectile_new` writes word 0; `projectile_request_state` (0x4bf0f0) raises it; 0x4bdc00 dispatches on 0/1/2; 0x4bf390 requests 1 for `projectileresponse_detonate` and 2 for `projectileresponse_disappear` |
| 0x232 | `material_response_index` | `projectile_new` writes word -1; 0x4bf390 writes the collision material type (or the value damage handling returned) and 0x4c0670 reads it back to pick `detonation_effect.tag_id` at row +0x74 |
| 0x234 | `ignore_object_index` | `projectile_new` walks object 0xc4 up the `parent_object` (0x11c) chain and stores the root; 0x4c0450 passes it to the collision sweep as the ignore handle; 0x4bdc00 reads it for the flyby-sound owner test and forces -1 after each collision; 0x4bf390 stores the overpenetrated object |
| 0x238 | `tracked_object_index` | `projectile_new` writes -1; `projectile_notify_object_deleted` (0x4bf0c0) clears it when that object dies; 0x4bdc00 steers toward it using tag `guided_angular_velocity`/30 |
| 0x23c | `contrail_attachment_index` | `projectile_new` writes -1 then scans the Object tag attachments block (count 0x140, pointer 0x144, stride 0x48) for group `'cont'`; 0x4bdc00 indexes `object.attachment_handles` with it (`object + 0x14c + index*4`) and `contrail_delete`s the handle |
| 0x240 | `detonation_timer` | `projectile_new` never writes the progress, only the rate; 0x4bdc00 accumulates and requests state 1 at 1.0; 0x4c0250 returns it for `projectilefunctionin_time_remaining`; 0x4bf1c0 / 0x4bf390 zero it on siblings; 0x4c0670 randomises or zeroes it; 0x4c0ac0 forces 1.0 |
| 0x244 | `detonation_timer_rate` | `projectile_new` `1/(t*30)`, and the same three-way tag-flag choice again in 0x4bf1c0 and 0x4bf390 |
| 0x248 | `arming_timer` | `projectile_new` leaves the progress zero; 0x4bdc00 accumulates it every tick and refuses to detonate in state 1 while it is under 1.0; same sibling zeroing/randomising as 0x240; 0x4c0ac0 forces 1.0 |
| 0x24c | `arming_timer_rate` | `projectile_new`: `1.0 / (Projectile.arming_time * 30)`, stored only when the product is at least one tick. **This is what names the pair** — the decompiled functions alone would only have shown "a second timer" |
| 0x250 | `distance_travelled` | 0x4bdc00 adds the integrated step and clips the tick against `maximum_range`; 0x4c0250 divides by `maximum_range` for `projectilefunctionin_range_remaining` |
| 0x254 | `deceleration_delay` | 0x4bdc00 accumulates it and skips the velocity decay while under 1.0; 0x4c0310 forces 1.0 when there is no damage range |
| 0x258 | `deceleration_delay_rate` | 0x4c0310: `air|water_damage_range[0] / initial_velocity` (verified in the disassembly: `fld [ecx+0x1d0]; fdiv [ecx+0x1e4]`), or 0 |
| 0x25c | `deceleration` | 0x4c0310 stores the return of 0x4c03f0, which is `(initial_velocity^2 - final_velocity^2) / (2 * (range[1] - range[0]))`; 0x4bdc00 multiplies it by the remaining tick fraction and subtracts from the speed |
| 0x260 | `deceleration_end_range` | 0x4c0310 (see the bug below); 0x4bdc00 compares `distance_travelled` against it when `deceleration` is 0 |
| 0x264 | `rotation_axis` | 0x4c0180, the exact analogue of `item_compute_rotation`: normalizes `object.angular_velocity` (0x8c) |
| 0x270 | `rotation_sine` | 0x4c0180 `fsin(|angular_velocity|)`, 0 when not rotating; 0x4bdc00 and 0x4be1b0 read the pair back for `vector3d_rotate_about_axis` |
| 0x274 | `rotation_cosine` | 0x4c0180 `fcos(...)`, 1.0 when not rotating |
| 0x278 | `thrown_grenade` | `projectile_new` writes byte 0. An image-wide byte-store scan (`c6 /0 disp32=0x278`) finds exactly one writer of 1 — `unit_throw_grenade_move_to_hand` at 0x0056e41a — and one reader, the `cmp byte [ebx+0x278], 1` at 0x4beab2 inside `projectile_update`, which gates the detonation broadcast. UNSURE of the name; the *facts* are exact |
| 0x279 | `network_state_valid` | 0x4c0ca0 and 0x4c0ed0 set 1; `projectile_new` clears it when 0x00719720 is 1 or 2 |
| 0x27a | `network_baseline_index` | 0x4c0ed0 increments; 0x4c0ca0 seeds from the creation message; 0x4c0f30 sends it; 0x4c1070 compares and adopts; 0x4c0b10 sends it |
| 0x27b | `network_sequence` | 0x4c0ed0 and 0x4c0ca0 zero it; 0x4c0f30 increments with a 0xff -> 0 wrap; 0x4c1070 rejects an update unless the incoming sequence is newer by less than 0x1e |
| 0x27c | `network_state` (0x18) | 0x4c0ed0 fills it from `object.position`/`object.velocity`; 0x4c0ca0 from the creation message; 0x4c0f30 hands its address to the delta encoder; 0x4c0b10 sends it; 0x4c1070 reads and rewrites it |
| 0x294 | `last_update_valid` | 0x4c1070 sets 1 |
| 0x295 | `pad_295[3]` | never touched |
| 0x298 | `last_update_state` (0x18) | 0x4c1070; ends exactly on object_size 0x2b0 |

### `projectile_network_state` (0x18)

`{ real_point3d position; real_vector3d velocity; }`. Two copies inside `projectile_data`, both
written field by field by 0x4c0ed0 / 0x4c0ca0 / 0x4c1070, and 0x4c1070 also mirrors them into
`object.velocity` and into the object interpolation block (see the objects.h corrections).

### `collision_result` (0x50) — *not owned by this module*

Produced by the collision module at 0x00505880, allocated by `projectile_collision_test`
(0x4c0450) and read by `projectile_response` (0x4bf390). Spelled in `types/projectiles.h`
because the projectile responder is what pins it; it should move to a `types/collision.h`.

The size is bounded on both sides:

* `projectile_update` does `sub esp,0x15c` plus three pushes and passes `lea edx,[esp+0x118]`
  as the output buffer, so the buffer is the top local and the frame leaves exactly
  `0x168 - 0x118 = 0x50` bytes above it;
* `projectile_response` reads a word at +0x4e.

Fields, from the `ebp`-relative accesses in 0x4bf390 (`mov ebp,[esp+0x110]` is param 2) plus
two from the caller frame:

| offset | field | evidence |
|---|---|---|
| 0x00 | `type` | `cmp word [ebp], 3` nine times; only 0, 2 and 3 are produced |
| 0x02 | `unknown_02` | never read |
| 0x04 | `unknown_04[8]` | never read |
| 0x0c | `leaf` (`bsp_leaf_reference`) | forwarded to `object_set_cluster_and_parent` as `param_2 + 6` (= +0x0c), which writes the pair into object 0x98/0x9c; also copied into the breakable-surface record |
| 0x14 | `t` | caller side: `fld [1.0]; fsub [esp+0x12c]` with the buffer at `esp+0x118`, giving `1 - t` as the remaining tick |
| 0x18 | `point` | `lea eax,[ebp+0x18]`; returned as the new projectile position and passed to the impact-noise call |
| 0x24 | `normal` | `lea ecx,[ebp+0x24]`; `[ebp+0x2c] > 0.3` is the ground test, confirmed on the caller side as `fld [esp+0x144]` = base+0x2c |
| 0x30 | `unknown_30` | never read |
| 0x34 | `material_type` | `mov dx, word [ebp+0x34]`; seeds `material_response_index` and is bounded against the tag block count |
| 0x36 | `unknown_36` | never read |
| 0x38 | `object_index` | nine reads; `object_apply_damage`, `object_attach_to_object`, the sibling sweep and `ignore_object_index` |
| 0x3c | `unknown_3c` | `object_apply_damage` argument 4 |
| 0x3e | `marker_index` | `object_attach_to_object(object_index, projectile, [ebp+0x3e])` and the attach message |
| 0x40 | `unknown_40` | never read |
| 0x44 | `surface_index` | forwarded to 0x004ffde0, the breakable-surface damage routine. UNSURE of the name |
| 0x48 | `unknown_48` | never read |
| 0x4c | `surface_flags` | `test byte [ebp+0x4c], 8` selects the breakable-surface path |
| 0x4d | `unknown_4d` | packed into the low half of the 0x004ffde0 argument |
| 0x4e | `unknown_4e` | `object_apply_damage` argument 5 |

`collision_result_type`: 0 is a **water/media surface** (the overpenetrate response toggles
object flag 0x10, calls `projectile_compute_deceleration` for the new medium and nudges the
position 0.001 back along the normal), 2 is a **structure BSP surface**, 3 is an **object**.
Value 1 never appears.

### The four network records

| struct | size | built by | applied by |
|---|---|---|---|
| `projectile_creation_message` | 0x54 | 0x4c0b10, message **0x1e** | 0x4c0ca0 |
| `projectile_detonation_message` | 0x10 | 0x4bda60, message **0x30** | 0x4bdb40 |
| `projectile_attach_message` | 0x0a | 0x4bf120, message **0x33** | 0x4bf1c0 |
| `projectile_network_update_header` | 0x07 | 0x4c0f30, message index `object_type_definition + 0x10` = **1** | 0x4c1070 |

Sizes are the stack footprint, not the wire size — `message_delta_encode_message` bit-packs
them. Every object handle goes through `hash_table_get` against the id table at
`PTR_DAT_00687130 + 0x0c` first, and a -1 result is written as 0.

The creation record layout is read straight off the stores in 0x4c0b10 (`local_54` -> +0x00
through `local_4` -> +0x50): definition_tag, projectile hash, name_index (object 0xb8), the
hashes of object 0xc0 and object 0xc4, `network_state.position`, `object.forward`,
`object.up`, `network_state.velocity`, `object.angular_velocity`, `network_baseline_index`.

For the attach message I traced the two register inputs of 0x4bf120: `ecx` is the projectile
index (`mov ebx,[esp+0x114]` at 0x4bfff5, then `mov ecx,ebx`) and `edi` is
`collision_result.object_index` (`mov edi,[ebp+0x38]` at 0x4c00e1), so the projectile handle is
first and the parent handle second.

---

## Unresolved

1. **`projectile_data.unknown_1f4[0x38]`, object 0x1f4..0x22c.** 56 bytes with no reader or
   writer anywhere in the module — I grepped all 22 packs and disassembled all five recovered
   functions, and `projectile_new` does not initialize a byte of it either, so it arrives zeroed
   from the object pool. It is real (the type row makes `projectile_data` 0xbc bytes and the
   network tail ends exactly on 0x2b0), so its consumers are in another module. Worth a
   targeted search once the damage, AI-aiming or HUD modules are typed.

2. **`deceleration_delay_rate` (0x258) looks inverted.** `projectile_compute_deceleration`
   stores `damage_range[0] / initial_velocity`, which is the *number of ticks* needed to travel
   the near damage range. The counter at 0x254 accumulates that value and gates the decay at
   1.0, so it completes after `initial_velocity / damage_range[0]` ticks — the reciprocal of the
   obvious intent ("do not decay until the projectile has flown past the near damage range").
   I verified the operand order in the disassembly (`fld [ecx+0x1d0]; fdiv [ecx+0x1e4]` and the
   water twin `fld [ecx+0x1dc]; fdiv [ecx+0x1e4]`), so the header documents the arithmetic as
   written. Either the field means something I have not worked out, or retail has the divide
   backwards.

3. **`deceleration_end_range` (0x260) is fed from the wrong tag field in the air branch.**
   Both branches of `projectile_compute_deceleration` do
   `mov eax,[ecx+0x1e0]; mov [edx+0x260],eax` — tag 0x1e0 is `water_damage_range[1]`. The air
   branch correctly reads `air_damage_range[0..1]` (0x1d0/0x1d4) for `deceleration` but then
   stores the *water* far range here. Kept as-is in the header, flagged as a retail bug.

4. **`thrown_grenade` (0x278)** — the two facts are exact (only
   `unit_throw_grenade_move_to_hand` writes 1, only the detonation broadcast reads it) but the
   name is my inference.

5. **`projectile_network_update_header.is_delta` (+0x06)** — 0x4c0f30 sets it to
   `(mode == 0)` while passing `(mode == 1)` as the encoder "is baseline" flag, and 0x4c1070
   treats a nonzero value as permission to adopt the incoming baseline. The polarity is
   consistent but the intent is not obvious; marked UNSURE.

6. **`collision_result` 0x02, 0x04..0x0c, 0x30, 0x36, 0x40, 0x48** are never read by this
   module, and 0x3c / 0x44 / 0x4d / 0x4e are named only from how they are forwarded. The
   collision module at 0x00505880 will settle them.

7. **The two collision masks** `0x1000e9` (point sweep) and `0x89` (the two radius-offset
   sweeps) that 0x4c0450 passes to 0x00505880 are recorded as constants but not decoded.

---

## Misattributed functions

### 0x4be1b0 `resolution_list_add_resolution` is not a function

It is the mid-body loop of `projectile_update` (0x4bdc00). Evidence:

* `callers=0` in `out/functions.json`;
* both decompilations contain the same `LAB_004be5ad` / `LAB_004be5c1` labels, i.e. 0x4bdc00
  already decompiles code that lives inside 0x4be1b0;
* 0x4be1b0 decompiles with `unaff_EBX` / `unaff_ESI` / `unaff_EDI` / `in_stack_...` inputs and
  no prologue of its own — the same signature Ghidra produces for a promoted label;
* 0x4bdc00 (size 1456) ends exactly at 0x4be1b0, and the real function body runs to 0x4beb30.

This is the same failure `types/items.h` recorded for the 0x4c62d0 / 0x4c62f0 / 0x4c6340 trio.
Usefully, the 0x4be1b0 decompilation prints the *absolute* offsets that 0x4bdc00 prints as
`puVar3[0x8b]`-style dword indices, which is how 0x22c/0x230/0x234/0x238/0x23c/0x248/0x24c/
0x250/0x254/0x25c/0x260/0x270/0x274/0x278 were cross-checked. The inherited name comes from an
unrelated symbol; the code reads `Projectile.final_velocity`, `air`/`water_gravity_scale` and
`maximum_range` and calls `projectile_detonate`.

### 0x4beb30, 0x4bee20, 0x4beec0 are AI ballistic-aiming helpers, not projectile state

They take a **Projectile tag pointer** and plain vectors, never an object index, and touch no
`projectile_data` field. 0x4beec0 is the dispatcher: it reads `tag + 0x1e4`
(`initial_velocity`) as the default speed when the caller passes no speed, then tests
`tag + 0x17c` bit 0x02 (`ai_must_use_ballistic_aiming`) together with
`tag + 0x1cc` (`air_gravity_scale`) `> 0` to pick between the gravity-arc solver 0x4beb30 and
the straight-line solver 0x4bee20, and reports which one it used through its last out
parameter. 0x4beb30 is pure math (gravity 0x0069c52c plus the float pool) and 0x4bee20 just
returns a direction, its length and the length over the given speed. They belong with the AI
aiming code; no types recorded for them here.

### 0x4bef80 owns nothing

Adds a velocity delta plus a random spin (from the `sphere_point_table` at 0x006b7af4) to an
**unparented** object, calls 0x4c0180 and clears object flag 0x20. Only object fields. It is
the detach counterpart of the attach responses, which set that flag.

### Renames this pass establishes

| address | current name | should be | evidence |
|---|---|---|---|
| 0x4bda60 | `FUN_004bda60` | `projectile_send_detonation` | builds `{hash, object.position}` and encodes message 0x30, then forces `object.network_role = 3` |
| 0x4bdb40 | `FUN_004bdb40` | `projectile_detonation_message_apply` | `object_try_and_get(0x20)`, role 3, reposition, `projectile_detonate`, raise state, `object_delete` |
| 0x4bdc00 | `FUN_004bdc00` | `projectile_update` | the projectile row +0x34 column; the item row +0x34 is 0x4bc5c0 (`item_update`) |
| 0x4be1b0 | `resolution_list_add_resolution` | delete; not a function | fragment of 0x4bdc00, see above |
| 0x4beb30 | `FUN_004beb30` | `projectile_solve_ballistic_arc` | gravity-arc solver, tag pointer only. UNSURE of the exact name |
| 0x4bee20 | `FUN_004bee20` | `projectile_solve_straight_line` | direction + length + length/speed |
| 0x4beec0 | `FUN_004beec0` | `projectile_get_aiming_vector` | dispatches on `ai_must_use_ballistic_aiming` and `air_gravity_scale` |
| 0x4bef80 | `FUN_004bef80` | `object_apply_impulse_and_spin` | not projectile-specific; clears object flag 0x20 |
| 0x4bf0f0 | `item_update_max_permutation_reached` | `projectile_request_state` | `if (*(int16 *)(object + 0x230) < cx) *(int16 *)(object + 0x230) = cx;` — nothing about permutations. `eax` = projectile index, `cx` = requested `projectile_state` |
| 0x4bf120 | `FUN_004bf120` | `projectile_send_attach` | message 0x33 with `{projectile hash, parent hash, marker}` |
| 0x4bf1c0 | `FUN_004bf1c0` | `projectile_attach_apply` | receiver of 0x33: zeroes velocity and angular_velocity, sets flags 0x08 and object flag 0x20, `object_attach_to_object`, seeds `detonation_timer_rate`, runs the super-combine sibling sweep |
| 0x4bf390 | `FUN_004bf390` | `projectile_response` | the whole `ProjectileResponse` state machine against `ProjectileMaterialResponse` |
| 0x4c0180 | `FUN_004c0180` | `projectile_compute_rotation` | byte-for-byte analogue of `item_compute_rotation` (0x4bd500) on projectile 0x264..0x278 |
| 0x4c0250 | `item_update_function_values` | `projectile_update_function_values` | reads the four `ProjectileFunctionIn` at tag 0x184 and writes `object.function_in_values` (0x124) |
| 0x4c0310 | `FUN_004c0310` | `projectile_compute_deceleration` | fills 0x254..0x260 from the air or water damage range depending on object flag 0x10 |
| 0x4c03f0 | `FUN_004c03f0` | `projectile_deceleration_from_range` | `(v0^2 - v1^2) / (2*(r1 - r0))` |
| 0x4c0450 | `FUN_004c0450` | `projectile_collision_test` | point sweep plus two `collision_radius`-offset sweeps; nothing to do with hovering above ground |
| 0x4c0670 | `item_detonate` | `projectile_detonate` | matches the CEA hint for the `"gravity"` string; super-combining sibling sweep, `attached_detonation_damage`, material `detonation_effect` |
| 0x4c0b10 | `FUN_004c0b10` | `projectile_send_creation` | message 0x1e |
| 0x4c0ca0 | `FUN_004c0ca0` | `projectile_create_from_network` | receiver of 0x1e |
| 0x4c0f30 | `FUN_004c0f30` | `projectile_build_network_update` | projectile row +0x6c; `object_try_and_get(0x20)`, type-row +0x10 as the message index |
| 0x4c1070 | `item_apply_network_update` | `projectile_apply_network_update` | projectile row +0x70 |
| **0x4bd7c0** | *(missing)* | `projectile_new` | projectile row +0x28 |
| **0x4bf0c0** | *(missing)* | `projectile_notify_object_deleted` | projectile row +0x3c |
| **0x4c0ac0** | *(missing)* | `projectile_force_detonate` | projectile row +0x44 |
| **0x4c0ed0** | *(missing)* | `projectile_network_baseline_take` | projectile row +0x68 |
| **0x4c1270** | *(missing)* | `projectile_is_old_enough` | projectile row +0x74 |

---

## Corrections this pass owes other headers

These are outside `types/projectiles.h` but were proved here, and someone should fold them in.

### `types/objects.h`: `object` 0x18/0x1c and 0x44/0x48 are the interpolation block

`projectile_apply_network_update` (0x4c1070) writes, for one accepted update:

```
object + 0x18 = 1                 (uint8)
object + 0x1c = position          (real_point3d, 0x1c..0x28)
object + 0x44 = 1                 (uint8)
object + 0x48 = velocity          (real_vector3d, 0x48..0x54)
```

`types/objects.h` currently has `unknown_018` (uint8), `unknown_019[7]`,
`player_visibility_mask` at 0x20 and `unknown_022[0x3a]` over that range, so at least
0x1c..0x28 and 0x44..0x54 are misdescribed and `player_visibility_mask` at 0x20 collides with
the position. `projectile_new` also zeroes `object + 0x09` alongside the three network bytes,
so that byte is network state too.

### `types/objects.h`: `object + 0x0c` is a game-tick stamp, not a datum handle

`projectile_is_old_enough` (0x4c1270): if `object[0x0c] == -1` return true, else return
`game_time >= object[0x0c] + [0x006894c8]`. The field is documented as
`datum_index unknown_00c` today.

### `types/objects.h`: `object + 0xc0` and `object + 0xc4`

`projectile_send_creation` runs both through `hash_table_get` against the *object* network id
table, so both are object handles, and `projectile_new` walks 0xc4 up the `parent_object` chain
to find the firing unit. 0xc4 is the creating object; `owner_linkage` at 0xc0 is the other one.
`objects.h` has 0xc4 as `unknown_0c4`.

### `types/objects.h`: `object_type_definition + 0x04`

Documented as `int32_t category` ("0 = delete immediately, 3 = delete recursively"). The bytes
are the tag group fourcc: `bipd`, `vehi`, `weap`, `eqip`, `garb`, `proj`, `scen`, `mach`,
`ctrl`, `lifi`, `plac`, `ssce`.

### `types/objects.h`: object flags this module pins

0x10 = in water (`projectile_new` probes the medium through the structure BSP globals and sets
or clears it; `projectile_compute_deceleration` and the gravity pick branch on it; the water
overpenetrate response toggles it). 0x20 = attached / physics suspended (both attach paths set
it, 0x4bef80 clears it). 0x2000 and 0xc0000 are set by `projectile_new`. 0x0800 gates the
relink in `projectile_apply_network_update`, which also sets 0x8000000 ("has taken a network
update"). 0x4000000 is set after a successful attach broadcast.

### `types/items.h`: the projectile appendix can be trimmed

The `projectile_data` sketch in `out/phase4/items_types_notes.md` was right about almost
everything and is what made this pass fast. Two corrections: 0x230 is a two-byte `state` enum
(not a "high-water mark") with a separate `material_response_index` at 0x232, and the byte at
0x278 is a real field the sketch skipped (it listed 0x279 as the first network byte). The
sketch also read 0x248/0x24c as a second detonation timer; `projectile_new` shows it is the
arming timer.
