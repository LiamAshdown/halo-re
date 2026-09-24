# hs module: type recovery notes

Header: `types/hs.h`. Smoke test: `out/phase4/hs_smoke.c`
(`C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/hs_smoke.c`, clean, and clean
again with `-std=c99 -Wall -Wextra`). Every struct size was also checked numerically against
its 32-bit target size plus 4 per pointer.

Sources: `out/phase4/hs_functions.md`, `out/phase2/hs/00.md`, `out/phase2/hs/01.md`,
`out/phase2/hs/02.md`, raw instruction bytes and direct reads of `bin/halo.exe` .rdata/.data
for every constant table (PE VA -> file offset: .rdata 0x63a000 -> 0x23a000,
.data 0x676000 -> 0x276000), and the scenario tag blocks already in `types/tags.h`.
Everything below is evidence unless it says "unresolved" or "guess".

## Scenario offsets used throughout (all confirmed against `types/tags.h`)

Compiled from `offsetof` on the existing `Scenario`:

| offset | field | how the module uses it |
|---|---|---|
| 0x204 / 0x208 | `object_names` count / pointer, stride 0x24 | `hs_parse_object_name`, `hs_object_names_for_each` |
| 0x474 | `script_syntax_data.size` | stamped with 0x5ccac by `hs_allocate_script_node_table` |
| 0x480 | `script_syntax_data.pointer` | the live `data_array *` is hot-swapped in here |
| 0x488 / 0x494 | `script_string_data.size` / `.pointer` | `hs_compiled_source` aliases it during postprocess |
| 0x49c / 0x4a0 | `scripts` count / pointer, stride 0x5c | `hs_script_find_by_name`, `hs_thread_find_by_script_name` |
| 0x4a8 / 0x4ac | `globals` count / pointer, stride 0x5c | `chimera__get_global_index`, `hs_global_get_name` |
| 0x4b4 / 0x4b8 | `references` count / pointer, stride 0x28 | `hs_parse_tag_reference` (0x486ce0) |
| 0x4c0 / 0x4c4 | `source_files` count / pointer, stride 0x34 | `hs_compile_source` |
| 0x4e4 / 0x4e8 | `cutscene_flags` count / pointer, stride 0x5c | 0x488870, 0x488960 |

`0x5ccac` is `0x38 + 0x4a39 * 0x14`, which independently fixes both the syntax node size
(0x14) and the table capacity (0x4a39) that `data_new("script node", 0x4a39)` allocates.

## hs_syntax_node (0x14)

Every field comes from `* 0x14` stride arithmetic:

- `+0x00` identifier: the datum header; `hs_syntax_node_garbage_collect` and
  `hs_compile_postprocess` both walk the array reading `*(short *)(data + index * size)`.
- `+0x02` index_union: `hs_tokenize` seeds it with 0xffff; `hs_parse` copies the expected type
  into it for primitives; `hs_resolve_identifier_as_function_or_script` writes the function or
  script index; `hs_compile_postprocess` re-derives it with `hs_find_function_by_name`.
- `+0x04` type: `hs_parse` writes the expected type here when it is still 0;
  `hs_parse_if`/`hs_parse_set` back-propagate a resolved type into it.
- `+0x06` flags: `& 1` primitive (`hs_tokenize` sets it from `*cursor != '('`), `& 2` script
  call (`hs_resolve_identifier_as_function_or_script`), `& 4` global reference
  (`hs_parse_variable` ORs it in), `& 8` garbage collectable (the only bit
  `hs_syntax_node_garbage_collect` keeps). Matches `ScenarioScriptNodeFlags` in `types/tags.h`.
- `+0x08` next_node: `hs_tokenize_nonprimitive` threads the child list through it and
  `hs_get_parameter_indices` walks it.
- `+0x0c` source_offset: `hs_tokenize_primitive` writes `cursor - hs_compiled_source`;
  `hs_verify_source_offset` bounds checks it against `hs_compiled_source_length`.
- `+0x10` data: `hs_parse_real` stores a float, `hs_parse_variable` stores the packed global
  reference, `hs_parse_object_name` stores an int16 index, `hs_parse_tag_reference` stores the
  TagID from `ScenarioReference+0x24`, and every non-primitive stores the first child index.

## hs_function_definition (0x1c + 2 * parameter_count)

`hs_function_definitions` at `0x00688b58` is 0x20a pointers (the loop bounds in both
`hs_find_function_by_name` and `hs_doc` are 0x20a, and `0x688b58 + 0x20a*4 == 0x00689380`,
exactly where the autocomplete procedure table starts -- the tables are adjacent).

