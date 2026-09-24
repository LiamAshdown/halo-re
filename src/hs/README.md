# `hs` — HaloScript: compiler, runtime and the object-list container

Retail Halo PC `halo.exe` 1.0.10, `0x482b20 .. 0x48b340` (34,848 bytes of code, 125 functions),
plain C / MSVC 7.1 / x86. Every file in this directory is one function, rewritten from its Ghidra
decompilation against `types/hs.h`, with the original decompile preserved verbatim at the bottom
of the file inside `#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py hs` → **121 ok, 0 failed**.
121 files for 125 functions; the other four are not hs code — see *Misattributed functions*.

## What the module contains

Five families, which really are one pipeline: text in, a syntax-node graph in the middle, and a
cooperative interpreter over that graph.

| Family | Range | What it is |
|---|---|---|
| `sv_*` evaluate stubs | `0x482b20`–`0x4830b0` | 13 one-argument built-ins that forward a dedicated-server console setting to its native setter |
| globals, autocomplete, source | `0x483420`–`0x484400` | global-name lookup, the console tab-completion walkers, `data\…\scripts\*.hsc` discovery |
| compiler | `0x484400`–`0x487030` | tokenizer → parser → post-link pass, plus `hs_add_global` / `hs_add_script` and `hs_doc` |
| script built-ins | `0x487030`–`0x48a250` | the object / effect / damage / trigger-volume glue the script language exposes |
| runtime | `0x48a250`–`0x48b340` | threads, the evaluation stack, typed globals, type coercion, object lists |

The pipeline:

```
hs_rebuild_source            enumerate data\<level>\scripts\*.hsc and global_scripts.hsc
  hs_compile_source
    hs_tokenize              text -> a tree of hs_syntax_node, one datum each
      hs_tokenize_primitive  atoms; lowercased unless hs_preserve_token_case is set
      hs_tokenize_nonprimitive
    hs_parse                 assigns each node a type; resolves names to functions/scripts/globals
      hs_parse_primitive     dispatches through hs_parse_primitive_procedures[type]
      hs_parse_nonprimitive  the special forms: if, cond (desugared), set, begin, sleep, ...
    hs_compile_postprocess   final type check over the whole node table

hs_runtime_update            once per tick
  hs_thread_evaluate_step    walk one thread until its stack empties or its tick budget runs out
    <definition->evaluate>   the built-in's handler; carves scratch out of the current frame
      hs_thread_push         evaluate a child: either resolve it inline, or push a frame
      hs_thread_return       coerce the finished value and store it through the PARENT frame
```

### The one thing to understand before reading any evaluate handler

`hs_stack_frame::result_address` (`+0x08`) does **not** say where *this* frame stores its own
result. `hs_thread_push` writes it onto the frame it is *suspending*, immediately before creating
the child's frame, and `hs_thread_return` stores through `thread->stack->previous->result_address`.
So a parent with several children rewrites that single slot before each one, and a freshly pushed
frame's own `+0x08` is meaningless until it pushes a child of its own.

Every evaluate handler therefore hands `hs_thread_push` the address of one of *its own* scratch
fields — `child_value`, `results[i]`, the `ticks` slot, and so on. The first pass of this rewrite
passed `frame->result_address` at six call sites; all six are fixed and each carries the
instruction address that proves the real destination.

## Struct layouts

All of these live in `types/hs.h`; the tables below are the summary. Offsets are byte offsets from
the struct base. `#pragma pack(push,1)` is in force.

### `hs_syntax_node` — size `0x14`

The compiled expression tree. One datum in `hs_syntax_data`; the same 20 bytes the scenario tag
carries as `ScenarioScriptNode` inside `script_syntax_data`.

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `identifier` (datum header) |
| `0x02` | `int16_t` | `index_union` — constant type, function index, or script index; `hs_tokenize` seeds `0xffff` |
| `0x04` | `hs_type_t` | `type` — 0 until parsed; `hs_parse` writes the expected type here |
| `0x06` | `uint16_t` | `flags` — see below |
| `0x08` | `datum_index` | `next_node` — next sibling, `-1` at the end of a list |
| `0x0c` | `int32_t` | `source_offset` — byte offset into `hs_compiled_source`, `-1` when unknown |
| `0x10` | union | `data` — boolean / short / long / real / `char *` / global ref / scenario index / tag ref / `first_child` |

