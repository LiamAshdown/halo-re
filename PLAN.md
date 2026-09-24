# Halo CE (halo.exe 1.0.10) multi-agent decompilation plan

Target: halo.exe 01.00.10.0621, PE32 x86, MSVC 7.1, 6495 functions, ~540 named after Ghidra auto-analysis.
Workspace: C:\Users\Liam-\halo-re (Ghidra project HaloCE, out/halo_decompiled.c, out/functions.csv).

## 1. Goal and definition of done

Two tiers. Tier A is fully automatable with agents; Tier B has a long human tail.

| Tier | Deliverable | Done when |
|---|---|---|
| A. Annotated database + readable source | Every engine function named, typed, assigned to a module, and rewritten as clean C with recovered structs | 100% of non-library functions have name + signature + module; clean C compiles as a type-checked library against the headers |
| B. Runnable reimplementation | Cleaned functions verified by hooking them into the real game one at a time | Each function passes an in-game hook test; modules can be swapped wholesale |
| (Optional) C. Byte-matching decomp | Source that MSVC 7.1 compiles to the original bytes | objdiff-style per-function match, decomp.dev-style progress |

Recommendation: drive Tier A to completion with agents first, run Tier B verification per module as A lands, and treat C as a later decision once VS .NET 2003 is set up. Tier A is the prerequisite for both.

## 2. Ground rules that make parallel agents safe

- **Ghidra is a build artifact, not the source of truth.** The truth is plain text in git:
  - `symbols/functions.txt`   `0x004xxxxx name confidence source`
  - `symbols/globals.txt`     same, for DAT_ addresses
  - `types/*.h`               recovered structs and enums (Ghidra parses C headers directly)
  - `modules.json`            function address -> module assignment
  - `src/<module>/<name>.c`   clean C, one function per file, with an evidence header comment
- A script (`scripts/ApplySymbols.java`, to be written) applies those files to the Ghidra project headless, then re-runs the export. Agents never open the .gpr. Every agent output is a patch to text files, so parallel work merges with git.
- **Every rename carries evidence.** A name is accepted only with a source tag: `opensauce`, `chimera`, `cea-pdb`, `string:<text>`, `callgraph:<neighbor>`, or `inferred`. Inferred names below 0.7 confidence go to a review queue, not the symbol file.
- **Function ownership is exclusive per pass.** A batch is a contiguous slice of one module; two agents never touch the same address in the same pass.
- **Library code is fenced off first.** Functions that Ghidra's FID marked as MSVCRT, D3DX9, or libogg/vorbis, plus thunks, are tagged `lib` in modules.json and skipped everywhere. Expect this to remove 1000+ functions from the workload.

## 3. Symbol sources (do these before any agent touches a function)

| Source | What it gives | How to ingest | Legal status |
|---|---|---|---|
| OpenSauce, `OpenSauce/Halo1/Halo1_CE/Memory/1.10/*.inl` | Hundreds of named 1.10 function and global addresses via FUNC_PTR / ENGINE_PTR / ENGINE_DPTR macros. **Caveat found in Phase 0:** these target Custom Edition haloce.exe, and this repo targets retail halo.exe. Same source, same build day, irregular layout drift. Only addresses that land exactly on a retail function entry are applied (96 of 267, at 0.7); globals are hint-only; hook-site offsets are dropped. | tools/parse_opensauce.py + tools/merge_symbols.py | Community, clean |
| Chimera, `src/chimera/signature/` | Named byte-pattern signatures for 1.10 | Resolve each pattern against bin/halo.exe with a scanner script, emit address + name | GPLv3, clean |
| invader (SnowyMouse) | Complete tag and map format definitions | Generate types/tags.h from its JSON definitions | GPLv3, clean |
| halo-re/halo `kb.json` | Confirmed Xbox function/data declarations | Match to PC by string refs | Clean-room |
| surreptitiousresearch/halocea | Verbose Blam engine function names, struct layouts, enums from the 2011 CEA prototype PDB | Name list plus headers; match to PC functions by string refs and callee sets | Derived from a leaked prototype. Names only, never code. Your decision whether to use it; punpckhdq/halo does. |

Expected coverage after this phase: roughly 1500 to 2500 named functions, and the biggest structs (object, unit, player, tag header, game state) defined.

## 4. Phases

### Phase 0. Bootstrap (scripts, no LLM agents; Fable in this session)
1. Write `scripts/ApplySymbols.java` (read the text files, apply names, prototypes, and headers, re-export).
2. Pull the clean sources above, convert to symbols/ and types/, apply, re-export. (The interrupted Chimera/OpenSauce import is this step.)
3. Tag library functions via FID and thunk status into modules.json.
4. Write `scripts/ContextPack.java`: for one address, emit a self-contained markdown pack: decompiled C, referenced strings, callees with names, callers, globals touched, and top candidate names from the symbol lists by string overlap. This is the unit of input for every agent.

Done when: symbols applied, out/ re-exported, context packs generate for any address in under a second.

### Phase 1. Partition into modules (Opus 5, about 5 agents)
Input: full call graph (functions.csv plus callee edges), strings per function, symbols so far.
Task: cluster into Blam subsystems. The engine's own names are the module list: cache/tags, memory, math, objects, units, weapons, vehicles, effects, physics/collision, ai, game_engine, scenario/script (HSC), rasterizer, render, sound, input, network/game_server, ui/menus, saved games, cheats/console.
Method: seed each module with functions whose strings or bootstrap names identify them, then propagate along call edges; unresolved clusters get an Opus review pass.
Output: modules.json with a confidence per assignment.

Done when: every non-lib function has a module, and the 20 largest functions (for example 0x5c5129 at 21 KB, almost certainly a script or tag dispatch table) have been looked at by hand.

### Phase 2. Symbol matching (Sonnet 5 bulk, Opus 5 for the hard queue)
Batches of 40 to 60 functions per agent, sliced by module.
Per function the agent gets the context pack and must output: proposed name, signature (calling convention matters: this binary mixes cdecl, fastcall, and custom register-passing helpers), evidence tag, confidence.
Sonnet handles everything with a string or callgraph anchor. Anything below 0.7 goes to an Opus queue with the whole module's context.
Two rounds: after round 1 is applied and re-exported, callee names improve the decompiled C, so round 2 catches what round 1 could not.

Done when: fewer than 5% of non-lib functions remain FUN_.