- `+0x00` return_type: `hs_parse_nonprimitive` reads `*psVar4` (the first int16) as the return
  type and compares it with `hs_types_are_compatible`; `hs_thread_return` reads the same field.
- `+0x02`: zero in all 0x20a records (swept in the image).
- `+0x04` name: `hs_format_function_signature` sprintf's it first; `hs_find_function_by_name`
  compares against `*(char **)(defn + 4)`.
- `+0x08` parse: `hs_parse_nonprimitive` tail-calls `(**(code **)(psVar4 + 4))(index, node)`,
  which is byte offset 8. Sanity check against the image: `if` -> 0x484780 (`hs_parse_if`),
  `set` -> 0x484be0 (`hs_parse_set`), `cond` -> 0x484b40, `and`/`or` -> 0x484db0,
  `+`/`-`/`*`/`/`/`min`/`max` -> 0x484ea0 (`FUN_00485060`/`FUN_00485150` are sibling parse
  callbacks in this family), ordinary functions -> 0x487440.
- `+0x0c` evaluate: `hs_thread_evaluate` calls `(**(code **)(defn + 0xc))(index, thread, first)`.
  Image check: `begin_random` -> 0x488c60 (`hs_evaluate_random`), `and`/`or` -> 0x489120,
  `+` -> 0x489250, `sleep_until` -> 0x489800. `cond` has a NULL evaluate because it is
  desugared into nested `if` nodes at parse time by `hs_parse_cond_recursive`.
- `+0x10` info and `+0x14` param_info: `hs_doc` prints `*(char **)(defn + 0x10)`;
  `hs_format_function_signature` uses `+0x14` when non-NULL and otherwise formats
  `parameters[]`. 480 of the 522 records have a NULL param_info.
- `+0x18` gametype_flags: only five distinct values across the table (0, 0x5f, 0x4f, 0x1d,
  0x49), the same alphabet as `hs_global_definition+0x0c`, and the exact shape
  `hs_gametype_flag_satisfied` (0x4835b0) tests bit by bit for bits 0..6.
- `+0x1a` parameter_count and `+0x1c` parameters: `hs_format_function_signature` indexes
  `*(short *)(defn + index*2 + 0x1c)` for `index < *(short *)(defn + 0x1a)`. Record stride in
  `.rdata` is `0x1c + 2*parameter_count` rounded to 4 (e.g. `print` with one `string`
  parameter is 0x1e, `object_set_permutation` with three is 0x22).

## hs_global_definition (0x10)

491 (0x1eb) pointers at `0x0068b398`; `chimera__get_global_index` loops to 0x1eb and
`hs_runtime_initialize` reserves exactly 0x1eb runtime slots.

- `+0x00` name, `+0x04` type, `+0x06` pad, `+0x08` address, `+0x0c` gametype_flags, from a
  sweep of all 491 records: type is only ever 5/6/7/8 (boolean, real, short, long), pad is
  always 0, 102 records have a non-NULL address, six have a nonzero `+0x0c`.
- `hs_global_read_value` / `hs_global_write_value` switch on `*(short *)(defn + 4)` and
  dereference `*(void **)(defn + 8)` at the width the case implies, which is what proves the
  two fields independently of the sweep.

## hs_global (0x08)

`hs_global_get_value` reads `*(int *)(data + 4 + index * 8)`, and both
`hs_global_read_value`/`hs_global_write_value` use the same `(index & 0x7fff) * 8 + 4`
address as the runtime storage. `hs_runtime_initialize` allocates 0x400 slots and calls
`datum_new_at_index_with_salt` 0x1eb times; `hs_scenario_scripts_initialize` places scenario
global `i` at slot `0x1eb + i` and `hs_dispose_dynamic_globals` (0x48a130) deletes everything
from 0x1eb upward. `+0x02` is never touched anywhere in the module -- unresolved.

## hs_thread (0x218) and hs_stack_frame (0x10 header)

Stride 0x218 appears in fifteen functions. Fields:

- `+0x02` type and `+0x03` flags: `hs_thread_new` takes the type as a stack argument and zeroes
  the flags; `hs_runtime_update` treats type 2 as "a command thread is pending";
  `hs_evaluate_expression` (0x48a250) creates type 2 and `hs_thread_evaluate` deletes a type-2
  thread when its stack empties. Type 0 threads index `Scenario::scripts`; the transient
  global-initializer thread in `hs_scenario_scripts_initialize` is type 1.
- `+0x04` script_index: `hs_thread_find_by_script_index` (0x48a960) compares it against a
  script index, `hs_thread_find_by_script_name` uses it to index `Scenario::scripts` at
  stride 0x5c.