Flags: `0x01` primitive, `0x02` script call (index is a script, not a function), `0x04` global
variable, `0x08` garbage-collectable, `0x10` local variable.

### `hs_function_definition` — size `0x1c + 2*parameter_count`

`hs_function_definitions` at `0x00688b58` holds `0x20a` pointers into a run of these starting at
`0x00657660`.

| Off | Type | Field |
|---|---|---|
| `0x00` | `hs_type_t` | `return_type` — `_hs_type_passthrough` for `begin`/`if`/`cond`/`set` |
| `0x02` | `int16_t` | `unknown_02` — zero in all `0x20a` records |
| `0x04` | `char *` | `name` |
| `0x08` | `void *` | `parse` — `char (*)(int16_t function_index, datum_index node)` |
| `0x0c` | `void *` | `evaluate` — `void (*)(int16_t index, datum_index thread, char first)`; NULL for `cond` |
| `0x10` | `char *` | `info` — documentation sentence, never NULL |
| `0x14` | `char *` | `param_info` — hand-written argument list, NULL for 480 of 522 |
| `0x18` | `int16_t` | `gametype_flags` — 0 means always available |
| `0x1a` | `int16_t` | `parameter_count` |
| `0x1c` | `hs_type_t[]` | `parameters` |

### `hs_global_definition` — size `0x10`

`hs_global_definitions` at `0x0068b398` holds `0x1eb` pointers.

| Off | Type | Field |
|---|---|---|
| `0x00` | `char *` | `name` |
| `0x04` | `hs_type_t` | `type` — only boolean / real / short / long occur |
| `0x06` | `int16_t` | `pad_06` — zero in all records |
| `0x08` | `void *` | `address` — the engine variable; NULL for 389 of 491 |
| `0x0c` | `uint32_t` | `gametype_flags` — 0 on all but six records |

### `hs_global` — size `0x08`

Runtime storage, one per global, in `hs_globals_data` (`0x400` entries). Builtins occupy indices
`0 .. 0x1ea`; a scenario global with index *i* lives at `0x1eb + i`.

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `identifier` (datum header; `hs_scenario_scripts_initialize` stamps `0xaced`) |
| `0x02` | `int16_t` | `unknown_02` — never read or written by this module |
| `0x04` | union | `value` — boolean / short / long / real / `char *` / `datum_index` |

A **packed `hs_global_reference`** (`uint16_t`) is a different encoding and the two must not be
confused: bit 15 set means builtin (index is `ref & 0x7fff`), clear means scenario-defined (datum
index is `(ref & 0x7fff) + 0x1eb`), and `0xffff` means unknown.

### `hs_stack_frame` — header `0x10`, scratch follows

| Off | Type | Field |
|---|---|---|
| `0x00` | `hs_stack_frame *` | `previous` — the root frame stores 0 |
| `0x04` | `datum_index` | `syntax_node` — the node this frame is evaluating |
| `0x08` | `void *` | `result_address` — where the **child** of this frame stores its result (see above) |
| `0x0c` | `int16_t` | `size` — bytes of scratch handed out so far |
| `0x0e` | `uint8_t[]` | `scratch` |

`hs_thread_push` places the next frame at `frame + 0x10 + frame->size`; every handler carves
scratch with `p = frame + 0x0e + frame->size; frame->size += n`. So the scratch area starts two
bytes *before* the next frame's base.

