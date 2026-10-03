# C++ conversion status ledger (maintained by the lead / overnight loop)

Integration branch: `cxx-phase1` (never `master`; nothing is pushed or merged to master without the user).
Agent branches are `worktree-agent-<id>`. Baseline symbol sets: `symbols/exports/<module>.txt`.
Process per finished agent: merge its branch into cxx-phase1, run `python tools/check_module_symbols.py check <its modules>`;
if it fails, `git reset --hard` to the commit before the merge and log it below. After a group of merges, run the full
build (hide bin/halo.exe and out/functions.* first, restore after), smoke-test the exe for 2 minutes and read
build/cxx/Release/halo_standalone.log (no EXCEPTION / MISSING FUNCTION; DIAG lines appear). If the build or smoke test
fails, revert the latest merges one by one until it is green and log the offender.

## Wave 1 (whole modules, Sonnet unless noted)
| agent id | modules | status | merged |
|---|---|---|---|
| a58adc644eb7b93a4 | memory, cseries, cache | done | pending (commit bbe6378f; 3 checks 0 missing/0 extra; view structs, no registries) |
| a8c68560df27ca293 | structures, scenario, bitmaps, models, text, shaders | done | pending (all 6 checks 0 missing/0 extra; text uses a Strategy) |
| a690688f77ce90677 | camera, cutscene, devices, dialogs, projectiles | done | yes (e92165ab; all 5 checks 0 missing/0 extra; handle classes, no registry because dispatch tables are fixed data in standalone/data) |
| a30d258f6c14fe84c | input, main, physics | done | yes (f1a0498d; checks 0 missing/0 extra; static-function service classes) |
| a48cce8b581c8e3c3 | items, effects | done | yes (e93f959d; check items 115/115, effects 162/162; classes in anonymous namespaces, no Strategy/State patterns yet) |
| a18f610ee74d844a2 | saved_games, render (relaunched after stale-base stop) | done | pending (branch worktree-agent-a18f610ee74d844a2, commit d61c3c75) |
| a828a5b449c367e7c | units | done | yes (9a6e0ae8; check units 410 base, 0 missing, 263 new halo:: symbols; UnitView/BipedView/VehicleView; no UnitTypeBehavior registry) |
| a8e2e4a6addbda413 | objects | done | pending (commit 0f5ac3b4; 328 base, 0 missing, 0 extra) |
| a9d9f7699c4305c1b | sound | done | pending (commit 24145f56; 225 base 0 missing 0 extra; AudioDevice/EaxBackend interfaces; 2 small behaviour changes: channel_set_parameters 3 args, sound_initialize via audio_device()) |
| a1b3c49331e6b7427 (Opus) | math pilot (+ glm); writes docs/CPP_CONVENTIONS_MATH_PILOT.md | running | no |

## Wave 2 queue (large modules split by file list; start when wave 1 slots free up; brief = docs/AGENT_BRIEF_TEMPLATE.md)
Lists are in tools/cxx_work/wave2_*.txt. Agents for a shared module must NOT edit shared structs (see the template, rule 4).
ai_actor_1 (197), ai_actor_2 (197), ai_rest (231), hs_1 (209), hs_2 (209), hs_3 (207), game_game_1 (169), game_game_2 (168),
game_rest (184), interface_ui (241), interface_rest_1 (148), interface_rest_2 (148), networking_network (283),
networking_rest (273), rasterizer (239), shell (163).
Launch at most 10 agents at a time. Mark launched ones here with their agent id.
Launched: ai_actor_1 = a816627a3d7da40e7, ai_actor_2 = ab96a7b6b4db18803

## Log
- 2026-10-03: C++ phase 1 done (all src compiles as C++20, extern "C" wrapping, link and smoke test OK on cxx-phase1).