- `+0x08` wake_tick: `hs_runtime_update` runs a thread when `0 <= wake_tick <= current tick`;
  `hs_evaluate_sleep` sets it to `tick + max(ticks,1)`; `hs_thread_new` sets -2 for a dormant
  script and 0 otherwise; `hs_thread_evaluate` parks a finished startup/dormant script at -1.
- `+0x0c` saved_wake_tick: `hs_thread_restart` (0x48a790) copies it back into `+8` when
  flag bit 1 is set, then clears the bit.
- `+0x10` stack: `hs_thread_new` sets it to `thread + 0x18`, and `hs_thread_evaluate` compares
  it against `thread + 0x18` (`lea ecx,[esi+0x18]; cmp eax,ecx` at 0x48a3c0) to detect an idle
  thread, which fixes the stack area at `+0x18` and therefore the 0x200-byte size.
- `+0x14` result: raw bytes at 0x48a293 are `lea ebx,[esi+0x14]` immediately before the call to
  `hs_thread_push`, i.e. the root frame's result address, and 0x48a2bb reads `mov eax,[ebx]`.
- frame layout from `hs_thread_push` (0x48a560): `new = frame + 0x10 + frame->size`,
  `new->previous = frame`, `new->size = 0`, `new->syntax_node = node`, and the *outgoing*
  frame gets `frame->result_address = EBX`. `hs_thread_pop_frame` (0x48a770) does
  `thread->stack = *thread->stack`, and `hs_thread_return` (0x48a640) writes through
  `**(void ***)(*thread->stack + 8)`. Scratch is handed out at `frame + 0x0e + frame->size`,
  so the two bytes between the end of the scratch and the next frame are slack.

## Evaluate-handler scratch layouts

These are not allocated objects; each is a fixed sequence of
`p = frame + 0x0e + frame->size; frame->size += n` in the order the handler performs them.
Sizes are the sum of the bumps: variadic collector 0x8a (4 + 0x80 + 2 + 4), the 0x489d50
collector 0x88 (4 + 4 + 0x80), random 0x0a (2 + 4 + 4, so at most 64 alternatives can be
tracked in the two words of "already chosen" bits), and/or 0x09 (4 + 4 + 1), arithmetic 0x0e
(2 + 4 + 4 + 4), sleep 0x12 (4 + 4 + 4 + 4 + 2), typed argument list `4*n + 6`.
Field *names* inside these are inferred from use, not from any table -- treat them as the
weakest part of the header.

## object_list_header / object_list_reference (0x0c each)

`object_lists_initialize` creates both arrays. Node layout is exact from the raw bytes of
`object_list_reference_add` (0x48b2a0):

```
mov [ecx+4],edx        ; node->object_index = value
mov edx,[esi+8]        ; header->first_reference
mov [ecx+8],edx        ; node->next = that
mov [esi+8],eax        ; header->first_reference = node
inc word ptr [esi+6]   ; header->count++
```

The one field that needed reconciling is `+0x04`. `object_lists_dispose_empty` (0x48b340) tests
`cmp word ptr [ecx+eax*4+4],0` -- a *different* int16 from the one `reference_add` increments.
`hs_scenario_scripts_initialize` is the writer: for every scenario global of type
`_hs_type_object_list` it does `*(short *)(headers->data + 4 + index*0xc) += 1`. So `+0x04` is
a holder count (how many hs globals point at this list) and `dispose_empty` sweeps lists that
nothing refers to any more, not lists that are merely empty. `+0x02` on both records is never
touched -- unresolved.

## .rdata and .data tables (all read directly out of bin/halo.exe)

- `hs_type_names[0x31]` at `0x00688a78`: the 49 strings are in exactly `ScenarioScriptValueType`
  order (`unparsed`, `special form`, ... `scenery_name`), which is what pins `hs_type`.
- `hs_script_type_names[5]` at `0x00688b3c`: startup, dormant, continuous, static, stub.
  `0x00688b50` is a pointer to the shared `""` at `0x0065512c`, then the function table starts.
- `hs_parse_primitive_procedures[0x31]` at `0x0065b668`, indexed by type: slots 0..4 NULL,
  5 boolean, 6 `hs_parse_real`, 7/8 share a short/long parser, 9 string, 10 script,
  0x0b `0x00487030`, 0x0c..0x17 one parser each, 0x18..0x1f all `0x00486ce0`,
  0x20..0x24 all `0x00486dc0`, 0x25..0x2a all `0x004872f0`, 0x2b..0x30 all
  `hs_parse_object_name`. The word immediately after the table is 0x1eb, which is a nice
  independent confirmation that the table is exactly 0x31 entries long.
