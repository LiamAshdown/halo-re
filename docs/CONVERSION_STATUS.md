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