### `hs_thread` — size `0x218`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `identifier` (datum header) |
| `0x02` | `uint8_t` | `type` — 0 script, 1 global initializer, 2 console/command |
| `0x03` | `uint8_t` | `flags` — bit 0 "pushed a frame", bit 1 "wake_tick saved" |
| `0x04` | `int32_t` | `script_index` — index into `Scenario::scripts`, `-1` when none |
| `0x08` | `int32_t` | `wake_tick` — 0 runs now, `-1` parks forever, `-2` is a dormant script's initial state |
| `0x0c` | `int32_t` | `saved_wake_tick` |
| `0x10` | `hs_stack_frame *` | `stack` — equals `&stack_data` when idle |
| `0x14` | `int32_t` | `result` — only `hs_evaluate_expression` uses it |
| `0x18` | `uint8_t[512]` | `stack_data` |

### Evaluate-handler scratch layouts

Not separately allocated: each is a run of `frame->size` bumps in the order shown, recomputed
identically on every resumption.

| Handler | Address | Layout (offset → field) |
|---|---|---|
| `hs_evaluate_variadic_arguments` | `0x48ad60` | `0x00` `evaluated_count` (4), `0x04` `values[32]` (0x80), `0x84` `argument_count` (2), `0x86` `next_node` (4) |
| `hs_evaluate_argument_list` | `0x489d50` | `0x00` `next_node` (4), `0x04` `count` (4), `0x08` `values[32]` (0x80) |
| `hs_evaluate_random` | `0x488c60` | `0x00` `child_count` (2), `0x02` `chosen` bitmap (4), `0x06` `child_value` (4) |
| `hs_evaluate_boolean_and_or` | `0x489120` | `0x00` `next_node` (4), `0x04` `child_value` (4, low byte only), `0x08` `result` (1) |
| `hs_evaluate_arithmetic_reduce` | `0x489250` | `0x00` `term_count` (2), `0x02` `next_node` (4), `0x06` `child_value` (4), `0x0a` `accumulator` (4) |
| `hs_evaluate_sleep` | `0x489800` | `0x00` `condition` (4, byte), `0x04` `ticks` (4, word), `0x08` `timeout_ticks` (4), `0x0c` `start_tick` (4), `0x10` `stage` (2) |
| `hs_evaluate_typed_arguments` | `0x48a850` | `0x00` `results[parameter_count]`, then `index` (2), then `next_node` (4) |

The two *collectors* (`0x489d50`, `0x48ad60`) are the odd ones out: they keep no child-value slot
in the frame at all, and instead hand `hs_thread_push` the address of their own already-consumed
stack parameter. See *Known gaps* #1.

### `object_list_header` / `object_list_reference` — both size `0x0c`

`object_lists_initialize` (`0x48b250`) creates both arrays: 0x30 headers, 0x80 references.

| Off | `object_list_header` | `object_list_reference` |
|---|---|---|
| `0x00` | `int16_t identifier` | `int16_t identifier` |
| `0x02` | `int16_t unknown_02` | `int16_t unknown_02` |
| `0x04` | `int16_t reference_count` — holders; `object_lists_dispose_empty` deletes at 0 | `datum_index object_index` |
| `0x06` | `int16_t count` — nodes in the chain | — |
| `0x08` | `datum_index first_reference` | `datum_index next` |

### `hs_enum_definition` — size `0x08`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `count` |
| `0x04` | `char **` | `names` — matched case-insensitively |

Indexed by `hs_parse_enum` as `(&hs_enum_definitions[-0x20])[type]`, i.e. the record for
`_hs_type_game_difficulty` sits at `0x0065b634` and the `-0x20` bias is folded into the base
address. Only five records exist (types `0x20`..`0x24`); the bytes below `0x0065b634` belong to the
`hs_function_definition` run, so the array must not be indexed outside that window.

### Foreign-module slices (temporary)

`types/hs.h` ends with partial views of records the objects / units / game / effects / files
modules own. They carry the `hs_` prefix so the real definitions can land in those modules' own
headers later without colliding; only the offsets hs actually touches are named.

