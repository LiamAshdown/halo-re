// ai_communication_broadcast  (Ghidra: already named)
// address 0x42d340, size 5603 bytes -- the largest function in the ai module.
// name confidence: 0.55   rewrite confidence: 0.3
// evidence: out/phase4/ai_functions.md / out/phase2/results/ai_05.json: "Looks up an event id
//   (param_1) in DAT_008802e0 to find a chain of DAT_00655aa0 'ai conversation' entries sharing
//   that id, filters candidates by difficulty/team/object-type masks, scores and randomly
//   selects one, then queues it for playback via unit_animation_change_priority_check/unit_commit_speech and triggers
//   follow-up look/order calls (ai_propagate_communication_reaction, ai_communication_play_event_line, unit_try_start_scripted_action_animation, actor_issue_order_or_vocalize); called
//   with 21 distinct sites throughout the module as the central dialogue trigger." All 21
//   in-module call sites already agree on a plain 7-argument __cdecl shape (never a name
//   collision, always the same argument count), which this file's own disassembly confirms:
//   every real caller cleans up 0x1c (28 = 7*4) bytes and the prologue is a flat
//   `sub esp,0x4cc` with no register-passed arguments at all (unlike almost everything else in
//   this module).
// register convention: plain __cdecl, all seven arguments on the stack.
// blam-cc: stack -> event_code, unit_index, object_a, reason, object_b, object_c, extra_data
//
// This file was rewritten from a combination of the Ghidra pseudocode and a manual
// `objdump -d -M intel --start-address=0x42d340 --stop-address=0x42e930` pass, because the
// module's own README flagged this function as deliberately deferred: "It builds a 0x38-byte
// candidate record on its stack and hands it to at least four different readers that each
// interpret a different, overlapping field set." That candidate record's layout is recovered
// below from pure stack-offset arithmetic (every `local_N` Ghidra name in the original
// decompile sits at address `entry_esp - N`, confirmed against the disassembly's
// `sub esp,0x4cc` + 4 pushes prologue), not from guessing field meaning, so the BYTE OFFSETS
// are trustworthy even where the FIELD NAMES below are UNSURE.
//
// Three genuine Ghidra decompiler errors were found and are fixed here (all in the tail, after
// the winning candidate is picked):
//  1. `ai_propagate_communication_reaction(fVar11,&local_438)` -- Ghidra's first argument is wrong. The real assembly
//     (0x42e703-0x42e70c) pushes `esi` (the candidate's participant_object_index, field +0x14),
//     not the value it labels `fVar11` (field +0x1c). Second argument confirmed correct.
//  2. `ai_communication_play_event_line(fVar11,(uint)uVar16,1,pfVar18[8],&local_438)` -- three of five arguments are
//     wrong. The real call (0x42e711-0x42e721) pushes, in order, (participant_object_index
//     [+0x14], direction_class [+0x06], 1, object_result [+0x20], &local_430), not
//     (other_object_index [+0x1c], entry_index [+0x34], 1, object_result [+0x20], &local_438).
//     Only the third and fourth arguments match Ghidra's rendering.
//  3. A conditional call to `ai_communication_record_line_played` inside the early-return
//     ("already played") branch (0x42e6ee-0x42e703) is compiled into the binary but is
//     unreachable: the register it branches on (`dl`) is unconditionally set to 1 at 0x42e673
//     and never modified before the test, so the branch that would skip the call is always
//     taken. Ghidra's own decompile already omits this dead call; this rewrite does the same and
//     records the finding here instead of reintroducing dead code.
// One argument-identity slip was found in the main scoring loop and fixed the same way: the
// second argument of `ai_select_communication_target` is `[esp+0x64]` (order_fallback), not the `local_47c`
// Ghidra's pseudocode prints; confirmed at 0x42e026/0x42e02c.
//
// UNSURE (could not be fully re-derived from disassembly within this pass; see inline notes):
//  - The exact trip count and per-slot meaning of the "recency direction" precompute loop
//    (Phase 0 below) rests on Ghidra's own loop-counter rendering (its two int-as-float bit
//    patterns were decoded by hand: 7.00649e-45 is the bit pattern for the integer 5, not the
//    16 a first pass assumed), not a full manual trace of the `__ftol` calls inside it, whose
//    float operand is pushed by FPU instructions Ghidra does not attribute to any argument. The
//    output arrays' total size (32 flag bytes, 32 recency values) IS independently confirmed by
//    the function's own explicit zero-fill ranges.
//  - `ai_communication_line_definition` (the 0x28-byte row table at 0x00655aa0) is a TYPES-GAP
//    local struct; only the fields this function actually reads are named, several with UNSURE
//    tags, by analogy with the already-typed sibling table `ai_communication_event_definition`
//    (0x00656b08, types/ai.h).
//  - `actor_issue_order_or_vocalize`'s call at the very end (0x42e8f7-0x42e90f) has four live registers
//    (EAX=-1, ECX and EBX both equal to the candidate's speaker_actor_index, EDI equal to the
//    candidate's other_object_index) plus two stack arguments, none of which Ghidra's own
//    decompile shows; reproduced from the disassembly with a blam-cc comment. Which of ECX/EBX
//    the callee actually reads is not established.
//  - actor+0xb8 / object+0xb8 (read here as "team") is the same open header disagreement
//    documented in actor_attach_to_unit.c and this module's README ("object+0xb8 is the team,
//    not name_index"); reproduced raw with the same comment those files use.
//  - ai_globals.unknown_14/18/1c/20/24/28 are declared `datum_index` in types/ai.h but are read
//    here (Phase 0) as plain int32 tick timestamps subtracted from the current tick and clamped
//    to >=0; kept as a local `int32_t*` reinterpretation rather than changing the header, the
//    same way ai_communication_record_line_played.c already treats the same block.
//  - actor+0xa0 (read via `mode_data + 4`, a single byte) falls inside the actor.mode_data union
//    whose per-mode layout this module has never resolved (see README).
//  - The candidate weight formula's exact operand pairing
//    (`local_418 * probability * local_450 * local_484 * local_454 * local_498`) is reproduced
//    positionally; two of its six factors (weight_i/weight_scalar below) are themselves
//    `__ftol` results whose FPU source is not independently traced.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include <string.h>

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // FABS

// The two records this function builds -- ai_communication_line_definition (the 0x28-byte row
// of the static table at 0x00655aa0) and ai_communication_candidate (the 0x38-byte per-candidate
// stack record) -- were TYPES-GAP locals in this file when it was first written and are now
// folded into types/ai.h, where their comments carry the same UNSURE tags. Folding them found
// two real layout bugs in the local copies: under pack(1) the int16 at +0x10 was followed
// directly by the dword documented at +0x14 (landing it at +0x12), and the int16 at +0x28 by
// the dword documented at +0x2c (landing it at +0x2a). The header versions carry the two
// missing pad bytes, so every offset past +0x10 is now what the disassembly actually uses.

extern game_time_globals *game_time;   // 0x006f1d6c
extern data_array *object_data;        // 0x008603b0
extern data_array *actor_data;         // 0x00880360
extern void *actor_type_procs[16];     // 0x006853b8
extern data_array *encounter_data;     // 0x008802c8
extern uint8_t team_relationship_flags; // 0x006b0b84, base of the 0x2d-dword team-relationship block
extern ai_globals *ai_globals_ptr;     // 0x00880354
extern int16_t ai_communication_event_index[]; // 0x008802e0, indexed by event_code, gives the
                                                // starting row index into DAT_00655aa0, or -1
extern ai_communication_line_definition ai_communication_lines[]; // 0x00655aa0
extern float ai_communication_direction_table[][5]; // 0x00655950, 10 rows read per call
                                                     // (columns 0,1,3,4; column 2 unread here)
extern int16_t ai_communication_class_tier[];  // 0x006558c4, stride 2, indexed by class_index
extern int16_t ai_communication_class_order[]; // 0x006558f4, stride 2, indexed by tier
extern int16_t DAT_00655904[];         // 0x00655904, stride 2, indexed by tier
extern int16_t DAT_00655914[];         // 0x00655914, stride 2, indexed by other_object_index
extern int32_t communication_line_base; // 0x006f0c9c, already declared in
                                        // ai_communication_record_line_played.c
extern int32_t random_seed_global;     // 0x00719cd0
extern char ai_marker_name_a[];        // 0x0066bfa0
extern int32_t DAT_00725204;           // 0x00725204, UNSURE: a difficulty/tick threshold

extern void ai_communication_record_line_played(datum_index object_index, int16_t tier,
                                                  int16_t communication_line_id,
                                                  int16_t conversation_line_id); // 0x42f9e0, already rewritten
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX
extern void ai_mark_recognized_objects_for_reaction(uint32_t team_a, uint32_t team_b, uint32_t reaction_flags); // 0x42ba80, not yet rewritten
extern void ai_propagate_communication_reaction(datum_index participant_object_index, void *queue_header); // 0x42e9c0, not yet rewritten
extern uint32_t ai_select_communication_target(uint32_t speaker_actor_index, uint32_t order_fallback,
                              int16_t table_arg_a, uint32_t seat_or_object,
                              float *out_weight); // 0x42ec90, not yet rewritten
extern void ai_communication_play_event_line(datum_index participant_object_index, int16_t direction_class,
                          int32_t always_one, uint32_t object_result,
                          void *queue_header); // 0x42eee0, not yet rewritten
extern uint32_t ai_communication_select_speaker_in_reference(int32_t probability_const, int32_t event_code, uint32_t tier,
                              uint32_t direction_class, int16_t table_arg_a,
                              int16_t table_arg_b); // 0x42ff80, not yet rewritten
extern uint32_t ai_communication_select_speaker_by_team(int32_t kind, datum_index unit_index, datum_index other_unit_index,
                              float radius, int32_t event_code, uint32_t tier,
                              uint32_t direction_class, int16_t table_arg_a,
                              int16_t table_arg_b, uint32_t flags); // 0x4300d0, not yet rewritten