- `hs_type_conversion_procedures` at `0x0068bc10`: `0x31 * 0x31` function pointers indexed
  `[destination][source]` (`(dest * 0x31 + src) * 4` in `hs_types_are_compatible`, 0x48ad10 and
  `hs_thread_return`). 70 of the 2401 slots are non-NULL, rows 4..23 and columns 5..48, and row
  4 (void) is fully populated, which is what identifies the row as the destination.
- `hs_enum_definitions` at `0x0065b634`: five `{int32 count; char **names}` records --
  (4, easy/normal/hard/impossible), (10, default/player/human/covenant/flood/sentinel/unused6..9),
  (12, ai default states), (16, actor types), (5, top_left/top_right/bottom_left/bottom_right/center).
  These are types 0x20..0x24, and `hs_parse_enum` indexes `base + type*8` with the -0x20 bias
  folded into the base address, so Ghidra shows the base as `PTR_LAB_0065b538`. Do not index
  the array below 0x20: the preceding bytes are `hs_function_definition` records.
- `hs_object_type_masks[6]` at `0x00657538` = `ffff 0003 0002 0004 0380 0040`.
  `hs_parse_object_name` reaches it as `0x006574e2 + type*2` (types 0x2b..0x30) and
  `hs_type_mask_is_subset` as `0x00657538 + biased*2`; the raw bytes of
  `hs_types_are_compatible` show `add ecx,-0x25` / `add eax,-0x25` (object family) and
  `add ecx,-0x2b` / `add eax,-0x2b` (name family) immediately before the tail call, which is
  what proves the two expressions are the same six-entry table.
- `hs_tag_group_for_type[8]` at `0x00657544`, types 0x18..0x1f:
  `snd!`, `effe`, `jpt!`, `lsnd`, `antr`, `actv`, `jpt!`, `obje`. `hs_parse_tag_reference`
  reaches it as `0x006574e4 + type*4` and compares against `ScenarioReference+0x18`.
- `hs_type_value_sizes[0x31]` at `0x00657568`: 0 for types 0..4, 1 for boolean, 4 for real,
  2 for short, 4 for long/string, then 2 or 4 per type. Placement is derived purely from the
  table's adjacency to the two tables above and from the sizes matching each type -- **no
  recovered function in this module reads it** (`0x00487440`, the generic function-call parse
  proc, is the likely reader and was not recovered). Treat the base address as a guess.
- `hs_space_characters` at `0x0065b660` = `' '`, tab; `hs_newline_characters` at `0x0065b664`
  = newline, carriage return. Both are the two-entry arrays `skip_whitespace` and
  `hs_tokenize_primitive` loop over.
- `hs_autocomplete_procedures[0x12]` at `0x00689380`: 18 entries, matching the `iVar1 = 0x12`
  loop in `chimera__autocomplete_gather`.

## Unresolved offsets

| where | what |
|---|---|
| `hs_global` `+0x02` | never read or written anywhere in the module |
| `object_list_header` `+0x02`, `object_list_reference` `+0x02` | same |
| `hs_function_definition` `+0x02` | zero in all 522 records; no reader found |
| globals `0x006b14c4`..`0x006b14cf` | 12 bytes between `hs_compiled_source` and `hs_syntax_data_dirty`, untouched by all 125 functions |
| globals `0x006b15e1`..`0x006b15e7` | 7 bytes between `hs_postprocessing` and `hs_runtime_active`, untouched |
| `hs_type_value_sizes` base | placement inferred from adjacency only, see above |
| scratch field names | the layouts are exact, the names are inferred from use |
| `hs_thread` type 1 | only used by the transient global-initializer thread; the engine name is a guess |

## Misattributed or misnamed functions

- **0x48a720 `hs_global_get_value_pointer`** returns the *value*, not a pointer:
  `return *(int *)(hs_globals_data->data + 4 + slot * 8)`. `hs_scenario_scripts_initialize`
  uses the result directly as an object-list datum handle. Should be `hs_global_get_value`.
- **0x483450 `hs_global_get_address`** returns the global's *name*: for a builtin it returns
  `*(char **)definition` (field `+0x00`, the name) and for a scenario global the base of the
  `ScenarioGlobal`, whose first field is the `TagString` name.
  `hs_parse_variable` passes the result straight into a `%s`. Should be `hs_global_get_name`.