Operating notes (as run from 2026-09-13):
- Budget: the user is on a 5x plan; sessions are capped at 5 agents. Batches are 60 functions; small tail batches are packed with the next module so no agent slot is wasted.
- Apply threshold lowered from 0.7 to 0.5 after session 1: agents were honest and conservative, and 0.5 to 0.69 names were as sound as the rest. Confidence and evidence stay in symbols/agent_phase2.txt so anything can be stripped later. Below 0.5 is parked in symbols/review_queue.txt for the second pass.
- Mechanical pass first (tools/mechanical_names.py): 180 names for zero agents from CEA string literals and Chimera entry signatures.
- Per session: workflow (5 Sonnet agents) -> tools/merge_phase2.py -> tools/merge_symbols.py -> headless ApplySymbols + exports -> tools/make_batches.py.
- Session log: 1 = math, memory, cache (204 fns, ~700K tokens). 2 = hs, objects 0-1 (245 fns, ~830K). 3 = objects 2-4, game 0-2 (312 fns, ~1.0M). 4 = game 3-7, units 0 (308 fns, ~1.07M). 5 = units 1-4, ai 0-1: first attempt lost to the usage limit at 19:20, relaunched 19:22, done (309 fns, ~1.06M). 6 = ai 2-6 (300 fns, ~0.97M). 7 = ai 7-8, items 0-1, physics 0 (260 fns, ~0.93M). 8 = physics 1, networking 0-3 (260 fns, ~0.88M). 9 = networking 4-7, interface 0 (274 fns, ~0.79M). 10 = interface 1-5 (300 fns, ~0.97M). 11 = interface 6, rasterizer 0-3 (244 fns, ~0.97M). Loop cancelled after session 11; new loop 2026-09-16. 12 = sound 0-2, effects 0-2 (258 fns, ~0.87M). 13 = saved_games 0-1, shell 0-1, render 0 (291 fns, ~0.84M). 14 = render 1, input 0-1, structures, camera, main (252 fns, ~0.80M). 15 = the 10 small modules (149 fns, ~0.55M). NAMING PASS 1 COMPLETE over all 85 batches. Memory pilot DONE 2026-09-16 (4 agents, ~1.0M tokens): types/memory.h (21 structs, 6 enums), src/memory/ 59 files + README, gcc gate 59/59 clean, Opus review found and fixed 2 behaviour bugs + 6 sign-extension slips in the Sonnet rewrites. Two zlib functions (0x4d3980, 0x4d3a30) were misattributed to memory and belong to lib. User re-armed the loop and said continue: default is module rewrites: math DONE (90 files, gate clean, review fixed 9 semantic errors incl. 3DNow-vs-SSE misname and periodic-function table math, ~1.3M tokens), cache: types done (13 structs; found data_file_read is a fall-through tail not a function, and data_iterator is 0x10 not 0x0c), cache DONE (49 files, gate clean, review fixed 8 call sites where Ghidra dropped register arguments, ~1.4M tokens incl. the lost leg). hs DONE (121 files + README; types/hs.h gained 7 foreign-module slices folded back from per-file TYPES-GAP copies; Opus review fixed 12 of the 13 hs_thread_push call sites, which had all been given frame->result_address instead of the handler's own scratch slot, plus hs_thread_return writing through the wrong frame, 3 wrong hs_thread_return values, the missing global-type lookup in hs_thread_push, a reference-vs-datum-index conflation in hs_scenario_scripts_initialize, and a re-derivation of hs_rebuild_source that Ghidra had cut at 288 of its real 616 bytes; 3 apparent contradictions resolved as genuine retail behaviour, not decompiler noise; 4 in-range functions are misattributed and were skipped). Repo gate: 319 files, all clean. Then objects (2 sessions), then naming pass 2. Hook harness (Phase 5) DEFERRED by user on 2026-09-16: do not build it until asked. Generic workflow: tools/workflows/module_rewrite.js with args from tools/phase4_prep.py <module>. From session 5 the /loop runs the cycle automatically, still 5 agents per session. ai MODULE REVIEW DONE (Opus, 2026-09-20): the two Sonnet rewrite batches left 34 in-range functions unwritten (the whole encounter/squad lifecycle, 0x433ea0..0x438580); this review wrote all 34, so src/ai is now 506 files covering 506 of the module's 519 addresses (13 are documented misattributions, incl. ai_communication_broadcast @0x42d340 which is deliberately deferred). types/ai.h gained encounter_iterator, ai_object_attention_record and ai_scored_candidate, absorbed 12 TYPES-GAP local typedefs (only 2 foreign-module return shims remain local), and had eight layout errors corrected: encounter_squad_state +0x00/+0x04 are the two starting-location masks (not int16 unknowns), +0x0c is respawn_budget and +0x12 squad_delay_ticks (the phase-2 'grenade cooldown' pair was off by one field), +0x1c / encounter+0x34 / encounter_platoon_state+0x0c are all float average_vitality, encounter+0x0e/+0x10 are the activation delay and tick, actor+0x0b is swarm_pending, prop+0x100 is cluster_index, swarm_component+0x02 is a flags byte. Four functions renamed for the same reason (encounter_new, encounter_activate/_deactivate, encounter_remove_actor from the 'squad_*' phase-2 names, plus the two spawn-delay decayers); 36 rename lines appended to symbols/agent_phase4_ai.txt. Extern declarations are now name-consistent module-wide (653 FUN_ references retargeted); 126 addresses still disagree on signature, all of them Ghidra operand-count losses. Two real behaviour bugs found and fixed in the earlier batches' output (current-vs-next actor confusion in the encounter member walk). Gate: ai 506/506, whole repo 2053/2053, ai_smoke extended with 18 new offset assertions. src/ai/README.md regenerated (1394 lines).

### Phase 3. Type recovery (Opus 5, one agent per module, sequential within a module)
Per module: define the structs behind the module's globals and the object pointers its functions take, in `types/<module>.h`, using invader definitions and community layouts as reference. Apply, re-export, check that field accesses in the decompiled C become named.
Three iterations of apply / re-export / review are normal; Ghidra output improves sharply as types land.

Done when: the module's functions decompile without raw `*(int *)(param + 0x1c)` accesses in hot paths.

### Phase 4. Clean-C rewrite (Sonnet 5 per function, Opus 5 per module review)
Per function: rewrite the Ghidra C into readable C against the recovered headers. Rules: preserve semantics exactly, no invented behavior, keep the Ghidra output in a trailing comment block for diffing, mark every uncertainty with `// UNSURE:`.
Gate: src/ must compile as a static library with a modern MSVC or clang (`/c`, warnings as errors on implicit declarations). This catches most hallucinated fields and signatures.
Opus review per module: consistency, naming, duplicated helpers, and a sample of 10% of functions re-read against the disassembly.

Done when: the module compiles and its review is signed off.

### Phase 5. Verification by hooking (Opus 5 for the harness, then Sonnet 5 per function)
Build a hook DLL (same mechanism Chimera uses) that, for a chosen function, patches a jump from the original address to the cleaned C compiled into the DLL. Run the game with a scripted scenario (load a30, spawn, fire, save) and compare against unhooked behavior and a log of function inputs and outputs.
Start with pure functions (math, tag lookup, string tables), then stateful leaf modules, then game_engine.

Done when: a module can be entirely replaced by the DLL with no visible behavior change.

### Phase 6 (optional). Byte matching
Requires Visual Studio .NET 2003 (MSVC 7.1) in a Windows XP VM or a compat install, plus an objdiff-style comparison tool. Only worth it if you want decomp.dev-style progress. Pursue per function on top of the Phase 4 source.

## 5. Agent roster and model choice

| Role | Model | Why |
|---|---|---|
| Orchestrator, scripts, merges, final review | Fable 5.1 (this session) | Holds the whole pipeline and the ground rules |
| Partitioning, type recovery, hard-queue matching, module reviews, hook harness | Opus 5 | Needs whole-module reasoning and structure recovery |
| Bulk per-function matching and clean-C rewrites | Sonnet 5 | Thousands of bounded, well-specified tasks |
| lib/thunk classification, CSV bookkeeping | Haiku 4.5 or plain scripts | Trivial |

Orchestration: the Workflow tool (a pipeline of agent() calls with JSON output schemas) fits this exactly, with the session default of about 15 concurrent agents. You opt in explicitly each run by saying "use a workflow" or "ultracode"; nothing launches on its own.

## 6. Budget estimate (order of magnitude)

| Item | Estimate |
|---|---|
| Non-lib functions | ~5000 |
| Input tokens per function per pass (context pack) | 4K to 8K |
| Output per function | ~1K |
| One full Sonnet pass | ~30M input, ~5M output |
| Passes needed (2 matching + 1 rewrite) | 3 |
| Opus work (partition, types, reviews, harness) | ~20M input total |
| Agent runs at 50 functions per batch | ~100 per pass |

## 7. Order of attack

1. Phase 0 and 1 in full.
2. Phases 2 to 5 module by module, in this order: math, memory, cache/tags, scenario/script, objects, units/weapons/vehicles, physics, effects, ai, game_engine, sound, input, network, ui, rasterizer/render last (Direct3D-heavy, least reusable).
3. Re-run Phase 2 globally once half the modules have types, because names propagate across module boundaries.

## 8. Risks

- **Hallucinated names and fields.** Mitigated by mandatory evidence tags, the compile gate, and the 10% disassembly re-read per module.
- **Parallel agents disagreeing on a shared struct.** Mitigated by exclusive per-module ownership of `types/<module>.h` and sequential type passes within a module.
- **CEA symbols come from leaked material.** Use them as naming hints only, tag them `cea-pdb` so they can be stripped, and never copy code.
- **Distribution.** halo.exe, the .map files, and the raw Ghidra export stay private. Publish only src/, types/, symbols/, and modules.json.

## 9. Immediate next actions

1. Phase 0 steps 1 to 4.
2. Decide on CEA symbol use.
3. Say "use a workflow" when you want Phase 1 launched.
- objects session 1 (2026-09-16): types/objects.h 26 structs; the widget type table read from the binary proved the "lightning" functions are the GLOW widget (14 renames). First-half rewrites 110 files gate-clean; review lost to the usage limit ~18:30, resumed 19:31. Session 2 = second half 0x4f6610..0x4ffda0 split 0x4fae10 with skipTypes.
- objects session 1 review done 2026-09-16 evening; session 2 (second half, skipTypes) launched.
- objects session 2 done 2026-09-16 (232 files in module, repo gate 551 clean, ~1.85M tokens). Rewritten so far: memory, math, cache, hs, objects = 551 files. Now: apply phase-4 renames to Ghidra, then naming pass 2 over the parked queue for the not-yet-rewritten modules.
- Naming pass 2 started 2026-09-16: 53 batches / 2284 functions over the 26 not-yet-rewritten modules (tools/make_batches_pass2.py, packs carry the parked pass-1 proposal). Session P2-1 = p2_ai 0-4. Same 5-agent workflow, module names prefixed p2_.
- Naming pass 2 STOPPED after one session (ai 0-4): 13 of 300 names cleared 0.5 for ~0.92M tokens. Decision 2026-09-16: the rewrite pipeline names these functions better as a by-product; continue module rewrites in dependency order instead: units (2 sessions), items+devices, physics, game (4), effects+structures, ai (4), networking (4), interface/saved_games/shell/main/input, then rasterizer/render/sound/rest. Parked queue gets a final short pass at the end.
- units session 1 (2026-09-16/17): types/units.h (unit 0x2d8, biped 0x84, vehicle 0xf4 extensions proved by the object_type_definition table; found the "unit_update" at 0x5590a0 is biped_update, real unit_update is 0x5625b0); rewrites A/B done (A left 3 large fns), review lost to the limit ~23:30, resumed 00:32 with a catch-up task. Session 2 = 0x566de0..0x575e30 split 0x56e280 skipTypes.
- units session 1 review done 2026-09-17 00:5x (85 files gate-clean); session 2 launched (0x566de0..0x575e30, skipTypes).
- units DONE 2026-09-17 (~230 files; ~3.8M tokens over both sessions). Next: items, then devices+physics.
- items session (2026-09-17): types/items.h (item/weapon/equipment/garbage sizes from the object_type_definition table); 22 functions in the items range are PROJECTILES (projectile_data overlaps item_data) and were skipped: follow-up = a "projectiles" module session over those 22 addresses with types/projectiles.h built from out/phase4/items_types_notes.md. Review lost to the limit ~04:30, resumed 05:32.
- items DONE 2026-09-17 ~06:10 (review complete). projectiles running; devices launched alongside.
- devices DONE 2026-09-17 (types agent: 4 functions in the range are recorded-animation playback, belong to cutscene; flagged objects.h offset 0xb8 as team index vs name_index conflict). physics launched while projectiles finishes.
- projectiles DONE 2026-09-17 (17 files; 5 functions Ghidra never split out; retail bug: 0x4c0310 air branch reads water_damage_range). TODO later: an Opus "types reconciliation" pass folding corrections owed to objects.h (0x18/0x1c interpolation block, 0xb8 team index, +0x04 fourcc in object_type_definition) and memory.h (data_iterator 0x10). game = 4 sessions.
- 2026-09-17 ~09:30 usage limit hit both physics (types done; 13 "antenna_*" fns are really object physics mass-point code) and game session 1 (types/game.h 31 structs; player 0x200 / team 0x40 strides from objdump; 0x006f1d20 is current_game_engine not game_is_server). Resumed physics 10:30; game s1 resumes after physics. Policy: one module session at a time from now on, the limit is the binding constraint.
- physics DONE 2026-09-17 (types agent: the 13 "antenna_*" functions are object mass-point physics; 0x500090 is an effects function). game s1 resumed.
- 2026-09-17 ~11:00 WEEKLY usage limit hit (reset Sep 18 23:00); game s1 rewrite/review lost; resumed 2026-09-18 23:02. Repo gate 1040 files clean (memory, math, cache, hs, objects, units, items, projectiles, devices, physics).
- game s1 DONE 2026-09-19 (rewriter rebuilt game_engine_build_sorted_player_list from objdump because Ghidra lost its jump table). s2 launched: 0x460890..0x467f70 split 0x464430.
- game s2 DONE 2026-09-19 (215 files in module; review fixed 8 player-stride sites). s3 launched: 0x468010..0x473560 split 0x46efe0.
- game s3 rewrites done 2026-09-19 (two Ghidra decompiler errors fixed from objdump: 0x46f1a0 lost its back half, 0x472020 misread network_client). Review lost to the limit ~03:00, resumed 04:01 (task 0: write 0x4710b0 and 0x471ae0).
- game s3 DONE 2026-09-19 ~04:45 (324/428; reviewer transcribed the two deferred control-input functions from objdump; 17 fixes). s4 launched: 0x4735b0..0x551d30 split 0x4776d0.
- game s4 first half done 2026-09-19 (0x4735b0..0x4776d0, 37 files; 0x476847 skipped as a
  duplicate tail fragment of 0x476760, not misattributed). Corrected two real Ghidra bugs from
  objdump: player_reset_after_unit_change's stray "+0x1fffe28" pointer (main_switch_structure_bsp
  actually indexes the current player, not player_data->data alone) and several zero-arg calls
  into already-named sibling functions (game_engine_resolve_player_team, player_respawn,
  game_engine_apply_player_grenade_counts, FUN_004e7b50) whose real arguments the disassembly
  still carries in registers. game_engine_players_update_server/_client (0x4740a0/0x474590) and
  players_client_catchup_on_server_updates (0x476d40) are the batch's least-verified files: Ghidra
  badly mis-tracks their ESP-relative stack locals (return addresses rendered as assignments,
    a shared 0x10-byte + 0x20-byte per-player pair merged into three overlapping pseudo-locals),
  and the catch-up file's inner unit_control_data build could not be fully disambiguated from the
  disassembly in this pass. 0x475270 (game_engine_reattach_player_unit_unused, callers=0, clearly
  superseded by 0x475c60) got a lower-rigor best-effort pass given it is dead code. Repo gate 1390
  files clean. Second half next: 0x4776d0(exclusive)..0x551d30.