- Note: concurrent `git merge --ff-only` into an agent worktree by both the lead and the agent causes an index.lock race; the lead does it once right after spawning and the agent only verifies.
- harness/gen_link.py scans src/*/*.c for extern address comments; converted modules no longer appear there (standalone link tables are not affected). Revisit if the regenerate tools are needed.

- INTEGRATION BUILD #1 FINISHED GREEN (links; 120 s smoke: alive, 106 DIAG lines, 0 EXCEPTION/MISSING; retail files restored). Was: in progress (started by the lead after merging items, effects, units, camera group, input/main/physics; retail files hidden as *.hidden while it runs; log build/cxx_wave1_build.log). The supervising loop must not start another build, must not MERGE anything (it would change sources under the build), and must not touch the hidden files until this entry says "finished". Pending merges after it finishes: a18f610ee74d844a2 (saved_games, render: done, checks 0 missing/0 extra; view classes + c_api shims).

- Pending merges after integration build 1 finishes (in this order, run the module symbol checks after each): a18f610ee74d844a2 (saved_games, render), a8e2e4a6addbda413 (objects), a8c68560df27ca293 (structures, scenario, bitmaps, models, text, shaders), a816627a3d7da40e7 (ai_actor_1: commit cd6044c0, ai check 793 base 0 missing 0 extra). Then run integration build #2.
- Wave 2 launched: ai_rest = a3286720ac0c33a33, hs_1 = ac1b96a5f35fa3abd, hs_2 = a19b5ccfa208639bd, hs_3 = a86799ccf54aaf388 (worktrees fast-forwarded by the lead). When fast-forwarding an agent worktree, first delete the untracked files the agent copied (docs/CPP_CONVENTIONS.md, tools/check_module_symbols.py) or git refuses.

- Also pending after build 1: a9d9f7699c4305c1b (sound). Watch the audio path in the smoke test.
- Wave 2 batch 2 launched: game_game_1 = adf15f46e5ca3f177, game_game_2 = a8e668d413881d3cf, game_rest = ad3b932c3cbff9f0b, interface_ui = a484a43168222aa00. Pending merges now also include a58adc644eb7b93a4 (memory, cseries, cache).

- Merged into cxx-phase1 after build 1 (all symbol checks 0 missing/0 extra): saved_games, render, objects, structures, scenario, bitmaps, models, text, shaders, ai (actor slice 1), sound, memory, cseries, cache, hs (slice 2, a19b5ccfa208639bd, Command pattern groups). INTEGRATION BUILD #2 FINISHED GREEN (links; 120 s smoke alive, 102 DIAG, 0 EXCEPTION/MISSING; retail files restored). Was: in progress (log build/cxx_wave2_build.log; retail files hidden as *.hidden; do not merge or touch them until this says finished).
- Pending merge after build 2: a86799ccf54aaf388 (hs slice 3, commit 3d560311, hs check 0 missing/0 extra; CommandGroup marker interface). Launched interface_rest_1 = aa46dd7cd93338617. Resumed game_game_1 = adf15f46e5ca3f177 (it was blocked on reading files via the shell; told to use the Read tool).
- Pending merge after build 2: ab96a7b6b4db18803 (ai actor slice 2, commit 6f75fc3f, ActorMode/ActorTypeBehavior registries wrapping the data tables), ac1b96a5f35fa3abd (hs slice 1, commit 0bddd3d4). Launched interface_rest_2 = a5033e01c6bad62e5, networking_network = a150475c633b39bdc.
- Done and pending merge into build 3: a3286720ac0c33a33 (ai rest, d587473e), a484a43168222aa00 (interface ui, 85104ea0), ad3b932c3cbff9f0b (game rest, 8569081d), adf15f46e5ca3f177 (game slice 1, fcb15b2c), a8e668d413881d3cf (game slice 2, 548a515c), plus hs3, ai actor 2, hs1 listed above.
- Merged into cxx-phase1 after build 2: hs3, ai actor 2, hs1, ai rest, interface ui, game rest, game slice 1, game slice 2 (symbol checks for ai, hs, interface, game running; then integration build 3). Launched networking_rest = a1148d10c1c112938, rasterizer = a0c44d572856a4b6e, shell = aaac3f052ccb96a4b. All wave-2 queue items are now launched or done except none; remaining work after wave 2: a design-pass wave (real polymorphism/registries/State where only static facades exist), docblock review pass, harness/gen_hooks.py + gen_link.py to read .cpp (they scan src/*/*.c for blam-cc and address comments).
- Symbol checks after the batch-3 merge: ai, hs, interface, game all 0 missing/0 extra. INTEGRATION BUILD #3 FINISHED GREEN (links, no hs class clashes; 120 s smoke alive, 106 DIAG, 0 EXCEPTION/MISSING; retail restored). Was: in progress (log build/cxx_wave3_build.log; retail files hidden; do not merge or touch them until this says finished). Watch for class-name clashes between hs slices in namespace halo::hs (hs1 DeviceCommands vs hs2 DeviceCommands).
- interface rest slice 2 (a5033e01c6bad62e5, commit 2800651b, interface check 0 missing/0 extra) done; merging into cxx-phase1 (build 4 later).
- math pilot (a1b3c49331e6b7427) done and merged (bf9acfd6): bit-exact test passes (46,042 calls), glm in 20 functions, docs/CPP_CONVENTIONS_MATH_PILOT.md; symbol check now ignores compiler constants and allows glm::. interface_rest_2 (a5033e01c6bad62e5) merged (7ba0e917).
- NO-ASM track launched: eq (globals.asm 390 EQU leftovers -> C, Opus) = a37e3085366f16104; entries (code_entries.asm + bridges.asm -> C/C++) = a63308f5865d60149. Next step after both: eliminate standalone/image/*.asm (rdata/data/tls/rsrc; rsrc needs a .rc) and then project(halo C CXX) without ASM_MASM.
- Still running: interface_rest_1 aa46dd7cd93338617, networking_network a150475c633b39bdc, networking_rest a1148d10c1c112938, rasterizer a0c44d572856a4b6e, shell aaac3f052ccb96a4b.
- Merged after build 3: interface_rest_1 (aa46dd7cd93338617), networking_network (a150475c633b39bdc), math pilot, interface_rest_2; checks interface/networking/math 0 missing/0 extra. INTEGRATION BUILD #4 FAILED to link (3 issues, fixed in a4c57ef0 and the commit before: block-scope externs in net1_channel/ifr2_hud, /NODEFAULTLIB:libcpmt.lib); relink = build 4b FINISHED GREEN (links; 120 s smoke alive, 102 DIAG, 0 EXCEPTION/MISSING; retail restored; first build without code_entries.asm/bridges.asm). Was: in progress (log build/cxx_wave4_build.log; retail files hidden; do not merge or touch them until this says finished).
- Merged: no-asm entries agent a63308f5865d60149 (code_entries.asm and bridges.asm replaced by code_entries.c / bridges.cpp; commit a4c57ef0). Done and pending merge after build 4b: networking_rest a1148d10c1c112938 (branch worktree-agent-a1148d10c1c112938; its report misnames the branch). Remaining running: rasterizer a0c44d572856a4b6e, shell aaac3f052ccb96a4b, eq a37e3085366f16104. Note for all agents: block-scope `extern` declarations inside functions get C++ linkage; declare externs at file scope inside extern "C".
- networking_rest (a1148d10c1c112938) merged after build 4b; networking check 0 missing/0 extra. Next: wait for rasterizer, shell, eq; then build 5.
- 04:00 usage-limit interruption: rasterizer (a0c44d572856a4b6e, 3 commits), shell (aaac3f052ccb96a4b, 1 commit + uncommitted) and eq (a37e3085366f16104, uncommitted) were cut off and RESUMED with SendMessage; interface_rest_1 had already finished and was merged. State of cxx-phase1: last green integration build = 4b (with networking_rest merged afterwards, not yet built). After rasterizer, shell, eq finish: merge, symbol checks, build 5, then eliminate standalone/image/*.asm.