| Slice | Owner | Named offsets |
|---|---|---|
| `hs_object_header_entry` (`0x0c`) | objects | `0x00` identifier, `0x03` type flag, `0x08` data |
| `hs_object_record` (partial, `0x1f5` named) | objects | `0x00` tag_id (UNSURE), `0x04` placement kind, `0xb4` type, `0xdc` max health, `0xe4` health, `0x114`/`0x118`/`0x11c` sibling/child/parent, `0x1f4` flags |
| `hs_object_iterator_state` (`0x0c`) | objects | `0x00` type filter (`-1` = any), `0x04` next index, `0x08` handle |
| `hs_player_record` (stride `0x200`) | game | `0x34` unit handle |
| `hs_game_time_globals` | game | `0x00`..`0x02` budget flags, `0x0c` current tick |
| `hs_damage_request` (`0x54`) | effects | `0x00` damage effect, `0x08`/`0x0c` causer/attacker, `0x14` sound impulse, `0x18` sound index, `0x1c` position, `0x28` direction, `0x40`/`0x44` scales |
| `file_reference` (`0x10c`) | files | opaque; hs only stamps `0x00` = `0x66696c6f`, `0x04` flags bit 0, `0x06` = `0xffff`, and passes `0x08` (the path) to `path_append_component` |

## Misattributed functions

Four addresses inside the module range are not hs code; no file was written for them.

1. **`0x483f40`** — not a function at all. The bytes are `add edi, 0x10c`, the cursor advance in
   `hs_rebuild_source`'s enumeration loop; a backward branch made Ghidra split it out as an entry
   point and cut `hs_rebuild_source` short at 288 bytes. The real function runs `0x483e20`–
   `0x484087` (616 bytes). It had been named `hs_null_evaluate`.
2. **`0x483d20`** (245 bytes) — the `qsort` comparator `hs_rebuild_source` passes. It builds a
   0x100-byte path out of a `file_reference` via `0x5560d0` and compares; the record and both
   helpers belong to the **files** module. Referenced here only as an `extern`.
3. **`0x4875c0`** (102 bytes) — a generic "index of a string in a `char **` table" helper:
   `(count, table)` on the stack, the search string in EAX, returns the index in AX or `0xffff`.
   Used by `hs_add_global`, `hs_add_script` and the autocomplete walkers, but it is **cseries/text**
   code, not hs.
4. **`0x48ab80`** (9 bytes) — `fld dword [esp+4]` / `jmp 0x6391b4`. A CRT float thunk, despite the
   inherited Ghidra name `hs_object_orient`.

## Known gaps

1. **The two argument collectors hand out a C-stack address as a result destination.**
   `hs_evaluate_argument_list` (`0x489e18`) and `hs_evaluate_variadic_arguments` (`0x48ae4e`) both
   do `lea ebx,[esp+0x20]` — the address of their own already-consumed `first`/`value` stack
   parameter — and read it back immediately after `hs_thread_push`. For a primitive child that is
   fine, because push resolves it inline. For a non-primitive child the value only arrives later,
   via `hs_thread_return` writing through the address push recorded, by which time that C frame has
   unwound. It presumably works because the call chain `hs_thread_evaluate_step` → handler →
   collector has a fixed frame layout, so the slot lands at the same address on every resumption.
   Written literally; **this is the single most worthwhile thing to confirm with a hook.**
2. **`hs_add_global` (`0x485b60`) has no success path.** Every return is a bare `xor al,al`, and the
   error "i couldn't allocate space for this global." is set precisely when `hs_parse` *succeeds*.
   The reading that makes it consistent: retail has no runtime storage to allocate a script global
   into (`Scenario::globals` is a fixed tag block baked by tool/Sapien), so the runtime compiler
   validates a `(global …)` form completely and then always fails. Globals in shipped scripts
   arrive already compiled inside the scenario tag and never go through this function.
3. **`hs_parse_cond_recursive`'s "needs a result" error is dead code.** `0x4849a0` computes
   `sete cl` over `(next_node == 0)` and then compares `ecx` against `-1`, which can never match.
   A `cond` pair with a condition but no result therefore falls into the desugaring splice instead
   of erroring. The intended test was almost certainly `next_node == k_datum_index_none`.