extern void actor_issue_order_or_vocalize(int32_t fallback_marker /*EAX*/, uint32_t speaker_actor_index /*ECX*/,
                          uint32_t speaker_actor_index_ebx /*EBX, same value as ECX*/,
                          uint32_t other_object_index /*EDI*/, int32_t order_kind,
                          int16_t follow_up_order); // 0x4302e0, not yet rewritten
extern float ai_communication_rate_player_proximity(int32_t mode, int32_t param_2, int32_t param_3); // 0x4303f0, not yet rewritten
extern uint8_t team_pair_override_adjust_counter(uint32_t self_team_packed, uint8_t witness_is_hostile,
                             uint8_t *out_should_shout); // 0x45bfc0, not yet rewritten (game module)
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void unit_animation_change_priority_check(uint32_t tier, int32_t always_one, float *look_marker,
                          int16_t *look_node, uint32_t *unused_out); // 0x560d00, not yet rewritten (units module)
extern void unit_commit_speech(datum_index participant_object_index /*EAX*/,
                          void *queue_descriptor /*ECX*/); // 0x560f20, not yet rewritten (units module)
extern int8_t unit_scripted_action_animation_exists(void); // 0x569470, not yet rewritten (units module), UNSURE args
extern void unit_try_start_scripted_action_animation(datum_index participant_object_index, int16_t raw_field_0a,
                          float *direction_vector); // 0x569530, not yet rewritten (units module)

