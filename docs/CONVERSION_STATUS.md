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
| a58adc644eb7b93a4 | memory, cseries, cache | running | no |
| a8c68560df27ca293 | structures, scenario, bitmaps, models, text, shaders | running | no |
| a690688f77ce90677 | camera, cutscene, devices, dialogs, projectiles | running | no |
| a30d258f6c14fe84c | input, main, physics | running | no |
| a48cce8b581c8e3c3 | items, effects | running | no |
| (see below) | saved_games, render | relaunched | no |
| a828a5b449c367e7c | units | running | no |
| a8e2e4a6addbda413 | objects | running | no |
| a9d9f7699c4305c1b | sound | running | no |
| a1b3c49331e6b7427 (Opus) | math pilot (+ glm); writes docs/CPP_CONVENTIONS_MATH_PILOT.md | running | no |

## Wave 2 queue (large modules split by file list; start when wave 1 slots free up; brief = docs/AGENT_BRIEF_TEMPLATE.md)
Lists are in tools/cxx_work/wave2_*.txt. Agents for a shared module must NOT edit shared structs (see the template, rule 4).
ai_actor_1 (197), ai_actor_2 (197), ai_rest (231), hs_1 (209), hs_2 (209), hs_3 (207), game_game_1 (169), game_game_2 (168),
game_rest (184), interface_ui (241), interface_rest_1 (148), interface_rest_2 (148), networking_network (283),
networking_rest (273), rasterizer (239), shell (163).
Launch at most 10 agents at a time. Mark launched ones here with their agent id.

## Log
- 2026-10-03: C++ phase 1 done (all src compiles as C++20, extern "C" wrapping, link and smoke test OK on cxx-phase1).