- **0x487030 `hs_compile_and_evaluate`** is `hs_parse_trigger_volume`: it is slot 0x0b of
  `hs_parse_primitive_procedures` and forwards to `hs_parse_scenario_datum` with stride 0x60,
  which is `ScenarioTriggerVolume`. The real `hs_compile_and_evaluate` is **0x484400**, today
  named `chimera__execute_script` (it is the function carrying the `"(set %s)"` literal).
- **0x483f40 `hs_null_evaluate`** is not a function: it is a mid-body entry point inside
  `hs_rebuild_source` (0x483e20). The two decompiles are the same `.hsc` enumeration loop, and
  the bytes at 0x483f40 are `add edi,0x10c` followed at 0x483f48 by `jl 0x483f2f`, a backward
  branch into the caller -- no prologue, no frame of its own. No types are owed.
- **0x483420 `FUN_00483420`** is `hs_global_get_type` (mirror of 0x483450 returning `+0x04`).
- **0x483480 `chimera__get_global_index`** is `hs_find_global_by_name`; **0x483770**,
  **0x483690**, **0x4836f0**, **0x483840**, **0x483860**, **0x483c90** are the engine's
  `hs_enumerate_*` autocomplete family. The `chimera__` prefix is a Chimera symbol name, not
  a Bungie one.
- **0x483430 `hs_null_with_params_evaluate`** is five bytes: `mov ax,[eax+4]; ret`. It has no
  callers and `+0x04` is equally `hs_global_definition::type` and `hs_syntax_node::type`, so
  which struct it belongs to is undecidable from this module. No type claimed.
- **0x483d20** is a `qsort` comparator over the **files** module's 0x10c-byte `file_reference`
  record (it calls `path_append_component` and the two `file_reference` accessors at 0x556000
  and 0x5560d0). Its struct belongs to `files`, not here; only the 0x10c stride and the
  eight-entry limit are recorded in `hs.h`.
- **0x4875c0** is a generic "index of this string in a `char *` array" helper
  (`FUN_004875c0(count, table)`), used with `hs_script_type_names` and `hs_type_names`. It is
  library-ish and probably belongs to `cseries`/`text`; no hs-specific type.
- **0x48aaf0 `hs_string_is_single_char`** actually returns "the string is empty"
  (`strlen(s + 1) - ... == 0` reduces to `s[0] == 0` after the first character is skipped).
  Cosmetic; no type impact.
- **0x48ab80 `hs_object_orient`** is a 9-byte float thunk: `fld dword ptr [esp+4]` then
  `jmp 0x6391b4`, a CRT floating point helper. Nothing to do with objects or orientation.

### Functions whose structs belong to other modules (no types defined here)

`FUN_00487630`, `FUN_00487750`, `FUN_00487820`, `FUN_004878f0`, `FUN_004879b0`,
`FUN_00487ad0`, `FUN_00487c10`, `FUN_00487d20`, `FUN_00487dd0`, `FUN_00487f50`,
`FUN_00488570`, `FUN_00488600`, `FUN_00488670`, `FUN_00488740`, `hs_objects_delete_by_type`,
`FUN_00488870`, `FUN_004888f0`, `FUN_00488960`, `FUN_00488a40`, `hs_sound_get_gain_reference`
are script-function implementations. They read the `objects` module's object headers
(`0x008603b0`, stride 0x0c, object data pointer at `+0x08`; object fields `+0xb4` type,
`+0xdc`/`+0xe4` maximum/current health, `+0x114`/`+0x118`/`+0x11c` sibling/child/parent),
the `game` module's `players` array (`0x0087a480`, stride 0x200, unit handle at `+0x34`), the
0x54-byte damage request the `effects`/`damage` module owns, and `tag_instance` from
`types/cache.h`. Only the object-list containers, which `object_lists_initialize` creates
inside this module, are defined in `hs.h`.

### Functions referenced by the tables but not recovered

`hs_function_definitions` points at parse procedures at `0x00484b40` (cond),
`0x00484db0` (and/or), `0x00484ea0` (arithmetic), `0x00485280` (sleep), `0x00485310`
(sleep_until), `0x00485460` (inspect), `0x00487440` (generic call) and `0x00487530`, and
`hs_parse_primitive_procedures` points at `0x00486a10`, `0x00486b80`, `0x00486c50`,
`0x00486c80`, `0x00487060`..`0x00487200`, `0x004872f0`, `0x00487360`, `0x004873c0`,
`0x00487400`. None of these appear in `out/phase4/hs_functions.md`, so they were not exported
by Ghidra even though they sit inside the module's address range. `0x00487440` in particular
is the one function that would settle `hs_type_value_sizes`.