// blam-cc: stack -> event_code, unit_index, object_a, reason, object_b, object_c, extra_data
// Central AI communication/dialogue dispatcher. Resolves the two given unit indices (and, via
// their actor records, the source unit's encounter), builds a small per-source/target flag
// block, then walks the chain of ai_communication_lines rows sharing event_code, scoring every
// row that passes its team/kind/relationship filters into a local candidate array. One
// candidate is picked (deterministically if only one qualifies, otherwise by a weighted random
// draw), and its selection is either recorded as a silent repeat or dispatched as a fresh
// queued line with an optional look-at order.
void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a,
                                 int32_t reason, datum_index object_b, datum_index object_c,
                                 uint32_t *extra_data)
{
    int32_t current_tick = game_time->game_time;

    // ---- Phase 0: per-team-pair "how long since we last spoke in roughly this direction"
    // precompute. ai_globals+0x1c is 6 int32 tick stamps (typed datum_index in types/ai.h; see
    // file header); this loop samples them as two overlapping windows of 3 and produces, for
    // 2 "sides" x 10 rows, a boolean "recently reacted" flag and a recency value, consumed
    // later inside the main scoring loop while these stack slots still hold these values (the
    // same physical stack region is reused for a different purpose after the candidate loop
    // finishes -- see Phase F).
    int32_t *team_pair_ticks = (int32_t *)((uint8_t *)ai_globals_ptr + 0x1c);
    uint8_t  recent_dir_flag[32];   // 2 "sides" * 16 bytes/side
    int16_t  recent_dir_value[32];  // 2 "sides" * 16 shorts/side
    int32_t side, table_row, round, inner;

    memset(recent_dir_flag, 0, sizeof(recent_dir_flag));
    memset(recent_dir_value, 0, sizeof(recent_dir_value));

    for (side = 0; side < 2; side++)
    {
        int32_t gap_near_raw = current_tick - team_pair_ticks[side - 2];
        int32_t gap_mid_raw = current_tick - team_pair_ticks[side];
        int32_t gap_far_raw = current_tick - team_pair_ticks[side + 2];
        int16_t gap_far, gap_mid, gap_near;
        uint8_t *flag_out;
        int16_t *value_out;

        if (gap_far_raw < 0) gap_far_raw = 0;
        gap_far = (int16_t)gap_far_raw;
        if (gap_mid_raw < 0) gap_mid_raw = 0;
        gap_mid = (int16_t)gap_mid_raw;
        if (gap_near_raw < 0) gap_near_raw = 0;
        gap_near = (int16_t)gap_near_raw;

        flag_out = &recent_dir_flag[side * 16];
        value_out = &recent_dir_value[side * 16];
        flag_out[0] = 1;
        flag_out[1] = 1;
        table_row = 0;
        // UNSURE: trip count decoded from Ghidra's int-as-float bit patterns (7.00649e-45 is
        // the bit pattern for the integer 5).
        for (round = 0; round < 5; round++)
        {
            for (inner = 0; inner < 2; inner++)
            {
                const float *row = ai_communication_direction_table[table_row];
                uint8_t recently = 0;
                int16_t best = 0;

                if (0.0f < row[0])
                {
                    int16_t v = gap_mid; // UNSURE: __ftol's real FPU operand is not traced
                    if (0 < v) { recently = 1; best = v; }
                }
                if (0.0f < row[1])
                {
                    int16_t v = gap_far;
                    if (0 < v) { recently = 1; if (best <= v) best = v; }
                }
                if (row[3] <= 0.0f)
                {
                    // recently unchanged (matches LAB_0042db3b)
                }
                else
                {
                    int16_t v = gap_near;
                    if (v >= 1)
                    {
                        recently = 1;
                        if (best <= v) best = v;
                        if (0.0f < row[4] && (float)value_out[table_row] < row[4] * 30.0f)
                            recently = 0;
                    }
                }
                value_out[table_row] = best;
                flag_out[2 + table_row] = recently;
                table_row++;
            }
        }
    }

    // ---- Phase A: resolve the two units (self/other) into actors and object-type masks.
    {
    object *self_object = (object *)0, *other_object = (object *)0;
    datum_index self_actor_index = (datum_index)k_datum_index_none;
    datum_index other_actor_index = (datum_index)k_datum_index_none;
    actor *self_actor = (actor *)0, *other_actor = (actor *)0;
    encounter *self_encounter = (encounter *)0;
    uint32_t self_type_mask = 0, other_type_mask = 0;
    uint32_t self_team_packed = (uint32_t)-1, other_team_packed = (uint32_t)-1; // local_490 / local_4c8
    uint8_t self_capability[2] = { 0, 1 };   // local_4a0 (byte [1] = near-threat flag)
    uint8_t other_capability[2] = { 0, 1 };  // local_4a4
    int16_t seat_or_difficulty;

    if (reason == -1) reason = 0;
    if ((int16_t)object_b == -1) object_b = 0;

    if (unit_index != (datum_index)k_datum_index_none)
    {
        int16_t self_object_team;
        self_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
        self_object_team = *(int16_t *)((uint8_t *)self_object + 0xb8); // UNSURE: object+0xb8, see actor_attach_to_unit
        self_actor_index = *(datum_index *)((uint8_t *)self_object + 0x1f4); // unit_data.actor_index
        self_team_packed = (uint32_t)(uint16_t)self_object_team | 0xffff0000u;

        switch (self_object_team)
        {
        case 1: self_type_mask = 1; break;
        case 2: self_type_mask = 2; break;
        case 3: self_type_mask = 4; break;
        case 4: self_type_mask = 0x38; break;
        case 5: self_type_mask = 0x40; break;
        default: self_type_mask = 0; break;
        }

        if (self_actor_index == (datum_index)k_datum_index_none)
        {
            if (*(int32_t *)((uint8_t *)self_object + 0x218) != -1) self_type_mask = 1;
        }
        else
        {
            self_actor = &((actor *)actor_data->data)[self_actor_index & 0xffff];
            self_type_mask = *(uint16_t *)((uint8_t *)actor_type_procs[self_actor->type] + 4);
            if (self_actor->tally.group_c_total < 1)
                self_capability[1] = (self_actor->tally.group_a_total > 0) ? 0 : 1;
            else
                self_capability[1] = 1;
            if (self_actor->encounter_index != (datum_index)k_datum_index_none)
                self_encounter = &((encounter *)encounter_data->data)[self_actor->encounter_index & 0xffff];
        }
    }

    if (object_a != (datum_index)k_datum_index_none)
    {
        int16_t other_object_team;
        other_object = ((object_header *)object_data->data)[object_a & 0xffff].data;
        other_actor_index = *(datum_index *)((uint8_t *)other_object + 0x1f4);
        other_object_team = *(int16_t *)((uint8_t *)other_object + 0xb8); // UNSURE: see above
        other_team_packed = (uint32_t)(uint16_t)other_object_team;

        switch (other_object_team)
        {
        case 1: other_type_mask = 1; break;
        case 2: other_type_mask = 2; break;
        case 3: other_type_mask = 4; break;
        case 4: other_type_mask = 0x38; break;
        case 5: other_type_mask = 0x40; break;
        default: other_type_mask = 0; break;
        }

        if (other_actor_index == (datum_index)k_datum_index_none)
        {
            if (*(int32_t *)((uint8_t *)other_object + 0x218) != -1) other_type_mask = 1;
        }
        else
        {
            other_actor = &((actor *)actor_data->data)[other_actor_index & 0xffff];
            other_type_mask = *(uint16_t *)((uint8_t *)actor_type_procs[other_actor->type] + 4);
            if (other_actor->tally.group_c_total < 1)
                other_capability[0] = (other_actor->tally.group_a_total > 0) ? 1 : 0;
            else
                other_capability[0] = 1;
        }
    }

    seat_or_difficulty = (int16_t)object_b; // sVar15

    // local_45c/local_4c9 and local_40c/local_4a5: function-scoped "found a witness" caches
    // that the friendly-fire block below and the row loop's case 2 / case 4 share and reuse
    // (a search performed once, here or in a row, is never repeated for the remainder of this
    // call).
    {
    uint32_t case2_cached_result = (uint32_t)-1; // local_45c
    uint8_t  case2_pending = 1;                  // local_4c9
    uint32_t case4_cached_result = (uint32_t)-1; // local_40c
    uint8_t  case4_pending = 1;                  // local_4a5

    // ---- Friendly-fire / hijack witness handling: only when both units resolved to actors
    // and their teams are enemies (per team_relationship_flags's "secondary_bits" 10x10 map).
    if (self_actor != (actor *)0 && other_actor != (actor *)0)
    {
        int16_t self_team = (int16_t)self_team_packed;
        int16_t other_team = (int16_t)other_team_packed;
        if (self_team != other_team && self_team >= 0 && self_team < 10 &&
            other_team >= 0 && other_team < 10)
        {
            int32_t pair_index = other_team + self_team * 10;
            uint32_t *secondary_bits = (uint32_t *)(&team_relationship_flags + 0x94);
            if ((secondary_bits[pair_index >> 5] & (1u << (pair_index & 0x1f))) != 0)
            {
                uint8_t witness_is_hostile = 0;
                uint8_t should_shout = 0;

                if (event_code == 0)
                {
                    if (reason == 3)
                    {
                        witness_is_hostile = 1;
                        should_shout = 1;
                        reason = 4;
                    }
                    else
                    {
                        uint32_t witness_object;
                        if (self_encounter != (encounter *)0)
                        {
                            if (self_encounter->unknown_46 == 0 &&
                                self_encounter->unknown_50 != (datum_index)k_datum_index_none &&
                                (int32_t)self_encounter->unknown_50 < 0x10e)
                                witness_is_hostile = 0;
                            else
                                witness_is_hostile = 1;
                        }
                        witness_object = ai_communication_select_speaker_by_team(0, unit_index, object_a, 18.0f, 0, 6,
                                                       (uint32_t)-1, -1, -1, 0);
                        case2_cached_result = witness_object;
                        if (witness_object != (uint32_t)-1)
                        {
                            case2_pending = 0;
                            should_shout = 1;
                        }
                        if (2 < seat_or_difficulty &&
                            (seat_or_difficulty < 5 || seat_or_difficulty == 9) &&
                            !witness_is_hostile)
                            should_shout = 0;
                        if (seat_or_difficulty == 3)
                            witness_is_hostile = 0;
                        else if (witness_is_hostile)
                            reason = 4;
                    }

                    if (should_shout != 0)
                    {
                        uint8_t shout_flag;
                        should_shout = 0;
                        shout_flag = team_pair_override_adjust_counter(self_team_packed, witness_is_hostile, &should_shout);
                        if (should_shout != 0)
                            ai_mark_recognized_objects_for_reaction(other_team_packed, self_team_packed, shout_flag);
                    }
                }
                if (teams_are_enemies((int16_t)self_team_packed, (int16_t)other_team_packed) != 0) reason = 4; // 0x42d7ac: CX [esp+0x14], DX [esp+0x4c] (UNSURE which local is which)
            }
        }
    }

    // ---- Phase B: an 8-byte "burst/vitality" flag block, either from self_actor's own tally
    // or, if self_actor's encounter has a byte at +0x45 set, from the encounter's own fields.
    {
    uint8_t combat_flags[8];
    memset(combat_flags, 0, sizeof(combat_flags));
    if (self_actor == (actor *)0)
    {
        combat_flags[0] = 1;
        combat_flags[4] = 1;
    }
    else
    {
        int32_t danger_object;
        uint8_t byte44, byte45;
        uint8_t base_flag; // whether unknown_274's low byte is clear
        int16_t burst_grade = self_actor->unknown_6e;

        if (self_encounter == (encounter *)0)
        {
            danger_object = self_actor->unknown_278;
            byte44 = (self_actor->unknown_27c == 0) ? 0 : 1;
            byte45 = 0;
        }
        else
        {
            danger_object = (int32_t)self_encounter->unknown_50;
            byte44 = (*(uint8_t *)((uint8_t *)self_encounter + 0x44) == 0) ? 0 : 1;
            byte45 = *(uint8_t *)((uint8_t *)self_encounter + 0x45);
        }
        base_flag = (self_actor->unknown_274[0] == 0) ? 1 : 0;

        combat_flags[0] = base_flag;
        if (self_encounter == (encounter *)0)
        {
            if (self_actor->unknown_27c != 0 || self_actor->unknown_278 != -1)
                combat_flags[0] = 1;
        }
        else
        {
            if (byte44 != 0)
                combat_flags[0] = 1;
        }
        if (danger_object == -1 || danger_object > 0xb3) combat_flags[0] = 1;

        if (burst_grade < 3 && (danger_object == -1 || danger_object > 0x4a) &&
            (byte44 != 0 || burst_grade > 0))
            combat_flags[0] = 1;

        combat_flags[4] = (burst_grade < 6) ? 1 : 0;
        if (self_actor->target_combat_status > 9)
            combat_flags[4] = byte45 != 0 ? byte44 : combat_flags[4];
    }

    // ---- Phase C: build reason_buckets[5] from `reason`.
    {
    uint8_t reason_buckets[5];
    memset(reason_buckets, 0, sizeof(reason_buckets));
    if (reason != -1)
    {
        reason_buckets[reason] = 1;
        if (reason == 4) reason_buckets[3] = 1;
    }

    // ---- Phase D: walk the chain of ai_communication_lines rows sharing event_code, scoring
    // each candidate that passes its filters. This section stays close to Ghidra's own control
    // flow and variable roles (documented per-line) rather than renaming everything, because
    // several call-argument identities here were only spot-checked, not exhaustively re-derived
    // from disassembly -- see the file header's UNSURE list.
    if (ai_globals_ptr->communication_valid != 0)
    {
    int16_t chain_start = ai_communication_event_index[(int16_t)event_code];
    if (chain_start != -1 && ai_communication_lines[chain_start].event_id == (int16_t)event_code)
    {
    ai_communication_candidate candidates[16];
    int32_t candidate_count = 0;
    float total_weight = 0.0f;
    uint8_t any_global_broadcast = 0;
    int32_t entry_index = chain_start;
    const ai_communication_line_definition *row = &ai_communication_lines[chain_start];

    do
    {
        int16_t class_index = row->class_index;

        if (((row->required_kind != -1 && reason_buckets[row->required_kind] == 0) ||
             (current_tick < DAT_00725204 && class_index < 6 && (row->flags & 0x40) == 0) ||
             (row->relationship_gate != -1 && combat_flags[row->relationship_gate] == 0) ||
             (row->source_type_mask != 0xffff &&
              (unit_index == (datum_index)k_datum_index_none ||
               (row->source_type_mask & (uint16_t)self_type_mask) == 0)) ||
             (row->target_type_mask != 0xffff &&
              (object_a == (datum_index)k_datum_index_none ||
               (row->target_type_mask & (uint16_t)other_type_mask) == 0)) ||
             (row->required_seat != -1 && row->required_seat != seat_or_difficulty)))
            goto next_row;

        {
        int16_t tier = ai_communication_class_tier[class_index]; // local_488
        uint32_t other_object_index = 0;        // uVar20: the resolved participant's object
        actor *direction_actor = (actor *)0;    // iVar19 / local_494, when it holds an actor pointer
        uint32_t speaker_actor_index = (uint32_t)-1; // local_474
        uint32_t order_fallback = (uint32_t)-1;      // local_47c
        int16_t participant_flags[2];                // puVar27 target: self_capability/other_capability
        uint8_t have_participant_flags = 0;
        uint8_t skip_target_object_search = 0;  // local_4ca

        participant_flags[0] = 0; participant_flags[1] = 0;

        switch (row->participant_selector)
        {
        case 0:
            have_participant_flags = 1;
            participant_flags[0] = (int16_t)self_capability[0];
            participant_flags[1] = (int16_t)self_capability[1];
            speaker_actor_index = unit_index;
            order_fallback = object_b;
            other_object_index = unit_index;
            direction_actor = self_actor;
            break;
        case 1:
            have_participant_flags = 1;
            participant_flags[0] = (int16_t)other_capability[0];
            participant_flags[1] = (int16_t)other_capability[1];
            speaker_actor_index = object_a;
            order_fallback = unit_index;
            other_object_index = object_a;
            direction_actor = other_actor;
            break;
        case 2:
        {
            uint32_t search_result;
            order_fallback = object_a;
            search_result = case2_cached_result;
            if (case2_pending)
            {
                uint8_t line_flags = row->flags;
                uint8_t base_flag = line_flags & 1;
                uint8_t combined_flag = base_flag | 2;
                if (line_flags & 0x10) combined_flag = base_flag | 6;
                if (line_flags & 0x20) combined_flag |= 8;
                if (self_encounter == (encounter *)0)
                {
                    search_result = ai_communication_select_speaker_by_team(1, unit_index, object_a, 10.0f, event_code,
                                                  (uint32_t)tier, (uint32_t)row->table_arg_a,
                                                  row->table_arg_a, row->table_arg_b,
                                                  combined_flag | 0x10);
                    case2_pending = 0;
                }
                else
                {
                    search_result = ai_communication_select_speaker_in_reference(10.0f, event_code, (uint32_t)tier,
                                                  (uint32_t)row->table_arg_a, row->table_arg_a,
                                                  row->table_arg_b);
                    case2_pending = 0;
                    // UNSURE: the encounter-present branch leaves local_4b8 (other_object_index)
                    // as whatever it already was; reproduced by simply not writing it here.
                }
                case2_cached_result = search_result;
            }
            speaker_actor_index = search_result;
            goto assign_direction_from_speaker;
        }
        default:
            goto next_row;
        case 4:
        {
            uint32_t search_result = case4_cached_result;
            order_fallback = object_a;
            if (case4_pending)
            {
                uint8_t line_flags = row->flags;
                uint8_t base_flag = line_flags & 1;
                uint8_t combined_flag = base_flag | 2;
                if (line_flags & 0x10) combined_flag = base_flag | 6;
                if (line_flags & 0x20) combined_flag |= 8;
                search_result = ai_communication_select_speaker_by_team(2, unit_index, object_a, 12.0f, event_code,
                                              (uint32_t)tier, (uint32_t)row->table_arg_a,
                                              row->table_arg_a, row->table_arg_b, combined_flag);
                case4_pending = 0;
                case4_cached_result = search_result;
            }
        assign_direction_from_speaker:
            speaker_actor_index = search_result;
            have_participant_flags = 0; // puVar27 = local_4ac, not written by this rewrite
            if (search_result != (uint32_t)-1)
            {
                actor *sa = &((actor *)actor_data->data)[search_result & 0xffff];
                other_object_index = sa->unit_index;
                direction_actor = sa;
            }
            else
            {
                goto next_row;
            }
            break;
        }
        }

        // ---- shared post-selection gating (switchD_0042dd60_caseD_3) ----
        {
        uint8_t use_fallback_search = 0; // bVar14 / local_4ca after this block
        uint8_t reject_candidate = 0;
        object *result_object = (object *)0;

        if (other_object_index == (datum_index)k_datum_index_none)
        {
            reject_candidate = 1;
        }
        else
        {
            result_object = ((object_header *)object_data->data)[other_object_index & 0xffff].data;
            if ((*(uint8_t *)((uint8_t *)result_object + 0x106) & 4) != 0 ||
                *(int16_t *)((uint8_t *)result_object + 0xb4) == 1)
            {
                reject_candidate = 1;
            }
            else if (*(int32_t *)((uint8_t *)result_object + 0x218) != -1 &&
                     *(int32_t *)((uint8_t *)result_object + 0x1f4) == -1)
            {
                if ((row->flags & 8) == 0) reject_candidate = 1;
                else use_fallback_search = 1;
            }
        }

        if (!((direction_actor == (actor *)0 ||
               (direction_actor->awareness_level != 0 &&
                (direction_actor->mode != 0xb ||
                 *(int8_t *)(direction_actor->mode_data + 4) != 0))) &&
              !reject_candidate))
            goto next_row;

        {
        float object_weight = 1.0f;
        uint32_t object_result = other_object_index; // ai_select_communication_target's fallback search result (local_4b0)

        if (use_fallback_search)
        {
            object_result = ai_select_communication_target(order_fallback, order_fallback, row->table_arg_a,
                                          object_b, &object_weight);
            if (order_fallback == unit_index)
                case2_cached_result = object_result; // mirrors "local_45c = local_4b0"
            if (object_result == (uint32_t)-1) goto next_row;
        }

        if (row->capability_index != -1 && have_participant_flags &&
            ((const int16_t *)participant_flags)[row->capability_index] == 0)
            goto next_row;

        {
        float scale, recency_fraction = 1.0f;
        uint32_t cone_index = 0;

        if (!use_fallback_search)
        {
            scale = ai_communication_rate_player_proximity(1, 0, 0);
            if (scale == 0.0f) goto next_row;
            if (speaker_actor_index != (uint32_t)-1)
            {
                int16_t side = -1;
                actor *sa2;
                uint16_t sa2_flags;
                object *sobj = ((object_header *)object_data->data)[speaker_actor_index & 0xffff].data;
                datum_index sa2_index = *(datum_index *)((uint8_t *)sobj + 0x1f4);
                sa2 = &((actor *)actor_data->data)[sa2_index & 0xffff];
                sa2_flags = *(uint16_t *)((uint8_t *)actor_type_procs[sa2->type] + 4);
                if ((sa2_flags & 2) == 0) { if ((sa2_flags & 4) != 0) side = 1; }
                else side = 0;

                if (side != -1)
                {
                    uint32_t flag_idx = (uint32_t)(scale < 2.0f) +
                                         (uint32_t)(order_fallback + side * 8) * 2;
                    if (recent_dir_flag[flag_idx & 0x1f] != 0) goto next_row;
                    cone_index = (uint16_t)recent_dir_value[flag_idx & 0x1f];
                    if ((int16_t)tier < 7)
                    {
                        int32_t *entry = (int32_t *)(communication_line_base +
                            (side + entry_index * 2) * 8);
                        int32_t last_a = entry[0];
                        if (last_a != -1)
                        {
                            int32_t delta = current_tick - last_a;
                            recency_fraction = (float)delta * 0.0011111111f;
                            if (recency_fraction < 0.0f) recency_fraction = 0.0f;
                            else if (recency_fraction > 1.0f) recency_fraction = 1.0f;
                        }
                        {
                            int32_t last_b = entry[1];
                            if (last_b != -1)
                            {
                                int32_t delta2 = last_b - current_tick;
                                if (scale < 2.0f) delta2 += 30;
                                if (delta2 > 0) goto next_row;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            cone_index = (uint16_t)DAT_00655914[order_fallback];
            scale = 2.0f;
        }

        {
        int16_t weight_i = (int16_t)cone_index;      // __ftol placeholder (see UNSURE)
        int16_t weight_scalar = (int16_t)cone_index; // second __ftol placeholder
        uint32_t look_target = (uint32_t)-1;
        int16_t follow_up_order;
        float weight;

        if (self_type_mask == 1 && !use_fallback_search)
            weight_i = (int16_t)(weight_i + 30);

        switch (row->look_target_selector)
        {
        case 1: look_target = unit_index; break;
        case 2: look_target = other_object_index; break;
        case 3: look_target = order_fallback; break;
        case 4:
            if (self_actor == (actor *)0 || self_actor->firing_position_index < 1)
                look_target = (uint32_t)-1;
            // UNSURE: candidate.look_node_b source when this case applies
            break;
        default:
            look_target = (uint32_t)-1;
            break;
        }

        follow_up_order = row->fallback_order;
        if (follow_up_order == -1 || follow_up_order == 1)
            follow_up_order = ai_communication_class_order[(int16_t)cone_index];

        weight = scale * row->probability * recency_fraction * object_weight *
                 (float)weight_i * (float)weight_scalar; // UNSURE: exact operand pairing

        if (weight > 0.0f && candidate_count < 16)
        {
            ai_communication_candidate *cand = &candidates[candidate_count];
            uint16_t look_node_a = 0, look_node_b = 0;
            uint8_t broadcast_bit;

            if (look_target != (uint32_t)-1)
            {
                look_node_a = 1;
                look_node_b = (uint16_t)look_target;
            }

            memset(cand, 0, sizeof(*cand));
            cand->entry_index = (uint16_t)entry_index;
            cand->already_played = 0;
            cand->score = weight;
            cand->participant_object_index = order_fallback;
            cand->speaker_actor_index = speaker_actor_index;
            cand->raw_field_0a = row->table_arg_b;
            cand->other_object_index = other_object_index;
            cand->object_result = object_result;
            cand->direction_class = row->table_arg_a;
            cand->line_delay_low = (int16_t)recency_fraction;
            cand->look_marker = (int16_t)look_node_b;
            cand->result_index = (int16_t)(weight_i + weight_scalar);
            cand->follow_up_order = follow_up_order;
            cand->look_node_a = (int16_t)look_node_a;
            cand->look_node_b = (int16_t)look_node_b;
            cand->order_target = look_target;
            cand->order_fallback = look_target;
            cand->tier = tier;
            broadcast_bit = (uint8_t)((row->flags >> 1) & 1);
            cand->global_broadcast = broadcast_bit;
            if (broadcast_bit) any_global_broadcast = 1;
            candidate_count++;
            total_weight += weight;
        }
        }
        }
        }
        }
        }

    next_row:
        entry_index++;
        row++;
    } while (row->event_id == (int16_t)event_code);

    if (candidate_count >= 1)
    {
        ai_communication_candidate *chosen = &candidates[0];

        // ---- Phase E: weighted random selection among scored candidates.
        if (any_global_broadcast)
        {
            int32_t i;
            total_weight = 0.0f;
            for (i = 0; i < candidate_count; i++)
            {
                if (!candidates[i].global_broadcast) candidates[i].score = 0.0f;
                total_weight += candidates[i].score;
            }
        }
        if (candidate_count > 1)
        {
            float draw, running = 0.0f;
            int32_t i, picked = 0;
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            draw = (float)(uint16_t)(random_seed_global >> 0x10) * 1.5259022e-05f * total_weight;
            for (i = 0; i < candidate_count - 1; i++)
            {
                running += candidates[i].score;
                if (draw <= running) break;
                picked++;
            }
            chosen = &candidates[picked];
        }

        // ---- Phase F: assemble the queue descriptor and dispatch (verified against
        // objdump -d 0x42e520..0x42e930; see file header for the three corrected call sites).
        // FIXED (0x42e60c..0x42e70c): the 0x20-byte body (at [esp+0xa4]) is built before the already-played test
        // and used by both branches: that path hands &body to ai_propagate_communication_reaction and &body + 8 to
        // ai_communication_play_event_line (the draft passed NULL to both). The [ebx+0x08] tier switch before it only
        // reaches 0x42f9e0 when DL is clear, and DL is always 1 there, so it never calls it.
        // 0x10-byte header + 0x20-byte body, contiguous on the stack in the original
        // (Ghidra's local_3ec/local_3e8/.../local_3dc and local_438/.../local_41c).
        struct
        {
            int16_t tier;
            int16_t direction_class;
            uint32_t order_fallback;
            int16_t look_marker;
            int16_t result_index;
            int16_t marker_length; // constant 0x18
            int16_t pad_0e;
            uint32_t other_object_index;      // local_438
            uint32_t event_and_entry;         // local_434: low16=event_code, hi16=entry_index
            uint32_t seat_flag_pad;            // local_430: byte0-1=seat_or_difficulty, byte2=1
            uint32_t look_node_pair;           // local_42c: {look_node_a, look_node_b}
            uint32_t order_target_raw;         // local_428
            uint32_t object_c_masked;          // local_424
            uint32_t extra_data_0;             // local_420
            uint32_t extra_data_1;             // local_41c
        } queue_descriptor;

        queue_descriptor.tier = chosen->tier;
        queue_descriptor.direction_class = chosen->direction_class;
        queue_descriptor.order_fallback = chosen->order_fallback;
        queue_descriptor.look_marker = chosen->look_marker;
        queue_descriptor.result_index = chosen->result_index;
        queue_descriptor.marker_length = 0x18;
        queue_descriptor.pad_0e = 0;
        queue_descriptor.other_object_index = chosen->other_object_index;
        queue_descriptor.event_and_entry = ((uint32_t)chosen->entry_index << 16) | (uint16_t)event_code;
        queue_descriptor.seat_flag_pad = ((uint32_t)1 << 16) | (uint16_t)seat_or_difficulty;
        queue_descriptor.look_node_pair = ((uint32_t)(uint16_t)chosen->look_node_b << 16) |
                                           (uint16_t)chosen->look_node_a;
        queue_descriptor.order_target_raw = chosen->order_target;
        queue_descriptor.object_c_masked = ((object_c == 0xffff) ? 0u : (uint32_t)-1) &
                                            (uint32_t)object_c;
        if (extra_data == (uint32_t *)0)
        {
            queue_descriptor.extra_data_0 = 0;
            queue_descriptor.extra_data_1 = 0;
        }
        else
        {
            queue_descriptor.extra_data_0 = extra_data[0];
            queue_descriptor.extra_data_1 = extra_data[1];
        }

        if (chosen->already_played != 0)
        {
            ai_propagate_communication_reaction(chosen->participant_object_index, &queue_descriptor.other_object_index);
            ai_communication_play_event_line(chosen->participant_object_index, chosen->direction_class, 1,
                         chosen->object_result, (uint8_t *)&queue_descriptor.other_object_index + 8);
        }
        else
        {
            unit_commit_speech(chosen->participant_object_index, &queue_descriptor);

            if (chosen->raw_field_0a != -1)
            {
                object *speaker_object = ((object_header *)object_data->data)[
                    chosen->participant_object_index & 0xffff].data;
                float look_dir_x = *(float *)((uint8_t *)speaker_object + 0x74);
                float look_dir_y = *(float *)((uint8_t *)speaker_object + 0x78);

                if (chosen->look_node_b != -1)
                {
                    object_marker marker;
                    float pos_a_x, pos_a_y, dx, dy, dist;
                    object_get_node_local_transform(chosen->participant_object_index, ai_marker_name_a,
                                                     &marker, 1);
                    pos_a_x = marker.node_transform.position.x;
                    pos_a_y = marker.node_transform.position.y;
                    object_get_node_local_transform((datum_index)(uint16_t)chosen->look_node_a,
                                                     ai_marker_name_a, &marker, 1);
                    dx = marker.node_transform.position.x - pos_a_x;
                    dy = marker.node_transform.position.y - pos_a_y;
                    dist = (float)sqrt((double)(dx * dx + dy * dy));
                    if ((float)fabs((double)dist) >= 0.0001f)
                    {
                        look_dir_x = dx * (1.0f / dist);
                        look_dir_y = dy * (1.0f / dist);
                    }
                }
                {
                    float look_dir[2];
                    look_dir[0] = look_dir_x;
                    look_dir[1] = look_dir_y;
                    unit_try_start_scripted_action_animation(chosen->participant_object_index, chosen->raw_field_0a, look_dir);
                }
            }
            if (chosen->speaker_actor_index != (uint32_t)-1)
            {
                actor_issue_order_or_vocalize(/*EAX*/ -1, /*ECX*/ chosen->speaker_actor_index,
                             /*EBX*/ chosen->speaker_actor_index, /*EDI*/ chosen->other_object_index,
                             /*stack*/ 9, chosen->follow_up_order);
            }

            ai_communication_record_line_played(chosen->participant_object_index, chosen->tier,
                                                 (int16_t)chosen->entry_index, -1);
        }
    }
    }
    }
    }
    }
    }
    }
}

#if 0
Original Ghidra decompilation (0x42d340):

/* WARNING: Removing unreachable block (ram,0x0042e6ee) */

void ai_communication_broadcast
               (undefined4 param_1,uint param_2,uint param_3,short param_4,undefined4 param_5,
               ushort param_6,undefined4 *param_7)

{
  int *piVar1;
  short sVar2;
  float fVar3;
  undefined4 uVar4;
  undefined1 uVar5;
  char cVar6;
  byte bVar7;
  ushort uVar8;
  short sVar9;
  short sVar10;
  float fVar11;
  uint uVar12;
  int iVar13;
  byte bVar14;
  short sVar15;
  ushort uVar16;
  short sVar17;
  float *pfVar18;
  int iVar19;
  uint uVar20;
  int iVar21;
  int iVar22;
  float *pfVar23;
  short *psVar24;
  undefined4 *puVar25;
  ushort *puVar26;
  undefined2 *puVar27;
  undefined4 *puVar28;
  bool bVar29;
  float10 fVar30;
  byte local_4ca;
  char local_4c9;
  uint local_4c8;
  undefined4 local_4c4;
  undefined4 local_4c0;
  int *local_4bc;
  uint local_4b8;
  char local_4b1;
  uint local_4b0;
  undefined2 *local_4ac;
  char local_4a5;
  undefined2 local_4a4 [2];
  undefined2 local_4a0;
  float local_49c;
  float local_498;
  int local_494;
  undefined4 local_490;
  undefined1 *local_48c;
  uint local_488;
  float *local_484;
  uint local_480;
  uint local_47c;
  uint local_478;
  uint local_474;
  float local_470;
  undefined1 *local_46c;
  float local_468;
  uint local_464;
  int local_460;
  uint local_45c;
  uint local_458;
  float local_454;
  float local_450;
  uint local_44c;
  char local_448 [8];
  int local_440;
  uint local_43c;
  undefined4 local_438;
  undefined4 local_434;
  undefined4 local_430;
  undefined4 local_42c;
  float local_428;
  undefined4 local_424;
  undefined4 *local_420;
  undefined4 local_41c;
  float local_418;
  int local_414;
  int local_410;
  uint local_40c;
  uint local_408;
  float local_404;
  float local_400;
  undefined4 local_3fc;
  float local_3f8;
  float local_3f4;
  undefined4 local_3f0;
  ushort local_3ec [2];
  float local_3e8;
  undefined2 local_3e4;
  undefined2 local_3e2;
  undefined2 local_3e0;
  undefined4 local_3dc [20];
  float local_38c;
  float local_388;
  undefined4 local_384;
  float local_380;
  byte local_37c [16];
  uint auStack_36c [4];
  short asStack_35c [4];
  uint local_354 [2];
  undefined2 local_34c [422];

  iVar19 = 0;
  iVar22 = 0;
  local_440 = *(int *)(DAT_006f1d6c + 0xc);
  local_470 = 0.0;
  local_468 = 0.0;
  local_4b1 = '\0';
  local_44c = 0xffffffff;
  local_4bc = (int *)0x0;
  local_43c = 0xffffffff;
  local_414 = 0;
  local_408 = 0xffffffff;
  local_410 = 0;
  local_45c = 0xffffffff;
  local_40c = 0xffffffff;
  local_490 = 0xffffffff;
  local_4c8 = 0xffffffff;
  local_464 = 0;
  local_458 = 0;
  local_4c9 = '\x01';
  local_4a5 = '\x01';
  if (param_4 == -1) {
    param_4 = 0;
  }
  if ((short)param_5 == -1) {
    param_5 = 0;
  }
  local_4a4[0] = 0;
  local_4a0 = 0;
  if (param_2 != 0xffffffff) {
    iVar19 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
    sVar15 = *(short *)(iVar19 + 0xb8);
    local_43c = *(uint *)(iVar19 + 500);
    local_490 = CONCAT22(0xffff,sVar15);
    local_464 = 0;
    if (sVar15 == 1) {
      local_464 = 1;
    }
    else if (sVar15 == 2) {
      local_464 = 2;
    }
    else if (sVar15 == 3) {
      local_464 = 4;
    }
    else if (sVar15 == 4) {
      local_464 = 0x38;
    }
    else if (sVar15 == 5) {
      local_464 = 0x40;
    }
    if (local_43c == 0xffffffff) {
      if (*(int *)(iVar19 + 0x218) != -1) {
        local_464 = 1;
      }
    }
    else {
      iVar13 = *(int *)(DAT_00880360 + 0x34);
      iVar21 = (local_43c & 0xffff) * 0x724;
      local_44c = *(uint *)(iVar21 + 0x34 + iVar13);
      local_414 = iVar21 + iVar13;
      local_464 = (uint)*(ushort *)((&PTR_PTR_006853b8)[*(short *)(iVar21 + 4 + iVar13)] + 4);
      if (*(char *)(local_414 + 0x245) < '\x01') {
        if ('\0' < *(char *)(local_414 + 0x200)) {
          local_4a0._1_1_ = 0;
          goto LAB_0042d4e2;
        }
      }
      else {
        local_4a0._1_1_ = 1;
LAB_0042d4e2:
        local_4a0 = CONCAT11(local_4a0._1_1_,1);
      }
      if (local_44c != 0xffffffff) {
        local_4bc = (int *)((local_44c & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34));
      }
    }
  }
  iVar13 = local_414;
  if (param_3 != 0xffffffff) {
    iVar22 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_3 & 0xffff) * 0xc);
    local_408 = *(uint *)(iVar22 + 500);
    uVar16 = *(ushort *)(iVar22 + 0xb8);
    local_4c8 = (uint)uVar16;
    local_458 = 0;
    if (uVar16 == 1) {
      local_458 = 1;
    }
    else if (uVar16 == 2) {
      local_458 = 2;
    }
    else if (uVar16 == 3) {
      local_458 = 4;
    }
    else if (uVar16 == 4) {
      local_458 = 0x38;
    }
    else if (uVar16 == 5) {
      local_458 = 0x40;
    }
    if (local_408 == 0xffffffff) {
      if (*(int *)(iVar22 + 0x218) != -1) {
        local_458 = 1;
      }
    }
    else {
      iVar21 = (local_408 & 0xffff) * 0x724;
      local_410 = iVar21 + *(int *)(DAT_00880360 + 0x34);
      local_458 = (uint)*(ushort *)
                         ((&PTR_PTR_006853b8)
                          [*(short *)(iVar21 + 4 + *(int *)(DAT_00880360 + 0x34))] + 4);
      if (*(char *)(local_410 + 0x245) < '\x01') {
        if ('\0' < *(char *)(local_410 + 0x200)) {
          local_4a4[0] = 1;
        }
      }
      else {
        local_4a4[0] = 0x101;
      }
    }
  }
  sVar15 = (short)param_5;
  if ((iVar19 != 0) && (iVar22 != 0)) {
    sVar9 = (short)local_490;
    if ((sVar9 != (short)local_4c8) &&
       ((((-1 < sVar9 && (sVar9 < 10)) && (-1 < (short)local_4c8)) &&
        (((short)local_4c8 < 10 &&
         (iVar19 = (int)(short)local_4c8 + sVar9 * 10,
         (*(uint *)(DAT_006b0b84 + 0x94 + (iVar19 >> 5) * 4) & 1 << ((byte)iVar19 & 0x1f)) != 0)))))
       ) {
      bVar29 = false;
      local_4ca = 0;
      if ((short)param_1 == 0) {
        if (param_4 == 3) {
          bVar29 = true;
          local_4ca = 1;
LAB_0042d759:
          param_4 = 4;
        }
        else {
          if (local_4bc != (int *)0x0) {
            if (((*(char *)((int)local_4bc + 0x46) == '\0') &&
                (*(int *)((int)local_4bc + 0x50) != -1)) &&
               (*(int *)((int)local_4bc + 0x50) < 0x10e)) {
              bVar29 = false;
            }
            else {
              bVar29 = true;
            }
          }
          local_45c = FUN_004300d0(0,param_2,param_3,0x41900000,0,6,0xffffffff,0xffffffff,0xffffffff
                                   ,0);
          if (local_45c != 0xffffffff) {
            local_4c9 = '\0';
            local_4ca = 1;
          }
          if ((2 < sVar15) && (((sVar15 < 5 || (sVar15 == 9)) && (!bVar29)))) {
            local_4ca = 0;
          }
          if (sVar15 == 3) {
            bVar29 = false;
          }
          else if (bVar29) goto LAB_0042d759;
        }
        uVar4 = local_490;
        uVar20 = local_4c8;
        if (local_4ca != 0) {
          local_4ca = 0;
          uVar5 = FUN_0045bfc0(local_490,bVar29,&local_4ca);
          local_488 = CONCAT31(local_488._1_3_,uVar5);
          if (local_4ca != 0) {
            FUN_0042ba80(uVar20,uVar4,local_488);
          }
        }
      }
      cVar6 = FUN_0045bd50();
      if (cVar6 != '\0') {
        param_4 = 4;
      }
    }
  }
  if (iVar13 == 0) {
    local_4c4 = 2.3694278e-38;
    local_4c0 = (float)CONCAT22(local_4c0._2_2_,0x101);
  }
  else {
    if (local_4bc == (int *)0x0) {
      bVar29 = *(char *)(iVar13 + 0x274) == '\0';
      local_4c4._0_3_ = (uint3)bVar29;
      cVar6 = *(char *)(iVar13 + 0x27c);
      if (cVar6 == '\0') {
        local_4c4._0_2_ = CONCAT11(1,bVar29);
        local_4c4._0_3_ = (uint3)(ushort)local_4c4;
        if (*(int *)(iVar13 + 0x278) == -1) goto LAB_0042d812;
      }
      else {
LAB_0042d812:
        local_4c4._0_3_ = (uint3)local_4c4 & 0xff;
      }
      if (((*(int *)(iVar13 + 0x270) == -1) || (*(int *)(iVar13 + 0x278) == -1)) ||
         (0xb3 < *(int *)(iVar13 + 0x278))) {
        local_4c4._0_3_ = CONCAT12(1,(ushort)local_4c4);
      }
      sVar9 = *(short *)(iVar13 + 0x6e);
      if (((sVar9 < 3) && ((*(int *)(iVar13 + 0x278) == -1 || (0x4a < *(int *)(iVar13 + 0x278)))))
         && ((cVar6 != '\0' || (0 < sVar9)))) {
        local_4c4 = (float)CONCAT13(1,(uint3)local_4c4);
      }
      else {
        local_4c4 = (float)(uint)(uint3)local_4c4;
      }
      local_4c0 = (float)CONCAT31(local_4c0._1_3_,sVar9 < 6);
      if (9 < *(short *)(iVar13 + 0x268)) {
LAB_0042d924:
        local_4c0._0_2_ = CONCAT11(1,(byte)local_4c0);
        if (cVar6 != '\0') goto LAB_0042d930;
      }
    }
    else {
      iVar19 = *(int *)((int)local_4bc + 0x50);
      bVar29 = *(char *)(iVar13 + 0x274) == '\0';
      local_4c4._0_3_ = (uint3)bVar29;
      if (iVar19 == -1) {
LAB_0042d8ae:
        local_4c4._0_3_ = (uint3)local_4c4 & 0xff;
      }
      else {
        local_4c4._0_2_ = CONCAT11(1,bVar29);
        local_4c4._0_3_ = (uint3)(ushort)local_4c4;
        if (*(char *)((int)local_4bc + 0x44) != '\0') goto LAB_0042d8ae;
      }
      if ((iVar19 == -1) || (0xb3 < iVar19)) {
        local_4c4._0_3_ = CONCAT12(1,(ushort)local_4c4);
        if (*(char *)((int)local_4bc + 0x44) == '\0') goto LAB_0042d8cb;
      }
      else {
LAB_0042d8cb:
        local_4c4._0_3_ = (uint3)local_4c4 & 0xffff;
      }
      sVar9 = *(short *)(iVar13 + 0x6e);
      if (((sVar9 < 3) && ((iVar19 == -1 || (0x4a < iVar19)))) &&
         ((*(char *)((int)local_4bc + 0x44) != '\0' || (0 < sVar9)))) {
        local_4c4 = (float)CONCAT13(1,(uint3)local_4c4);
      }
      else {
        local_4c4 = (float)(uint)(uint3)local_4c4;
      }
      if ((sVar9 < 6) && ((iVar19 == -1 || (0x4a < iVar19)))) {
        local_4c0 = (float)CONCAT31(local_4c0._1_3_,1);
      }
      else {
        local_4c0 = (float)((uint)local_4c0._1_3_ << 8);
      }
      if (*(char *)((int)local_4bc + 0x45) != '\0') {
        cVar6 = *(char *)((int)local_4bc + 0x44);
        goto LAB_0042d924;
      }
    }
    local_4c0._0_2_ = (ushort)(byte)local_4c0;
  }
LAB_0042d930:
  local_448[0] = '\0';
  local_448[1] = '\0';
  local_448[2] = '\0';
  local_448[3] = '\0';
  local_448[4] = 0;
  if ((param_4 != -1) && (local_448[param_4] = '\x01', param_4 == 4)) {
    local_448[3] = 1;
  }
  local_438 = 0.0;
  local_434 = 0;
  local_430 = 0;
  local_42c = 0;
  local_428 = 0.0;
  local_424 = 0;
  local_420 = (undefined4 *)0x0;
  local_41c = 0;
  puVar26 = local_3ec;
  for (iVar19 = 0x10; iVar19 != 0; iVar19 = iVar19 + -1) {
    puVar26[0] = 0;
    puVar26[1] = 0;
    puVar26 = puVar26 + 2;
  }
  local_484 = &local_3e8;
  local_48c = (undefined1 *)((int)&local_438 + 1);
  local_4bc = (int *)(DAT_00880354 + 0x1c);
  local_480 = 0;
  local_494 = 2;
  do {
    iVar13 = local_440;
    uVar20 = local_480;
    iVar19 = local_4bc[-2];
    iVar22 = *local_4bc;
    local_498 = 7.00649e-45;
    fVar11 = (float)(local_440 - local_4bc[2] & (local_440 - local_4bc[2] < 0) - 1);
    *(short *)((int)&local_49c + local_480) = SUB42(fVar11,0);
    local_450 = fVar11;
    uVar12 = iVar13 - iVar22;
    fVar11 = (float)(uVar12 & ((int)uVar12 < 0) - 1);
    *(short *)((int)&local_488 + uVar20) = SUB42(fVar11,0);
    pfVar18 = local_484;
    local_454 = fVar11;
    uVar12 = iVar13 - iVar19;
    uVar12 = uVar12 & ((int)uVar12 < 0) - 1;
    *(short *)((int)&local_4b0 + uVar20) = (short)uVar12;
    local_488 = uVar12;
    local_48c[-1] = 1;
    *local_48c = 1;
    pfVar23 = (float *)&DAT_00655954;
    local_46c = local_48c + 1;
    do {
      local_4ac = (undefined2 *)0x2;
      do {
        bVar29 = false;
        uVar16 = 0;
        if (0.0 < pfVar23[-1]) {
          local_4b0 = (uint)(short)local_488;
          uVar8 = __ftol();
          if (0 < (short)uVar8) {
            bVar29 = true;
            uVar16 = ((short)uVar8 < 0) - 1 & uVar8;
          }
        }
        if (0.0 < *pfVar23) {
          local_4b0 = (uint)local_454._0_2_;
          uVar8 = __ftol();
          if ((0 < (short)uVar8) && (bVar29 = true, (short)uVar16 <= (short)uVar8)) {
            uVar16 = uVar8;
          }
        }
        if (pfVar23[2] <= 0.0) {
LAB_0042db3b:
          if (bVar29) goto LAB_0042db3f;
        }
        else {
          local_4b0 = (uint)local_450._0_2_;
          uVar8 = __ftol();
          if ((short)uVar8 < 1) goto LAB_0042db3b;
          bVar29 = true;
          if ((short)uVar16 <= (short)uVar8) {
            uVar16 = uVar8;
          }
LAB_0042db3f:
          if ((0.0 < pfVar23[3]) &&
             (local_4b0 = (int)(short)*(ushort *)pfVar18,
             (float)(int)(short)*(ushort *)pfVar18 < pfVar23[3] * 30.0)) {
            bVar29 = false;
          }
        }
        *(ushort *)pfVar18 = uVar16;
        pfVar23 = pfVar23 + 5;
        pfVar18 = (float *)((int)pfVar18 + 2);
        *local_46c = bVar29;
        local_46c = local_46c + 1;
        local_4ac = (undefined2 *)((int)local_4ac + -1);
      } while (local_4ac != (undefined2 *)0x0);
      local_498 = (float)((int)local_498 + -1);
    } while (local_498 != 0.0);
    local_4bc = local_4bc + 1;
    local_494 = local_494 + -1;
    local_480 = local_480 + 2;
    local_484 = local_484 + 8;
    local_48c = local_48c + 0x10;
  } while (local_494 != 0);
  if (*(char *)(DAT_00880354 + 0x10) == '\0') {
    return;
  }
  sVar9 = (&DAT_008802e0)[(short)param_1];
  local_4bc = (int *)(int)sVar9;
  if (sVar9 == -1) {
    return;
  }
  psVar24 = (short *)(&DAT_00655aa0 + sVar9 * 0x28);
  if (*(short *)(&DAT_00655aa0 + sVar9 * 0x28) != (short)param_1) {
    return;
  }
  do {
    sVar9 = psVar24[1];
    local_4c8 = CONCAT22(local_4c8._2_2_,sVar9);
    if (((((((psVar24[0xe] != -1) && (local_448[psVar24[0xe]] == '\0')) ||
           ((*(int *)(DAT_006f1d6c + 0xc) < DAT_00725204 &&
            ((sVar9 < 6 && ((*(byte *)(psVar24 + 0xc) & 0x40) == 0)))))) ||
          ((psVar24[0xf] != -1 && (*(char *)((int)&local_4c4 + (int)psVar24[0xf]) == '\0')))) ||
         ((psVar24[0x10] != 0xffff &&
          ((param_2 == 0xffffffff || ((ushort)(psVar24[0x10] & (ushort)local_464) == 0)))))) ||
        ((psVar24[0x11] != 0xffff &&
         ((param_3 == 0xffffffff || ((ushort)(psVar24[0x11] & (ushort)local_458) == 0)))))) ||
       ((psVar24[0x12] != -1 && (psVar24[0x12] != sVar15)))) goto LAB_0042e524;
    local_488 = (uint)*(ushort *)(&DAT_006558c4 + sVar9 * 2);
    local_478 = (int)sVar9;
    uVar20 = 0xffffffff;
    iVar19 = 0;
    local_474 = 0xffffffff;
    local_4b8 = 0xffffffff;
    local_494 = 0;
    local_47c = 0xffffffff;
    local_48c = (undefined1 *)0x0;
    local_46c = (undefined1 *)0x0;
    local_4ac = (undefined2 *)0x0;
    local_4ca = 0;
    local_4b0 = 0xffffffff;
    local_460 = 0;
    local_498 = 1.0;
    local_454 = 1.0;
    puVar27 = (undefined2 *)0x0;
    switch(psVar24[4]) {
    case 0:
      puVar27 = &local_4a0;
      local_474 = local_43c;
      local_47c = param_3;
      uVar20 = param_2;
      iVar19 = local_414;
      break;
    case 1:
      puVar27 = local_4a4;
      local_474 = local_408;
      local_47c = param_2;
      uVar20 = param_3;
      iVar19 = local_410;
      break;
    case 2:
      local_47c = param_3;
      uVar12 = local_45c;
      if (local_4c9 != '\0') {
        uVar16 = psVar24[0xc];
        bVar14 = (byte)uVar16 & 1;
        bVar7 = bVar14 | 2;
        if ((uVar16 & 0x10) != 0) {
          bVar7 = bVar14 | 6;
        }
        if ((uVar16 & 0x20) != 0) {
          bVar7 = bVar7 | 8;
        }
        if (local_44c == 0xffffffff) {
          local_45c = FUN_004300d0(0,param_2,param_3,0x41900000,param_1,local_4c8,local_488,
                                   psVar24[2],psVar24[3],bVar7 | 0x10);
          local_4c9 = '\0';
          uVar12 = local_45c;
        }
        else {
          local_45c = FUN_0042ff80(0x41200000,param_1,local_4c8,local_488,psVar24[2],psVar24[3]);
          local_4c9 = '\0';
          uVar12 = local_45c;
          uVar20 = local_4b8;
        }
      }
      goto LAB_0042df55;
    default:
      goto switchD_0042dd60_caseD_3;
    case 4:
      local_47c = param_3;
      uVar12 = local_40c;
      if (local_4a5 != '\0') {
        uVar16 = psVar24[0xc];
        bVar14 = (byte)uVar16 & 1;
        bVar7 = bVar14 | 2;
        if ((uVar16 & 0x10) != 0) {
          bVar7 = bVar14 | 6;
        }
        if ((uVar16 & 0x20) != 0) {
          bVar7 = bVar7 | 8;
        }
        local_40c = FUN_004300d0(2,param_2,param_3,0x41400000,param_1,local_4c8,local_488,psVar24[2]
                                 ,psVar24[3],bVar7);
        local_4a5 = '\0';
        uVar12 = local_40c;
      }
LAB_0042df55:
      local_474 = uVar12;
      puVar27 = local_4ac;
      if (uVar12 != 0xffffffff) {
        iVar19 = (uVar12 & 0xffff) * 0x724;
        uVar20 = *(uint *)(iVar19 + 0x18 + *(int *)(DAT_00880360 + 0x34));
        iVar19 = iVar19 + *(int *)(DAT_00880360 + 0x34);
        break;
      }
      goto switchD_0042dd60_caseD_3;
    }
    local_494 = iVar19;
    local_4b8 = uVar20;
switchD_0042dd60_caseD_3:
    uVar12 = local_4b8;
    bVar29 = false;
    if (((uVar20 == 0xffffffff) ||
        (iVar22 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar20 & 0xffff) * 0xc),
        (*(byte *)(iVar22 + 0x106) & 4) != 0)) || (*(short *)(iVar22 + 0xb4) == 1)) {
LAB_0042dfd3:
      bVar29 = true;
    }
    else if ((*(int *)(iVar22 + 0x218) != -1) && (*(int *)(iVar22 + 500) == -1)) {
      if ((*(byte *)(psVar24 + 0xc) & 8) == 0) goto LAB_0042dfd3;
      local_4ca = 1;
    }
    bVar14 = local_4ca;
    if (((iVar19 == 0) ||
        ((*(short *)(iVar19 + 0x6a) != 0 &&
         ((*(short *)(local_494 + 0x6c) != 0xb || (*(char *)(local_494 + 0xa0) != '\0')))))) &&
       (!bVar29)) {
      if (local_4ca != 0) {
        local_4b0 = FUN_0042ec90(local_4b8,local_47c,psVar24[2],param_5,&local_454);
        if (uVar12 == param_2) {
          local_4c9 = '\0';
          local_45c = local_4b0;
        }
        if (local_4b0 == 0xffffffff) goto LAB_0042e524;
      }
      if (((psVar24[0xd] != -1) && (puVar27 != (undefined2 *)0x0)) &&
         (*(char *)((int)psVar24[0xd] + (int)puVar27) == '\0')) goto LAB_0042e524;
      if (bVar14 == 0) {
        fVar30 = (float10)FUN_004303f0(1,0,0);
        local_484 = (float *)(float)fVar30;
        if ((float)local_484 == 0.0) goto LAB_0042e524;
        if (local_474 != 0xffffffff) {
          sVar9 = -1;
          if ((*(ushort *)
                ((&PTR_PTR_006853b8)
                 [*(short *)((local_474 & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] + 4)
              & 2) == 0) {
            if ((*(ushort *)
                  ((&PTR_PTR_006853b8)
                   [*(short *)((local_474 & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34))] +
                  4) & 4) != 0) {
              sVar9 = 1;
            }
          }
          else {
            sVar9 = 0;
          }
          if (sVar9 != -1) {
            iVar19 = (uint)((float)local_484 < 2.0) + (local_478 + sVar9 * 8) * 2;
            if (*(char *)((int)&local_438 + iVar19) != '\0') goto LAB_0042e524;
            local_460 = CONCAT22(local_460._2_2_,local_3ec[iVar19]);
            if ((short)local_4c8 < 7) {
              piVar1 = (int *)(DAT_006f0c9c + ((int)sVar9 + (short)local_4bc * 2) * 8);
              iVar19 = *piVar1;
              if (iVar19 != -1) {
                local_478 = local_440 - iVar19;
                local_498 = (float)(int)local_478 * 0.0011111111;
                if (0.0 <= local_498) {
                  if (1.0 < local_498) {
                    local_498 = 1.0;
                  }
                }
                else {
                  local_498 = 0.0;
                }
              }
              iVar19 = piVar1[1];
              if (iVar19 != -1) {
                iVar19 = iVar19 - local_440;
                if ((float)local_484 < 2.0) {
                  iVar19 = iVar19 + 0x1e;
                }
                if (0 < iVar19) goto LAB_0042e524;
              }
            }
          }
        }
      }
      else {
        local_4c8 = CONCAT22(local_4c8._2_2_,*(undefined2 *)(&DAT_00655914 + local_478 * 2));
        local_484 = (float *)&DAT_40000000;
      }
      local_4ac = (undefined2 *)__ftol();
      iVar19 = local_460;
      if (((short)local_464 == 1) && (local_4ca == 0)) {
        local_4ac = (undefined2 *)((int)local_4ac + 0x1e);
      }
      local_4ac = (undefined2 *)((int)local_4ac + local_460);
      sVar9 = __ftol();
      switch(psVar24[6]) {
      case 1:
        uVar20 = param_2;
        break;
      case 2:
        uVar20 = local_4b8;
        break;
      case 3:
        uVar20 = local_47c;
        break;
      case 4:
        if ((local_43c == 0xffffffff) ||
           (iVar22 = (local_43c & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34),
           *(short *)(iVar22 + 0x280) < 1)) goto switchD_0042e25c_default;
        local_48c = (undefined1 *)0x2;
        local_480 = *(uint *)(iVar22 + 0x28c);
        goto LAB_0042e2c7;
      default:
        goto switchD_0042e25c_default;
      }
      if (uVar20 != 0xffffffff) {
        local_48c = (undefined1 *)0x1;
        local_480 = uVar20;
LAB_0042e2c7:
        uVar16 = psVar24[7];
        local_46c = (undefined1 *)(uint)uVar16;
        if ((uVar16 == 0xffff) || (uVar16 == 1)) {
          local_46c = (undefined1 *)(uint)*(ushort *)(&DAT_00655904 + (short)local_4c8 * 2);
        }
      }
switchD_0042e25c_default:
      sVar17 = psVar24[5];
      if ((sVar17 == -1) || (sVar17 == 1)) {
        sVar17 = *(short *)(&DAT_006558f4 + (short)local_4c8 * 2);
      }
      sVar2 = psVar24[3];
      local_460 = CONCAT22(local_460._2_2_,psVar24[2]);
      local_478 = 0xffffffff;
      local_450 = 1.0;
      local_418 = 1.0;
      if (local_4ca == 0) {
        local_49c = (float)FUN_00560d00(local_488,1,&local_49c,&local_460,&local_478);
        sVar10 = SUB42(local_49c,0);
        if ((sVar10 != 0) && (sVar10 == 1)) {
          local_450 = 0.3;
        }
        if (sVar10 == 0) goto LAB_0042e524;
        if (((sVar2 != -1) && (cVar6 = FUN_00569470(), cVar6 != '\0')) &&
           ((local_474 == 0xffffffff ||
            ((*(short *)(&DAT_00655258 +
                        *(short *)((local_474 & 0xffff) * 0x724 + 0x6c +
                                  *(int *)(DAT_00880360 + 0x34)) * 0x38) != 2 &&
             (*(short *)(local_494 + 0x6a) != 1)))))) {
          local_418 = 2.0;
        }
      }
      fVar11 = local_418 * *(float *)(psVar24 + 8) * local_450 * (float)local_484 * local_454 *
               local_498;
      if (0.0 < fVar11) {
        if (0xf < SUB42(local_470,0)) break;
        iVar22 = (int)SUB42(local_470,0);
        iVar13 = iVar22 * 0x38;
        local_34c[iVar22 * 0x1c] = (short)local_4bc;
        local_37c[iVar13 + 1] = local_4ca;
        (&local_380)[iVar22 * 0xe] = fVar11;
        auStack_36c[iVar22 * 0xe] = local_4b8;
        auStack_36c[iVar22 * 0xe + 1] = local_474;
        *(short *)(local_37c + iVar13 + 6) = psVar24[3];
        auStack_36c[iVar22 * 0xe + 2] = local_47c;
        auStack_36c[iVar22 * 0xe + 3] = local_4b0;
        *(short *)(local_37c + iVar13 + 4) = (short)local_488;
        *(undefined2 *)(local_37c + iVar13 + 10) = local_4ac._0_2_;
        *(undefined2 *)(local_37c + iVar13 + 8) = local_49c._0_2_;
        *(undefined2 *)(local_37c + iVar13 + 2) = (undefined2)local_460;
        local_354[iVar22 * 0xe + 1] = local_478;
        asStack_35c[iVar22 * 0x1c + 1] = (short)local_46c;
        asStack_35c[iVar22 * 0x1c + 2] = (short)local_48c;
        local_354[iVar22 * 0xe] = local_480;
        bVar14 = *(byte *)(psVar24 + 0xc);
        *(short *)(local_37c + iVar13 + 0xc) = sVar9 + (short)iVar19;
        asStack_35c[iVar22 * 0x1c] = sVar17;
        bVar14 = bVar14 >> 1 & 1;
        local_37c[iVar13] = bVar14;
        if (bVar14 != 0) {
          local_4b1 = '\x01';
        }
        local_470 = (float)((int)local_470 + 1);
        local_468 = fVar11 + local_468;
      }
    }
LAB_0042e524:
    psVar24 = psVar24 + 0x14;
    local_4bc = (int *)((int)local_4bc + 1);
  } while (*psVar24 == (short)param_1);
  uVar4 = local_430;
  sVar9 = SUB42(local_470,0);
  if (sVar9 < 1) {
    return;
  }
  pfVar18 = &local_380;
  if ((local_4b1 != '\0') && (local_468 = 0.0, 0 < sVar9)) {
    uVar20 = (uint)local_470 & 0xffff;
    pfVar23 = pfVar18;
    do {
      if (*(char *)(pfVar23 + 1) == '\0') {
        *pfVar23 = 0.0;
      }
      uVar20 = uVar20 - 1;
      local_468 = local_468 + *pfVar23;
      pfVar23 = pfVar23 + 0xe;
    } while (uVar20 != 0);
  }
  if (1 < sVar9) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    sVar17 = 0;
    local_49c = (float)(random_seed_global >> 0x10) * 1.5259022e-05 * local_468;
    fVar11 = 0.0;
    if (0 < sVar9 + -1) {
      iVar19 = 0;
      do {
        fVar11 = fVar11 + (&local_380)[iVar19 * 0xe];
        if (local_49c <= fVar11) break;
        sVar17 = sVar17 + 1;
        iVar19 = (int)sVar17;
      } while (iVar19 < sVar9 + -1);
    }
    pfVar18 = &local_380 + sVar17 * 0xe;
  }
  local_430 = CONCAT22(local_430._2_2_,sVar15);
  uVar16 = *(ushort *)(pfVar18 + 0xd);
  fVar11 = pfVar18[7];
  local_42c = CONCAT22(local_42c._2_2_,*(undefined2 *)((int)pfVar18 + 0x26));
  local_434 = CONCAT22(uVar16,(undefined2)local_434);
  local_428 = pfVar18[0xb];
  local_49c = (float)(uint)uVar16;
  local_42c = *(undefined4 *)((int)pfVar18 + 0x26);
  local_434 = CONCAT22(uVar16,(short)param_1);
  local_438 = fVar11;
  local_430._3_1_ = SUB41(uVar4,3);
  local_430._0_3_ = CONCAT12(1,sVar15);
  local_424 = CONCAT22(local_424._2_2_,(param_6 == 0xffff) - 1 & param_6);
  if (param_7 == (undefined4 *)0x0) {
    local_420 = param_7;
    local_41c = 0;
  }
  else {
    local_41c = param_7[1];
    local_420 = (undefined4 *)*param_7;
  }
  if (*(char *)((int)pfVar18 + 5) != '\0') {
    fVar11 = pfVar18[5];
    uVar16 = *(ushort *)((int)pfVar18 + 6);
    switch(*(undefined2 *)(pfVar18 + 2)) {
    case 0:
    case 1:
    case 2:
    case 7:
    case 10:
      break;
    default:
    }
    local_49c = (float)(uint)uVar16;
    FUN_0042e9c0(fVar11,&local_438);
    FUN_0042eee0(fVar11,(uint)uVar16,1,pfVar18[8],&local_438);
    return;
  }
  local_3ec[0] = *(ushort *)(pfVar18 + 2);
  local_3e4 = *(undefined2 *)((int)pfVar18 + 0xe);
  local_3e2 = *(undefined2 *)(pfVar18 + 4);
  local_3e0 = 0x18;
  local_488 = (uint)*(ushort *)(pfVar18 + 2);
  local_3ec[1] = *(undefined2 *)((int)pfVar18 + 6);
  local_3e8 = pfVar18[0xc];
  puVar25 = &local_438;
  puVar28 = local_3dc;
  for (iVar19 = 8; iVar19 != 0; iVar19 = iVar19 + -1) {
    *puVar28 = *puVar25;
    puVar25 = puVar25 + 1;
    puVar28 = puVar28 + 1;
  }
  fVar3 = pfVar18[5];
  FUN_00560f20();
  if (*(short *)((int)pfVar18 + 10) == -1) goto LAB_0042e8f7;
  iVar19 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)fVar3 & 0xffff) * 0xc);
  local_4c4 = *(float *)(iVar19 + 0x74);
  local_4c0 = *(float *)(iVar19 + 0x78);
  if (fVar11 != -NAN) {
    object_get_node_local_transform(fVar3,&DAT_0066bfa0,local_3ec,1);
    local_3f8 = local_38c;
    local_3f4 = local_388;
    local_3f0 = local_384;
    object_get_node_local_transform(fVar11,&DAT_0066bfa0,local_3ec,1);
    local_404 = local_38c;
    local_38c = local_38c - local_3f8;
    local_400 = local_388;
    local_388 = local_388 - local_3f4;
    local_3fc = local_384;
    local_470 = SQRT(local_38c * local_38c + local_388 * local_388);
    if (0.0001 <= ABS(local_470)) {
      local_4c4 = (1.0 / local_470) * local_38c;
      local_4c0 = local_388 * (1.0 / local_470);
      if (local_470 != 0.0) goto LAB_0042e8e2;
    }
    local_4c4 = *(float *)(iVar19 + 0x74);
    local_4c0 = *(float *)(iVar19 + 0x78);
  }
LAB_0042e8e2:
  FUN_00569530(fVar3,*(undefined2 *)((int)pfVar18 + 10),&local_4c4);
LAB_0042e8f7:
  if (pfVar18[6] != -NAN) {
    FUN_004302e0(9,*(undefined2 *)(pfVar18 + 9));
  }
  ai_communication_record_line_played(local_488,local_49c,0xffffffff);
  return;
}
#endif