- game s4 DONE + MODULE COMPLETE 2026-09-19. Second-half rewriter wrote 0x4776d0(exclusive)..0x551d30
  (62 files), skipping 0x47bf23 and 0x47c3d0 as mid-body fragments (both confirmed by objdump in the
  review). Review pass: 422 files for the module's 428 addresses, `build_check.py game` 422/0, repo
  1389/0 (one file fewer than s4's 1390 because a duplicate was removed). Fixes: deleted the
  duplicate 0x4776d0 file the two rewriters both wrote; renamed and retyped the whole
  0x461ad0/0x461c60/0x461d90/0x461e60 chain, which was named as damage scaling and is actually the
  player STARTING-LOCATION suitability scorer (0x4776d0 is 0x461d90's only caller and passes a
  ScenarioPlayerStartingLocation* in EAX); renamed 0x477670 position_history_reset_all_slots ->
  object_placement_data_set_change_colors (its ECX is the object_placement_data player_respawn just
  built and the four 3-float slots at +0x58 are Blam's change_colors[4], which also means
  types/objects.h's network_vectors[4] and src/objects' object_set_position_network are misnamed -
  left for that module); 7 behaviour bugs (main_switch_structure_bsp's invented outer goto skipped
  the per-player interaction reset AND the 0x478400/0x478500 dispatch, plus a missing nibble write;
  a half-cleared kill_streak dword; two AL-only returns declared 32-bit; game_engine_get_player_color
  dropped its return value and mis-indirected 0x686b10/0x686b18; koth_player_tick lost all three
  announcer sound indices; player_attach_unit_to_parent threw away the position it computes and had
  the sign inverted); 9 call sites where Ghidra dropped register arguments; every remaining local
  TYPES-GAP typedef folded into types/game.h (position_update_record, vehicle_update_body/_record,
  player_update_record, client_update_carry, win32_find_dataa, k_user_save_path_slot_stride);
  33 of 45 cross-file declaration disagreements resolved and 26 odd apostrophes removed from
  types/game.h comments (CParser rule). src/game/README.md rewritten for the whole module.
  NOT FIXED, needs hooks: 12 cross-module callee conventions (0x56d400, 0x56d990, 0x569bf0,
  0x56d6e0, 0x566970, 0x557950, 0x557990, 0x447740, 0x4e9cd0) where two rewrites read different
  registers; game_state_new still has a 2-arg view in src/hs; 0x47bef0 and 0x47c3a0 are real
  function entries MISSING from modules.json (it carries their mid-body fragments instead).
  Next: objects (2 sessions), then naming pass 2.
- game DONE 2026-09-19 (4 sessions, ~5.8M tokens, 423 files). effects launched.
- effects: types/effects.h done 2026-09-19 (24 structs; 7 "particle_system_*" names actually operate on the effect table; 15 fns 0x456730..0x457d50 are a player_effect subsystem); rewrites lost to the limit ~08:00, resumed 09:01.
- effects DONE 2026-09-19 (integration pass). 111 of 124 functions in src/effects/ + README.md; 13 in-range addresses are misattributed (9 math/structures/bitmaps at 0x44d820..0x44db30, 0x453330 players, 0x4588e0 math, 0x458990 render, 0x458b50 structures) and are still UNWRITTEN anywhere in src/. Gate: effects 111/111, whole tree 1500/1500, all 12 smoke files clean. types/effects.h gained 3 folded TYPES-GAP structs (decal_flood_vertex_record, decal_flood_accumulator, effect_marker_node_context) and 4 more were replaced by the types/tags.h ModelCollisionGeometryBSP* records they duplicated; no local typedefs left in src/effects. Opus review fixed 24 semantic defects across 12 files: an inverted must_be_deterministic_pc RNG selector and a goto aimed at the wrong one of two adjacent labels in effect_update; a single-instead-of-double RNG advance, a marker/node scale applied to the direction vector as well as the velocity, effect.velocity*30 added to the wrong vector, the colour selector read from a_scales_values instead of flags, and a missing ColorARGB alpha lerp in effect_spawn_particles; two `self + 0x10` pointer-stride bugs (effect+0x10 is &location, not 0x10*0xfc bytes past the record); effect_random_velocity_vector's param_3/param_4 proved to be OUTPUT pointers, which resolved the cross-file signature conflict the rewriter had spawned a follow-up task for (task_203b9b85 can be dropped); the 3 vectors effect_event_apply reads proved to be one contiguous 9-float caller block (up/forward/position, two in EAX/ECX); signed (int16_t) casts on 5 unsigned RNG draws plus two wrong axis pairings in player_effect_build_camera_shake_matrix; an inverted 0xffff node-index mask and a dropped context dword in the two effect_new wrappers; 0x00686b04 read as an inline vector array when it holds a pointer; and a NULL passed for a pointer decal_flood_surfaces dereferences. decal_flood_surfaces was rewritten from 0.2 to 0.45 confidence once its BSP tables resolved to tags.h types. 349 UNSURE markers remain; the 6 weakest functions are listed in src/effects/README.md. Names in symbols/agent_phase4_effects.txt (111 rows, 11 of them renames). structures next.
- effects DONE 2026-09-19 (review complete; one open conflict on effect_random_velocity_vector signature flagged for a disassembly check). structures launched.
- structures DONE 2026-09-19 (types agent: 0x53f150 is sound-environment interpolation, belongs to sound). ai s1 launched: types + 0x401090..0x4142d0 split 0x40cdf0; s2 0x414560..0x421af0/0x41abd0; s3 0x421bc0..0x431680/0x42acd0; s4 0x431d10..0x43ecf0/0x438580.
- ai s1 DONE 2026-09-19 (types/ai.h 24 structs from game_state_new sizes; 0x430830..0x431e70 is ai_conversation not squads; actor still ~55% unknown bytes). s2 launched.
- ai s2 DONE 2026-09-19 except 7 giant functions (0x416790, 0x4180c0, 0x4193d0, 0x41abd0, 0x41c8f0, 0x41d7e0, 0x420ec0) -> dedicated catch-up session via new onlyA/onlyB workflow args.
- ai catch-up: 6 of 7 giants written before the limit hit ~18:30; relaunched 19:03 for 0x41abd0 (+ re-check of 0x41c8f0) and the review.
- ai catch-up review DONE 2026-09-19 (Opus). Gate: ai 193/193, whole tree 1740/1740, ai_smoke clean.
  All 7 giants now have files; 0x416790 actor_movement_update was the one non-compiling file in the
  tree and was re-derived from objdump 0x416790..0x417390 (its EAX and ECX arguments to 0x4180c0 are
  actor+0x42e and a caller-local sidestep flag, NOT actor.unknown_50a / actor.flying as the earlier
  draft assumed; the flipped-vehicle recovery is a 3D normalize, not 2D; the carrying test reads
  actor.unit_index not unknown_0c; Actor.stationary_movement_dist replaced a stub callee). 24 other
  semantic/signature defects fixed across 20 files, the worst being two merged shared tails in
  actor_update_melee_combat_action (LAB_0040d32a pokes the encounter, LAB_0040d4a6 does not - three
  paths had gained a 0x10 broadcast and a counter bump), object_set_position_and_orientation given
  the from-point as its forward vector in actor_squad_action_execute, 0x00696718 read as inline data
  when it is a pointer to the {1,0,0} forward vector (actor_target_data_refresh), actor_should_throw_
  grenade returning int32 when the original returns in AL only, and four actor_movement_set_destination_
  point call sites missing their EAX destination. ai_communication_broadcast normalized to one 7-arg
  prototype (every call site cleans 0x1c); 0x006f1d6c, 0x00696714, 0x00696720, 0x00746f98/9c
  normalized; 5 duplicated TYPES-GAP typedefs folded into types/ai.h; types/ai.h gained named fields
  at actor 0x430..0x453 and 0x5dc..0x5eb; CParser apostrophe rule fixed. 7 rows appended to
  symbols/agent_phase4_ai.txt. src/ai/README.md updated (193 rows, new defects + open questions).
  NOT FIXED, needs hooks: 0x41bb30 takes EBX + SIX stack args (add esp,0x18 at all five call sites)
  but src/ai/actor_dispatch_look_handler_by_posture.c defines 3 params - that file is wrong end to
  end and carries a defect banner; actor+0x270 is still misnamed target_unit_index across ~20 files;
  actor+0x5a4/0x5b0/0x5bc are direction vectors, not position caches; 0x00746f9c is bsp_generation
  (int32) in 5 ai files and structure_bsp (ScenarioStructureBSP*) in 30 files elsewhere;
  actor_movement_test_obstacle_ray returns void per its own file but int16 per its only caller.
- ai catch-up DONE 2026-09-19 (193 files 0x401090..0x421af0, no holes; 0x41c8f0 rewritten from an esp-tracked disassembly, 6 confirmed bugs in the earlier draft). s3 launched: 0x421bc0..0x431680 split 0x42acd0.
- ai s3 DONE 2026-09-19 (329 files in module; conversation-system names corrected). s4 launched: 0x431d10..0x43ecf0 split 0x438580.
- ai s4 first launch 2026-09-19 ~22:00 died instantly on the session limit; relaunched 2026-09-20 00:00.
- ai s4 second half DONE 2026-09-20 (0x438580(exclusive)..0x43ecf0, 62 files, no holes;
  0x43b2f0/0x43c340/0x43c380/0x43c400 skipped as math-module misattributions per
  out/phase4/ai_types_notes.md). Gate: ai 472/472, whole tree 2019/2019. Drive-by fix: 6
  pre-existing files elsewhere in src/ai (ai_object_list_respawn_members/spawn_members/
  clear_orders_with_weapon/reset_or_wake_awareness/set_unit_flag_400/set_unit_flag_800) had a
  parameter named the same as the `object_list_header` type, which made every one of them fail
  to compile; renamed the parameter to object_list_header_handle in all six.
  Notable finds this half: path_find_set_avoid_sphere (0x43a070) turned out to write the
  already-named path_find_request.avoid_position/avoid_radius/avoid_weight/avoid_object_index
  fields, not an unnamed span, once cross-checked against path_find_score_avoidance_penalty
  (0x43b3b0) and path_find_run's own avoidance branch; path_find_gather_adjacent_edges'
  (0x43b1c0) output fields are floats copied bit-for-bit through an int-typed pointer, not
  truncated integers, confirmed because path_find_run (0x43a8b0) reads the same bytes back
  with real float arithmetic; path_find_hash_lookup_vertex (0x43b2b0) and
  ai_search_gather_obstacles's own FUN_0043c8f0-family sibling both decompiled as `void` but
  are read as returning a value by every caller, fixed to return explicitly.
  NOT FIXED, needs a disassembly-based follow-up: ai_search_choose_shorter_corner (0x43d240)
  and path_find_test_segment_unobstructed (0x43de90) each call another function 4-9 times
  with zero visible arguments per call and could not be reconstructed beyond a structural
  guess (rewrite confidence 0.05); path_find_run's own permission-bitmap test, and the whole
  path_find_trace_cluster_boundary*/path_find_trace_bsp_boundary family, reach into an
  un-established structure_bsp cluster-boundary layout (rewrite confidence ~0.1 throughout);
  ai_search_evaluate_edge_cost (0x43b830) and ai_search_expand_point_neighbors (0x43ba60) call
  each other and FUN_0043d790 with inconsistent argument counts across call sites that
  contradict each other. This closes out the ai module (519/519 functions across all four
  sessions).
- ai s4 DONE 2026-09-20 (reviewer wrote the 34 deferred encounter/squad functions). 9 math helpers in the ai range reassigned to math (need a small math catch-up later). ai catch-up launched for 0x42d340, 0x40b080, 0x417a30, 0x42c940.
- ai catch-up 0x42d340 DONE 2026-09-20: ai_communication_broadcast (5603 bytes, the module's
  largest function, previously deferred whole) rewritten to src/ai/ai_communication_broadcast.c
  from Ghidra pseudocode plus a manual `objdump -d 0x42d340..0x42e930` pass. The 0x38-byte
  candidate scratch record and the 0x28-byte `ai_communication_lines` row table (both local
  TYPES-GAP structs) are recovered from stack-offset arithmetic, not guesswork. Fixed 3 dropped/
  wrong register arguments in the tail dispatch (FUN_0042e9c0, FUN_0042eee0) and confirmed one
  conditional `ai_communication_record_line_played` call there is genuinely dead code (branch
  register unconditionally 1); fixed one argument-identity slip in the main scoring loop
  (FUN_0042ec90's 2nd arg is local_478, not local_47c per Ghidra). Rewrite confidence 0.3 -- see
  the file header and src/ai/README.md for the full UNSURE list (row-table field semantics,
  Phase-0 precompute loop trip count, FUN_004302e0's register convention). Gate: ai 510/510,
  whole tree 2057/2057. The other 3 addresses from this catch-up batch (0x40b080, 0x417a30,
  0x42c940) were not touched this session; 0x40b080 and 0x42c940 are misattributed
  (non-ai/dead code) per out/phase4/ai_types_notes.md and out/phase4/ai_functions.md.
- ai phase-4 REVIEW PASS DONE 2026-09-20. Gate: ai 510/510, whole tree 2057/2057. No in-range
  function was left unwritten by the catch-up rewriters (0x40b080, 0x417a30, 0x42c940, 0x42d340
  all landed), so 510 files now cover all 519 module addresses minus the 9 math/qsort
  misattributions. Folded the last 4 local typedefs into types/ai.h: ai_communication_line_
  definition and ai_communication_candidate (from ai_communication_broadcast.c), and one shared
  bool_float_return replacing the two identical (AL bool, ST0 float) records in
  actor_rate_potential_target.c and actor_target_hearing_check.c. Folding them exposed 3 layout
  bugs: the candidate record was missing pad bytes at +0x12 and +0x2a (every offset past +0x10
  was two bytes short under pack(1)); the line row had unknown_19[3] where +0x1a is read
  directly (DAT_00655aba), inflating the row to 0x2a; and swarm.unknown_08[8] hid the first
  float of the aggregate position, so actor_refresh_combat_context averaged 2 of 3 components
  (now swarm.aggregate_position at 0x0c). Spot-checked 8 functions line by line against
  tools/pack.py (0x417a30, 0x42c940, 0x4297a0, 0x424090, 0x405520, 0x40b080, 0x42d340,
  0x42c3e0) and fixed 7 more defects: actor_refresh_combat_context tested a vector component
  instead of vector2d_normalize_with_length's returned length (branch inverted for negative i)
  and read +0x14c off the wrong tag pointer; actor_evaluate_custom_charge_trigger had the
  random charge gate INVERTED and read the prop iterator from the wrong dword with no actor
  index; actor_squad_action_execute's whole atom_type==0x18 branch was fiction (compared
  iterator results to an Actor tag pointer, tested distance against 0.0 instead of a running
  minimum, used the object slot instead of the delay slot, read player records as props) and
  was rewritten; ai_reset_fire_group_assignments let a swarm actor with no swarm record fall
  into the re-link tail instead of advancing; ai_scan_for_recent_combat_activity used <= where
  the NAN-packed original is strictly <. Two mechanical sweeps came back clean (no narrow
  Ghidra return widened to 32-bit; no signed-char comparison dropped to unsigned). NOTE for
  future sweeps: sizeof under the gate is NOT a stride check for any struct holding a pointer,
  because the gate compiles 64-bit (object_header is 0x10 there, 0x0c in the binary).
  NOT FIXED, needs a trace: objdump shows actor_process_vehicle_seat_exit (0x40b080) performs
  three fsubs at 0x40b275/0x40b293/0x40b2b7 (rider_node0.position MINUS marker.position, a
  delta) plus one more fsub [esp+0x3c] at 0x40b379, none of which Ghidra or the file reproduce;
  the fadd [ebx+0x5c|0x60|0x64] at 0x40b353.. does confirm the out-point is
  model_node0.position + rider->position. Also still open: the 0x417a30/0x428650
  actor_movement_action_cancel collision (call sites are split between the two), actor+0x270
  used as a prop index by live code (0x424090) while types/ai.h names it target_unit_index, and
  305 extern arity disagreements against rewritten definitions (145 declare (void); of the
  other 160 the worst repeats are actor_find_prop_for_object 13, actor_set_units_active 12,
  actor_find_or_create_shared_prop 8). 3 rows appended to symbols/agent_phase4_ai.txt;
  src/ai/README.md rewritten (counts 510/9, new review-pass table, new types, top-5 open
  questions, per-section function counts regenerated).
- ai DONE 2026-09-20 (510 files; ~12M tokens over 4 sessions + 2 catch-ups). Two follow-ups flagged by agents: actor_movement_action_cancel name collision (0x417a30 vs 0x428650), object_get_node_local_transform convention mismatch. networking s1 launched: types + 0x440350..0x4d8620 split 0x4b80f0.
- networking s1: types/networking.h done 2026-09-20 (35 structs; session variant embeds game.h game_variant; two hook-named "functions" are mid-body addresses; the server browser is GameSpy SDK key/value calls with no recoverable record). Rewrites lost to the limit ~04:00, resumed 05:04.
- networking s1 DONE 2026-09-20 (93 files, gate clean; server browser is GameSpy key/value glue). s2 launched: 0x4d8a20..0x4df790 split 0x4dc190.
- networking s2 DONE 2026-09-20 (221 files; client_globals pad_eda is a live connection-mode discriminant, for the reconciliation pass). s3 launched: 0x4df840..0x4e6510 split 0x4e3160.
- networking s3: 77/125 files before the limit hit ~09:15; resumed 10:04.
- networking s3 first half (0x4df840..0x4e3160) DONE 2026-09-20: 30 files this session (29 new +
  network_banlist_load already landed from the earlier partial run), covering all 72 addresses in
  range except the mid-body 0x4e2d4b (misattributed, folded into sv_players.c). Found and fixed
  three cases where networking_functions.md's own low-confidence (0.3) message-type-number
  summaries were swapped relative to network_game_process_incoming_message's real dispatch
  switch (0x4e2630/0x4e26a0, 0x4e2790/0x4e2810/0x4e2870, 0x4e28d0/0x4e2930) -- the dispatcher's
  literal call table was trusted instead; see each affected file's own header note. Also found
  sv_single_flag_force_reset's real behaviour is nothing like Ghidra's pseudo-C (two
  "Removing unreachable block" warnings hid the whole change-notification path); rewritten from
  disassembly. Gate: networking 275/338 (pre-existing 63 game_variant-include failures,
  unrelated to this batch, all predate this session), whole tree 2332/2395, no new failures from
  this batch. 97 UNSURE marks, 1 TYPES-GAP (network_queued_update_record, an undocumented
  message-delta record modelled from local offsets only). Second half (0x4e3160..0x4e6510) is a
  separate agent's batch.
- networking phase-4 REVIEW PASS DONE 2026-09-20. Gate: networking 345/345, whole tree 2402/2402
  (was 2332/63 -- the 63 were every file that includes networking.h without math.h+game.h for
  game_variant, or without <wchar.h>; all fixed). Wrote the 5 functions the second rewriter
  deferred (0x4e5870, 0x4e5a30, 0x4e5d60, 0x4e6270, 0x4e6510), all from objdump disassembly,
  because Ghidra loses every register argument in that neighbourhood.
  The big find: the whole client update-decode family had FUN_004ec590 mis-wired. It takes the
  DECODE CONTEXT in EAX and the DESTINATION BUFFER in ECX (0x4e53a1, 0x4e56a2, 0x4e5764,
  0x4e5cb7, 0x4e5dd8, 0x4dbaa3, 0x4e20b8, 0x4de99d all `lea ecx,<scratch>` right before the
  call). Nine files were handing it the message_delta_decode_state and no destination at all --
  which is exactly why four of them carried notes claiming the decoded values "are never written
  by any traceable code path". They are written, through the ECX pointer nobody was passing.
  Also rewired: handle_remote_player_action_update (0x4e60c0) was called WITHOUT its EAX control
  record in all three in-tree callers (would have applied whatever was in EAX), and its
  is_baseline is a byte not a dword (two callers pass a stack slot with three stale upper bytes,
  Ghidra's CONCAT31); player_update_client_remote_player_position_update_from_network (0x4e6270)
  was called without its EAX player index and with the dword at header+4 instead of the two
  bytes at +4/+5; message_delta_read_changed_subfields (0x4ed1d0) takes the state in EDI.
  Spot-checked 10 functions line by line against tools/pack.py plus objdump and fixed 5 more
  defects: 0x4e5390 latched player+0xf0..0xf8 from the data_iterator that sits next to the decode
  scratch instead of the decoded position; 0x4e5720 dropped the local_player_index == -1 gate
  from its delta-path inner validation; 0x4e0ef0 passed the LIVE player entry where the binary
  passes a stack copy (its other loop does pass the live one -- the two loops genuinely differ)
  and cleared only network_machine::flags where the binary does a 16-bit store over flags AND
  unknown_0f; 0x4df290 was named and typed after the wrong object (its ESI is a
  network_server_globals*, proved by 0x4e19c0 computing ecx+0x3b8 == ::machines) and carried a
  phantom 7th stack argument that is really 0x4e19c0's EAX register argument -- renamed to
  network_server_advance_connect_state, 1 row appended to symbols/agent_phase4_networking.txt.
  TYPES-GAP closed: network_queued_update_record folded into types/networking.h as
  message_delta_decode_state (0x4ed1d0 takes the same record in EDI and reads message_type at
  +0x04, which is what joins the drain loop's record to the decoders'). network_map_list_entry
  and network_buffer_pair folded too, so no file under src/networking declares a struct any more.
  Six new types recovered from the update family: remote_player_update_header,
  remote_player_action_state, remote_player_biped_update_state (0x3c),
  remote_player_vehicle_update_state (0x70), local_player_update_ack (0x10),
  local_player_vehicle_update_ack (0x44). types/game.h vehicle_update_body's field names are now
  independently confirmed -- 0x4e6510's six copies land on object velocity/angular_velocity/
  forward/up in exactly the declared order.
  Extern arity disagreements inside the module: 26 -> 19. Remaining worst repeats
  data_packet_group_decode_packet (6 vs 8, 33 files), network_channel_remote_address_or_default
  (0 vs 2, 21), bit_stream_write_bits_chunked (1 vs 3, 16), network_prepare_challenge_packet
  (0 vs 2, 16). NOT FIXED, needs a trace: network_session_broadcast_to_all (0x4e19c0) reads TWO
  register arguments, ECX (server, declared) and EAX (`mov ebx,eax` at 0x4e19cd, undeclared);
  every call site sets EAX to the encoded size message_delta_encode_message just returned, so
  every rewritten call to it is one argument short. Also open: unit_snap_position_if_far
  (0x4772e0) is declared with 2 params in src/units but the call at 0x4e649b genuinely pushes a
  third (player::unit). Two original quirks preserved deliberately: 0x4e6510's "***Ignoring"
  branch counts position_updates where its "***Applying immediately" branch counts
  vehicle_updates, and it reuses the "Received pos update ..." formats verbatim; 0x4e5d60 skips
  the remap write-back, the local_player_index gate and the buffer zeroing its twin all do.
  src/networking/README.md rewritten (345-file table regenerated from the file headers, 979
  UNSURE marks, 35 files at rc <= 0.25, new struct chapter, new top-5 open questions).
- networking s3 DONE 2026-09-20 (345 files; reviewer wrote the 5 deferred player-update decoders). 0x4b8da0 + 0x4b8d30 reassigned to game -> pending game catch-up (onlyA). s4 launched: 0x4e6950..0x5781c0 split 0x4eb890.
- networking DONE 2026-09-20 (448 files; ~6.9M tokens over 4 sessions). game catch-up launched for 0x4b8da0 + 0x4b8d30; then interface (3 sessions).
- game catch-up DONE 2026-09-20: 0x4b8da0 (multiplayer_game_variant_description_generate,
  5301 bytes) written. 0x4b8d30 (unicode_string_list_get_string) was already written under
  src/game/ before this session. Ghidra's decompile of 0x4b8da0 was badly broken -- ~20 "locals"
  read but never assigned, and ~65 unicode_string_list_get_string/tag_lookup calls shown with no
  visible arguments -- because the real args are all register-passed (ESI/ECX/EDX destination
  buffers into the four already-rewritten server_browser_*_unpack/flags_unpack decoders; EAX=path/
  CX=index into unicode_string_list_get_string) and Ghidra's stack-frame analysis lost them.
  Recovered by disassembling bin/halo.exe 0x4b8da0..0x4b9db5 with capstone and a CFG-validated
  ESP-symbolic-execution pass (zero mismatches at any branch merge), which pinned every local's
  true address, every decoder's register convention, and every string-list call's (path, index)
  pair; cross-checked against the caller (server_browser_selected_variant_description_build.c,
  0x4b74e0) which had already recovered this function's real parameter list from its own call
  site. build_check: 2507/2507 (whole tree). No TYPES-GAP (reused server_browser_custom_options /
  server_browser_gametype1_decoded / server_browser_gametype3_options from types/networking.h).
  UNSURE: options.unknown_00[0] (Ghidra's local_260, tested in all 5 branches) is read but never
  written anywhere in this function or its 4 decoder calls -- left as a literal read, not resolved.
- game catch-up DONE 2026-09-20 (0x4b8da0 recovered via capstone + ESP symbolic execution; review skipped after the limit hit, both files cross-checked against the caller). interface s1 launched: types + 0x44c290..0x49b560 split 0x496c80.
- 2026-09-23: user instruction: Opus 5.5 for types, compile gate and review. module_rewrite.js now omits the model on those two agents so they inherit the session model (claude-opus-5-5); rewriters stay Sonnet.
- interface s1 second half (0x496c80..0x49b560) DONE 2026-09-23 (retry of a session that had stopped
  after 31/65 files, up to 0x499cb0): remaining 34 addresses closed as 30 files (34 addresses -
  4 folded, see below). Biggest finds: "multiplayer_map_list_dispose" @0x498160 and
  "ui_widget_load_by_name_or_tag" @0x49aa00 are not real functions -- both share interface_tick's
  and widget_instance_render's own stack frame/epilogue byte for byte (confirmed via objdump, not
  guessed) and are simply Ghidra mis-splitting each function's tail; folded into interface_tick.c
  and widget_instance_render.c respectively rather than written standalone (both had callers=0).
  render_ui_widgets @0x49b450 is similarly dead: its byte range sits INSIDE widget_instance_
  render_text_box's own 0x49b1d0..0x49b51d span, no prologue, shares that function's epilogue;
  skipped outright, not folded (its logic is already covered by render_text_box's ancestor
  function, widget_instance_render). render_ui_cursor @0x49a2e0 (already flagged misattributed in
  interface_types_notes.md) folded into widget_instance_handle_input_event.c likewise. Recovered
  chimera__load_ui_widget's, list_node_pop's and widget_instance_select_list_index's real register
  conventions from disassembly for interface_tick's rewrite (Ghidra's own caller-side arg lists
  for these were incomplete/wrong in several places); confirmed a "clamped controller index"
  expression in both split-screen viewport functions (0x498330, 0x4984c0) is unconditionally 0 by
  algebraic proof, not by inspection. Gate: interface 127/128 (whole tree), the one failure
  (src/interface/ui_draw_rotated_screen_quad.c, 0x494d70, outside this session's range and not
  touched here) is a pre-existing types/math.h double-inclusion issue unrelated to this batch.
  TYPES-GAP: ~15 anonymous externs for undocumented globals (loading-thread record, several
  console/network-wait state blocks, hud button-icon table base, split-screen viewport table,
  formatted-prompt scratch buffer and fallback strings) -- none added to types/interface.h.
  UNSURE count: high in the last 5 files written (ui_widget_list_item_activate,
  ui_widget_draw_formatted_prompt_string, widget_instance_render_text_box, widget_instance_
  render_list_head, and parts of widget_instance_render/widget_instance_handle_input_event) --
  these five are the deepest, most register-heavy functions in the module and this session's
  remaining time did not allow the same disassembly-verification rigor given to interface_tick
  and widget_create_children_from_tag; flagged with low rewrite-confidence headers and are the
  top candidates for a dedicated review pass. Also open: widget_instance_render_text_box.c's
  param_1+0x44..0x53 float-quad read collides with widget_instance's own list_items/item_count/
  extended_description/list_render_data fields at those exact offsets -- a real conflict, not
  resolved.
- 2026-09-23 06:01: previous Claude session ended mid interface s1 (132/139 written, gate clean). Relaunched with skipTypes; review now on the session model (Opus 5.5).
- interface s1 first half (0x44c290..0x496c80) VERIFIED COMPLETE on relaunch, no new files needed:
  all 75 listed addresses in range accounted for -- 73 already have src/interface/*.c files from
  the prior session, and the remaining 2 (0x495190, 0x4951f0, stale-named "first_person_weapons_
  update" / "first_person_weapon_render_update") were independently re-decompiled with
  tools/pack.py this session and confirmed to be Ghidra mis-splits: both read unaff_EDI/unaff_EBX/
  unaff_ESI register state with callers=0 and reproduce map_list_add_entry's (0x4950c0) own tail
  byte-for-byte (path lowercase loop, strrchr, cache_file_exists, count increment), matching
  interface_types_notes.md's existing call. Left unwritten/skipped per the misattribution rule,
  same disposition as the earlier session's notes. python tools/build_check.py interface: 132 ok,
  0 failed. No files changed, no TYPES-GAP, no new UNSURE marks. Next: second half of s1 is
  already DONE per the entry above, so the module-level next step is picking the next module
  (per the 5x-plan cadence, needs an explicit go).
- interface s1 REVIEW DONE 2026-09-23 (Opus, phase-4 review). Gate interface 132/132, whole tree
  2639/2639. No in-range files were missing (the 7 unwritten s1 addresses are the documented
  mis-splits 0x495190, 0x4951f0, 0x4974f0, 0x498160, 0x49a2e0, 0x49aa00, 0x49b450). 16 functions
  checked line by line against objdump; about 45 behaviour fixes across 47 files, mostly register
  arguments Ghidra dropped: chimera__load_ui_widget takes 7 args with arg 3 = parent widget (all 9
  call sites fixed, invented history record replaced); widget_history_node relaid (+0x04
  list_definition, +0x0a controller); ui_sound_effect is one-based; handle_input_event close table is
  int32 and its any/all test was inverted; list_item_activate run_function no longer suppresses the
  other actions and the final sound is the action kind; formatted prompt / prompt span / text box
  rewritten (Rectangle2D cursor, EDX text, literal strings not pointers, swapped quote/??? branches,
  3-arg search-replace, draw/clip rects); ui_draw_screen_quad arg 5 is a packed ARGB color (callers
  were float-converting it); cursor rect order; the dropped split-screen divider fills;
  interface_tick pending message is 0x00718fac; host_start params are map/variant names;
  fpw lighting uses tag +0x468 / fp-interface +0x0c; 0x006e4738 alpha is a float; several
  pointer-vs-array globals. Header: prompt_draw_state removed, first_person_light_parameters,
  ui_input_event and 3 function-pointer typedefs folded in (local copies deleted). Renames:
  ui_button_prompt_queue_icon_sound -> ui_button_prompt_draw_icon (draws, no sound),
  console_printf.c -> chimera__console_out.c (30 cross-module callers use that name).
  symbols/agent_phase4_interface.txt created (132 rows, 5 renames at 0.80); src/interface/README.md
  written. Open: s2 (0x49b560..0x4c9c80, 237 fns) not started; 30 chimera__console_out externs in
  game/networking lack the EAX color arg; game_engine_post_rasterize_post_game.c calls the
  formatted prompt without EDX text. Next: interface s2 (needs an explicit go per the cadence).
- interface s1 DONE 2026-09-23 (132 files; Opus 5.5 review ~45 behaviour fixes, mostly dropped register args; chimera__load_ui_widget takes 7 stack args). s2 launched: 0x49bac0..0x4ac0b0 split 0x4a6380.

- interface s2 part 1 REVIEW DONE 2026-09-23 (Opus, phase-4 review of 0x49bac0..0x4ac0b0). Gate
  interface 277/277, whole tree 2784/2784. 146 files in range (2 misattributed: 0x49c369 tail of
  ui_check_for_pause_game, 0x49d850 tail of the unlisted 0x49d7c0; map_list_find_index_by_path.c
  deleted). ~30 functions checked against objdump; behaviour fixes in ~70 files: chat_dispatch_incoming,
  hud_update_interaction_prompt (was hud_weapon_message_state_update), hud_draw_number (was
  hud_draw_ammo_digit, real extent to 0x4ac6ce), hud_meter_flash_color_blend, hud_meter_draw_fill,
  waypoints, FUN_004a2ad0/1c30/29a0/47c0/4ee0 rewritten from asm (Ghidra dropped bodies); heap_reallocate
  old/heap args in 12 files; strstr polarity in server_list_menu_update; working-copy select inverted in
  4 files; carousel table stride (0x2000 records); virtual keyboard validation_mode dword + sounds;
  set_profile_name/FUN_0049bac0/details refresh widget register args; 0x51c9a0 quad state in both s1 quad
  drawers; s1 audio gain extern swap. Header: validation_mode, list overlay note, 10 new structs +
  GUI/DirectInput fn-pointer typedefs (all TYPES-GAP copies folded). 6 renames at 0.80 appended to
  symbols/agent_phase4_interface.txt (+95 s2 rows); functions.txt not regenerated. README rewritten.
  Open: s2 part 2 (90 fns after 0x4ac0b0) not started; ~25 low-confidence s2 files not re-verified;
  cross-module externs with wrong arity (console_out, virtual_keyboard_open, color_interpolate,
  saved_game_enumerate_by_type, map_list_*).
- interface s2 DONE 2026-09-23 (277 files; Opus 5.5 review ~100 behaviour fixes, several functions rebuilt from objdump). FOLLOW-UP: 50 files in src/interface still named FUN_xxxxxx.c (rewriter A skipped naming) -> include in the end-of-project naming/reconciliation pass. s3 launched: 0x4ac6f0..0x4c9c80 split 0x4b0320.
- interface DONE 2026-09-23 (365 files + 11 non-functions; ~4.8M tokens over 3 sessions; s3 review re-derived 49 of 88 files from objdump and completed hud_messaging_update / hud_render_unit_interface; 0x4b2f8a covered inside hud_weapon_crosshairs_draw.c). rasterizer s1 launched: types + 0x5132b0..0x524980 split 0x51bcd0; s2 0x525030..0x537d60 split 0x52e2d0.
- rasterizer s1 DONE 2026-09-23 (149/151, the other 2 are switch-case labels; types/rasterizer.h 960 lines; 0x5134f0..0x513cf0 is lens flares not decals; vertex/index reserve names swapped; gamma not registry; cache.h 0x007c117c is MaxStreams). s2 launched.
- rasterizer DONE 2026-09-23 (224 files + 4 non-functions; s2 rewriter A shipped 13 "NOT A FAITHFUL REWRITE" placeholders and B skipped 0x533850; Opus 5.5 review replaced all 15 from objdump and rewrote 12 misidentified functions). sound launched: 0x543a30..0x5514d0 split 0x549fa0.
- 2026-09-23 Rich header decoded: cl 13.10.3077 (VS .NET 2003 RTM), link 7.10.3077; 262 engine C objects built with LTCG (/GL). This is why Blam functions use custom register conventions (LTCG invents them per function). Byte-matching would require reproducing the whole-program LTCG link; per-function objdiff matching is not feasible for those objects. Treat byte-identical as out of scope unless the user asks.
- sound DONE 2026-09-23 (132 files; ~2.9M tokens, the most expensive single-module session). Workflow prompt now carries the LTCG finding and a no-placeholder rule. saved_games launched.
- saved_games: types done 2026-09-23 (saved_player_profile 0x1ffc = blam.sav body; 0x555d30 is mid-function; interface FUN_0049cc80/0049ce00 read level progress at +0x11c but it is +0x11e -> reconciliation). Rewrites lost to the session limit ~20:00; resumed after reset.
- saved_games 0x537f70..0x53b6b0 (61 addresses) VERIFIED COMPLETE on relaunch: 58 already had
  src/saved_games/*.c files from the prior (lost) session; the 3 gaps were 0x53aa20, 0x53ad00,
  0x53ae10 (control_profile_clear_binding / control_profile_set_binding were already named by
  their callers in src/interface but unwritten; 0x53aa20 had no name, now
  control_profile_find_binding_for_action). All 3 re-derived and cross-checked against objdump
  (0x53aa20..0x53acf2, 0x53ad00..0x53adfd, 0x53ae10..0x53aff8), which also pinned down the
  7 back-to-back scan-code tables at 0x00714fb4..0x007157ba (keyboard/mouse/gamepad default
  binding tables, same array shapes as the matching saved_player_profile fields) -- declared as
  local externs, not added to types/saved_games.h. Found and preserved one real quirk:
  0x53aa20's gamepad pov-hat search does not return on the first match, it keeps scanning and
  the LAST matching pov index wins (every other table search in the same function returns
  immediately); flagged UNSURE, reproduced exactly. gate: these 3 files individually clean;
  python tools/build_check.py saved_games is 46/106 module-wide (unrelated pre-existing gap:
  60 files, including several already-committed ones like player_profile_get.c and
  control_profile_reset_analog_bindings.c, include only tags.h+saved_games.h and don't compile
  standalone because saved_games.h needs game.h/interface.h/networking.h types first -- not
  touched here since it's not in this session's 3-function scope; worth a dedicated pass).
  No TYPES-GAP (reused existing k_control_* enums for table dimensions). Next: the rest of the
  saved_games module (0x53b700..0x556170, ~52 addresses) still needs the same gap-check and
  rewrite pass (needs an explicit go per the cadence).
- saved_games 0x53b6b0..0x556170 (56 addresses) resumed after a second usage-limit hit: 42 were
  already written from an earlier session leg; this pass wrote the remaining 13 real functions
  (0x53bb50 saved_game_create_custom_variant, 0x53bc70 playlist_profile_create_default_profiles_
  on_disk, 0x53bee0 saved_game_get_variant, 0x53c0b0 game_variant_write_request_start, 0x53c150
  game_variant_write_thread_proc, 0x53c260 saved_game_files_initialize, 0x53c480 saved_game_files_
  dispose, 0x53d080 saved_game_get_directory_by_handle, 0x53d4a0 saved_game_find_by_name, 0x53d720
  saved_game_list_rebuild_index, 0x53daa0 saved_game_index_open_for_write, 0x53db40/0x53dde0
  saved_game_index_register_default_playlists/_profiles) and skipped the one documented
  misattribution (0x555d30, mid-function of file_enumerate_find_next, per the types notes).
  saved_game_check_storage_availability.c (0x53d120, already written) supplied objdump-confirmed
  savegame_find_first/_next/user_save_path_remove signatures reused verbatim by
  saved_game_list_rebuild_index.c. types/tags.h UnicodeStringList/UnicodeStringListString covered
  the ui\\default_multiplayer_game_setting_names / ui\\shell\\strings\\default_player_profile_names
  ustr-tag reads in three functions (0x53bc70/0x53db40/0x53dde0) that share the same fallback
  global (PTR_DAT_00671fac, modeled as default_ustr_fallback_string per saved_game_allocate_new_
  slot.c's earlier objdump-confirmed treatment). No TYPES-GAP. gate: these 13 files individually
  clean; python tools/build_check.py saved_games is 58/118 module-wide, the same 60 pre-existing
  files (untouched, out of this session's scope) still failing the documented missing-include gap.
  saved_games module is now fully written (118 files, 115 of 116 addresses covered; 0x555d30 is
  the one documented non-function).
- saved_games REVIEW DONE 2026-09-23 (Opus 5.5 gate/review). Gate 58/60 -> 118 ok / 0 failed
  (60 files had the include-order gap; one checkpoint callback typed char* vs void*). TYPES-GAP
  folded: file_enumeration_position, win32_file_attribute_data; saved_game_create_record dropped
  (stack artefact, create_slot now uses saved_game_index_entry); game_variant_file is 0x2000
  (every blam.lst writer/reader moves 0x2000). 28 functions objdump-reviewed line by line;
  behaviour fixes in ~30 files (dropped EAX/AX/EDX args to player_profile_load /
  player_profile_rename / XCreateSaveGame / XDeleteSaveGame (wide display name, not path),
  AL-only bool returns, 0x671fac is L"<missing string>" not a pointer, display mode struct,
  copy_files result/phase gating, allocate_new_slot loop + string index, inlined path append,
  0x2000 I/O sizes, 0.1885f -> 0x3e4104fc). Rename 0x53d1e0 saved_game_slot_exists_for_id ->
  saved_game_name_is_available; symbols/agent_phase4_saved_games.txt created (44 rows);
  functions.txt not regenerated. README written. Open: src/game savegame_index_* /
  XCreateSaveGame prototypes and src/interface FUN_0053d080/FUN_0053d1e0 callers -> reconciliation.
- saved_games DONE 2026-09-23 (118 files; restart after the limit reused types and the 102 files already written; review fixed 60 include failures). shell launched.
- shell DONE 2026-09-24 (65 files; 50 of 115 entries are MSVC STL/CRT or non-functions; review wrote exception_filter_crash_reporter and shell_detect_hardware_specs from objdump). CLEANUP LIST: functions Ghidra never created, reported by types agents across modules (shell 0x541b30 window proc, 0x5410d0, 0x57d370, 22 config setters; items/garbage/equipment vtable columns; effects particle physics procs 0x4552a0..0x455610; projectiles 5; biped 0x559e40/0x559f10/0x559f70; saved_games 0x5385d0/0x538ac0/0x539110; sound driver/EAX methods) -> needs a pass that creates them in Ghidra and rewrites them. input launched.
- input DONE 2026-09-24 (77 files; review wrote input_game_action_update 0x48cca0, which runs to ~0x48ec4f; reviewer ran merge_symbols.py, which is fine since functions.txt is generated). render launched.
- render DONE 2026-09-24 (66 files; 0x510410..0x510ba0 is an STL std::sort instantiation; 0x6b4c00 is data not code; review wrote render_contrail + render_objects_collect). camera launched.
- camera DONE 2026-09-24 (59 files incl. functions Ghidra never created; observer array starts at 0x006ac65c, stride 0x29c; review wrote the 10 deferred functions). main launched.
- main DONE 2026-09-24 (48 files; 0x4c7610 is the engine main loop -> main_loop.c; review wrote movie_play_bink, console_autocomplete_command, timedemo_benchmark_update, main_loop). All mid-size modules done. Small modules next, one session each: models, bitmaps, text, scenario, cutscene, cseries, shaders, dialogs, then math catch-up.
- models DONE 2026-09-24 (mostly the animation sampler: compressed quaternion/translation/scale curves; ~1.5M tokens). bitmaps launched.
- bitmaps DONE 2026-09-24 (27 files; bitmap_group_free renamed bitmap_data_free; src/main/screenshot_render.c still uses the old name -> naming pass; 0x006571f4 has two names, bitmaps vs rasterizer -> reconciliation). text launched.
- text DONE 2026-09-24 (22 files; narrow vs wide text paths; game.h hud_world_text_params colour block is a ColorARGB -> reconciliation). scenario launched.
- scenario DONE 2026-09-24 (17 files; 0x00746f90 is the collision BSP, 0x00746f9c the structure BSP pointer: both misnamed in other headers -> reconciliation). cutscene launched.
- cutscene DONE 2026-09-24 (12 files; letterbox bar and title rects rebuilt from objdump). qsort pair 0x449590/0x4496d0 reassigned to cseries. CLEANUP LIST += 0x449780 generic filled-rect helper (7 callers), 18 cutscene codec functions Ghidra never created (0x44a060..0x44a8b0). cseries launched.
- cseries DONE 2026-09-24 (9 files; 0x0087ac06 declared as two different types in interface.h/networking.h, real type uint8 -> reconciliation). shaders launched.
- shaders DONE 2026-09-24 (7 files; includes the numeric countdown timer; rasterizer.h lighting_extra/unknown_84 are render_animation -> reconciliation). dialogs launched.
- dialogs DONE 2026-09-24 (3 files; likely belong to shell; 0x57e2a0 hyperlink parent proc and 0x57e5a0 fatal error dialog proc have no Ghidra function -> missed-functions list). ALL MODULES DONE. math catch-up launched for the 9 helpers from the ai range.
- 2026-09-24: symbols/missing_functions.txt built from the types notes: 69 addresses Ghidra never created (cutscene 18, items 15, shell 14, units 10, projectiles 7, saved_games 3, cache 1, dialogs 1). Not captured by the regex and to add by hand: effects particle physics 0x4552a0/0x4554d0/0x455310/0x4554e0/0x455610/0x455350, sound driver/EAX table methods (0x545e20, 0x546f90, 0x548380 ...), dialogs 0x57e5a0. Camera ones were already written by its review.
- 2026-09-24: ALL MODULES DONE (math catch-up review fixed 4 behaviour bugs incl. a signed-vs-logical shift). CLEANUP PASS 1 started: 97 missed functions created in Ghidra (headless ApplySymbols via symbols/user_missing_functions.txt; the same run applied the 1732 phase-4 renames), re-exported, then tools/workflows/missed_functions.js launched (4 Sonnet rewriters by module group + Opus 5.5 review). Groups in out/phase4/missed_groups.json.
- 2026-09-24 CLEANUP PASS 1 DONE: 91 missed functions written (6 were fragments; review rebuilt sound_driver_initialize from objdump); full tree 3889 ok. CLEANUP PASS 2 (naming) DONE: 50 interface placeholders named (Sonnet agent), tools/propagate_names.py updated 1171 call-site names in 637 files (two script bugs fixed: first-#if-0 cut, function-pointer externs; 2 files with deliberate per-call-shape aliases for 0x43d790 restored and excluded -> reconciliation). tools/naming_report.py clean; full tree 3889 ok. Backups: out/backup_src_before_naming_pass.tar, out/backup_before_reconcile.tar. CLEANUP PASS 3 (types reconciliation) launched: tools/workflows/reconcile_types.js (collect -> 3 fixers by header -> review, all on the session model).

- 2026-09-24: cleanup pass 3 (reconcile_types) done: 85 corrections, 81 applied / 4 skipped / 0 open (R06, R53, R79 closed by hand: tools/propagate_globals.py unified 5 globals, 115 renames; ai_search_step/expand_point_neighbors call 0x43b830 with its real signature; breakable_surfaces_reset / breakable_surface_is_intact renamed). Gate 3889 ok, naming report clean. Coverage audit (tools/coverage_audit.py): 3876 engine functions have exactly one file, 50 explained, 137 orphans (~45 KB) that module agents flagged as out-of-module and nobody wrote -> cleanup pass 4 tools/workflows/orphan_functions.js, groups in out/phase4/orphan_groups.json.