4. **`file_reference_exists` (`0x555720`) is called twice on the same reference in
   `hs_rebuild_source`,** with the second result discarded, at `0x483ecf` and `0x48405b` — both
   times immediately before the "found a script file" flag is cleared. A pure existence predicate
   has no reason to be called twice on one argument, so the second site is most likely a different
   files-module routine (the one that actually appends the file's contents to the combined source
   buffer — the step this function's name implies and which is otherwise absent) folded onto the
   same address by `/OPT:ICF`. Preserved literally.
5. **No type header for objects / units / game / effects / files.** 15 files reach into those
   modules; the slices in `types/hs.h` cover only the offsets hs touches, and
   `hs_object_detach_and_place_at_location` (`0x487f50`, 1568 bytes, 22 callees) is still mostly
   raw word-indexed access. That function is the weakest in the module and should be revisited once
   `types/objects.h` exists.
6. **Register-implicit callee arguments.** Ghidra renders a number of calls with no argument list
   because the callee reads registers the caller holds live (`FUN_004f5aa0`, `FUN_00566910`,
   `FUN_0056ce30`, `FUN_0056d6a0`, `game_engine_compute_look_angles_from_vector`,
   `FUN_004f6080` at `0x4878f0`). Those are marked `// UNSURE:` at each site rather than invented.
   126 such markers remain module-wide, down from 209.
7. **`hs_coerce_value` and `hs_thread_return` index `hs_type_conversion_procedures` without a NULL
   test.** 2331 of the 2401 slots are NULL, so an incompatible type pair is a call through NULL;
   only the compiler's `hs_types_are_compatible` pass keeps it from happening. Not "fixed".
8. **`hs_thread_evaluate_step` dereferences a NULL script pointer** on the auto-push path when the
   thread's type is not `_hs_thread_script` (`0x48a3ed`). The decompile does not gate that path by
   thread type either; every caller happens to have pushed a frame already for non-script threads,
   so it may be unreachable.
9. **`hs_thread_evaluate_step`'s `first` argument carries stack garbage in its upper three bytes.**
   The byte is spilled to `[esp+0x14]` and re-loaded as a DWORD (`0x48a48c`) before being pushed as
   the evaluate handler's third argument. Modeled as a plain `char`.
10. **`hs_rebuild_source` builds `"data\<scenario tag directory>\scripts"`,** not `"data\scripts"`:
    the tag path comes from `tag_instances[global_scenario_index].path`. The first pass dropped
    that argument. The half of the function that concatenates each matched `.hsc` file's contents
    is still not visible anywhere in the disassembly — see gap #4.

## Functions and rewrite confidence

`name` is confidence in the symbol name, `rw` is confidence in the C rewrite, `U` counts `UNSURE`
markers in the file. Names and confidences are mirrored into
`symbols/agent_phase4_hs.txt` (60 rows, the ones that differ from what `symbols/functions.txt`
already had).

| Address | Function | Bytes | name | rw | U |
|---|---|---|---|---|---|
| `0x482b20` | `sv_rcon_password_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x482b70` | `sv_maxplayers_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x482cb0` | `sv_ban_penalty_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x482d00` | `sv_tk_grace_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x482d50` | `sv_tk_cooldown_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x482da0` | `sv_banlist_file_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x482ed0` | `sv_name_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x482f20` | `sv_password_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x482f70` | `sv_friendly_fire_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x482fc0` | `sv_timelimit_evaluate` | 77 | 0.70 | 0.90 | 0 |
| `0x483010` | `map_list_matching_substring_evaluate` | 78 | 0.70 | 0.90 | 0 |
| `0x483060` | `game_variant_list_matching_substring_evaluate` | 78 | 0.70 | 0.90 | 0 |
| `0x4830b0` | `sv_single_flag_force_reset_evaluate` | 78 | 0.70 | 0.90 | 0 |
| `0x483100` | `hs_allocate_script_node_table` | 139 | 0.80 | 0.85 | 0 |
| `0x483190` | `hs_scripts_compile_and_link` | 179 | 0.80 | 0.75 | 1 |
| `0x483250` | `hs_scripts_reload` | 91 | 0.80 | 0.80 | 0 |
| `0x4832b0` | `hs_scripts_free` | 87 | 0.80 | 0.80 | 0 |
| `0x483310` | `hs_syntax_node_garbage_collect` | 128 | 0.80 | 0.75 | 1 |
| `0x4833a0` | `hs_script_find_by_name` | 120 | 0.80 | 0.85 | 0 |
| `0x483420` | `hs_global_get_type` | 42 | 0.60 | 0.70 | 1 |
| `0x483430` | `hs_null_with_params_evaluate` | 5 | 0.60 | 0.75 | 3 |
| `0x483450` | `hs_global_get_name` | 42 | 0.85 | 0.85 | 0 |
| `0x483480` | `hs_find_global_by_name` | 145 | 0.80 | 0.80 | 0 |
| `0x483520` | `hs_find_function_by_name` | 144 | 0.90 | 0.80 | 0 |
| `0x4835b0` | `hs_gametype_flag_satisfied` | 69 | 0.75 | 0.70 | 0 |
| `0x483600` | `hs_gametype_flags_applicable` | 140 | 0.75 | 0.70 | 0 |
| `0x483690` | `hs_autocomplete_test_candidate` | 81 | 0.40 | 0.75 | 0 |
| `0x4836f0` | `hs_autocomplete_scan_candidates` | 119 | 0.40 | 0.70 | 0 |
| `0x483770` | `hs_autocomplete_scan_globals` | 197 | 0.50 | 0.45 | 2 |
| `0x483840` | `hs_enumerate_special_form_names` | 23 | 0.55 | 0.85 | 0 |
| `0x483860` | `hs_autocomplete_add_startup` | 19 | 0.50 | 0.55 | 1 |
| `0x483c90` | `hs_autocomplete_gather` | 137 | 0.50 | 0.70 | 0 |
| `0x483e20` | `hs_rebuild_source` | 288 | 0.70 | 0.75 | 4 |
| `0x484090` | `hs_compile_source` | 279 | 0.80 | 0.75 | 1 |
| `0x4841b0` | `hs_help_print_function` | 183 | 0.50 | 0.60 | 2 |
| `0x484270` | `hs_doc` | 136 | 0.80 | 0.80 | 0 |
| `0x484300` | `hs_format_function_signature` | 251 | 0.50 | 0.75 | 0 |
| `0x484400` | `hs_compile_and_evaluate` | 502 | 0.85 | 0.55 | 1 |
| `0x484780` | `hs_parse_if` | 352 | 0.50 | 0.60 | 0 |
| `0x4848f0` | `hs_parse_cond_recursive` | 581 | 0.90 | 0.85 | 4 |
| `0x484be0` | `hs_parse_set` | 451 | 0.90 | 0.55 | 1 |
| `0x484fb0` | `hs_get_parameter_indices` | 161 | 0.90 | 0.75 | 0 |
| `0x485060` | `hs_parse_two_numeric_arguments` | 236 | 0.30 | 0.55 | 1 |
| `0x485150` | `hs_parse_two_object_arguments` | 291 | 0.30 | 0.50 | 1 |
| `0x485540` | `hs_compile_expression` | 430 | 0.50 | 0.55 | 1 |
| `0x4856f0` | `hs_source_buffer_append` | 125 | 0.50 | 0.70 | 0 |
| `0x485770` | `hs_compile` | 295 | 0.90 | 0.75 | 1 |
| `0x4858a0` | `hs_verify_source_offset` | 27 | 0.80 | 0.85 | 0 |
| `0x4858c0` | `hs_compile_postprocess` | 672 | 0.90 | 0.55 | 3 |
| `0x485b60` | `hs_add_global` | 484 | 0.90 | 0.85 | 2 |
| `0x485d50` | `hs_add_script` | 964 | 0.90 | 0.85 | 2 |
| `0x486120` | `hs_tokenize` | 130 | 0.80 | 0.80 | 0 |
| `0x4861b0` | `hs_tokenize_primitive` | 212 | 0.90 | 0.70 | 0 |
| `0x486290` | `hs_tokenize_nonprimitive` | 190 | 0.90 | 0.65 | 1 |
| `0x486350` | `skip_whitespace` | 193 | 0.90 | 0.75 | 0 |
| `0x486420` | `hs_parse` | 83 | 0.60 | 0.80 | 0 |
| `0x486480` | `hs_parse_primitive` | 218 | 0.90 | 0.65 | 0 |
| `0x486560` | `hs_parse_variable` | 285 | 0.90 | 0.55 | 1 |
| `0x486680` | `hs_resolve_identifier_as_function_or_script` | 132 | 0.50 | 0.60 | 1 |
| `0x486710` | `hs_parse_nonprimitive` | 768 | 0.90 | 0.55 | 2 |
| `0x486ae0` | `hs_parse_real` | 155 | 0.90 | 0.85 | 0 |
| `0x486ce0` | `hs_parse_tag_reference` | 216 | 0.75 | 0.70 | 2 |
| `0x486dc0` | `hs_report_expected_enum_values` | 460 | 0.55 | 0.70 | 1 |
| `0x486f90` | `hs_parse_scenario_datum` | 149 | 0.85 | 0.75 | 1 |
| `0x487030` | `hs_parse_trigger_volume` | 36 | 0.85 | 0.85 | 0 |
| `0x487230` | `hs_parse_object_name` | 191 | 0.90 | 0.75 | 0 |
| `0x487630` | `hs_object_list_collect_player_units` | 279 | 0.30 | 0.60 | 3 |
| `0x487750` | `hs_reposition_players_outside_trigger_volume` | 200 | 0.35 | 0.45 | 2 |
| `0x487820` | `hs_object_list_test_trigger_volume` | 207 | 0.30 | 0.55 | 3 |
| `0x4878f0` | `hs_object_angle_predicate_helper` | 180 | 0.35 | 0.40 | 3 |
| `0x4879b0` | `hs_object_list_any_angle_match` | 274 | 0.30 | 0.45 | 0 |
| `0x487ad0` | `hs_object_list_any_angle_match_gated` | 308 | 0.25 | 0.40 | 1 |
| `0x487c10` | `hs_object_hierarchy_test` | 218 | 0.30 | 0.45 | 2 |
| `0x487d20` | `hs_object_name_cache_validate` | 98 | 0.25 | 0.50 | 3 |
| `0x487dd0` | `hs_object_runtime_cleanup` | 275 | 0.30 | 0.40 | 2 |
| `0x487ef0` | `hs_object_names_for_each` | 79 | 0.50 | 0.70 | 1 |
| `0x487f50` | `hs_object_detach_and_place_at_location` | 1568 | 0.35 | 0.35 | 15 |
| `0x488570` | `object_list_nth_reference` | 136 | 0.35 | 0.60 | 0 |
| `0x488600` | `hs_object_set_health_fraction` | 98 | 0.40 | 0.65 | 0 |
| `0x488670` | `hs_object_set_permutation_by_name` | 195 | 0.40 | 0.50 | 1 |
| `0x488740` | `hs_object_list_for_each` | 142 | 0.30 | 0.60 | 0 |
| `0x4887d0` | `hs_objects_delete_by_type` | 152 | 0.55 | 0.40 | 2 |
| `0x488870` | `hs_effect_spawn_at_location` | 113 | 0.40 | 0.40 | 4 |
| `0x4888f0` | `hs_effect_spawn_on_marker` | 97 | 0.30 | 0.30 | 1 |
| `0x488960` | `hs_damage_apply_at_location` | 220 | 0.30 | 0.40 | 3 |
| `0x488a40` | `hs_damage_apply_with_sound` | 198 | 0.40 | 0.40 | 3 |
| `0x488b10` | `hs_sound_get_gain_reference` | 115 | 0.85 | 0.70 | 1 |
| `0x488c60` | `hs_evaluate_random` | 508 | 0.55 | 0.80 | 3 |
| `0x489120` | `hs_evaluate_boolean_and_or` | 296 | 0.50 | 0.85 | 1 |
| `0x489250` | `hs_evaluate_arithmetic_reduce` | 373 | 0.55 | 0.85 | 1 |
| `0x489800` | `hs_evaluate_sleep` | 542 | 0.50 | 0.90 | 2 |
| `0x489d50` | `hs_evaluate_argument_list` | 278 | 0.35 | 0.85 | 1 |
| `0x489e70` | `hs_runtime_initialize` | 115 | 0.85 | 0.60 | 2 |
| `0x489ef0` | `hs_scenario_scripts_initialize` | 576 | 0.55 | 0.60 | 5 |
| `0x48a130` | `hs_dispose_dynamic_globals` | 110 | 0.40 | 0.55 | 1 |
| `0x48a1a0` | `hs_runtime_update` | 166 | 0.50 | 0.60 | 1 |
| `0x48a250` | `hs_evaluate_expression` | 124 | 0.60 | 0.85 | 2 |
| `0x48a2d0` | `hs_call_script_by_name` | 31 | 0.50 | 0.60 | 1 |
| `0x48a2f0` | `hs_thread_new` | 127 | 0.55 | 0.70 | 0 |
| `0x48a370` | `hs_thread_evaluate_step` | 480 | 0.50 | 0.85 | 3 |
| `0x48a560` | `hs_thread_push` | 212 | 0.85 | 0.90 | 1 |
| `0x48a640` | `hs_thread_return` | 212 | 0.85 | 0.90 | 3 |
| `0x48a720` | `hs_global_get_value` | 74 | 0.75 | 0.70 | 0 |
| `0x48a770` | `hs_thread_pop_frame` | 31 | 0.75 | 0.85 | 0 |
| `0x48a790` | `hs_thread_restart` | 192 | 0.60 | 0.85 | 1 |
| `0x48a850` | `hs_evaluate_typed_arguments` | 265 | 0.45 | 0.85 | 2 |
| `0x48a960` | `hs_thread_find_by_script_index` | 138 | 0.45 | 0.70 | 0 |
| `0x48a9f0` | `hs_thread_find_by_script_name` | 181 | 0.50 | 0.65 | 0 |
| `0x48aaf0` | `hs_string_is_empty` | 31 | 0.70 | 0.85 | 0 |
| `0x48ac10` | `hs_object_list_new_singleton` | 75 | 0.35 | 0.60 | 0 |
| `0x48ac60` | `hs_type_mask_is_subset` | 38 | 0.50 | 0.75 | 0 |
| `0x48ac90` | `hs_types_are_compatible` | 125 | 0.60 | 0.55 | 1 |
| `0x48ad10` | `hs_coerce_value` | 75 | 0.40 | 0.85 | 2 |
| `0x48ad60` | `hs_evaluate_variadic_arguments` | 338 | 0.50 | 0.60 | 2 |
| `0x48aec0` | `hs_global_read_value` | 202 | 0.50 | 0.60 | 0 |
| `0x48b030` | `hs_global_write_value` | 124 | 0.50 | 0.60 | 0 |
| `0x48b220` | `object_list_reference_chain_delete` | 40 | 0.60 | 0.85 | 0 |
| `0x48b250` | `object_lists_initialize` | 80 | 0.80 | 0.85 | 0 |
| `0x48b2a0` | `object_list_reference_add` | 76 | 0.60 | 0.80 | 0 |
| `0x48b2f0` | `object_list_get_first` | 70 | 0.50 | 0.70 | 0 |
| `0x48b340` | `object_lists_dispose_empty` | 152 | 0.55 | 0.75 | 0 |

After the Opus review pass: gate clean at **121 ok / 0 failed**, mean rewrite confidence
**0.70**, **13** files still below 0.50 and **127** `UNSURE` markers left (down from 209).
All 13 of the sub-0.50 files are the objects / units / game / effects glue blocked on gap #5,
plus `hs_autocomplete_scan_globals` (`0x483770`), whose caller-side table description is not
recovered. The compiler and runtime cores were re-derived from the retail instruction bytes
and sit at 0.85 or above.
