# Overnight log (started 2026-09-26 03:23)

Baseline: hooks.stable.txt = 2,060 functions (test 112, play-tested). Nothing below is play-tested until the morning.

## 03:42 iteration 1
- Top root blocker was the stub `FUN_006146b0` (transport send, 38 functions behind it). It is plain cdecl (4 stack args, no
  register or x87 inputs), but the callee-saved-register check in harness/gen_hooks.py rejected it because
  (a) CRT functions restore EBP with `leave` / SEH helpers, and (b) results cut off at the depth limit were cached as failures.
  Fixed both: `leave` counts as restoring EBP, the static CRT range 0x623000-0x63a000 counts as convention-compliant,
  depth cut-offs are no longer cached. Cleared stubs 37 -> 75.
- Every newly cleared stub's C declarations checked against the stack-argument count the original reads
  (tools/check_cleared_stub_args.py); 14 mismatches kept unsafe in harness/stub_unsafe.txt (floor(double) kept: the tracker
  reads 0 but it is 2 slots, as declared). network_channel_stream_flush's call to 0x6146b0 verified (4 args, 0x4ddc16..0x4ddc1c).
- Difftest of the 52 newly hookable: 7 match, 38 need live state, 4 senders crash in the rewrite only (they reach the
  message_delta_encode_message rewrite, which looks shaky -- item pointer unused -- although it has run in play since test 111),
  2 kill the tester. Those 6 are in known_bad.txt for review; NOT in the morning batch.
- Morning batch: harness/build/hooks.txt = stable 2,060 + 44 (build/overnight_new.txt): networking 15, objects 6
  (incl. object_delete / object_delete_unparented), shell 5, items 5, projectiles 3, sound 3, units 3, game 2, hs 1, interface 1.
## 03:53 iteration 2
- Root: data_packet_group_decode_packet (22 blocked). Both it and data_packet_group_decode_packet_body were wrong:
  * body (0x4d0c70): really EAX buffer, ESI definition, stack (remaining, dest, out_version, out_bytes_consumed); it builds
    its own byte_stream {buffer,0,remaining,0} on the frame, reads one version byte when the definition is versioned, and
    succeeds when the stream did not overflow; returns AL only. The C used a nonexistent 'input' stream parameter.
    Rewritten; regcheck agrees; difftest MATCH 104/104.
  * outer (0x4d09d0): EAX = &remaining length, stack (group, decoded_body, buffer, out_type, out_version, class); body is
    decoded from the start of the buffer (EAX = buffer at 0x4d0a55). Rewritten; regcheck agrees; difftest 4/118
    rewrite-only crashes on random input (likely in the rewritten byte-swap/decode helpers on garbage) -> known_bad, review.
- Stuck/next: the 27 callers declare 6 args without the EAX &remaining; each caller's C is loose (e.g. join_accepted
  passes expected_sequence as an output) -- needs per-file review, not a mechanical fix.
- Side effect: network_game_client_decode_sync_complete and network_game_message_decode_ingame_notification (both in the
  stable set) are no longer hookable -- they call data_packet_group_decode_packet with its OLD (wrong, 8-argument)
  signature, and it is now held back. They are left out of harness/build/hooks.txt (run as original code) until their
  calls are fixed; hooks.stable.txt itself is untouched.
## 04:00 iteration 3
- Root: shell_display_fatal_error_dialog (18 blocked). Argument 2 is dual-use at 0x57eaa1 (text pointer when the resource
  id is -1, else the help string's resource id); every call site passes the id form. Declared as uint32_t in the
  definition (cast to a pointer in the -1 branch); the 5 callers that declared a pointer updated, explicit (uint32_t)
  casts where they pass text. Its own call to dialog_box_show_localized (0x57ee1d: EBX proc, ESI module, push 0x66,
  push parent) now uses the definition's types with MAKEINTRESOURCE-style (const char *)0x66.
- Newly hookable (14): game_state checkpoint load/revert/persistent storage (9), profile_path_initialize,
  rasterizer_render_target_initialize, saved_game_validate_crc, saved_game_verify_version_and_checksum,
  shell_display_fatal_error_dialog. Difftest: game_state_perform_revert MATCH 200/200; 10 need live state; 3 "tester died"
  (the fatal dialog exits the process, and the other two reach it on error paths with random input -- expected).
  Batch now stable - 2 + 59.
## 04:08 iteration 4
- Root: message_delta_encode_message callers (18). Five pass the scratch buffer/size constants; their C had real bugs
  the compiler flagged once the declaration was fixed:
  * object_queue_pickup_denied_event: items must be a one-element array of pointers to the block (0x4efc37..0x4efc49
    stores &block in its argument slot and passes that slot's address); the C passed the block itself.
  * unit_dispatch_scripted_event_9 (0x56c39c..0x56c3bc): item record is {looked_up, event_byte}; the C used
    {byte, pointer} and the old broadcast signature. Rewritten.
  * unit_dispatch_scripted_event_1b (0x56dcf7..0x56dd64): item record {unit_hash, weapon_hash, event_byte}; rewritten
    (still a register skip: its notes say "param_1, in_ECX" -- next).
  * unit_broadcast_state_change_event: only a (void **) cast.
- Newly hookable: object_queue_pickup_denied_event (untested in isolation) -> in the batch.
  unit_broadcast_state_change_event and unit_dispatch_scripted_event_9 crash in the rewrite only (200/200), the same
  pattern as the 4 senders held back in iteration 1 -> known_bad; the common factor is the message_delta_encode_message
  rewrite, which is the next thing to investigate.
- Remaining encode callers (12) take buffer/size from their OWN stack arguments -> need per-file slot mapping.
## 04:13 iteration 5 -- IMPORTANT
- Investigated the common factor in the rewrite-only crashes: message_delta_encode_message (0x4ec940), which has been
  in the stable set (hooked) since test 111. Its C is wrong in several ways against 0x4ec9bf..0x4ecb57:
  all_fields gets ctx+0xc (should be ctx, EAX) and no item pointer (stack: changed, item, type); the per-item bit counts
  are read from ctx+0x50/0x54 (should be +0x44/+0x40); totals/budget/item count are kept in locals instead of the context
  fields (+0x14, +0x18, +0x38, +0x3c) the helpers read; the per-item context resets (+0x48.., +0x64..) are missing; the
  item count is written to a NULL stream (should be the context's bit stream at +0x1c, value in ECX). Its four helpers
  (prepare_item, all_fields, message_header, bit_stream_write_bits_chunked) are themselves register-convention skips.
- Action: message_delta_encode_message added to known_bad.txt, so it and everything reaching it (21 of tonight's batch
  and 5 more stable functions: the three build_*_update network functions plus the two decode callers) are left OUT of
  harness/build/hooks.txt and run as original code in the morning test. hooks.stable.txt is untouched; the list of stable
  functions held out is in build/overnight_held_out_stable.txt.
- Next: rewrite the four helpers from the binary, then the encoder itself.
## 04:17 iteration 6
- bit_stream_write_bits_chunked (0x4cf8f0) rewritten: EAX = stream (forwarded in ESI), ECX = pointer to an ARRAY of
  32-bit values advanced per 32-bit chunk, stack = total bit count. The draft took (count, value, stream) and repeated one
  value in every chunk (only right for <= 32 bits). regcheck agrees; difftest MATCH 176/176. Now in the batch.
- Its 24 C callers still use the old (count, value, stream) form, so the ones that were hookable drop out of the batch
  until fixed: 6 of tonight's (message_delta_encode_vector3d, network_channel_queue_message / scan_retransmit_timeouts /
  service_retransmit_only / stream_flush, network_session_broadcast_to_flagged) and 2 more stable ones
  (message_delta_float_array_encode, network_channel_stream_init) -- they run as original code meanwhile.
- Next: fix the chunked callers file by file (each passes a pointer to its value(s); check EAX/ECX at every call site).
## 04:23 iteration 7
- bit_stream_write_bits_chunked callers (new EAX stream / ECX values / stack count form), each checked at its call sites:
  * network_channel_queue_message (0x4dce9c/0x4dcea8): ECX = the header / body argument -> these are bit-array pointers.
  * network_channel_scan_retransmit_timeouts (0x4ddaba/0x4ddacc): ECX = slot+0x18 / +0x1c pointers (the C dereferenced them).
  * network_channel_stream_flush: the first write puts the packet's BYTE COUNT into the header chunk (0x4ddbcc,
    ECX = &byte_count; the C wrote 0), the second writes 0; and the used-bit count is bit_cursor + byte_cursor*8 - first_bit
    (0x4ddb63; the C used last_bit -- a wrong packet length).
  * network_channel_stream_init (0x4dd99d): writes a zero local. difftest MATCH 200/200.
  * message_delta_float_array_encode (0x4e9e85): ECX = the cursor.
- Back in the batch: queue_message, scan_retransmit_timeouts, service_retransmit_only, stream_flush,
  session_broadcast_to_flagged; float_array_encode and stream_init no longer held out of the stable set.
- Still to do: message_delta_encode_vector3d (two different locals), encoder header/prepare/all_fields, the encoder,
  and the Ghidra-placeholder senders (unaff_write_* arguments).
## 04:26 iteration 8
- message_delta_encode_vector3d (0x4eabe0): both chunked writes pass ECX = &the quantized value (0x4ead86, 0x4eae98);
  the rest of the C (delta/absolute quantization, the mode bit, the clamp) matches the binary. difftest MATCH 174/174.
  Back in the batch.
## 04:31 iteration 9
- message_delta_encode_prepare_item (0x4ecb60): body already matched; convention fixed (EAX ctx, AL result).
  regcheck agrees; difftest MATCH 200/200. In the batch. (Encoder helpers left: message_header (ESI ctx), all_fields.)
## 04:36 iteration 10
- message_delta_encode_message_header (0x4ecd00) rewritten: ESI = context; the bit stream is inline at ctx+0x1c (the C read
  a pointer at +0xc); the message type is written from ctx+4 BY ADDRESS in (ctx+0x84 += 6) bits, and the 2-bit parameter
  field from 0x71cfac (the C wrote 0 for both); AL result. regcheck agrees; difftest MATCH 176/176. In the batch.
  Encoder helpers left: all_fields (0x4ecc00), then the encoder.
## 04:41 iteration 11
- message_delta_encode_all_fields (0x4ecc00) rewritten: EAX ctx, stack (static_base, item, type_base) -- the draft dropped
  the item. Static-field pointer = static_base + binding->destination_offset (+4; the draft used +8, source_offset); a
  failing static field no longer returns early (loop continues, then returns 0); the per-field loop has no early exit;
  message_delta_encode_field gets EAX = type_base, ESI = ctx, stack (i, item); AL result. regcheck agrees. Difftest can't
  run it in isolation (needs real message definitions) -- it's a line-by-line transcription, in the batch; in SP it runs
  on every object deletion through the original encoder, so a mistake would show quickly (bisect candidate).
- The encoder's all_fields call now passes (ctx, baseline, item, type) as at 0x4eca3b. Encoder itself is next.
## 04:49 iteration 12 -- encoder done
- message_delta_encode_message (0x4ec940) rewritten from the disassembly: zeroed 0x94-byte context (type, flag, buffer,
  size, budget, inline bit stream at +0x1c, header bits, item count, running offset); header helper; per item
  prepare_item + all_fields(ctx, baseline, item, type), counted when field bits > 0 or forced, with the context totals
  updated and the per-item blocks (+0x48.., +0x64..) reset; a multi-item message writes (count - 1) in the definition's
  count width through the context stream. regcheck agrees; removed from known_bad.
- End-to-end check: the six senders that crashed only in the rewrite (iterations 1 and 4) now MATCH the original 200/200
  each -- game_engine_notify_player_interaction, game_engine_send_end_game_notification,
  game_engine_send_round_reset_message, player_notify_kill_streak_update, unit_broadcast_state_change_event,
  unit_dispatch_scripted_event_9. All six removed from known_bad.
- Batch: +29 (object deletion chain, weapon notifications, projectile sends, garbage collection, kill streaks...),
  now stable - 2 + 72. The encoder and the 3 build_*_update network functions are back in; only the two decode callers
  (still waiting on data_packet_group_decode_packet) stay held out.
## 04:53 iteration 13
- ai_communication_broadcast (0x42d340) callers aligned with the definition (all 7 stack args, checked at 0x566ba5 and
  0x55e413..0x55e467: (code, unit, -1, -1, -1, -1, 0)): actor_movement_update / actor_target_relationship_think
  (7th arg is a pointer), unit_enter_vehicle_seat (unit = EAX input) and unit_evaluate_flee_reaction (EDI object).
- STUCK: ai_communication_broadcast itself is a real placeholder -- two __ftol results guessed as cone_index
  (line ~720) in a 5.6 KB function; needs a proper rewrite of that section. actor_target_relationship_think is listed
  because its callees use Ghidra's guessed arguments. Nothing newly hookable from this unit.
## 04:57 iteration 14
- rasterizer_set_shader_stage_config (0x519200): the C read the device vtable once before the switch; the original loads
  it per call inside the cases, so an out-of-range mode never touches the device (that was the known_bad "rewrite
  crashed 58/58"). Fixed; cases 2 and 5 re-checked against the jump-table targets (0x5192bc, 0x519427). difftest MATCH
  58/58; removed from known_bad.
## 05:05 iteration 15
- string_convert_ascii_to_unicode (0x557990): returns EAX = dst, or 0 when even a truncated copy won't fit (0x5579cf); the
  definition said void. Parameters reordered to (dst, capacity, source) as every caller declares them (registers bound by
  name: EAX dst, EDI capacity, EBX source). game_engine_find_player_by_name (the one caller using the old order) fixed
  (0x473441 EDI = 0x800). The old known_bad "tester died" was the random source pointer; now difftest MATCH 200/200,
  find_player_by_name MATCH 200/200. Removed from known_bad.
## 05:09 iteration 16
- savegame_slot_handle_pack (0x53e630): the rewrite already matched the binary exactly (12-bit slot << 16 | type nibble,
  bit 30 from DL, bit 31 from the stack byte); "placeholder" in its notes referred to field names. regcheck agrees;
  difftest MATCH 200/200; removed from incomplete_rewrites. In the batch.
  Its callers are still blocked one level further: saved_game_enumerate_by_type (register skip) and saved_game_create_slot
  (XCreateSaveGame declared differently) -- next candidates.
## 05:17 iteration 17
- saved_game_enumerate_by_type (0x53c4e0): register note was prose ("pointer in EBX"); rewritten as
  "EBX -> capacity_and_count, stack -> type, out_handles, builtin_only" -- callers clean 0xc (3 stack args); regcheck agrees.
- savegame_index_get_slot_count: file_reference_get_size_by_path needs ESI = the save directory reference (0x53e473
  mov esi,0x721330); the call passed only &size. Fixed.
- Newly hookable: savegame_index_get_slot_count, saved_game_find_by_name. enumerate_by_type is next blocked by
  saved_game_list_rebuild_index -> savegame_find_first (declared differently).
## 05:24 iteration 18
- savegame_find_first (0x551bc0): both callers pass (root, find_data) but the definition took (find_data, root) -- both
  pointers, so the checker didn't flag it, but C-to-C calls swapped them. Reordered (registers bound by name: EAX
  find_data, stack root), return is the integer find handle (-1 on failure) as callers compare it. Its elided
  string_convert_ascii_to_unicode() call is now (find_data + 0x244, 0x80, find_data->cFileName) per 0x551c9a..0x551ca5.
- Newly hookable: savegame_find_first. saved_game_list_rebuild_index next blocks on string_format_wide_va_bounded (the
  per-call EDX count -- needs manual per-caller work).
## 05:29 iteration 19
- string_format_wide_va_bounded (0x557910, EDX = count in characters): 10 callers whose every binary call site loads one
  constant into EDX (and whose C call count matches the site count) now pass it first -- post-game / in-game score text
  (0x100 / 0x200), axis/button/POV text (0x18/0x17/0xe), chat (0x7f), profile name (0x3f), server browser (0x1f),
  saved-game slot (0x7f) and list rebuild (0xff). game_variant_list_matching_substring reverted (gained a warning).
  8 callers with mixed/variable counts left for per-call work.
- Newly hookable (13): the controls/binding display text chain, set_profile_name, UI carousels and tab sync.
  difftest: chimera__pov_text and input_get_binding_display_name MATCH 200/200; the rest need live state.
## 05:32 iteration 20
- string_convert_ascii_to_unicode callers: savegame_find_next (0x551daa, same as find_first: find_data+0x244, 0x80,
  cFileName) and sv_find_client_by_name_or_index (0x4e3f8d: EDI 0x1a bytes = 13 chars, EBX = name input).
- STUCK: XCreateSaveGame's convert (0x5517af) writes to a frame local ([esp+0x2d4] at the call) that has no C
  counterpart; its sprintf/convert stack offsets need deferred-cleanup accounting -- left for review.
- Newly hookable (5): savegame_find_next, saved_game_enumerate_by_type, saved_game_list_rebuild_index,
  saved_game_check_storage_availability, sv_find_client_by_name_or_index. Batch: stable - 2 + 105.
## 05:37 iteration 21
- chimera__console_out callers with MIXED colour sources: 9 files where the C call count equals the binary site count
  and every site's EAX source is known (xor eax,eax = default, or mov eax,[global]); colours assigned in address order
  (a wrong pairing would only change a console line's colour). sv_ban, sv_kick, sv_map, sv_map_reset, sv_status,
  map/game-variant listing, ban list print, end-game sequence. All compile cleanly.
- Newly hookable (3): map_list_matching_substring (+_evaluate), network_banlist_print. The sv_* commands are blocked
  by other callees still. Batch: stable - 2 + 108.
## 05:42 iteration 22
- saved_game_enumerate_by_type: definition parameters reordered to the callers' (type, out, builtin_only, &count)
  (EBX bound by name) -> the three 4-argument callers now match. player_profile_auto_select (0x49531f, 0x4953bc) and
  player_profile_subsystem_initialize pass their count variable's address as the EBX argument.
- Still to do: game_engine_get_variant_by_name, player_profile_check_storage_and_defaults, ui_build_profile_list,
  game_variant_list_matching_substring need a count local added (EBX = &count preset to the capacity at each site).
  Nothing newly hookable (these callers are blocked by other callees too).
## 05:47 iteration 23
- saved_game_enumerate_by_type EBX count added in game_engine_get_variant_by_name (0x4629f6, slot_count preset 100),
  player_profile_check_storage_and_defaults (0x49c699..0x49c6cb: ONE count preset 1 shared by both calls -- the second
  sees what the first left) and ui_build_profile_list (0x49dddc, preset 0x64). No new hookables (other callees block).
- Noted, not fixed: game_variant_list_matching_substring reads the returned count (0x4e46ce) but its C loops to 99.
## 05:53 iteration 24
- chimera__kill_feed (0x460a30): the network notify call had guessed arguments (0, -1); the original passes EAX = the
  recipient (EDI) and ECX = param_1 (0x460b48..0x460b5a). The engine-override call's signature confirmed at 0x460aa7.
  Everything else matched. Removed from incomplete_rewrites. Now blocked one level down:
  game_engine_notify_kill_event -> network_session_send_to_machine (declared differently) -- next.
## 06:01 iteration 25
- network_session_send_to_machine (0x4e1930) rewritten: the send is skipped only when the channel flag at +0xa98 is 1
  and force is 0 (the C had the condition inverted); queue call = (EDI channel, EBX = param_3 bits, stack data,
  &status, 1, reliable, unknown_a) with status = (param_1 != 0); AL result. difftest MATCH 200/200.
- game_engine_notify_kill_event: send call now passes EAX = the player's machine index (0x460959) and ESI = the
  server (0x46094a). difftest MATCH 200/200.
- Newly hookable (2). chimera__kill_feed next blocks on game_engine_build_kill_feed_message_text's declaration of
  input_get_binding_display_name. The other 20 send_to_machine callers need their EAX machine expression per file.
## 06:07 iteration 26 -- nothing fixed
- game_engine_build_kill_feed_message_text: its input_get_binding_display_name call is (EAX &record, ECX out) at
  0x45f07b, but the builder also has ~20 string_format_wide_va_bounded sites with variable counts and "UNSURE args";
  needs a per-case rewrite -> STUCK for the loop (blocks chimera__kill_feed).
- control_profile_find_or_create_gamepad_slot (0x53b470): C matches the binary step by step; record size 0x220 and
  gamepads offset 0x1108 confirmed by compiling a size probe; its three callees MATCH 200/200 each; yet it still
  differs 4/196 (rewrite-only crash on random input). Cause not found -> STUCK, left in known_bad.
## 06:11 iteration 27
- XDeleteSaveGame (0x5519a0): the first argument (EAX -> EDI) is the save's wide-character NAME -- the source of the
  unicode->ascii conversion at 0x5519c4 (ESI dest, EDI source, push 0x80) -- not a "validity token"; the conversion call
  had no source argument. Fixed; regcheck agrees. Newly hookable: XDeleteSaveGame.
- Next: savegame_index_remove_slot has all its file_reference seek/read/write/close calls elided -> full rewrite.
## 06:18 iteration 28
- savegame_index_remove_slot (0x53e4a0) rewritten: every file_reference call had elided arguments, and the loop never
  tracked a separate write offset. Verified per call: get_size_by_path(ESI ref, stack &size) 0x53e52a, open(ESI, 3)
  0x53e556, seek(EAX, ECX ref) 0x53e568/0x53e580/0x53e5a7, read/write(EDX ref, ECX record, ESI 0x206) 0x53e590/0x53e5b7,
  set_length(EAX size-0x206, ESI ref) 0x53e5e2, close(ESI) 0x53e5f6. regcheck OK. Not difftested (it would rewrite the
  real save index file).
- Newly hookable (2): savegame_index_remove_slot, saved_game_delete_by_handle. Batch now 113.
## 06:22 iteration 29
- Top root (13): player_profile_initialize declared saved_game_enumerate_by_type with EBX capacity first; reordered to
  the definition's (type, handles, builtin_only, EBX count). Binary 0x53a543: ESI=0 (xor at 0x53a3a1) pushed as type
  and builtin_only, EBX=&capacity preset 1, handles[0]=-1. Same fix in saved_game_delete_by_display_name (0x53ba21:
  push 0, &handles, 0; EBX=&count). regcheck OK for both, saved_games builds 0 failed.
- Nothing newly hookable: player_profile_initialize now blocks on control_profile_find_or_create_gamepad_slot (STUCK,
  iteration 26); saved_game_delete_by_display_name via input_apply_named_device_default_profile. hooks.txt unchanged (113).
## 06:33 iteration 30
- Root (10): animation_get_root_node_matrix / animation_get_frame_orientations (known_bad, "rewrite crashed"). Real
  cause one level down: model_nodes_get_default_transforms compared (int32_t)node < nodes.count where TagReflexive.count
  is uint32_t -> unsigned compare; the original is signed (jle 0x4d761a, jl 0x4d767c) so it skips a negative count.
  Cast to int32_t. difftest: model_nodes_get_default_transforms 0/95, animation_get_frame_orientations 0/101 (MATCH).
- animation_get_root_node_matrix: checked instruction by instruction (0x4d49b0..0x4d49f7); its remaining difftest
  difference (out+4) compares uninitialized stack: on every sample the original survives, the model's count is negative
  so orientations[0] is never written. All three removed from known_bad.
- Newly hookable (1): model_nodes_get_default_transforms. Batch 114. Next blocker for camera_debug_compute_pov and
  friends: animation_node_get_rotation (not hookable); _translation and _scale also crash on a few difftest inputs.
- Note for later: TagReflexive.count unsigned may cause the same signed/unsigned bug in other loops over reflexives.
## 06:48 iteration 31
- animation_keyframe_time_search (0x4d6b10, known_bad "hang"): the C is equivalent to the binary; ran the ORIGINAL's
  bytes (copied into executable memory, position-independent) against the C on 200,000 sorted inputs: 0 differ. The
  hang is the original's own infinite loop on unsorted random times. animation_node_get_rotation's call to it verified
  (EAX=count via EBP, EBX=fistp'd frame, push &times[first], 0x4d6c3a..0x4d6c41). Both off known_bad.
- All three curve evaluators (rotation/translation/scale): first_index = header >> 12 then movsx ecx,dx (0x4d6bbd,
  0x4d6d4f, 0x4d6ed1) -> (int16_t); the C used 20 unsigned bits. animation_node_get_scale also read the default value
  eagerly; the original reads it only on the count==0 / before-first / at-last paths (0x4d6ebd, 0x4d6f10, 0x4d6f3d).
  difftest now MATCH: scale 0/47, translation 0/33 (off known_bad), frame_orientations 0/501; rotation 0/1 (the seed
  hits the search hang early).
- model_animation_get_frame_delta: verified 0x4d4a00..0x4d4a75 line by line; its difftest difference is the same
  uninitialized-stack artifact as animation_get_root_node_matrix (negative random count -> arrays never written).
- Newly hookable (23): animation_keyframe_time_search, animation_node_get_rotation/_scale/_translation,
  animation_get_frame_orientations, animation_get_root_node_matrix, animation_aiming_screen_blend,
  animation_overlay_* (4), animation_replace_frame_orientations, model_animation_get_frame_delta, camera_control,
  camera_debug_compute_pov, camera_get_type_for_player, camera_update, director_game_state_loaded,
  director_set_flying_camera, flying_camera_compute_pov, local_player_get_weapon_hud_interface,
  render_local_player_gunner_seat_visible, render_object_is_camera_unit. Batch 137.
- PLAY-TEST NOTE: this touches every animated model and the camera. If the morning test breaks animation or the
  camera, bisect these 23 first (iterations 30 and 31).
## 06:49 iteration 32 -- investigated, deferred
- object_recalculate_bounding_radius (0x4f8310, 11 dependents): the "model_vertices_get_interpolated_frame" it calls is
  0x4d53f0 = animation_overlay_interpolated_frame_orientations (now hookable; call at 0x4f852b pushes frame then the
  orientations buffer, EDI animation). But the whole C is a confidence-0.15 transliteration of Ghidra: the
  animation_get_frame_orientations / model_nodes_get_default_transforms / matrix4x3_from_quaternion calls all have
  wrong or guessed arguments. Needs a full objdump rewrite of a hot per-object path -> STUCK for the unattended loop;
  better done in a supervised session.
## 06:54 iteration 33
- sound_pcm_buffer_read (0x545860, known_bad "tester died", 10 dependents): the clamp of requested vs remaining is
  SIGNED (cmp ecx,eax; jg at 0x545871) but the C compared unsigned; the cache size is a signed WORD (movsx at 0x545886)
  but the C read 32 bits. Both fixed. Rest verified against 0x545860..0x545919 (bytes_read written before the range
  check, source = samples + (position & ~1), re-read update, both returns, sprintf of the 42-char message at
  0x67196c). difftest cannot judge it: with random input the ORIGINAL's signed clamp keeps a negative remainder and its
  rep movs overwrites the tester (the old unsigned C never did) -> "died" came from the original. Off known_bad.
- Newly hookable (1): sound_pcm_buffer_read. Batch 138. Its callers now block on sound_stream_decoder_close_slot ->
  original _ov_clear (vorbis library).
## 06:58 iteration 34
- Root (11): sound_stream_decoder_close_slot -> original _ov_clear, denied in harness/stub_unsafe.txt. ov_clear /
  ov_crosslap / ov_read are libvorbisfile delay-load import thunks (0x614510 jmp [0x6a0080], helper 0x62315d keeps
  ECX/EDX), so check_cleared_stub_args could not count their arguments. Every binary call site matches the only C
  declarations (sound_ogg_stream_read.c, sound_stream_decoder_close_slot.c): crosslap 2 + clear 1 (add esp,0xc at
  0x54523c), clear 1 at 0x5457c3/0x545808/0x545834, read 7 (add esp,0x1c at 0x5452d1). Removed the three from
  stub_unsafe (ov_open_callbacks left: passes a callbacks struct by value, not checked).
- Newly hookable (1): sound_stream_decoder_close_slot. Batch 139. sound_channel_fill_pcm_data now blocks via
  sound_ogg_buffer_fill -> sound_ogg_stream_read -> original _sound_ogg_error_to_string.
## 07:03 iteration 35
- sound_ogg_error_to_string (0x544f70) was in incomplete_rewrites only because its OV_EIMPL message reads "Feature not
  implemented." (keyword false positive). Verified mechanically: decoded the byte table 0x545144 + dword table
  0x545108 for all 138 codes -0x8a..-1 plus the out-of-range default (0x5450ef); every target sprintf's string
  equals the C's case string; 15 sprintf calls into the 0x1000-byte local, as in the C. Removed.
- Newly hookable (2): sound_ogg_error_to_string, sound_ogg_stream_read (regcheck OK). Batch 141.
  sound_ogg_buffer_fill now blocks via sound_ogg_stream_open -> original _ov_close_thunk.
## 07:11 iteration 36
- sound_ogg_stream_open blocked on _ov_close_thunk: the four ov_callbacks trampolines (0x544d50 read, 0x544da0 seek,
  0x544dc0 close, 0x544de0 tell) are code labels missing from out/functions.json, so gen_hooks' stub clearing skipped
  them. New harness/extra_function_sizes.txt (sizes from objdump) + 6 lines in gen_hooks to read it: the normal
  live_in / x87 / callee-saved analysis then cleared all four.
- Next blocker was _ov_open_callbacks (stub_unsafe): its only call site (0x544f52) builds the callbacks struct in
  sub esp,0x10 (read, seek, close, tell) under pushes of 0, 0, vf, datasource: 8 dwords, add esp,0x20 = the C's
  flattened 8-parameter declaration (the only one). Removed from stub_unsafe. regcheck sound_ogg_stream_open OK.
- Newly hookable (12): sound_channel_fill_pcm_data, _lock_and_fill, _reset, _stream_update,
  sound_driver_channel_continue, _channel_stop, _end_frame, _set_paused, _stop_all, sound_ogg_buffer_fill,
  sound_ogg_stream_open, sound_update_streaming_channels. difftest: 9 match, 6 untested, 0 differ. Batch 153.
- PLAY-TEST NOTE: iterations 33-36 put the streaming-audio path (music/dialogue ogg decode) on C; if sound breaks,
  bisect these first.
## 07:15 iteration 37
- Root (9): sound_reopen_device -> original _sound_channel_parameters_proc_default. Like iteration 36: 0x54ce50
  (and its sibling proc_eax 0x54cf80) are never-decompiled code labels, only stored by address into the function
  pointer 0x6e36cc (0x54935d / 0x54948a); both cdecl with stack arguments (arg1 [esp+0xc] after 2 pushes, arg2/arg3
  [esp+0x1c]/[esp+0x20] after 5 = the C prototype). Extents from objdump (ret at 0x54cf7f / 0x54d01f) added to
  harness/extra_function_sizes.txt; the stub analysis cleared both.
- Newly hookable (4): sound_reopen_device, sound_driver_set_eax_enabled, sound_driver_set_quality,
  player_profile_apply_audio_options. difftest 3 match, 1 untested. Batch 157.
## 07:36 iteration 38
- Root (8): network_channels_open -> original _network_channel_gap_441020 (+0x441040/0x441060/0x4410b0/0x441200):
  socket callbacks the C only passes BY ADDRESS to original registration functions. Added their objdump extents to
  extra_function_sizes.txt; found and fixed a gen_hooks bug: those labels weren't function boundaries
  (known_entries cached earlier), so each scan ran into the next callback; now 0x441020/040/060 clear normally.
  0x4410b0/0x441200 fail only deep in the network library; since they are address-taken (LTCG keeps the standard
  convention for those) and only stored by FUN_00614800 at socket+0x2c (0x614808), they go in the new
  harness/stub_address_only.txt with that justification.
- Then _thunk_FUN_0061c3e0 (stub_unsafe): gt2CreateSocket-shaped (0 buffer sizes -> 0x10000 default); all 3 call
  sites (0x441378/0x4413ef/0x441425) push 5 args = the C declaration; the counter under-read (4) and its EBX/ESI/EDI
  'live-in' are shrink-wrapped saves. Removed; the analysis cleared it.
- Newly hookable (2): network_channels_open (regcheck OK), network_receive_queue_new (difftest: need live state).
  Batch 159.
## 07:40 iteration 39
- Root (7): player_profile_check_storage_and_defaults declared saved_game_find_by_name with 3 args
  (name, 0, find_flag=1). Binary 0x49c6ee: push 0, push 0x718e80 (last_profile_name), add esp,8 -> two, matching the
  definition (char *name, int16_t type). The "1" was Ghidra reading the enumeration count slot (preset 1) still on
  the stack. Declaration and call fixed; regcheck OK; interface builds 0 failed.
- Newly hookable (6): player_profile_check_storage_and_defaults, chimera__load_main_menu, cutscene_stop,
  display_error, network_disconnect_notify_dropped_machines, ui_start_campaign_from_level_one. difftest 1 match,
  5 need live state. Batch 165.
## 07:45 iteration 40
- Root (6): object_type_override_call_0x68 (known_bad: "calls a function pointer with no arguments"). The original
  pushes the object handle before the indirect call (0x4f45a1 push esi; call [eax+0x68]; add esp,4); same in 0x74
  (0x4f474e push edi) and 0x7c (0x4f47ad push esi). The C called all three with no arguments. Fixed to pass
  object_index (the callbacks are table-stored originals: standard cdecl). Bodies otherwise match the binary line by
  line (scan 15..0, 0x74 default AL=1). regcheck OK x3; objects 0 failed. 0x6c pushes 4 args -> left in known_bad.
- Newly hookable (3): object_type_override_call_0x68/_0x74/_0x7c (difftest: need live state). Batch 168.
  object_new_with_datum_role_control now blocks on its console_print_error_va declaration.
## 07:50 iteration 41
- console_print_error_va (0x4c67c0) takes clear_first in AL; ~10 callers declared (format, ...). Surveyed all 13
  binary call sites: every one sets AL=0 by xor right before the call, except screenshot_render (1, already right).
  New mechanical fixer tools/fix_print_error_callers.py (every site in the caller's range must be AL=0, and the C call
  count must match): 6 files / 8 calls fixed (game_engine_update_teleporter, hs_compile_source,
  hs_sound_get_gain_reference, network_game_client_update, object_new_with_datum_role_control,
  game_state_load_core). objects_garbage_collection skipped (C call count differs). Build 0 failed.
- Newly hookable (1): game_state_load_core (needs live state). Batch 169. object_new_with_datum_role_control next
  blocks on effect_new_on_object; network_game_client_update on message_delta_sample_ring_buffer_average.
## 07:59 iteration 42
- effect_new_on_object (0x4507a0) DEFINITION was wrong: it reads SIX stack args (11 of 12 sites add esp,0x18), the
  C declared 4. Args 5/6 are effect_set_placement's ECX color / EDX tint_source (0x4507c6..0x4507eb; the C passed
  0, 0); local_player_index_for_object takes ESI = object_index and effect_first_person_screen_timer_active ECX =
  object_index (0x450808); the C passed nothing. Fixed; regcheck OK; effects 0 failed.
- object_new_with_datum_role_control's call fixed: EAX = new object, ECX = definition +0xac (creation_effect),
  stack (new object, -1, 0, 0, 0, 0) (0x4f591d..0x4f592a). It now blocks on game_engine_remap_placement_by_type.
- Nothing newly hookable (effect_new_on_object itself -> original _effect_first_person_screen_timer_active).
- FOUND, NOT FIXED (next): item_update (0x4bcf05: ECX = [def+0x304]), weapon_play_trigger_tag_effect (0x4c4879:
  EAX esi, ECX edi, stack ebp,-1,[esp+0x10],eax,0,0) and unit_update (0x56364e, 0x56369b: ECX = [..+0x194]) call it
  with creator/definition shifted by one (creator=object, definition=-1). All three are unsafe for other reasons
  already, so nothing regressed; each needs its EAX/ECX sources mapped to C variables.
## 08:02 iteration 43
- effect_first_person_screen_timer_active (0x450680) was in incomplete_rewrites for "simplified", which only refers
  to junk the original leaves above AL; its one caller tests AL (0x45080f). The C matches 0x450680..0x4506c0 line
  by line (object_try_and_get(ECX, 1), +0x106 & 4, +0x41c != -1, +0x1e signed jge vs game_time+0xc). difftest 0/0
  (needs live objects). Removed.
- Newly hookable (1): effect_first_person_screen_timer_active. Batch 170. effect_new_on_object next blocks on
  original _effect_rebuild_markers.
## 08:07 iteration 44
- effect_rebuild_markers (0x451710, incomplete_rewrites "not reproduced"): effect_marker_new returns the new handle
  or -1 (EAX from datum_new, 0x4517d9..0x451844) and the caller breaks its inner loop on -1 (0x451799); the C had
  it void and never stopped. Also the first-person flag compared the resolver with the C function's address; the
  original compares with 0x492ad0 (0x45176e), so original callers would lose the flag -> accept either address.
  Rest matches (stride 0x6c, 16 markers, signed location count). regcheck OK x2; effects 0 failed.
- Newly hookable (1): effect_marker_new (needs live state). Batch 171. effect_rebuild_markers now blocks via
  first_person_weapon_get_marker_data -> model_markers_get_by_name declaration; effect_new_on_object also on
  original _effect_update.
## 08:13 iteration 45
- first_person_weapon_get_marker_data (0x492ad0): its model_markers_get_by_name call (0x492b50..0x492b6d) is
  ECX = weapon tag +0x468 (model tag id), EAX = marker_name (arg 2; the C said "never read"), stack (0,
  fp+0x1d8e node_remap, fp+0x108c node_matrices, 0 mirrored, out, maximum) -- the C passed six scrambled stack
  args and no registers. Fixed to the definition's (ECX model, EAX name, 6 stack) prototype. Struct offsets from
  types/interface.h (unknown_108c 0x108c, weapon_hud_element 0x1d8e). regcheck OK; interface 0 failed.
- Newly hookable (4): first_person_weapon_get_marker_data, effect_rebuild_markers,
  first_person_weapon_center_flashlight, unit_get_first_person_marker_transform (all need live state). Batch 175.
## 08:14 iteration 46 -- investigated
- effect_update (0x451a30, known_bad: untraced __ftol): not a one-spot fix -- the 557-line file also calls
  object_function_get_value with no arguments (twice), guesses object_get_root_object_index's argument and reads a
  just-deleted record; needs a full objdump rewrite -> STUCK for the loop (blocks effect_new_on_object).
## 08:15 iteration 47 -- investigated, STUCK (details for a supervised session)
- player_profile_apply_video_options (0x495580, 6 dependents): the C omits the whole display-mode build and all four
  device-reset calls are wrong. From objdump: local rasterizer_display_mode at [esp+0x18] = {width (int16)
  settings[0xa68] (EBX), height (int16)[0xa6a] (EBP), refresh (int16)[0xa6c], vsync settings[0xa6f]!=0}
  (0x495604..0x495636); inside the "device invalid or null" branch, after GetWindowRect(GetDesktopWindow()), clamp if
  height >= rect.bottom || width >= rect.right (unsigned): bottom > 600 ? 800x600 : 640x480 (0x495673..0x4956a2).
  Then if !0x71d16d: display_mode_differs(EDI=&mode); build_present_parameters(EAX=&mode, push &pp[esp+0x38]);
  device_reset(EBX=width?!, push &pp); device vtable +0x20(device, 0, 0x7c11f0); resize_game_window(EAX=height,
  ECX=width); local flag [esp+0x14]=1; 0x71d16d=0.
- Blocked on the callee: rasterizer_device_reset (confidence 0.3) reads EBX as a POINTER (extra_params[0..1]) but this
  caller leaves the width in EBX, and it passes an uninitialized pointer to D3D Reset. Fixing the caller on top of
  that would be guessing -> rewrite rasterizer_device_reset from objdump first.
## 08:23 iteration 48
- Root (6): rasterizer_resource_file_verify_signature -> original _FUN_00618350 (stub_unsafe). Its two helpers were
  called with no arguments: FUN_00618350(size, buffer, key) with a 4-dword key built on the stack (0x3fffffdd,
  0x7fc3, 0xe5, 0x3fffef; 0x519993..0x5199ba) and FUN_0061a730(buffer, size - 0x21, &reference) (0x5199bf..0x5199c9).
  Both cdecl, 3 stack args, no register inputs; the only C declarations were this file's (void) ones, the source of
  the stub_unsafe entries -> removed, the analysis clears both. The repz cmpsb 0x21 + sete compare matches the loop.
- Newly hookable (4): rasterizer_resource_file_verify_signature, rasterizer_load_file_and_verify,
  rasterizer_dx9_vertex_shaders_load_all, rasterizer_dx9_vertex_shaders_initialize. difftest 1 match, 3 untested.
  Batch 179. PLAY-TEST NOTE: this is the vertex-shader file load at startup; a failure would show at launch.
## 08:26 iteration 49
- chimera__rasterizer_draw_dynamic_triangles_static_vertices (0x51c1c0) was in incomplete_rewrites for
  "placeholder", which describes the EARLIER rewrite the phase-4 review replaced. Checked 0x51c1c0..0x51c303 line
  by line (chunking at 0x2710, GetDesc x2, SetSoftwareVertexProcessing ((sw ? 0x10 : 0) | decl.usage) & 0x10,
  SetStreamSource(0, hw, 0, stride), SetIndices, DrawIndexedPrimitive(4, 0, 0, count, (first_index + first) * 3,
  chunk), unconditional restore at 0x51c2e9 even when count <= 0). Struct offsets confirmed by compile probe
  (decl usage +8 / 0xc, slot first_index +0 / 0xc, buffer type +0, count +4, hardware_buffer +0x10). regcheck OK.
- Newly hookable (11): it + rasterizer_dynamic_geometry_draw_dispatch, _geometry_draw_fixed_function,
  _geometry_part_draw, _glass_diffuse_draw(+_fixed_function), _glass_tint_draw, _light_cone_draw,
  _object_shadow_structure_draw, _transparent_geometry_group_draw_vertices, _water_ripple_draw (D3D: not
  difftestable). Batch 190. PLAY-TEST NOTE: rendering (glass, water ripples, shadows, light cones).
## 08:28 iteration 50
- rasterizer_vertex_buffer_create (0x524980, 1670 bytes, 7 dependents): genuinely incomplete -- the ~20-case
  per-vertex-type reformat switch is replaced by memcpy, and the created buffer is never stored into the slot
  (param_1 unused). Full rewrite needed -> STUCK for the loop.
- structure_bsp_leaf_find_material_surface (0x554fa0; incomplete_rewrites "not rewritten" = its callee 0x4ce8c0,
  now src/math/triangle_point_barycentric_2d.c): REAL BUG -- the definition is (a EAX, v_ecx ECX, v_edx EDX, p ESI,
  out_u, out_v) but this caller declared (v0 EAX, v1 EDX, v2 ECX, ...), so the C-to-C call swapped vertices 2/3.
  Binary 0x555141..0x55514d: EAX &t[0], EDX &t[1], ECX &t[2] -> call now (&t[0], &t[2], &t[1], point, u, v).
  regcheck OK; structures 0 failed.
- Newly hookable (2): structure_bsp_leaf_find_material_surface, structure_bsp_resolve_position_to_surface (need
  live BSP). Batch 192.
## 08:36 iteration 51
- rasterizer_lens_flare_batch_apply_material (0x536b70, 6 dependents): both texture-binder calls had the wrong
  argument roles and no registers. Binary: resolve_and_cache_submap_c(EAX key[+4] second tag, DI 0 bitmap_type,
  stack (uint16 key[+0], 1, uint16 key[+8])) at 0x536b92..0x536bac (the push edi before it is a save, add esp,0xc);
  validate_and_rebind_texture(EAX -1, stack (uint16 key[+0], uint16 key[+8])) at 0x536bb7. Declarations now match
  the definitions. regcheck OK; rasterizer 0 failed (decal_and_font_system_reset warning pre-existing).
- Newly hookable (6): rasterizer_lens_flare_batch_apply_material, _batch_draw_slot, _batch_find_slot,
  _batch_flush_all, rasterizer_lens_flare_quad_add, rasterizer_effect_slot_release_active (D3D). Batch 198.
- Idea for a supervised session: callers can declare a register-convention function with the same TYPES but a
  different register ORDER than its definition (iteration 50 found one); check_prototypes can't see that. A checker
  comparing each extern's register comment with the definition's blam-cc line would find the rest.
## 08:41 iteration 52
- chimera__transparent_decal_zbias (0x519530, 6 dependents) and its twin rasterizer_apply_decal_zbias (0x5194e0),
  both known_bad "rewrite crashed 200/200": the C loaded (*rasterizer_device)->vtable BEFORE the RasterCaps
  (+0x24 = 0x7c10e4) tests; the original dereferences the device only inside each branch, so with no caps set it
  never touches a null device. Moved the deref into the branches. difftest MATCH 200/200 for both; off known_bad.
- Newly hookable (3): both + rasterizer_decal_pass_begin. Batch 201.
- Pattern to audit later: 47 rasterizer files cache 'vtable = *(void ***)rasterizer_device' up front; any whose
  original only reaches the device conditionally has the same early-null-deref difference.
## 08:45 iteration 53
- Audited the iteration-52 pattern: of the known_bad rasterizer "rewrite crashed" entries, only
  rasterizer_clear_decal_zbias (0x519580) also reads the device vtable up front; the original touches the device
  only inside each RasterCaps branch (0x51958c, 0x5195ad). Fixed the same way; difftest MATCH 200/200; off
  known_bad.
- Newly hookable (1): rasterizer_clear_decal_zbias. Batch 202.
## 08:52 iteration 54
- game_engine_remap_placement_by_type (0x4630b0) -- ALREADY IN hooks.stable.txt, and was wrong: every path returns
  EAX (the input handle on the early rets at 0x4630f3, or the tail-jumped resolver's result via jmp 0x462df0 /
  0x462c30) but the C was void. Its only original caller, object_new (0x4f54ef), stores that EAX as the
  definition tag -- reached only when current_game_engine != 0, i.e. MULTIPLAYER, so campaign play-tests could not
  see it. CORRECTION (iteration 55): not garbage -- gen_hooks' eax_return_register saw the original leaves its EAX
  input unchanged and made the adapter PRESERVE EAX, so the stable hook returned the input handle every time: right
  for types 0/1, but netgame-flag (type 2) and netgame-equipment (type 3) placements lost their remapped tag. Now returns the handle / resolver
  result; difftest MATCH 200/200. object_new_with_datum_role_control's declaration and call fixed too (EAX =
  definition_tag); it next blocks on its network_session_broadcast_to_flagged declaration.
- Nothing newly hookable; the stable function is corrected in place.
- NOTE FOR THE USER: with the stable set, multiplayer flag/equipment placements were not remapped; now fixed.
## 08:53 iteration 55 -- audit
- Checked why the void-EAX detector let 0x4630b0 through: it didn't -- gen_hooks classifies such functions with
  eax_return_register and, when the original passes its EAX input straight back, makes the adapter preserve EAX.
  That model misses tail-jump dispatchers whose OTHER paths return a callee's EAX. Candidate audit (supervised):
  void-in-C functions given an EAX-preserving adapter whose original has a jmp to another function.
- Ran that audit now (instrumented gen_hooks): of 38 void-in-C functions whose callers read EAX, none with an
  EAX-preserving adapter tail-jumps to another function. 0x4630b0 was the only case, and it is fixed. Clean.
## 09:01 iteration 56
- object_new_with_datum_role_control: network_session_broadcast_to_flagged needs EAX = body bit count; here EAX is
  object_type_override_get_0x64's result (0x4f58e3), tested > 0 and untouched until the call (0x4f5904); stack
  (1, scratch 0x871de0, 1, 0, 0, 3) matches. fix_broadcast_bits.py didn't cover it (its source isn't the
  message-delta encoder). Declaration + call fixed; regcheck OK; objects 0 failed.
- Nothing newly hookable: it next blocks on object_apply_network_placement's declaration (next firing).
## 09:06 iteration 57
- object_new_with_datum_role_control (object_new, 0x4f5480): three more helpers were called without their
  object_index: object_apply_network_placement(EAX = ebx new object, push &placement+0x58; 0x4f57fb),
  object_refresh_region_permutations(EBX; 0x4f5809), object_notify_node_array_if_animated(EAX; 0x4f5829).
  Declarations now match the definitions; regcheck OK; objects 0 failed.
- Nothing newly hookable: next blocker object_block_data_grow's declaration; widget_new() (0x4f5830 region) is
  still an UNSURE no-argument call to check.
## 09:12 iteration 58
- object_block_data_grow (0x4f7e50) rewritten from objdump: returns AL = 1 on success / 0 on failure (0x4f7edd,
  0x4f7ee2; the C was void -- gen_hooks had rightly skipped it); block_list_reallocate needs EBX = &header->data,
  EDX = block_size + extra, push pool (the C passed only the pool); then block_size += extra, field {size = extra,
  offset = old} at data + field_offset, zero the new bytes. regcheck OK.
- object_new: grow calls now pass EAX = new object (0x4f575c / 0x4f5784 / 0x4f5798); widget_new(EAX = new object)
  (0x4f5843). objects 0 failed.
- Newly hookable (1): object_block_data_grow. Batch 203. object_new next blocks on object_block_data_new.
## 09:21 iteration 59
- object_new (object_new_with_datum_role_control, 0x4f54b0) finished: object_block_data_new(EAX -1, object_data,
  size) at 0x4f5529; object_type_definitions_notify_0x24(EBX new object, push ebp = placement, the first stack arg)
  at 0x4f5571; object_set_collision_enabled(EAX new object, push model != -1) at 0x4f56e9. Over iterations 54-59
  every helper call in object_new was checked against the binary; no declaration disagrees with its definition now.
  regcheck OK; objects 0 failed.
- Still not hookable: only via object_recalculate_bounding_radius (STUCK, iteration 32: needs a full rewrite).
## 09:25 iteration 60 -- investigated
- game_unload_map root (8): cache_file_download_status_get (0x4434a0). Its "simplified" incomplete entry was a
  quote of the phase-4 summary; jump table 0x4434f0 decoded ([0x4434b9 x2, 0x4434c0, 0x4434de, 0x4434e2]) matches
  the C case by case -> entry removed. It is still skipped by gen_hooks: notes (EAX, ECX) vs binary live-in (EAX);
  the out-of-range default returns the pushed ECX slot (0x4434e9 mov eax,[esp]). Dropping the ECX parameter is safe
  only if cache_file_download_poll (0x442720) never returns outside 0..4, and it returns SI on several paths (not
  traced) -> STUCK until that's proved. Note game_unload_map itself now shows a different chain first
  (cache_file_unload -> structure_bsp_dispose_material_vertex_buffers).
- Nothing newly hookable this firing.
## 09:30 iteration 61
- structure_bsp_dispose_material_vertex_buffers (0x4431a0): REAL BUG -- IUnknown::Release (vtable +8) was called
  through a cdecl pointer type, so MSVC emitted add esp,4 after a __stdcall callee that already popped: ESP drifts
  4 bytes per released buffer (the original has no add esp after 0x443206 / 0x443228). Made the pointer
  __stdcall. Offsets confirmed by compile probe. Its "simplified" entry was "not simplified away". regcheck OK.
- Newly hookable (3): structure_bsp_dispose_material_vertex_buffers, structure_bsp_dispose, cache_file_unload.
  Batch 206. game_unload_map next blocks via cache_file_download_finish -> cache_file_slot_read_header.
- BUG CLASS FOUND (next firings): COM/D3D methods called through cdecl pointer types elsewhere --
  model_dispose_vertex_buffers (x2), rasterizer_dynamic_geometry_dispose (x2), network_stats_overlay_draw (x5),
  flag_render, chimera__bsp_poly_movsx_2, structure_debug_draw_surfaces_in_box(+_alt), _simple,
  structure_picked_polygon_draw, player_profile_apply_video_options. (ai/*, sv_players casts are game-engine
  function tables: genuinely cdecl, fine.)
## 09:34 iteration 62
- COM __stdcall bug class fixed in 9 files (21 casts/declarations): model_dispose_vertex_buffers,
  rasterizer_dynamic_geometry_dispose, network_stats_overlay_draw, flag_render, chimera__bsp_poly_movsx_2,
  structure_debug_draw_surfaces_in_box(+_alt), _simple, structure_picked_polygon_draw. Verified the originals have no
  add esp after the indirect calls (0x442fa6/0x442fc5, 0x51bd09/0x51bd4e, 0x552963). Build 0 failed.
- THREE ARE IN hooks.stable.txt (model_dispose_vertex_buffers, rasterizer_dynamic_geometry_dispose,
  structure_picked_polygon_draw): each release/debug-draw call drifted ESP by the argument size. They run on map
  unload / debug draw, which a campaign play-through may never hit -- corrected in place.
- player_profile_apply_video_options (STUCK, iteration 47) has one more such call; left with that file.
- Nothing newly hookable (the others are blocked elsewhere).
## 09:39 iteration 63
- COM audit, typedef'd pointers: two more cdecl typedefs for COM methods -- rasterizer_dynamic_index_slot_lock's
  d3d_lock_fn (IDirect3DIndexBuffer9::Lock; STABLE, per-frame; no add esp after 0x511eba) and
  rasterizer_screen_flash_render's d3d_call3_fn (0x52eea0..0x52eed9). Both __stdcall now; build 0 failed. With
  iterations 61-62 no COM call in src (outside the stuck player_profile_apply_video_options) is cdecl-typed.
- Nothing newly hookable.
## 09:44 iteration 64
- cache_file_slot_read_header (0x4435e0): its "not rewritten" entry referred to its callees 0x442c70 / 0x442ce0,
  which have since been rewritten (cache_io_read_file_ex_retry, cache_io_wait_for_flag). Call site verified
  (0x443657..0x443685: push ReadFileEx, slot file, &header; ESI &request, EBX 0x800 = k_cache_file_header_size,
  EDX 0, EDI 0x443b00; wait ESI = &flag) and declarations match the definitions; regcheck OK. Removed.
- Newly hookable (2): cache_file_slot_read_header, cache_file_download_finish. Batch 208. game_unload_map now
  blocks via render_pregame_view_initialize.
## 09:58 iteration 65
- game_unload_map chain -> render_pregame_view_initialize (0x4c8f20) was skipped "original has 0 distinct ret
  forms": it ends with jmp 0x50c590 (render_pregame_frame, EAX = &view) and never rets itself. gen_hooks now lets
  an original that only leaves through tail jumps inherit its targets' ret forms (new ret_cleanup_tail; targets
  accepted if they are call targets OR have a known size; only no-ret-form cache entries recomputed). Skips for
  "0 ret forms" 46 -> 8; adapters 3606 -> 3647.
- Safety audit re-run (the 0x4630b0 class: void in C, callers read EAX, original tail-jumps): none among the new.
- Newly hookable (16): cache_evict_entry, cache_io_sound_decode_thunk, console_message_delete, cutscene_start,
  decal_delete, flying_camera_enter_flying, light_delete, observer_initialize, rasterizer_force_bilinear_filtering,
  ui_chat_window_reset_position, virtual_keyboard_backspace, game_engine_load_from_variant, hud_state_reset,
  rasterizer_dx9_vertex_shaders_reload, sound_pause, swarm_add_component. difftest 6 match, 10 need live state,
  0 differ. Batch 224. render_pregame_view_initialize itself next blocks via sound_update.
## 10:03 iteration 66
- sound_update_listener: scenario_location_get_water_and_weather is (EBX point, stack leaf, weather_index_out);
  binary 0x54b9a3..0x54b9b7: EBX = 0x6ac6d0 = observers[0].camera.position, push 0, push 0x6ac6dc
  (&camera.leaf_index). The C passed (leaf, 0) with no point -> the underwater test sampled the wrong point.
  Fixed (+ #include objects.h for bsp_leaf_reference). regcheck OK; sound 0 failed.
- Newly hookable (2): sound_update, sound_update_listener (per-frame audio; need live state). Batch 226.
- The same callee has ~10 other callers with guessed signatures (actor_refresh_combat_context,
  actor_target_data_refresh, observer_avoid_collision, effect_marker_environment_probe, effect_spawn_particles,
  object_change_color_evaluate, weather_update_local_player, weapon_trigger_fire_or_reload, flag_cloth_update):
  each needs its EBX point mapped -- next firings. render_pregame_view_initialize next via render_pregame_frame.
## 10:08 iteration 67
- weather_update_local_player: water/weather call (0x458acf..0x458ae6) is EBX = camera_position (0x7c3114), push
  &instance->unknown_10 (leaf), push &instance->cluster_index (+0x18) -- the C passed no point and a local out
  (uninitialised if the callee skips the write). Fixed. regcheck OK; effects 0 failed.
- Nothing newly hookable: it next blocks via weather_instance_build_render_geometry -> original
  _build_sprites_end. Remaining water/weather callers: observer_avoid_collision (EBX = ebp), effect_spawn_particles,
  object_change_color_evaluate (EBX = lea [esp+0x44]), effect_marker_environment_probe, AI x2, weapon, flag cloth.
## 10:11 iteration 68
- observer_avoid_collision (0x448d40): water/weather call (0x448d98..0x448da1) is EBX = ebp = position (first stack
  arg, [esp+0xc4]; the same point as the find_leaf EDX), push &location, push 0 -- the C had no point. Fixed;
  regcheck OK; camera 0 failed.
- Newly hookable (2): observer_avoid_collision, observer_commit (camera per frame; need live state). Batch 228.
## 10:20 morning play-test 1: CRASH at startup -> fixed
- EXCEPTION c0000005 with EIP 0x001ad076 on the stack. Removing cache_file_slot_read_header +
  cache_file_download_finish: no crash. Root cause: cache_file_slot_read_header passed the C rewrite
  cache_io_completion_routine (cdecl) directly as the ReadFileEx APC; Windows calls APCs __stdcall and the original
  0x443b00 ends ret 0xc -> APC dispatcher 12 bytes off. Stable never hit it because original code passes 0x443b00,
  whose hook adapter does ret 0xc. Now passes 0x443b00 exactly as the binary does (0x443669). Both functions back
  in hooks.txt (2286).
- LESSON (audit candidate): any C that passes a C function's address to Windows or to original code as a callback
  must pass the original's address (or a __stdcall-correct function) -- cf. iteration 44's 0x492ad0 comparison.
## 10:30 morning play-test 2: BLACK SCREEN -> fixed
- Bisect: 57 rendering fns out -> picture; 19 state/startup out -> picture; 6 shader-loader fns out -> picture.
  Cause: rasterizer_dx9_vertex_shaders_initialize (0x5307b0) memset the whole vertex-shader table, wiping the static
  'enabled' flags (+4), so load_all created no shaders. The original clears only +0 of each 8-byte entry
  (0x5307c0). The rest of the chain (load_all, load_file_and_verify, verify_signature, reload,
  force_bilinear_filtering) re-checked line by line: matches. Full list back in hooks.txt (2286).
## 10:38 morning play-test 3: CRASH entering the campaign -> fixed
- datum_delete null deref via biped_integrate_movement_with_collision -> object_unlink_cluster_or_notify_parent ->
  cluster_reference_remove_all (all previously stable). Bisect: 21 object/unit fns out -> no crash. Cause:
  object_set_cluster_and_parent (from tonight's first 111, never play-tested) always linked into the
  noncollideable cluster list; the original picks 0x8603d0 / 0x8603c0 by object flag 0x2000000 (0x4f5d42), as the
  unlink does. Fixed. Stable promotion reverted until the campaign test passes.

# Session 2 (day, user out) -- started 11:13
## Open bugs from the day play-tests (PRIORITY)
- MISSING GUNS on NPCs with the stable set (2286). Present after promoting last night's 228; NOT fixed by removing
  the 21 object/unit fns (bisect_O). So it is in the other ~207 of last night's batch or an interaction.
  NPCs animate normally with the stable set.
- T-POSE + crash (encounter_recompute_morale, AI) when the rewritten object_recalculate_bounding_radius (e891407) and
  its 9 dependents are hooked. The rewrite is at fault (stable-only run animates). Not hooked in hooks.txt.
- hooks.txt = stable (2286); session2_new.txt = the 10 held back.
## 11:18 session 2, manual -- audits (nothing fixed)
- Guns: functions actually CALLED in the stable-only campaign run (harness call counts, 67 of last night's batch) are
  the only candidates. Checked against the binary, all match: render_object_is_camera_unit (callers test AL),
  game_engine_load_from_variant, rasterizer_model_draw_prepare_states (skinning/lighting/mode section 0x5270ef..0x5271a3),
  rasterizer_model_draw_restore_states (C only; its disassembly not yet compared line by line),
  unit_drop_inventory_weapons_except_current, object_type_definitions_notify_two_args_0x48. Next candidates (by call
  count): animation_aiming_screen_blend (7604), animation_overlay_* , rasterizer_object_shadow_*,
  structure_bsp_resolve_position_to_surface (3609), animation_replace_frame_orientations (124), effect_rebuild_markers.
- T-pose: in that run only object_recalculate_bounding_radius (88194 calls) and _recursive (36) of the 10 ran, so the
  rewrite itself is at fault. Re-checked: all helper conventions/argument orders, offsets 0x10/0x5c/0x74/0x80/0xa0/
  0xac/0xb0/0xb4/0xcc/0xd0/0xd2/0xd4/0xd6/0x11c/0x120/0x134/0x1ea/0x1ee/0x1f2, def 0x4/0x8/0x14/0x34/0x44/0x8c/0x158/
  0x15c, model 0xb8/0xbc, node 0x20/0x22/0x24, the multiply orders, the type mask, constants 0.0/1.0. Not found yet.
  Next: compare the COMPILED rewrite (build/obj/objects/object_recalculate_bounding_radius.obj) against the original
  block by block.
## 11:19 session 2 -- bisect prepared for the guns
- The 67 of last night's batch that ran in the stable campaign session (minus bisect_O), split by subsystem:
  build/bisect_guns_render.txt, _anim.txt, _rest.txt. Ready-made hooks files build/hooks_without_{render,anim,rest}.txt
  (stable minus that group): copy one over harness/build/hooks.txt, launch, check NPC guns.
## 11:24 session 2 -- T-pose: compiled code reviewed, still not found (STUCK for now)
- Compiled rewrite: call sequence and argument pushes match (frame_orientations (anim, model, frame, out), defaults
  (model, out), overlays, notify, blend, root/child matrices, 7 multiplies, transform_point); matrix4x3_multiply_procedure
  resolves to the game global 0x696664 (EQU). Every helper it calls was ALSO hooked in the stable run, so both paths
  used identical helper code -> the defect is in this function's own data flow, but no mismatch found against
  0x4f8310..0x4f8b03. Needs a runtime comparison (e.g. dump nodes[] for one biped from original vs rewrite) -- ask the
  user for a debug run, or build a harness that captures one object's inputs.
## 11:28 session 2 -- guns audit: game_state_load_checkpoint matches (0x538280..0x538314)
- validate_crc(ECX 0x440000, EDX 0x14c, EBX &header, push &header+0x148, 0), verify(&header, 0), difficulty check
  (+0x126), revert proc, read block (EAX 0x440000, push base), slot +0xe, callbacks, perform_save(0): all as the C.
- Static guns audit exhausted for the likely candidates; the bisect files are ready for the user. Loop moves to
  priority 3 (blockers) until the user can test.
## 11:38 session 2
- lens_flare_update_visibility root (10): rasterizer_lens_flare_occlusion_query_get_result returned int32_t* for a
  pixel count; caller multiplies it by 0xff (0x513821) and passes ESI = loop index (0x5137c0). Now int32_t both
  sides. Newly hookable (session2_new): lens_flare_update_visibility, chimera__cinematic_screen_effect,
  render_cinematic_screen_effect_update.
- object_recalculate_bounding_radius added to known_bad (T-pose) so its dependents stop counting as hookable.
- object_update_functions (0x4f92f0, 7 dependents): calls the CRT x87 fmod intrinsic 0x628cca (_CIfmod: operands on
  the FPU stack) with no arguments, discards floor()'s result, calls transition_function_evaluate() without
  arguments, and its header size (185) is far short of its code (past 0x4f9630). Needs a full objdump rewrite ->
  STUCK for the loop. (fmod sites: 0x4f94be fmod(value, +0x13c); 0x4f961a fmod(x + fn_value, 1.0 double 0x672af8).)
- player_delete (0x473ae0): network_machine_clear_flag_by_id is (EDX server, EDI machine_id); EDI = machine_index
  (mov edi,eax at entry 0x473ae2), EDX = [0x71c2d4] at 0x473afd; the C passed only the server. Fixed; regcheck OK;
  game 0 failed. Nothing newly hookable (next: game_engine_player_changed_object).
- item_set_holder (ECX item, EDX holder): 3 callers passed no holder. EDX = unit_index at 0x56d7fb
  (unit_ready_desired_weapon), 0x56d296 (unit_try_select_equipment, [esp+0x14] after 4 pushes), 0x569c77
  (unit_refresh_targeting_flag_and_weapons). Newly hookable -> session2_new: unit_refresh_targeting_flag_and_weapons,
  local_player_set_controlled_unit, recorded_animation_start, recorded_animations_update. unit_pickup_weapon's own
  convention is wrong (binary reads ECX = unit at entry; its C has unit_index on the stack) -> next.
- unit_pickup_weapon (0x56d400) register notes fixed: ECX unit, EAX weapon, stack pickup_mode (0x56d40f). None of its
  10 callers is stable (so not the guns cause). The callers all declare it wrongly -> next firings map EAX/ECX per
  site (actor_apply_unit_definition_properties and unit_spawn_with_starting_weapons first: NPC weapon spawning).
- unit_pickup_weapon: all five helper calls fixed against 0x56d4c7..0x56d5df (each was missing the unit or an
  argument). It next blocks via unit_find_next_zone_permitted_weapon_slot -> unit_check_weapon_use_permission.
- unit_find_next_zone_permitted_weapon_slot: permission check needs ESI = unit (EAX at entry, 0x56dbaf) and EDI =
  weapons[slot] (0x56dbe7); the C passed only the unit. Newly hookable (session2_new): unit_find_next_zone_permitted_
  weapon_slot. unit_pickup_weapon next blocks via unit_detach_from_seat -> datum_get declaration.
- unit_detach_from_seat: datum_get(EDX = unit +0x218 controlling_player, ESI = player_data) once at 0x56c97a; the C
  called it twice with no arguments. Fixed.
- object_for_each_light_attachment (EAX object, push register_in_table, invoke_callback): new
  tools/fix_light_attachment_callers.py; applied to 4 callers whose EAX object is verified
  (unit_detach_and_enter_named_seat, unit_detach_from_seat, unit_release_transient_state: frame [ebp+8];
  unit_try_select_equipment: EBX). Held back (EAX not traced): unit_detach_child_at_named_seat (esi),
  unit_ready_desired_weapon (esi/ebx, call order ambiguous), unit_seat_candidates_from_zone_and_enter ([esp+0x10]),
  unit_try_exit_controlled_seat (esi), unit_try_start_seat_exit_animation (edi).
- unit_detach_from_seat (0x56c640), next blocker object_set_position_and_orientation: NEEDS A SECTION REWRITE
  (0x56c6bb..~0x56c860), logged not fixed. From the binary: object_get_node_local_transform(vehicle, exit marker
  name = seat block +0x24 (stride 0x11c, unit tag +0x2e8), &marker [ebp-0xd8], 1); offset = [edi+0x28..0x30] - marker
  ([ebp-0x78..-0x70]) where EDI's source is not yet traced; node0 = unit model +0xbc, root translation +0x28 and
  [ebp-0xc] = node0 +0x68; position = offset + object position (+0x5c..+0x64), z minus node0 translation.z
  ([ebp-0x10]); object_set_position_and_orientation(stack unit, NULL, NULL; EDI = &position) at 0x56c7f0; then
  multiply(nodes[0] (object +0x1f2), [ebp-0xc], &m [ebp-0x68]) and copy m.forward -> +0x74, m.up -> +0x80. The C has
  placeholders (saved[3], matrix4x3_multiply(0,0,0)). STUCK for the loop.
- device_play_state_change_effect: effect_new_on_object (EAX creator, ECX definition, 6 stack) called as (object,
  tag_id, object, -1, position +0x208, power +0x1fc, NULL, NULL) at 0x44c1fd..0x44c214; the C had definition = -1 and
  everything shifted. Fixed (+ effects.h). regcheck OK; devices 0 failed.
- device_play_state_change_effect sound branch: sound_start_at_object_marker(ESI object, ECX [0x6966f8], EAX
  [0x696718], push tag, -1, 1.0, 0) at 0x44c1de..0x44c1f3; the C had four stack values only. Fixed.
- unit_predict_aim_target_position: collision_bsp_query_segment_init(EAX 1, ECX &result, bsp, 0, 0, &base, &delta,
  FLT_MAX) at 0x571e50..0x571ec1; the hit fraction is result.t and the return value result +0x8 (the C never set
  either). Fixed. (effect_update, 1358 bytes, left for a supervised session.)
- ai_reference_respawn_member: actor_squad_react_to_grenade(ESI actor = [esp+0x14], push prop, EAX 3) at
  0x432e56..0x432e5c; the C passed the prop as the actor. Fixed. Next: actor_find_or_create_shared_prop's
  actor_target_has_conflicting_neighbor declaration.
## 12:42 play-test: without the 16 anim fns -> guns still missing
## 12:44 play-test: without the 29 render fns -> GUNS BACK
## 12:45 play-test: without bisect_guns_rA (14 state fns) -> GUNS BACK
## 12:46 play-test: without bisect_guns_rA1 (7) -> GUNS BACK
## 12:47 play-test: without model_draw_prepare/restore + set_shader_stage_config -> GUNS BACK

# Step 1 (code gap) notes, 2026-09-26
- STUCK/BUG (MP client only): game_engine_players_update_client.c declares unit_apply_control_block as
  (void *record_or_field EDX, int32_t grenade_value ECX), but the definition (0x5639f0) is
  (EAX unit_index, EDX control, stack source_id). Its call sites need checking against 0x474590 before
  that function is trusted in a multiplayer client test. Its ctrl.weapon_index/zoom_level assignments from
  "carried_*" also look suspect. Not touched in the step-1 commit.
- unit_spawn_with_starting_weapons (0x572110) has rewrite confidence 0.2 (opaque decode step): on the
  incomplete list.

# Session 3 (step 1: close the code gap), notes at loop start 2026-09-26
Done and committed this session: 10 missing game functions rewritten + promoted after campaign test (d598e3e);
players_server_catchup_on_client_updates (0x4768c0, host only, unsafe: not hooked); physics chain under the
biped solver verified/rewritten from the binary: physics_sweep_capsule_step (real regs EDI origin, ESI delta,
EBX out_velocity, ECX exclude; stack flags, pill_height, pill_radius, out_position, max_contacts, contacts;
returns int16 count), physics_model_build_from_sphere_query, physics_shape_build_proxies_from_query (EDI result,
EAX matrix, stack bsp, margin, thickness, object_index, model), physics_shape_add_vertex_proxy (stack matrix,
height, radius, model), collision_gather_nearby_object_shapes, physics_model_slide_along_contacts (true size
2028), vector3d_project_onto_direction (0x506760, new), physics_shape_polygon_test_ray, physics_shape_test_ray,
physics_shape_vertex_to_sphere (memory order), real_matrix4x3_rotation_from_forward; biped_movement_solver_data
is 0xcc (result_blocked_distance at 0xc8).
Remaining step-1 code gap:
- physics_shape_vertex_to_sphere and physics_shape_polygon_test_ray are still in harness/known_bad.txt (old random
  difftest results). They were re-verified against the binary; difftest_all skipped them (not in the safe table
  while listed). Remove their known_bad lines, rebuild, re-run difftest; keep them out only if they still differ
  for a reason other than random-pointer artefacts.
- biped movement solver FUN_0055efd0 (5157 bytes, callers biped_integrate_movement[_with_collision]): not written.
  Frame: Ghidra local_X lives at esp0 + (0xafb0 - X) where esp0 = esp after the 4 register pushes; its Ghidra
  decompile (python tools/pack.py 0x55efd0) is structurally right but lost every helper argument; take those from
  scratchpad/annot.py-style disassembly. Helper conventions: real_matrix4x3_rotation_from_forward (ESI fwd, EBX
  left, EDI up), vector3d_normalize_with_length (ECX), vector3d_cross_product (EAX out, ECX a, push b; b x a),
  point3d_add_scaled (EAX out, ECX dir, push base, scale), vector2d_normalize_with_length (ECX),
  physics_sweep_capsule_step (see above; call at 0x55f766), structure_bsp_plane_fetch_signed (0x44dad0),
  vector3d_length (EAX), physics_model_build_from_sphere_query, physics_shape_test_ray. Contacts array
  16 x physics_model_contact at esp0+0xbc. Writes solver +0xc8.
- breakable surface damage FUN_00500090 (4764 bytes, callers breakable_surface_apply_damage /
  breakable_surface_damage_in_blast_radius): not written.
- biped_ground_adjust_solve (0x558000): calls sphere query / slide with invented signatures (model is the global
  0x6e4d08, slide EAX = start); needs a rewrite.
- object_collision_context_gather_sphere_shapes (0.30) and object_physics_add_mass_point_shapes (0.35): bodies
  unverified (their call conventions match).
- MP-client twin game_engine_players_update_client: unit_apply_control_block declared wrong (see Step 1 notes).
- Then build/incomplete_rewrites.txt (299) and harness/known_bad.txt (90).
- [firing 1, 2026-09-26 afternoon] Item 1 done: removed physics_shape_vertex_to_sphere and physics_shape_polygon_test_ray
  from known_bad.txt. difftest now reports rewrite crashes separately (harness/difftest_main.c): over 764 samples
  both functions have 0 value differences; every failure is a crash where a random int16 count (sphere/pill count,
  projection index) sends the write/read far outside the 1 KB buffer (the original also faults on over half the
  inputs). Kept out of known_bad. Newly hookable (build/step1_session_new.txt, NOT in hooks.txt):
  physics_shape_add_vertex_proxy, physics_shape_build_proxies_from_query, physics_shape_polygon_test_ray,
  physics_shape_test_ray, physics_shape_vertex_to_sphere, vector3d_project_onto_direction. Relinked.
- [firing 2] Item 2a: object_collision_context_gather_sphere_shapes (0x505200) body checked against the binary.
  Fixed: the permutation byte is zero-extended into DX and compared with 0xffff (never true), so 0xff is NOT
  skipped -- it is clamped to the last BSP; the region index is sign-extended. Everything else matched (matrix
  inverse / transform / sphere query radius = inverse.scale * radius_scale / proxies with the node matrix).
  TODO: siblings object_collision_context_test_pill / _test_point / _test_segment use the same
  `region_permutations[node->region]` + 0xff pattern; check each against its binary.
- [firing 3] Item 2b: object_physics_add_mass_point_shapes (0x507790) rewritten from the binary: the draft called
  physics_shape_vertex_to_sphere with an invented signature and dropped the transformed mass-point position.
  Now: position through the context matrix, one vertex_to_sphere per mass point (height x_offset, radius
  mass radius * scale + y_offset, material/surface -1, flags 0, breakable -1). Item 2 complete.
- [firing 4] Item 3, section 1/4: biped solver draft in build/drafts/biped_movement_solve.c (NOT in src/ until
  complete -- a partial C definition would replace the linked original for the C callers). Section 1 (target
  velocity, 0x55efd0..0x55f6d6) written and checked instruction by instruction: flying (rotation frame,
  acceleration clamp), 0x20 path, airborne (2D air control, gravity), ground (aiming frame for climbs_any_surface /
  lift onto ground plane / steep-ground frame with the x5 vertical, slope speed scaling with exact x87 compare
  semantics, acceleration clamp, 1/128 ground-normal push). Compiles clean. Note: flags bit 0 is "airborne", not
  "grounded" as types/units.h says (result bit 0 too) -- fix the enum names when the solver lands.
  Next: section 2 (sweep + ground-edge snapping, 0x55f6d6..0x55fce4).
- [firing 5] Item 3, section 2/4 (0x55f6d6..0x55fce4): capsule sweep (model flags 0 / 0xc0a0 / 0xc2a0 / 0x20c3a0, delta
  = result velocity + height_change, 16 contacts, result bit 3 when full) and ground-edge snapping (walk the edges of
  the ground surface, pick the nearest walkable neighbour the velocity heads into, step over onto it when within 2r,
  dot <= 0.0533 and |height| <= r/2, push velocity by 1/30, synthesize contact[0]) written and checked; compiles.
  Next: section 3 (ground contact choice, 0x55fce7..0x560088).
- [firing 6] Item 3, section 3/4 (0x55fce7..0x560088): ground contact choice written and checked -- lateral
  direction normalized (1e-8 guard), best contact = walkable first (snap surface / highest along velocity), else
  highest normal.k; 'moving' flag from flag-8 or non-scenery object contacts; landing rejected when steeper than
  cos_max_slope (NaN too) or by the unknown_5c/unknown_60 distance test; airborne writes k_default_resting_plane.
  Quirk kept as documented: the snap-surface test reads the CURRENT BEST's surface (contacts[-1] slot with no best).
  Compiles. Next: section 4 (0x560088..0x5603f5).
- [firing 7] Item 3, section 4/4 (0x560088..0x560400): contacted-object ranking (vehicles first, largest relative
  velocity -> solve+0x98), device-machine ground object (-> +0x9c, salt-checked lookup, type 7 via the int8 sign
  of 1<<type), results (swept position/velocity, +0xc8 = |swept - requested velocity|, z minus height_change) and the
  crouch stand-up probe (sphere query + up ray -> result bit 4). True size 5169 (Ghidra 5157: the take block at
  0x5603ef lies past it). Whole draft compiles. Next: move into src/units, switch both integrators from FUN_0055efd0,
  rename flags bit 0 to airborne, regcheck, build.
- [firing 8, user present] Solver moved into src/units/biped_movement_solve.c; both integrators call it; solver flag
  bit 0 renamed airborne. Unblocked the chain: real_matrix4x3_rotation_from_forward and
  physics_model_slide_along_contacts difftest 0 differ (400 / 57 samples) and came off harness/incomplete_rewrites.txt
  (NOTE: gen_hooks reads harness/incomplete_rewrites.txt, not build/incomplete_rewrites.txt);
  collision_bsp_query_sphere_node_recursive checked against 0x501a10..0x501c87 (NaN branches, signed ref count) and
  unlisted; integrators pass unit->actor_index (ECX) to actor_check_vehicle_mode_timeout. 16 newly hookable, incl.
  biped_movement_solve and the whole sweep/slide/sphere-query chain (build/step1_session_new.txt). Integrators still
  blocked: they declare biped_update_animation_frame_trigger differently. hooks.txt == stable.

# Standalone generator
- stage 1 (tools/gen_standalone.py -> build/standalone/): data blob (.rdata 240 KB + .data 172 KB of 2.1 MB) for
  0x63a000..0x890df0; 315 import slots (46 delay-load) from 15 DLLs; 1405 code pointers in data: 735 -> C rewrites,
  670 -> library code that the standalone replaces (D3DX vtables 567, CRT init tables, GameSpy, zlib/png/hwreq, EH
  funclets): dead data there. Entry chain: CRT 0x627f06 -> shell_winmain 0x5411e0 (rewritten, 0.7; never hookable).
  Next: stage 2 = loader.c (reserve + copy image, fill IATs, write the 735 C pointers) + a trial standalone link that
  lists every unresolved symbol (the 237 stubs into original code, library calls).
- [firing 9] Item 3 cont.: both integrators now call biped_update_animation_frame_trigger (0x55eaa0) with the binary's
  arguments: stack impact speed, ECX = the Biped tag (EDI, reloaded from the tag slot stored at 0x55bf06 / 0x55d064),
  ESI = the object. Next integrator blockers: biped_integrate_movement declares unit_get_crouch_height_offset
  differently; _with_collision declares biped_update_target_lock_timer differently.
- [firing 10] Item 3 cont.: (a) both integrators passed unit_get_crouch_height_offset no position -- the binary
  passes EAX = &solve.start_position (it is what fills it); the C left start_position UNINITIALISED (real bug on the
  C path, sweep origin garbage). Fixed. (b) biped_update_target_lock_timer (0x55e0a0) rewritten: EAX target, ECX
  biped, callees with their register args; the caller passes solve+0x98. Next integrator blockers: vector3d_cross_product
  declared differently (biped_integrate_movement), matrix4x3_transform_plane (_with_collision); target-lock via
  actor_find_or_create_shared_prop (known long-standing ai blocker).
- [firing 11] Item 3 cont.: vector3d_cross_product prototyped in both integrators (argument order checked at all
  four call sites: EAX out, ECX forward, push desired_facing). Next integrator blockers: vector3d_rotate_about_axis
  (biped_integrate_movement); _with_collision's melee-lunge block (0x55de..0x55dfxx: ray_intersects_sphere_test,
  object_collision_context_build/test_segment, collision_test_movement_segment, matrix4x3_transform_plane,
  unit_process_melee_special_interaction) is largely unresolved in C -- needs its own rewrite from the binary.
- [firing 12] Item 3 cont.: vector3d_rotate_about_axis prototyped in both integrators (EAX forward copy, ECX up,
  stack sin, cos -- checked at 0x55c374 / 0x55d4bc); the |dyaw| >= 1e-4 guard now follows the binary's jnp (NaN
  rotates).
- [firing 13, user present] weapon_get_zoom_fov (0x46fe10) is a difficulty-scaled value lookup (stack index, CX
  difficulty). All 13 external binary call sites pass CX = [0x006b0b80]+0x0e (game globals difficulty); 8 C callers
  declared one argument and dropped it, projectile_update passed the two reversed. tools/fix_difficulty_value_callers.py
  fixed all 9 files (no caller is in the stable set, so play is unaffected; C-to-C paths were wrong).
  ai_get_difficulty_request newly hookable. biped_integrate_movement next blocker: unit_update_up_vector calls original
  FUN_00628140 (CRT area).
- [firing 14, user present] unit_update_up_vector (0x560800) rewritten from the binary (was 0.15: invented operand
  pairings): flying frame rolled by unknown_510, climb-any-surface frame (turn toward the ground normal at most 10
  degrees past a flip, forward = T x (forward x T)), dead bodies align to the ground via acos (0x628140 is the CRT
  _CIacos), everyone else levels to world up. biped_data.flags bit 0 comment corrected (airborne).
  biped_integrate_movement is now HOOKABLE (with the solver chain). _with_collision still needs its melee-lunge block.
- [firing 15] INVESTIGATED (not yet rewritten): _with_collision's melee-lunge block (0x55de09..0x55dfc3) and its callee
  unit_process_melee_special_interaction (0x56ff40; true end 0x5701a1 = 610 bytes, not 512; confidence 0.2; only caller
  is the lunge block). Real call: EAX = attacker (the biped), 7 stack args: (target = biped+0x4f4 melee target,
  a2, a3, a4 = dwords from the segment-test result in the caller's frame, point* = the lunge contact point, plane* =
  the transformed contact plane, result* = the movement-segment result). Body: (1) Unit tag flags 0x2000 and target is
  a biped (type 0) with +0xe4 > 0 and its tag flag 0x400000 -> unit_cause_melee_damage(attacker, 1, target, a2, a3,
  a4, plane) [7 stack args; its C note says EAX = unit -- recheck], object_set_health_frozen_flag(EAX attacker),
  object_delete(EAX attacker). (2) else tag flag 0x1000 and target type in {biped, vehicle}, alive, target's parent
  chain not the attacker and all bipeds: zero velocity (+0x68) and angular (+0x8c) from 0x696714, forward = -plane
  normal, up rebuilt via cross products (b x a helper), object_set_position_and_relink(ESI point?, push result*),
  object_attach_to_object(a2, attacker, a3), flags |= 0x20, unit flags |= 0x8000, unit_try_ready_weapon(1, 0).
  NEXT: confirm the conventions of unit_cause_melee_damage, object_set_position_and_relink, object_attach_to_object,
  unit_try_ready_weapon; map a2..a4 in the caller's frame; then rewrite both.
- [firing 16, user present] TOP BLOCKER data_packet_group_decode_packet (31): its own rewrite matches the binary (0 value
  differences / 351 samples; the crashes come from random group->types pointers) -> off known_bad. It now shows as
  'declares X' for 26+ network handlers, and the real problem is structural: at every one of its 34 call sites EAX is
  &length (the handler's own STACK length argument, minus the 2-byte header at 16 of them), but the C handlers have no
  length parameter, and the C client dispatcher network_game_message_decode_dispatch (0x4db6b0: EAX client, EDX record,
  EDI length, stack context) calls all 18 handlers with no arguments. Handler conventions differ (beacon: EAX client,
  EDX buffer, push length; most others push context/length/buffer) -> read each handler's own prologue.
  Done: network_game_client_decode_beacon_reply (template). Next: remaining handlers one by one (list = the 34 call
  sites in the firing-16 survey), then both dispatchers. Beacon is now blocked by network_game_search_results_add_or_update.
- [firing 17] decode-packet callers, handler 2: network_game_client_decode_pong_reply (stack length + sender address;
  decoded pong = send time, remote time) and the chain under it: network_connection_retransmit_if_overdue (0x4d93b0) has
  a stack remote_time and calls message_delta_sample_record_and_append(EAX send, ECX now, EDX remote, stack ring) and
  ..._ring_buffer_average(ECX ring); record_and_append's ring is its STACK argument (0x4ed328), not ESI. Also
  network_game_client_update passes the ring to the average (0x4db0ca). Unlisted from incomplete. Now blocked by
  message_delta_sample_ring_buffer_append (incomplete: 'simplified') -- next.
- [firing 18] message_delta_sample_ring_buffer_append (0x4ed390) fixed: the fill branch resets write_cursor to 0
  (0x4ed3be -> 0x4ed3f4; the old difftest +0x8 difference) and the average sums SIGN-extended samples (cdq/adc).
  Off known_bad. Newly hookable: ring_buffer_append, record_and_append, retransmit_if_overdue, pong_reply.
  beacon_reply still waits on network_game_search_results_add_or_update (incomplete).
- [firing 19] decode-packet callers: the 'ESI client; stack buffer, length, sender_address' handler family (each
  checks the sender against network_channel_remote_address_or_default, then decodes &length-2). Rewrote join_complete
  (sender deref'd once, not twice), sync_complete (length was passed as a pointer), player_config_value (2-byte value
  -> client+0xed8) -- all hookable. New helper tools/replace_function.py. 22 callers left.
- [firing 20] decode-packet callers, ESI-client family: and_discard_ingame_message (state 4, class 6),
  and_discard_join_message (state 2, class 2), join_finalize_ack (state 2, class 2, then
  network_client_timer_default_or_disconnect(EAX client)) -- all hookable. 19 callers left; next in this family:
  join_accepted, join_finalize_message, settings_or_ack, ingame_notification, settings_request (larger bodies).
- [firing 21] decode-packet callers: join_accepted (state 1, class 2 -> network_session_player_join_notify(EAX client,
  ECX &decoded); sender deref'd once) and join_finalize_message (another sender -> 1; else finalize_join's result)
  rewritten; both now blocked deeper (bit_stream_write_bits_chunked declared differently in join_notify;
  network_game_scenario_load_request under finalize_join). 17 decode callers left.
- [firing 22] object_recalculate_bounding_radius is logged STUCK (needs a runtime comparison) -> took XCreateSaveGame
  (17): EAX is the save game's Unicode NAME, the source of the ASCII name all paths use (the draft called it an unused
  token and converted nothing: every save path was built from an uninitialised buffer on the C path); ret 0x10
  stdcall; the Name= line converts into a scratch buffer (EAX dst, EBX src, EDI 0x80); the file receives its own path.
  Root cleared; newly hookable incl. XCreateSaveGame, saved_game_name_is_available.
- [firing 23] decode-packet callers: network_game_message_decode_ingame_notification rewritten (ESI family; state 4,
  class 6; returns the decode result; +0xedc default 8; handoff flag + chat_close unless hosting with bit 2).
  Hookable. 16 decode callers left (settings_or_ack, settings_request, replicated_command, state_update_chunk,
  the server-side 'role' handlers).

# Standalone generator / first boot (2026-09-26)
- tools/gen_standalone.py (stage 1) + tools/gen_standalone_link.py (stage 2) + standalone/loader.c build
  build/standalone/halo_rebuilt.exe; tools/standalone_boot.sh relinks, boots 60 s, prints the symbolized log.
- Boot now: loader -> shell_winmain -> config.txt (hwreq parser) -> window -> Direct3D -> loading splash shown ->
  shaders\fx.bin decrypts and verifies (TEA + MD5 in C, checked against Python) -> D3DXCreateEffect fails for all 122
  effects: fx.bin holds 2003-era compiled effects (chunks start 0xFFFFFFFF, not fx_2_0 0xFEFF0901); June 2010 D3DX
  parses them as HLSL text ("X3000 illegal character"). BLOCKER: needs a decision (convert the effect binaries, or
  keep the static 2003 D3DX).
- STUCK (arguments dropped via `(void)` externs; need call-site analysis, left as named traps):
  pow 0x6283c0 in game_engine_rate_location_ally_bonus, hud_draw_teammate_nameplate, hud_update_teammate_nameplate_fade;
  fmod 0x628cca in object_set_position_network (x2), object_update_functions (x2); floor 0x623e40 result dropped in
  object_update_functions; acos 0x628140 in biped_ground_adjust_apply_node_rotations, biped_ground_adjust_solve_node (x2);
  FUN_00625b7a (wcslen) void in server_browser_list_row_populate; fopen_00624186 mode 0 in objects_dump_memory.
- Data tables in the original .text read by C (need C tables before those paths run standalone):
  actor_movement_apply_steering_dispatch 0x418a04, actor_look_handler_table 0x41be00, jump_table_004a0268/_index_004a0283,
  hud_anchor_offset_handlers 0x4ab8b8, DAT_0051e2a0, DAT_00535fd0, unit_reaction_animation_handlers 0x561604.
- (later) fx.bin SOLVED: tools/convert_fx.py converts the 122 effects from the 2003 compiled format to fx_2_0
  (state ops remapped through both D3DX state tables, sampler states too, legacy FXLC float4 movs regenerated by the
  June 2010 compiler, resource element index added, header capacity dword). tools/fx_check.c loads all 122 and
  runs every pass: 122/122 ok. The loader redirects CreateFileA("shaders\fx.bin") to build/standalone/override/.
- standalone/d3dx_compat.c: 2003 ID3DXEffect vtable (0x00644d10, 71 slots) proxied onto June 2010 (Pass ->
  EndPass+BeginPass, slots 54..70 remapped).
- Boot now: splash, shaders, device reset, game_initialize (objects, widgets, decals, AI pools) -> main_loop ->
  UI map load (chimera__load_ui_map -> game_stop_current_map NULL read). 
- FINDING: ~952 function entries are only referenced from data tables (callbacks) and were never Ghidra
  functions, so never in the rewrite list (tools/gen_standalone.py traps them as unlisted_XXXXXX).
- Mis-declared callees fixed on the boot path: game_state_new (16 files), cluster_partition_new (2),
  rasterizer_vertex_buffer_slot_allocate, cache_new, rasterizer_gamma_brightness_to_exponent,
  rasterizer_display_mode_differs, D3D calls with missing stdcall args in rasterizer_device_reset.
- Still data without addresses (linker leaves them at 0): decal_clip_buffers, s_unarmed.
- Boot now also: UI map loads (cache_file_load, model buffers), game_start_new_map through ambient noise,
  observers, weather/sound pools -> scenario object placement (0x4f3ba0, misnamed
  objects_update_control_bindings, confidence 0.35) -> object creation -> bsp3d_node_find_leaf garbage BSP.
  Next: rewrite 0x4f3ba0 as scenario placement from the disassembly and walk the object creation chain.

## MAIN MENU REACHED (2026-09-26)
- halo_rebuilt.exe (standalone: all game code runs from our C; the original .text is mapped non-executable)
  boots through splash, UI map load and scenario placement, runs the UI scripts, and renders the main menu
  (CAMPAIGN / MULTIPLAYER / PROFILES / SETTINGS / CREDITS / QUIT) with the animated background; 60 s with no
  fatal fault. Screenshot: build/standalone/boot3.png (scratchpad/shot.ps1 captures the window).
- ui.map's scripts call only three hs builtins (read from the scenario's script nodes): begin, sleep, camera_set.
  New C: hs_evaluate_begin 0x488b90, hs_evaluate_if 0x488e60, hs_evaluate_set 0x488fd0,
  hs_evaluate_sleep_ticks 0x489650 ("sleep"; hs_evaluate_sleep.c @0x489800 is sleep_until),
  hs_evaluate_camera_set 0x47ee40. ~510 other hs builtin evaluators still have no C (trap on first use).
- Sound: sound_channel_parameters_proc_default 0x54ce50 / _eax 0x54cf80, Ogg memory-stream callbacks
  ov_read/seek/close/tell_thunk 0x544d50..0x544de0.
- Mis-declared calls fixed: flags_update -> datum_next (1 arg instead of DX/EDI);
  game_engine_advance_simulation_ticks -> update_run_catchup_ticks (BX ticks) and
  network_game_server_per_frame_tick (CX, ESI); update_run_catchup_ticks rewritten (get_history_entry got a
  null tick pointer); update_server_queue_push_history (EDX ticks dropped, fullness test inverted) and its
  networking caller.
- STUCK/open: object_test_in_atmosphere_zone still calls datum_next() with no arguments (UNSURE draft);
  network_game_server_per_frame_tick.c forwards an EAX `entry` that 0x4e03c0 overwrites at entry (host path).
  A first-chance AV at 0x30254720 (outside our image; a driver/DLL thread) is logged once per boot and handled.
- cdb works for the real call chain: cdb -g -G -o -c "sxe -c \"r; kb 12; .kill; qd\" av; g" halo_rebuilt.exe
  (-o follows the suspended child the loader relaunches).

# Campaign track (2026-09-26)
- Retail console refuses map_name: hs_gametype_flags_applicable needs definition flags bit 0 whenever the context mask
  has bit 0 (always, unless forbidden), and only 34 hs functions have it (help, quit, cls, connect, sv_*, profile_load...).
  So -exec/init.txt cannot load a campaign; the campaign has to be reached through the UI.
- New C: Xbox ADPCM decoders sound_adpcm_decode_mono 0x54e920 / _stereo 0x54ea60; the 16 hs autocomplete collectors
  0x483880..0x483c50. Fixed: hs_autocomplete_scan_globals (spurious 4th arg; flags come from the definition +0xc),
  hs_autocomplete_add_startup (end index 5), both hs_autocomplete_gather callers (argument order).
- Loader: preloads the system dinput8.dll so the Halo folder's harness proxy (halo_rewrite.dll) no longer loads into
  the standalone (it was the source of the recurring AV at 0x30254720); traps now log a symbolized backtrace.
- Tools: tools/standalone_campaign.sh (exec route, kept for console tests), tools/standalone_ui_run.sh + scratchpad/keys.ps1
  (drive the menu with scan-code keystrokes, screenshots), scratchpad/cdb_trace.sh (breakpoint logging in the child).
- Open: first keystroke run (ENTER x3) got no menu reaction; investigating input path.
- NOTE FOR PLAY-TEST: hs_autocomplete_add_startup is in hooks.stable.txt and its behaviour changed (end index 4 -> 5,
  verified: 0x483865 mov eax,5; the console autocomplete now also offers "stub", as retail does). Needs a harness
  rebuild + re-test by the user; revert commit f8e963f's change to that file if unwanted.
- Level load via CONTINUE progresses into scenario placement and AI encounter spawning. Rewritten from the binary
  (drafts were 0.15-0.4): unit_update_stance_and_jump (unit ping reaction), effect_update, object_create_attachments,
  object_update_functions, object_initialize_change_colors (merged 3 Ghidra fragments), vehicle_blend_animations
  0x5718e0, device_blend_animations 0x44bc20, actor_apply_unit_definition_properties, actor_fill_unit_position_context,
  actor_refresh_combat_context, encounter_squad_spawn_actor, ui_level_select_confirm_choice,
  saved_game_load_checkpoint_by_name; sweeps: effect_new_on_object (13 call sites), color_interpolate (4),
  actor_set_units_active (12, BL flag per binary site).
- Generator: entries after a jmp or a .text jump table are now detected (27 table slots that pointed at original
  code); gen_link reads `extern T (*name[N])(...)` as data (3 particle tables had become traps).
- OPEN: actor_reset_squad_link_for_type_change (0x4290f0) now takes (EAX actor, EBX encounter, stack squad); its
  callers ai_squads_merge (0x4337a0), ai_unit_remap_actor_to_squad (0x433a3b),
  encounter_propagate_platoon_state_to_actors (0x435888) and ai_unit_set_squad_reference (0x439ede) still pass
  guessed arguments -- trace EBX at each site before relying on squad merges/maneuvers.
- OPEN: 0x00686b04 is a POINTER to a default colour/vector; game_engine_koth_* and lightning_render declare it as
  an inline array (multiplayer / lightning only). decal_place's geometry packing is still not reproduced.
- Test harness: loader key driver HALO_STANDALONE_KEYS (in-process keystrokes); CONTINUE = "30000:ENTER,3000:ENTER".
  Profile backup: build/profile_backup/Halo_2026-09-26 (the standalone may write checkpoints to the real profile).

## 2026-09-27 play-test session (campaign a10 via CONTINUE)
Fixed in this stretch: transparent group lighting source; the 10 unlisted structure bsp activate/deactivate
procs; the game state revert hook and after-load procs 5/8/10/12; the ECX request buffer for
unit_update_animation_state_machine (11 callers); unit_update powered seats; the limp body solve chain
(0x557a90/0x557b80/0x558000/0x558a20) rewritten from objdump; effect_event_apply (0x452cf0) and
decal_spawn_for_response (0x44ece0) rewritten; every sound_start_at_object_marker caller on the 7-argument
stable prototype.
OPEN:
- STUCK unit_can_see_point (0x56f800) / unit_melee_attack_scan (0x56f550): 0x56f800 starts mid-function
  (reads [esp+0x13c] first), so the two files look like a mis-split of one melee scan; the material hit
  effect call at 0x56fb73 passes ECX ebx / EDX [esp+0x144], not traced -- both C callers pass -1 for now
  (no sound). Needs a boundary check and rewrite.
- vehicle_update (0x570ee0) is a draft: unit_update_steering_deviation_effects is called with only the
  object (0x57162f also passes EAX = a frame array and ECX = a direction), unit_update_marker_skid_effects
  and unit_update_ground_contact_counter lack their arguments. Rewrite from objdump before driving.
- objects_recompute_cluster_membership (0x4f7570) rewritten: the draft passed bsp3d_node_find_leaf its
  arguments out of order, so re-linked objects (bipeds after a load) landed in bogus clusters and were
  never collected for rendering (debugger trace: 3 NPC bipeds updating, none reaching render_object).
- OPEN: bsp3d_node_find_leaf is declared (void) and its result misread in ai_broadcast_communication_event,
  camera_observer_find_best_target, game_engine_reattach_player_unit_unused, hs_damage_apply_at_location,
  hs_damage_apply_with_sound, unit_find_placement_position -- each needs a rewrite of the surrounding code.
- OPEN: scratchpad/call_arity.py lists ~1360 calls whose argument count differs from the C definition
  (none in stable callers); work through by module, hot paths first.

## 2026-09-27 overnight: a10 crewmen AI
The two near crewmen (actors e362/e363, units e3f8/e3f9) were being put to sleep every tick and the AI crashed
in a chain of drafts that called helpers without their register arguments. Now all five bipeds update every
tick for 300 s with no crash (the only fault left is keystone.dll during the forced shutdown).
Fixed/rewritten (all from objdump):
- dormant flag: actor_update_squad_link_state, actor_toggle_active_state, actor_squad_react_to_grenade passed the
  inverted BL to actor_set_units_active (renamed its parameter `dormant`; STABLE, code unchanged).
- actor index salt: 8 loops over actor_iterator_next derived a bare index from the pointer; now use
  iterator.actor_index (the "is this me" checks failed, so actors perceived their own unit).
- perception: actor_target_evaluate_squad_link, actor_find_or_allocate_prop, actor_init_prop_from_object (teams),
  actor_target_data_refresh (calls), actor_evaluate_engagement_reachability (AX/CX clusters, ESI/EDI points; 7
  callers), 0x41bb30 perception range test (720 bytes, 6 stack args), actor_danger_update_reaction,
  unit_add_marker_relative_offset (+3 callers), actor_consider_combat_mode, unit_get_weapon_marker_indices.
- movement: actor_update_movement_destination (fight process), actor_select/claim_firing_position (path context
  argument), actor_movement_set_destination_firing_position, the guard/uncover/avoid process procs
  (0x4049d0, 0x408300, 0x4017b0), actor_check_melee_target_reachable, actor_find_best_firing_position calls,
  actor_firing_position_near_point, path_find_compute_heuristic (vertex id), path_find_reconstruct_path,
  path_find_find_unobstructed_ancestor.
- new: fight mode tick/update (0x403540, 0x4035b0), crew type update, alert mode procs.
- actor_update_activation_state called the mode's enter proc with no arguments; it is the +0x10 tick proc.
- teams: ai_allegiance's worker (0x433ba0) dropped both teams; 9 callers of teams_are_enemies /
  team_pair_flag_test passed no teams (incl. object_apply_damage and the LOS damage friendly-fire test).
- units: melee start (0x569a20/0x569b30), begin_throw_grenade and scripted action animation dropped the
  direction pointer (ECX to 0x5704d0); actor_apply_queued_look_to_unit rebuilt the unit_control_data.
- STABLE touched: actor_get_threat_weapon_object_index now reads the variant tag (+0x5c) like the binary
  (the stable version read +0x58); actor_set_units_active parameter renamed only.
OPEN:
- Crew hostility explained: CONTINUE restores the profile checkpoint, and the real profile's savegame.bin had been
  overwritten by earlier standalone runs (01:11 and 05:15 today) with a state carrying the old broken ai_allegiance
  entry (0,0,...). The 2026-09-24 backup (build/profile_backup/Halo_2026-09-26, the original game's save) has a
  proper player/human override (1,2, threshold 5, 300 ticks) but marked hostile, so crew vs player hostility is the
  saved state, not a code bug. mission_a10 had run ai_allegiance before that checkpoint (thread 0x4f resumes inside
  the cinematic_skip `if`).
- PROFILE: test runs now use `-path build/standalone/profile_sandbox` (a copy of the backup) via
  scratchpad/cdb_auto.sh, so they never write the real profile. The real profile (Documents/My Games/Halo) still
  holds the saves written by the earlier standalone runs; restore it from the backup if the original save is wanted
  (not done automatically). Launching halo_rebuilt.exe by hand without -path writes the real profile.
- ai_notify_actors_of_encounter_state_change (STABLE) still derives a bare actor index from the pointer.
- ai_communication_broadcast: the teams_are_enemies locals at 0x42d7ac ([esp+0x14], [esp+0x4c]) are mapped to
  self/other_team_packed by name only; the 0x42ba80 call there pushes (esi, edi, [esp+0x54]) which does not
  match the C argument order -- verify.
- unit_detach_from_parent (0x570140) is a fragment of the MISSING 0x570000; 0x402679 and 0x47c2e0 (melee
  starts) and 0x404cb4 (a dormant call) are inside unwritten functions.
- 17 AI files still carry SIGNATURE-CONFLICT externs (grep SIGNATURE-CONFLICT); 420 (void)-extern calls to
  functions that take arguments remain (scratchpad/voidext.py).

## 2026-09-27 (loop) -- a10 intro cinematic plays, purple tint fixed
Result: from the pristine save (sandboxed profile) CONTINUE loads a10 and the opening cinematic plays with
correct lighting (Keyes on the bridge, NAV displays); 200 s under cdb with no crash (the only exception is
keystone.dll faulting in its DLL detach after `timeout` kills cdb, exit code 143).
- PURPLE TINT: object_build_effect_parameter_block (0x4f2ff0) had its parameters in a different order from its
  caller; rewritten (ambient 0.4*lm+0.03, distant lights from the lightmap and base colours, shadow vector).
- particle systems bsp activate proc (0x454080), AI bsp deactivate proc (0x42c940), sky animation call,
  hs object teleport (0x487f50), effect marker node table resolver (0x451850/0x451930) -- as before, now committed.
- unit_set_or_test_seat_and_weapon_label (0x5651e0) REWRITTEN: the draft skipped weapon slot 0 and stopped at the
  first match, so single-weapon graphs (vehicles) never got +0x2a0/+0x2a1 and unit_try_set_animation_state indexed
  block -1 (the block_animation crash). Param 3 is "apply" (0 = test only).
- unit_ready_desired_weapon (0x56d6e0) REWRITTEN (stack unit, force; every caller passes 1); 4 callers fixed.
- unit_enter_vehicle_seat (0x566970) REWRITTEN (EAX unit, stack vehicle, seat): position delta into the seat
  marker frame, enter animation slot 7, occupant recompute on the vehicle. Callers fixed: actor enter-vehicle mode
  (0x408d4f), ai_process_vehicle_entry_queue (REWRITTEN, placement request was dropped), spawn loadout, named seat.
- light_volume_render (0x4fe900) queued the wrong arguments; NEW light_volume_render_procedure (0x4fea80, the
  lens flare callback, was unwritten). object_attachment_get_blended_marker really blends a frame with itself in
  the binary (edx on both sides) -- the draft is right.
- NEW antenna_render_geometry (0x4fb340); antenna_update_physics called point_physics_tick by another name (trap).
OPEN:
- point_physics_tick (0x50b530) is 0.45 confidence and now runs for antennas/particles/flags -- verify.
- ~345k first-chance access violations per 200 s run under cdb (handled somewhere); find the source.
- real profile still holds saves written by earlier standalone runs; backup in build/profile_backup (not restored).

## 2026-09-27 (loop, later) -- a10 reaches gameplay, 400 s without a crash
Result: CONTINUE -> the full opening cinematic (bridge, cryo bay crew) -> first-person gameplay in the cryo
bay; the run stays up 400 s (screenshots scratchpad/shot_270.png / shot_400.png: correct lighting, crew visible).
The view is static because the level's calibration sequence waits for player input.
Fixed this stretch (all rewritten or checked against objdump):
- structure surface callbacks passed literal original addresses (0x511f20..0x511f80) -> ~1000 exceptions/s
  through the harness redirect; now the C rewrites.
- colour helpers: HUD meter / waypoint int->real (out, packed) order, HUD meter / weather / lens flare
  color_interpolate (EAX color1, ECX color0, dest, flags, t) -- the HUD crash on the first gameplay frame.
- AI pathfinding end to end: path_find_run + search loop (map at +0x64), avoidance penalty segment,
  trace_bsp_boundary, test_segment_unobstructed, trace_cluster_boundary, choose_shorter_corner,
  simplify_waypoints, the obstacle point search (add_node, nearest_visible_point, evaluate_edge_cost,
  expand_point_neighbors, step, gather_obstacles, navigate_around_obstacles) and reconstruct_path.
  Verified unchanged: heaps, context_init/run, covering point, tangents, ray/circle, flood fill, partition,
  append, trace_from_vertex, heights_are_close, circle tangent/portal crossing. types/ai.h path_find_waypoint.
- game_engine_build_visible_cluster_bitmask: bit_vector_or lost 3 of 4 arguments.
- unit_rotate_basis_about_axis (0x55e6b0) takes only the object (axis = angular velocity); idle fidget cross
  product (global up x unit up).
OPEN:
- 0x43c5d0..: evaluate_edge_cost's second side pair traces +normal twice (binary quirk, reproduced).
- actor_check_step_obstruction calls 0x429570 with no arguments (check its registers).
- point_physics_tick (0.45), path_find_push_start_node (0.35), path_find_compute_heuristic (0.25),
  path_find_test_direct_reachability / validate_and_record_goal (0.3) still to verify.
- a mis-carved function file (biped_build_update_delta_unit_grenade_count_mod1, 0x55e9ff) lies inside
  biped_apply_idle_fidget (0x55e940).

## 2026-09-27 (loop) -- units: biped_update and the seat exit family
- biped_update (0x5590a0) REWRITTEN (was 0.3 with ~12 argument-less calls); smoke test OK (crew cinematic +
  gameplay 330 s).
- 0x56b5f0 unit_try_exit_controlled_seat, 0x56c470 unit_try_start_seat_exit_animation, 0x56c640
  unit_detach_from_seat, 0x56ab50 unit_detach_child_at_named_seat REWRITTEN: all share the seat exit /
  detach code biped_update inlines (sections compared by mnemonic sequence).
- 0x56ab10 always sets state 0x25; weapon_set_state and seat overlays a/b permutation/overlay calls fixed.
OPEN (units, by size, all < 0.5): unit_update 0x5625b0 (0.15, 4765 B) -- next; unit_apply_damage_effects
  0x5674a0 (0.2, 3199 B; inlines the seat exit again); vehicle_update chain; unit_release_transient_state;
  unit_seat_candidates_from_zone_and_enter; unit_detach_and_enter_named_seat; unit_can_see_point (0.1).
  AI copies of the exit block: actor_process_vehicle_seat_exit, ai_reference_units_exit_vehicles.

## 2026-09-27 (loop) -- unit_update + speech arbiter
- unit_update (0x5625b0, 4765 B, was 0.15) REWRITTEN; smoke test OK.
- unit_dispatch_reaction_animation (0x5614a0): Ghidra split it at its jump table and the draft called labels
  inside it; REWRITTEN (355 B). unit_animation_change_priority_check (0x560d00) REWRITTEN with its DL
  follow_fallback argument; callers fixed: unit_play_default_reaction_sound (chain = the sound, commit mode
  max(r,2), record line with the unit), unit_choose_combat_reaction_animation (call + mode only),
  actor_squad_action_execute case 0x10, actor_update_grenade_eligibility_state.
OPEN:
- ai_communication_* (broadcast 0x42d340, play_event_line 0x42eee0, line_fade_multiplier 0x42f8c0 -- own
  convention EAX priority / ECX chain ptr / BX range, conversation_current_line_is_ready 0x431e70) still call
  0x560d00 with the old argument layouts: needs a subsystem pass.
- unit_choose_combat_reaction_animation (0x561140, 0.2): the low-damage tail differs (+0x3ee = 0x3c).
- unit_commit_speech (0x560f20, 0.35) not yet verified.

## 2026-09-27 (loop) -- movement verified; the damage pipeline
- standalone/loader.c test key driver: W A S D E F G Q R X keys, `KEY~ms` holds, MUP/MDOWN/MLEFT/MRIGHT mouse
  moves (40 counts each) and FIRE. Smoke test: after the a10 intro, mouse look and walking both work (the view
  turns and the player moves through the cryo bay), no faults over the whole script.
- object_apply_damage (0x4ee5e0, 2939 B, was 0.2) REWRITTEN from objdump, and every callee on its path checked
  against the binary and rewritten or fixed where the draft dropped register arguments:
  object_apply_shield_damage 0x4ef820 (EBX damage record; transition type CX; shield-low effect EAX/ECX;
  stun ticks value), object_apply_body_damage 0x4ef2a0 (13 stack args; regions, deathless cheat, kill /
  teardown / damaged effect, hit effect registers), object_damage_notify_and_impulse 0x4efcf0 (target in
  EAX/ECX for every impulse call, item_accelerate target), unit_apply_damage_effects 0x5674a0 (3199 B; the
  seated-vehicle eject is the biped_update inline, helpers copied), player_effect_mark_damage_direction
  0x456cf0 + its network dispatch 0x456ad0, player_effect_send_network_update 0x456bc0 (EBX direction),
  unit_point_in_front_and_asleep 0x56bc80 (EDI unit), object_notify_pickup_or_refresh_probe 0x4ee3c0 (send
  args), unit_broadcast_state_change_event 0x566c00 (takes the new 0x20-byte unit_state_change_record BY
  VALUE; both callers copy it with rep movs).
OPEN:
- network_game_action_apply (0x4da320): every handler gets EAX = the action entry (and some ESI = ECX
  argument); the C declares ~45 of them as (void). Only case 0xb is fixed. Multiplayer only.
- network_session_send_to_machine callers in src/game (ctf/koth broadcasts, 0x46be40
  game_engine_queue_multiplayer_sound) pass constants where the binary passes EAX machine / ESI session.
- ai_reference_units_exit_vehicles still calls unit_state_is_scripted_animation() with no argument.
- damage path not yet exercised in a smoke test (a10's opening has no combat); needs a later level or a
  scripted damage source to confirm.
- also rewritten from objdump in this stretch: object_update_vitality_and_regeneration 0x4ed510 (shield
  recharge / overcharge / kill requests / damage timers; unlock BL 1 was 0), damage_effect_new_at_location
  0x4f0010 (the 12-value effect spawns; 0x69672c / 0x696718 are POINTERS), object_destroy_region's notify
  (EBX + region index + flags), ai_reference_units_exit_vehicles 0x433ea0 (the biped_update seat-exit
  sequence again), unit_validate_and_clear_weapon_switch 0x5659c0, player_swap_to_weapon 0x479240 and
  player_execute_weapon_drop_interaction 0x4790d0 (both read unit fields through an object pointer cast to
  unit_data, 0x1f4 bytes early), game_engine_notify_player_interaction 0x478ff0 (FOUR stack arguments: mode,
  type, seat, object), player_check_vehicle_boarding_interaction 0x4788a0 (really the item-touch handler:
  ammo, grenades, powerups, weapon pickup / swap prompt), player_execute_pending_interaction 0x4793a0 (the
  melee interaction writes its direction into the TARGET, not the unit).
- smoke test (a10, walk + look + fire + action key): no faults, deterministic end position.
OPEN:
- 0x4f0250 object_damage_effect_dispatch is a fragment of damage_effect_new_at_location (no callers).
- game_engine_spawn_player_starting_loadout / game_engine_apply_player_spawn_loadout_message still call
  unit_pickup_weapon with the old layout (multiplayer).
- object_physics_check_impact_damage 0x508b70 (0.35, 1355 B, 44 KB frame): vehicle-vs-biped impacts.
- actor_process_vehicle_seat_exit 0x40b080 (0.35, no callers in this build): another seat-exit copy.

## 2026-09-27 (loop, user testing) -- a10 cryotube start fixed
- The player started beside the cryotube instead of in it. Debugger trace: after tutorial_setup's
  unit_enter_vehicle the player unit had no parent / seat. unit_detach_and_enter_named_seat (0x569d40, the engine
  side of unit_enter_vehicle) REWRITTEN from objdump (seat search by label with _stricmp, occupancy test, seat
  label, unit_enter_vehicle_seat; the already-seated exit is the biped_update inline, helpers copied). User
  confirms: the game now starts inside the pod.
- Marker positions: object_get_node_local_transform's marker holds the node-relative transform at +0x04 (position
  +0x2c) and the WORLD transform at +0x38 (position +0x60). unit_get_camera_position, unit_get_primary/secondary
  _eye_marker_position, unit_point_within_look_cone (all STABLE), object_test_in_atmosphere_zone and
  ai_propagate_communication_reaction read +0x60 in the binary but +0x2c in C -- fixed. sound_impulse_start,
  looping_sound_new genuinely read +0x2c / +0x08 (checked).
- hs_object_runtime_cleanup / hs_objects_delete_by_type: role 0 objects are unparented (EDI object) AND deleted
  recursively; the draft passed nothing and skipped the delete. block_list_compact gets the object pool (EBX).
- unit_drop_current_weapon call arguments (weapon_put_away ESI/AL, action notify EAX, out-of-ammo EAX, zero-
  extended start slot); unit_release_transient_state (0x568610) REWRITTEN; its sibling 0x568cb0 calls fixed
  (object +0x334, not unit_data +0x334).
- AI prop release: actor_replace_object_reference (stack actor, ESI new, EDI old) and actor_unlink_prop (EAX
  actor, EDI prop) callers fixed in actor_target_scan_potential_targets (6), actor_target_data_release (+ the
  conflict test and dialogue calls), actor_clear_perceived_props (EDI is the prop, not its object),
  ai_clear_object_references, actor_target_relationship_think (10 call sites).
OPEN: an a10 NPC "randomly disappears" (user report). Not a script deletion (traced: only two object_destroy
  calls, both intro objects); render collection / cull sphere / bounding sphere / biped cluster relink all match
  the binary.

## 2026-09-27 -- the vanishing cryo tech (STUCK without a runtime trace; user asked not to boot the game)
- User: the yellow-suited cryo tech vanishes at gameplay start. Traces: no object_delete / ai_erase / ai_kill / death
  touches him; the only deletions then are two steam-jet emitters (e3fa0005, e3fb0007, script object_destroy).
  tutorial_setup's ai_place creates crewmen e3f80001 / e3f90003 at that moment (the ones earlier notes found with
  object +0x10 bit 0 set = model not drawn, 0x50eebd).
- In the binary bit 0 is set only by 0x4f6850 (object set visible; STABLE, matches): at creation (visible when the
  tag has a model) and by unit_update_animation_state_machine state 0x1a (seat enter: visible = !seat flag bit 0,
  reading the PARENT's seat -- garbage for an unparented unit). Plus weapons being holstered / picked up.
  No C writes bit 0 on the wrong object (checked all |= 1 sites).
- NEXT (needs one unattended trace): break on 0x4f6850 with visible == 0 and on unit_try_set_animation_state
  (0x565f90) with state 0x1a for e3f8/e3f9; dump the caller. Suspects: actor_new_and_attach_to_unit (0x426ac0),
  actor_apply_unit_definition_properties (0x426cf0), the tutorial's ai_command_list animation commands.
- actor_place_new_unit (0x427080, 0.15) decoded: matches except the host-only role table index (tag index used
  instead of the object type) -- not the cause.

## 2026-09-27 (loop, static only, user out) -- AI vehicle boarding chain rewritten
Motive: the vanishing a10 cryo tech. The a10 crewmen are placed at tutorial_setup; the one route by which an AI
unit is hidden without being deleted is unit_enter_vehicle_seat (state 0x1a copies the seat's hidden flag), and
every AI caller of it sat in low-confidence drafts. All rewritten from objdump, ai builds 0 failed, relinked
(1 unresolved, 130 traps, unchanged):
- 0x5640a0 unit_find_weapon_marker_transform (seat entry / seat marker / enter-hint points), 0x4091d0
  actor_evaluate_search_node (seat evaluation), 0x408f30 alert range (sum order, upright test).
- 0x4095c0 actor_avoid_obstacle_and_project: really the approach point round the vehicle, dropped onto the BSP.
  The draft read the ACTOR tag at +0x17c / +0x280 (those are vehicle tag fields) and lost EAX/ECX/EDX.
- 0x409070 actor_find_best_search_node: best seat; the direction output was missing and the entry/hint swapped.
- 0x408a30 / 0x408920: mode 9 order builders (EBX vehicle). Drafts had no vehicle at all.
- 0x408ba0 actor_investigate_disturbance_update: the mode 9 per-tick update. The draft passed the close / facing /
  in-front flags crossed, so "close" came from the in-front test and +0xc4 got the wrong flag; seat entry
  (unit_enter_vehicle_seat) could fire when the actor was merely in front of the seat.
- 0x40e260 (misnamed actor_play_first_valid_vocalization): board the first usable seat (EAX list, ECX vehicle).
  Callers fixed: actor_squad_action_execute case 9 (the object iterator layout was scrambled: mask 2 = vehicles,
  handle +8; the sample pair holds the vehicle, not a float), ai_object_process_nearby_actors (0x433cc0,
  ai_go_to_vehicle's engine side: EAX ai reference; seat search on the vehicle).
OPEN: whether the cryo tech is hidden this way needs the user's runtime check (no boot while they are out).
OPEN: the command-list / think callers of 0x408920 (0x40b029) and the ai_go_to_vehicle evaluators are unlisted
  callbacks (traps if reached).

## 2026-09-27 (loop, static only) -- ai_command_list atoms rewritten (drives the a10 crew)
The a10 cryo techs are scripted with ai_command_list (tutorial_setup / tutorial_introduction / tutorial_looking
...). The atoms run through actor_mode_obey_process -> actor_squad_action_list_process (0x406e30, checked) ->
execute / is_complete / reset_entry. All three were low-confidence drafts with dropped register arguments:
- 0x405520 actor_squad_action_execute REWRITTEN (all 28 atoms). Draft bugs: "go to" returned 0 on success and
  cancelled movement for a garbage actor; "look" atoms (4, 23-25) called actor_begin_vocalization without the
  actor and set the timer from its return value; animation mode wrote aim +0x00 (not +0x02); move-in-direction
  kept an unnormalized vector; the vehicle atom's object iterator was scrambled; teleport, recording, script,
  grenade and vocalize atoms had missing register arguments.
- 0x4066d0 actor_squad_action_is_complete REWRITTEN. Draft bugs: "go to and face" compared the facing against
  the raw offset (so an actor could stop before turning or never finish), no position fetch for another unit's
  move-in-direction, shoot timer from the atom instead of the variant +0x84.
- 0x406c50 reset_entry: the animate atom's biped flag cleanup now gets its unit (was object_try_and_get(1)).
- Checked, confidence raised: biped_update_idle_basis, biped_apply_idle_fidget, unit_apply_scale_change (really
  the scripted kill / vitality set), actor_squad_action_list_process.
- Hidden-bit audit: besides the seat state (0x565787) and set-visible at creation, only weapon holsters and
  player release set object +0x10 bit 0; all match. So a unit vanishing without deletion needs either a seat
  entry or a bad transform / position -- the teleport / move atoms above are the prime static suspects.

## 2026-09-27 (loop, static only) -- crew per-tick AI and movement
- Crew type update (0x423890) helpers: actor_process_order_request REWRITTEN (default orders now use the
  0x655590 kind table; idle fallback passes kind 0; builder results honoured); actor_wants_reload_or_swap and
  encounter_squad_clear_spawn_delay (ECX encounter / EDX squad, ESI packed squad reference) fixed;
  actor_update_grenade_and_morale_reactions REWRITTEN; actor_gate_jump_traversal's pain reaction gets CX / DL /
  ESI; combat behaviour (+0xa4 word), state-transition check, swarm iterator verified.
- unit_is_in_busy_animation_state takes the UNIT in ECX (actor +0x18): four AI callers passed the actor index
  (actor_queue_secondary_action, actor_action_has_queued_secondary [STABLE, proven by objdump],
  actor_check_grenade_facing_and_commit, actor_reset_queued_look_vector).
- actor_movement_apply_steering (0x4180c0) REWRITTEN: the draft "called" the cached-axis case labels at 0x418a04
  as functions (code addresses inside the original image) and returned; it also rotated the wrong vector.
- actor_movement_choose_avoidance_direction (0x4193d0) REWRITTEN: the draft declared the ray test and the
  sample interpolation with wrong arguments. actor_movement_test_obstacle_ray now returns its hit kind (AX).
OPEN: ai_communication_broadcast (0x42d340, 5.6 KB, 0.3) and actor_update_look_target (0x415480, 0.25) are the
  remaining large low-confidence per-tick AI functions.

## 2026-09-27 (loop, static only) -- actor mode callbacks (trap risk) and more per-tick fixes
- FOUND: the actor mode table (0x65524c, 0x38 per mode: name, ?, data size, combat grade, then 9 procs at +0x10
  enter, +0x14 process (returns AL), +0x18 tick, +0x1c update, +0x20 exit, +0x24 look weights (actor, out[4]),
  +0x28 replace reference (actor, old, new), +0x2c movement cancelled, +0x30 target cleared; all cdecl) had 42
  procs with NO C -- any actor entering sleep / alert / flee / uncover / guard / search / wait / vehicle / charge /
  converse / avoid would have hit a trap and ended the game (only obey, the a10 crew's mode, and fight were
  complete). NOTE the names sit one entry early in the table dump from 0x655254: mode 11 is obey, 9 vehicle,
  10 charge.
- WRITTEN from objdump (new files, 0.9): 25 small callbacks (scratchpad/modes1..4.py via newmode.py).
  REMAINING (larger, OPEN): flee 0x4037f0 / 0x403af0 / 0x403b90, uncover 0x4081e0 / 0x408470 / 0x408680, guard
  0x404b90 / 0x404d60, search 0x407940 / 0x407a10 / 0x407d80 / 0x407f40, wait 0x409b30 / 0x409cc0, charge
  0x401da0 (2.9 KB) / 0x402af0, converse 0x402d70 / 0x402e70. scratchpad/modeprocs.py lists what is left.
- Fixes: unit_update_random_turn_angle rotated the OBJECT's forward vector (bending the unit's orientation)
  instead of the desired facing, and called the wander helper without arguments; unit_commit_speech's dead-unit
  test; actor_update_look_target was missing the body-turn / aim-follow block (0x415fb6..0x41609a);
  actor_update_squad_link_state's staleness test (kind 4..5 only in grade 3; active movement type +0x46c).
- Verified: actor_update_activation_state, actor_movement_advance_waypoint, objects_update, actor_delete_or_
  release_unit, unit_apply_scale_change (scripted kill), idle basis / fidget.

## 2026-09-27 (loop, static only) -- all 42 actor mode callbacks written
- Every proc of the actor mode table (0x65524c) now has C (scratchpad/modes1..15.py, src/ai/actor_mode_*.c, 0.9
  each, written from objdump; relinked, 1 unresolved / 130 traps as before). Largest: charge process 0x401da0
  (2.9 KB), guard update 0x404d60, search process 0x407a10, flee process 0x4037f0. scratchpad/modeprocs.py
  prints nothing left.
- Notes worth keeping: 0x422070 (actor_record_perception_event) preserves EDX, so guard update's broadcast gets 2;
  0x407b33 leaves the marker call's five arguments pushed under the reachability call's four (harmless in C).
NEXT: the actor TYPE table (0x6853b8 -> 0x20-byte records: name, ..., +0x14 update proc, +0x18 / +0x1c) -- only
  crew (0x423890) has C; elite / grunt / jackal / marine / hunter / flood / sentinel / infection / mounted weapon
  procs are unlisted and would trap as soon as such an actor updates.

## 2026-09-27 (loop, static only) -- actor type updates for a10 (marine, grunt, jackal, elite)
- The actor type table (0x6853b8 -> records: name, flags, +0x14 update) had C only for crew. WRITTEN from objdump:
  actor_type_marine_update 0x4261d0, actor_type_grunt_update 0x424590, actor_type_jackal_update 0x425f70,
  actor_type_elite_update 0x423a90, and every helper they reach that had no C: escalation checks 0x40a7f0 /
  0x40a860 / 0x40a950 / 0x40a9e0 and 0x40aa70 (EDI actor, stack threshold), 0x40ac30 (vehicle recently left),
  0x40ac70 actor_seek_vehicle_to_board (vehicle offers in ai_globals +0x3b6/+0x3b8), 0x40b770 grenade decision
  (EDI actor), 0x40c530 grenade evasion (EAX actor, stack allow, alt). scratchpad/uncovered.py <roots> now finds
  nothing reachable from those four without C.
- Still unlisted: hunter 0x424810, engineer 0x423d40, flood 0x423f30, infection 0x424980 / 0x424c20 / 0x425c70,
  flood carrier 0x423740, sentinel 0x4264d0, mounted weapon 0x4263f0 (not in a10's opening).
- OPEN (verify): actor_process_vehicle_seat_exit 0x40b080 (0.35) is now on every marine / grunt / jackal / elite
  tick.

## 2026-09-27 (loop, static only) -- player melee scan
- unit_melee_attack_scan 0x56f550 REWRITTEN from objdump as the whole function (1839 bytes; the old 681-byte
  header stopped mid-loop). Fixed: the 5x5 cone used perp components in the wrong order and a bogus cross product
  (now aim*0.8 + (row*perp + col*(aim x perp))*0.1); vehicle impulse now aim * accel scale * 0.035 on the struck
  object; damage record now carries flags 1 / controlling player +0x218; breakable surfaces get the damage record
  plus both stack args; 0x44b5d0 gets the machine; 0x56f210 gets (material, effect tag, unit); secondary push
  sets flags 8.
- unit_can_see_point 0x56f800 marked FRAGMENT (tail of the above).
- breakable_surface_apply_damage 0x4ffde0: takes a second stack arg (collision surface) passed on to 0x500090
  (was 0).
- vehicle_update 0x570ee0 REWRITTEN from objdump (0.15 -> 0.85). Big one: the draft inverted the parent test, so
  every FREE vehicle had its velocity zeroed and skipped physics while attached ones ran it (the cryotube and
  any a10 vehicle). Also fixed: flip test uses up.k <= 0.9 (was +0x4dc), altitude band uses position.z / vel.k
  (was +0x338 / +0x4dc), hard-landing damage tests vel.k, scalar helpers get their rates/range/value pointers,
  skid/steering/ground-contact updaters get the contact buffer and the pre-physics velocity.
- OPEN: vehicle_calculate_turret/steering_wheel/lean_controls (0x572b60/0x572cd0/0x572df0) also take a buffer in
  EDI/ESI (esp+0x88) that their C signatures lack.
- unit_choose_combat_reaction_animation 0x561140 verified vs objdump (0.2 -> 0.85); fixed the 0x42c2a0 call (EDX
  unit, BX 1/4, DI 2; was argless).
- unit_test_placement_candidate 0x55aa20 REWRITTEN: really a ground probe (ECX unit, ESI dir = global down in
  every caller, EBX normal out, stack distance / out point) returning the BSP surface or -1. The draft took no
  unit and returned the hit flag, so biped_get_cached_look_at_position stored 0/1 as the ground surface
  (+0x4dc) instead of -1/surface. All four callers fixed (cached look-at, evade 0x55e190 incl. its landing-speed
  test, flee 0x55e2d0 incl. velocity probe and angular-velocity test, ai_reference_face_starting_location).
- biped_get_cached_look_at_position declared object_get_position with swapped args (fixed).
- unit_spawn_with_starting_weapons 0x572110 is a network message handler (skipped, MP).
- NEW static pass: tools/check_prototypes.py (existing) lists extern-vs-definition mismatches; filtered with
  scratchpad/protocount.py (prototyped externs whose call passes a different arg count) -> 513 sites in the
  gameplay modules (scratchpad/protocount.txt). Working through the a10-hot ones.
- AI perception props: actor_allocate_paired_prop 0x43e910 / _with_kind 0x43e980 called datum_new with no array,
  the prop init without its actor and the copy without operands (fixed, 0.95). actor_target_data_acquire
  0x41f7d0 REWRITTEN (0.2 -> 0.9): arg 2 is the object (the draft ignored it and looked up the actor as an
  object); its callers actor_dispatch_squad_order (looked the ordered prop up in actor_data with the owner
  handle), actor_forward_target_object_reference (passed 2 of 4 args) and actor_react_to_seen_target (passed
  the seen unit's pointer instead of the player's +0x40 handle; 90-tick window is inclusive) fixed.
- ai_communication_broadcast 0x42d340 (5.6 KB, every AI dialogue event) REWRITTEN from objdump (0.3 -> 0.85). The
  draft's front half (team-pair recognition, gates, recency table) did not follow the binary and 6 of its calls
  passed invented operand lists (speaker searches, proximity, team-pair counter, commit speech, follow-up order).
  Kept byte offsets of the 0x38 candidate record; header padding bytes (+0x0b, +0x16) now zero instead of stale
  stack bytes.
- ai_propagate_communication_reaction 0x42e9c0 REWRITTEN (0.25 -> 0.9): never advanced its actor index (every
  prop lookup used none), called the prop lookup without the object, and the firing-position / hearing / squad
  order helpers without operands. Runs after every spoken line (unit_update_animation_timers).
- ai_dispatch_queued_order 0x42f840 REWRITTEN (-> 0.9): takes the actor on the stack (was dropped); count is the
  header word +0xe, target +0x10, variant +0xc; both vocalize calls now get their register operands.
- unit_process_melee_special_interaction 0x56ff40 REWRITTEN as the whole 610-byte function with its 7 stack args
  (infection forms: die on frying shields / attach to the struck node). biped_integrate_movement_with_collision's
  lunge trace (0x55de09) passed a float as a pointer to ray_intersects_sphere_test (crash on any biped melee lunge
  that reaches it) and no operands to the context build / plane transform / special interaction: fixed.
- object_set_position_and_relink 0x4f5350 takes a third (stack) arg, the leaf location, passed to 0x4f5c30 (was
  always 0). Callers fixed: item_update (record leaf), impact damage 0x508e04 and the aim-teleport 0x56bfc0 (both
  relinked a NULL position -> crash when reached), placement 0x55a9c3 (wrong buffer; OPEN: pass its leaf once
  unit_find_placement_position is rewritten), accelerate / detonate / grenade release (0).
- unit_sample_camera_shake_from_velocity 0x56bfc0 is really "teleport to the aim point" (rewritten, 0.95).
- weapon_fire_trigger 0x4c3f10: camouflage depower gets the firing player (0x474db0 result, was discarded), the
  non-local HUD cue gets (EBX weapon, EAX action), the post-fire AI alert gets (EDX holder, BX trigger +0x2e, DI 1).
- weapon_update 0x4c1530: animation_state_advance gets (graph, object +0xd0 state, 0, 1) (was one int);
  effect_stop gets its handle (overheat +0x2cc / trigger +0x20) instead of "1"; overheat notify gets (weapon,
  0xf / 0x10); blur permutation goes to the holder when the weapon is hidden+attached, with (-1, on) args.
- projectile_update 0x4bdc00 REWRITTEN from objdump (every projectile, every tick). Fixed: vector3d_cross_product
  called with NULLs when orienting along velocity (crash on the first oriented shot), homing cross product
  reversed (turned away) and argless rotate / eye-marker calls, deceleration/speed-up/vanish/gravity/range paths
  re-derived (the committed velocity is the steered copy), fly-by sound placement (closest point, direction,
  leaf), relink with the collision record's leaf (was uninitialised), contrail advance with its handle.
- item_update 0x4bc5c0 REWRITTEN from objdump (every free item, every tick). The draft's tumble tail passed NULL to
  vector3d_cross_product / normalize and vectorless rotations (crash for any tumbling dropped weapon), and the
  settle / bounce / support / material-effect / sound helpers lacked operands.
- Remaining NULL-helper call in live code: unit_seat_candidates_from_zone_and_enter matrix4x3_multiply(0,0,0)
  (OPEN, next).
- unit_seat_candidates_from_zone_and_enter 0x56a4c0 (vehicle_load_magic) REWRITTEN (0.2 -> 0.9): the re-seat path
  relinked NULL and multiplied NULL matrices; now reuses biped_update.c's detach / history helpers.
- actor_update_firing_state 0x40e7b0 REWRITTEN from objdump (0.3 -> 0.85; every armed actor, every tick). Fixed:
  argless unit_get_camera_position (vehicle firing origin written through garbage), grenade top-up / lead /
  drift / line-of-fire / override-target helpers without operands, aim point fields, the "friend in the way"
  logic (0x42b190 returns clear, not blocked), burst timer via fistp.
- actor_update_melee_combat_action 0x40cdf0 (really "choose the next combat mode"; every combat tick via
  actor_update_combat_behavior) REWRITTEN (0.3 -> 0.85): the alert stages, pursuit note, placement flags,
  support evaluation and target state helpers were called without operands; firing-position query rebuilt.
- trigger_create_projectiles 0x4c4c40 (every shot) REWRITTEN from objdump (0.2 -> 0.85), including the autoaim /
  first-person offset / actor aim block the draft left out.
- object_reposition_to_spawn_location 0x4f7b70 REWRITTEN: takes a second stack arg (object to ignore); the draft
  passed its result buffer as the sweep's exclude object (garbage result pointer -> crash on the first player
  shot / weapon drop / grenade throw). Callers fixed: trigger_create_projectiles (camera, holder),
  unit_drop_object_from_hand + unit_release_thrown_grenade (were passing (point *)0xffffffff; now the camera, -1),
  projectile_detonate (relink at the parent's position, sweep back to the stuck point).
- unit_drop_object_from_hand 0x56ed00 REWRITTEN (0.25 -> 0.9): item_accelerate lacked the item, the root velocity
  helper lacked its operands.
- unit_release_thrown_grenade 0x56e440 REWRITTEN (0.3 -> 0.9; every grenade throw): actor throw vector now gets
  the grenade position, player launch point from the camera + globals offsets, early-release blend, reposition
  from the camera.
- actor_compute_grenade_throw_vector 0x410a60 REWRITTEN (0.35 -> 0.85): takes the grenade position (stack arg 1)
  and hands it to the lob solver (the draft passed NULL -> crash on the first AI grenade), impact check gets its
  point, the 30-degree facing clamp re-derived.
- Grenade path: actor_solve_grenade_lob 0x410780 and actor_commit_grenade_toss 0x411180 (0.15 -> 0.9) passed
  no register operands to the arc check 0x42b5d0 (velocity EAX, actor ECX, start EDX) and to the launch-velocity
  solver; fixed / rewritten. actor_get_grenade_launch_velocity 0x410980 verified.
- NEW scan scratchpad/unproto.py: calls through unprototyped externs with the wrong argument count (14 in the
  gameplay modules). Fixed:
  - object_damage_apply_line_of_sight 0x4eddb0 REWRITTEN (0.3 -> 0.85): one-argument cross product (crash on the
    first explosion reaching a visible unit), unprototyped segment tests; falloff / team / difficulty rules.
  - object_damage_effect_dispatch 0x4f0250 is a FRAGMENT of damage_effect_new_at_location (marked).
  - objects_garbage_collection: block_list_compact now gets the object pool (EBX) at all three sites (was argless).
  - actor_update_aim_wander / actor_get_target_state_flags / actor_flee_look_away: missing actor operands; the
    flee look-away now builds its look order and switches to guard (mode 6), not mode 0 with NULL.
  - ai_communication_play_event_line 0x42eee0 REWRITTEN (0.35 -> 0.85).
- OPEN from unproto: antenna/glow build_sprite 8 vs 11 (render), lightning_render cross product 1 vs 3,
  light_transient_add pack-normal 0 vs 1, unit_apply_network_control_update datum_get (MP).
- light_transient_add: the two normal packs get the light's forward / up (stack args 2 / 3; were argless).
- glow_render 0x4fe570 REWRITTEN: fills the sprite batch and passes it to build_sprite / build_sprites_end (the
  draft never initialised it).
- OPEN (not in a10's opening): antenna_render_wire build_sprite registers, lightning_render cross product.
- actor_update_special_mode 0x40d820 verified; the three target-alert stages get (prop, actor) (were argless).
- actor_update_crouch_state 0x4213b0: combat alert flag (actor, 0), ranged attack vector (prop, actor, out),
  recognition entry (actor, +0x3b8, 1) now get their operands.
- actor_schedule_grenade_throw 0x402f80 REWRITTEN (0.3 -> 0.9; misnamed: queues a look toward whatever last
  damaged the actor): find-prop / eye-marker / datum_get were argless.
- actor_evaluate_grenade_target_position 0x40de70 REWRITTEN (0.25 -> 0.9; misnamed: sidestep out of the
  target's line of fire), actor_find_nearest_grenade_ally 0x40e540 REWRITTEN (0.25 -> 0.9): both called their
  helpers (step probe, dodge animation check, secondary action, ally validation, cursor, prop lookups) argless.
- actor_attempt_grenade_throw 0x428ab0 REWRITTEN as the whole 926-byte actor death handler (0.2 -> 0.9): death
  grenade pull (variant +0x94 / +0x98), grenade drop, dropped weapon ammo (+0x1d4..+0x1e2), actor_delete, morale.
  The draft called the weapon lookup / countdown / ammo setters argless. 0x428d35
  actor_died_unit_grenade_count_mod marked FRAGMENT.
- Prop lookups (actor_find_or_create_shared_prop 0x43eb30 = object EAX + (actor, create, flag); actor_find_prop_for_object
  0x43ea80 = object stack + actor ECX): every caller passed the actor as the object or dropped it. Fixed:
  - actor_mark_prop_seen_with_delta 0x428840 REWRITTEN (0.2 -> 0.9): (object EAX, actor, delta, direction); marks the
    prop and its pair seen, then the directional reaction gets (direction, prop if kind 2..3, actor) -- was
    (NULL, actor, none). Its caller actor_react_to_threat_event passes the relationship object and extra_param.
  - actor_squad_react_to_grenade_for_vehicle_occupants 0x42bd70 REWRITTEN (0.4 -> 0.9).
  - actor_target_is_close_and_recognized 0x42f480: object is arg 1.
  - ai_alert_actors_in_grenade_radius 0x42a0e0 REWRITTEN (0.2 -> 0.9): 3 stack args (object, stimulus, gate), both
    hearing checks get all 6 operands; ai_refresh_unit_stimulus_and_alert 0x42c2a0 REWRITTEN (0.4 -> 0.95) to pass
    them (object, DI, BX).
  - actor_get_squad_recent_attacker_target 0x41f6b0 REWRITTEN (0.2 -> 0.95).
  - actor_issue_order_or_vocalize 0x4302e0: prop lookup gets the actor, eye marker gets (vehicle, ctx + 4).
  - actor_scan_ally_death_panic_reaction, ai_communication_rate_speaker, ai_conversation_resolve_participants,
    actor_scan_allies_for_backup_request: prop lookups get their ECX actor.
- actor_scale_value_by_ally_exposure 0x420c90 returns AL (1 = 2+ allies already alerted, roll skipped); the draft was
  void. Callers fixed: actor_scan_ally_death_panic_reaction (the roll now happens) and STABLE
  actor_scan_backup_and_panic_reaction (proven different at 0x423346..0x42338a: it accepted every scaled roll).
- ai_unit_create_actor 0x435420: object_try_and_get got no unit, actor_new no variant, and the type byte was read as
  uint32 [0xd]. 0x6853b8 is an ARRAY of type definitions: ai_unit_create_actor and encounter_choose_vocalizations
  declared it `uint32_t **` (a pointer variable), so indexing read type 0's definition as the table. Fixed both.
  OPEN idea: scan for other array globals declared as pointer variables.
- actor_update_target_lead_position 0x429570 REWRITTEN (0.35 -> 0.95; the actor's cached location + point): vehicle
  prediction and the biped lookup were argless.
- NOTE: harness/build/hooks.txt differs from hooks.stable.txt only in line endings (CRLF vs LF); content identical.
- NEW scans scratchpad/arrscan.py + ptrscan2.py + derefchk.py (full .text listing in scratchpad/full.asm): data externs
  whose declaration shape disagrees with how halo.exe uses the address. Fixed:
  - Pointer variables declared as arrays / values (C copied the pointer bytes instead of the data):
    0x686b04 global_white_color (object_placement_data_initialize: every object's 4 placement colours at +0x58 were
    garbage, now *ptr = white; also koth reset), 0x69672c global_down3d_pointer (unit_predict_aim_target_position),
    0x6966f8 global_zero_vector3d_pointer (unit_clamp_direction_to_aim_or_look_bounds), 0x6851f4 / 0x685200 HUD
    text message colours.
  - 0x69bfdc object_type_definitions is an ARRAY; 5 files declared it as a pointer variable and indexed it:
    actor_place_new_unit, unit_drop_grenades, unit_submit_periodic_network_update, cheat_spawn_objects_near_camera,
    network_game_broadcast_team_object_updates.
- actor_place_new_unit 0x427080 REWRITTEN (0.15 -> 0.95; places every encounter actor's unit): type-table index was
  the tag index, [0x719720] compared as a dword, forward.k unset, object_delete_unparented missing its EDI operand.
- OPEN (MP): game_engine_koth_submit_hill_marker_geometry default_axis_b 0x686b0c is a pointer declared as a vector.
- Pre-existing: select_players_to_display.c(114) C4716 must return a value (game module warning).
- ptrscan2 widened (indexed derefs, init stores allowed). NEW real bugs, fixed:
  - 0x6b0b84 is a POINTER to the team-relationship block. ai_recompute_all_relationship_flags (the prop hostile flag
    +0x60 and +0x61 for EVERY actor/prop pair), ai_communication_select_speaker_by_team and
    unit_record_recent_damage_and_react (friendly fire test) read the 10x10 bit matrices at 0x6b0b84 + 0x94 / 0xa4
    in static memory instead of through the pointer -- AI friend/enemy classification was garbage. HIGH IMPACT on
    AI behaviour in a10 (marines / crew vs player). Candidate for the vanishing / misbehaving NPC.
  - 0x6b7af4 random_point_table is a POINTER (count word at 0x6b7af8): random_get_table_point returned static
    garbage; player_find_placement_position also scaled the index twice.
- 0x719720 (game connection word) declared int32 in 7 files (object_update, unit drop/pickup/throw, control block);
  now int16 (0x719722 is the screenshot counter, so it only mattered after a screenshot).
- False positives checked: cluster reference groups (struct, first field a pointer), game_time / game_state_cursor
  (ints cast at use), rasterizer_effects slot array.
- OPEN (widthscan.py): other WIDE / NARROW scalar globals mostly in UI / networking / rasterizer debug toggles;
  gameplay ones left: weather_instance_count 0x6b0ae0 (int32 vs word), 0x71973c in main_switch_structure_bsp (int32
  vs byte), 0x689450 object_compute_level_of_detail_pixels (int32 vs word), 0x7196d8 game_state_load_checkpoint
  (uint16 vs dword).
- Unit placement chain (vehicle exit / seat detach / biped_update / script detach-and-place):
  - unit_reset_orientation_and_find_position 0x55add0 REWRITTEN (0.5 -> 0.95): takes EDI = the seat parent (all 14
    callers load it) and places around it: 27-point grid at 2x pill radius, then the parent's bounding centre /
    radius. The draft passed zeros (radius 0, no grid, no parent). All 13 C callers now pass the parent.
  - unit_find_placement_position 0x55a500 REWRITTEN (0.25 -> 0.9): offset table 0x65e660 in the unit frame or world
    axes; leaf / clear-position / pill / reference-object pill / two-way segment checks; relink with the scenario
    location (closes the OPEN "should pass its leaf to the relink").
  - collision_test_movement_pill 0x506040 REWRITTEN (0.4 -> 0.9): radius is its 3rd stack argument; query gets all
    6 operands; last leaf goes to +0xc (the draft overwrote the first).
  - unit_find_nearest_valid_surface_plane 0x560630 REWRITTEN (0.15 -> 0.9; from biped_create for tag +0x2f4 bit 6).
  - unit_propagate_position_delta_to_children 0x570cb0: object_set_position_and_recalculate gets (position, unit).
- OPEN: player_attach_unit_to_parent 0x475c60 (0.25, 1096 bytes; called by hs detach-and-place and the BSP-switch
  player reattach) is garbled -- argless seat/weapon calls, reads the driver field as the parent. Stopgap: passes
  `driver` to the placement reset. Same for game_engine_reattach_player_unit_unused. REWRITE NEXT.
- OPEN (MP): unit_spawn_with_starting_weapons 0x572110 (network game action) set_position argless.
- player_attach_unit_to_parent 0x475c60 REWRITTEN (0.25 -> 0.9): seat-exit inline (same as unit_detach_from_seat),
  scripted event 9, client history drop, then 0x4757b0 with all 3 args. Closes the stopgap OPEN.
- player_find_placement_position 0x4757b0 REWRITTEN (0.25 -> 0.9): third argument (the point) was missing; ring of 9
  around the target's root (3 x collision radius + bounding radius), 8 random jitters each, trigger-volume veto,
  facing / look angles / teleport effect. Callers: hs detach-and-place, BSP-switch reattach.
- physics_point_find_clear_position / physics_point_walk_toward_target: object_collision_test_cluster_group now gets
  its EDI position (current point / each candidate / the walk state's position). These gate every placement.
- object_physics_tick 0x507840 REWRITTEN (0.35 -> 0.9): powered mass point matrices from the caller's states
  (+0x1c quaternion -> +0x2c matrix, transposed); the draft wrote into the Physics tag's block from a NULL quaternion.
- object_physics_integrate_and_test_at_rest 0x5097e0: FORCE comes in ECX (linear), TORQUE on the stack (angular);
  the draft applied one vector to both -- every rigid body got its torque as linear acceleration. The rest of this
  1692-byte function is still 0.30: OPEN.
- OPEN: object_physics_check_impact_damage 0x508b70 (0.35; crouch / context point test / unit_apply_impulse argless),
  object_physics_handle_nearby_object_impacts 0x508a10 (0.3), object_physics_resolve_mass_point_overlap.
- object_recalculate_bounding_radius: header was stale (already fully rewritten in e891407, 2027 bytes); the three
  "clones" 0x4f84e2 / 0x4f8834 / 0x4f8a70 are its fragments (marked).
- object_set_scale_and_refresh_nodes 0x4f96a0 (a10 script object_set_scale): the script's tick count is a second stack
  argument tail-passed in DX to 0x4f6b70; the draft passed 0. hs_evaluate_object_set_scale passes it.
- actor_find_or_create_shared_prop 0x43eb30 (0.2 -> 0.9): conflicting-neighbour check gets the actor (EAX); the combat
  flag reset gets (prop ECX, actor, 0, noticed).
- actor_target_reset_combat_flags 0x41baf0 REWRITTEN (0.6 -> 1.0): tail-calls actor_queue_sighted_target_dialogue(actor,
  prop, noticed); the draft called it with no arguments.
- actor_find_prop_for_object verified (0.9).
- NEW scan scratchpad/tailscan.py: tail jumps whose C call passes fewer arguments than the target takes. Fixed:
  - unit_get_active_weapon_scale 0x565ab0: (unit EAX, zoom level stack) -> weapon_get_zoom_magnification(weapon, zoom);
    game_engine_build_local_player_control_input passed the ZOOM LEVEL as the unit (zoomed look sensitivity).
  - ai_reference_resolve_squad_datum 0x432c80 (0.4 -> 0.9): reinforcement tail call gets (encounter ECX, squad AX).
  - 0x565a70 "unit_clear_weapon_switch_state" is the tail of unit_validate_and_clear_weapon_switch (C already complete;
    marked FRAGMENT, header size fixed to 226).
- NEW scans scratchpad/sizescan3.py (functions whose jump-reachable code runs past the header size). Findings:
  object_update_functions (really 929 bytes; rewrite already covered it) and object_recalculate_bounding_radius
  headers fixed, their "clones" marked FRAGMENT.
- actor_evaluate_combat_state_transition 0x40c620 REWRITTEN (0.15 -> 0.9; real size 1608, the header's 1443 stopped
  short): engage-range entry into combat (consider 2), vehicle gunner consider 4, hold / fall back to guard (mode 3).
  Called by actor_update_combat_behavior, escalate, conditional transition -- every actor.
- OPEN (sizescan3): encounters_update_activation 0x437e20 (0.45), encounter_redistribute_squads_toward_targets (0.15),
  actor_replace_object_reference (0.3), unit_dispatch_seat_exit_message (0.2), decal_place (0.15) extend past
  their header sizes and are unrewritten.
- Vanishing-NPC trail (static): verified against objdump, all faithful -- encounters_update_activation (header fixed,
  0.9; actors outside every player-visible cluster for 90 ticks get their units' header active bit cleared),
  actor_set_units_active, object_mark_pending_delete / object_clear_pending_delete_flag,
  game_engine_build_visible_cluster_bitmask (player root clusters' PVS rows at +0x14c), object_set_position_and_relink,
  object_set_cluster_and_parent (relink into a non-visible cluster DELETES objects flagged 0x80000), and the constructor's
  0x80000 save/restore + inactive-delete check. Our C sets 0x80000 nowhere the binary does not.
  Runtime check for the user: if the cryo tech vanishes, is his object deleted (datum freed) or only inactive (header +2
  bit 0 clear)? And is his object +0x9c cluster -1 at that moment?
- actor_dispatch_type_vtable_0x1c 0x4266d0: forwards its three stack arguments to the actor type's proc (swarm actor
  velocity: param_1, speed_limit, out_velocity); the draft passed only the actor. actor_get_requested_velocity fixed.
- actor_select_move_position 0x4014c0 REWRITTEN (0.35 -> 0.9): squad move positions (mask: current, within 0.5, other
  group letter, ally-occupied), weighted pick (mode 5, weight +0x10 -- the draft dropped it) or forward / alternate /
  ping-pong stepping.
- game_engine_players_update_server reviewed around the visibility-bitmap rebuild: matches (header still says 0.2).
- actor_target_hearing_check 0x41c030 REWRITTEN (0.2 -> 0.9): range scaling (behind 0.8, awareness 0.7 / 0.4, gate
  0.2 / 0.45 / 0.7, deafening 0.25, stance 0.7) and the cluster sound distance; the draft called the deafening test and
  the PAS lookup with no operands.
- ai_broadcast_communication_event 0x429fc0 REWRITTEN (0.2 -> 0.9): (gate EAX, point ECX, source, type) -- noise events
  (projectile impacts / detonations via ai_accumulate_repeated_event) now reach nearby actors' dialogue, danger and flee
  reactions. ai_accumulate_repeated_event passes (gate, &entry position, source, id, count); it was (type, id, count).
- actor_get_firing_positions 0x41c1e0: a swarm actor's position context now comes from its nearest component's unit
  (EBX) into the caller's block; the draft passed nothing.
- ai_conversation_update 0x430a70 REWRITTEN (0.25 -> 0.9): line helpers get the conversation; a finished conversation is
  STOPPED (the draft instead fell into the participant pass, so conversations never ended -- scripts waiting on
  ai_conversation_status would hang); participants get their conversation / partner refs while it is active (the
  draft did that only once finished). a10's opening conversations run through this.
- ai_conversation_current_line_is_ready 0x431e70 REWRITTEN (0.3 -> 0.9): speech priority claim (0x560d00 priority 6)
  and the 0x30-byte speech commit for the speaker unit, unspatialized sound otherwise, done-speaking test, post-line
  delay, bit-3 handshake. The draft called all four helpers without operands -- conversation lines never started.
- ai_communication_line_fade_multiplier 0x42f8c0 REWRITTEN (0.2 -> 0.9): the speech priority query gets its 7
  operands (EAX chain / ECX index pass through, BX = line class); repeat-delay silence / 60-tick fade-in.
  ai_communication_rate_speaker passes them.
- ai_conversation_resolve_participants 0x430fc0: participants entering conversation mode (12) now get the 0x14-byte
  mode data from ai_conversation_get_run_to_player_range (EDX out, ESI conversation); the draft passed NULL, so
  actor_set_mode kept the PREVIOUS mode's data as the conversation's (run-to range / target unit) -- a conversation
  participant (the a10 cryo tech) could head for a stale target. Player look-cone test gets (unit ECX, participant
  +0x120 EDI). CANDIDATE for the vanishing / wandering cryo tech.
- NEW scan scratchpad/setmode_scan.py: every other actor_set_mode call passes the same kind of mode data as the binary.
- unit_try_start_scripted_action_animation 0x569530 REWRITTEN (0.3 -> 0.9): the command's priority reaches the node
  transform reset (DX) and animation_choose_random_permutation gets (graph, first animation, 1) -- the draft passed only
  the stream, so AI gestures (ai_communication_broadcast, actor_apply_queued_look_to_unit) set a GARBAGE animation
  index on the unit. Bad animation index -> bad node matrices -> a unit that stops rendering: CANDIDATE for the
  vanishing cryo tech (he gestures while talking).
- unit_scripted_action_animation_exists verified (0.9).
- unit_snap_to_min_ground_height 0x55ecf0 REWRITTEN (0.3 -> 0.9; really the biped jump launch, from biped_update):
  jump speed along up, AI requested velocity (0x417fa0 with all 5 operands -- the draft passed 2), airborne flag,
  jump sound triggers.
- unit_pick_and_ready_next_weapon: the slot finder gets the unit (EAX); the draft dropped it (runs on every seat exit).
- unit_pickup_weapon verified (0.9; index zero-extended like the binary).
- unit_get_forward_vector_or_marker_normal 0x569720: a seated unit's forward is transformed by its parent marker node
  (EAX out, EDX forward, stack matrix); the draft passed only the matrix.
- actor_get_body_axis_vector 0x405390 (0.3 -> 0.9): called that two-argument function with THREE arguments, so it wrote
  the vector through the unit index as a pointer (memory corruption whenever an actor rated another unit's axis).
- NEW check: callers passing MORE arguments than the definition (misalignment class). actor_reassign_vehicle_seat's
  tail call tidied (EAX = self); actor_delete / damage_apply_area_effect / ai_accumulate_repeated_event extras are
  harmless (first argument right, cdecl).
- unit_initialize_random_turn_angle (flee enter / tick): actor_resolve_wander_or_look_direction gets (actor, scratch).
- actor_look_get_wait_ticks 0x415150 (0.5 -> 0.9): takes the actor (EAX) for the threat-weapon (+0x410) wait scale; the
  draft had no actor and called a different helper argless; fistp rounding. Its 5 call sites (resolve_look_target,
  look_randomize_direction, update_look_target x3) pass the actor.
- actor_link_to_unit_cluster: team stamp gets (encounter EAX, unit ECX).
- Vanishing-NPC trail, recordings: a10 uses recording_play_and_delete 14x (the unit is DELETED one tick after its
  recording ends). Verified faithful: recorded_animation_start (0.9), recorded_animations_update (0.9), the compressed
  codec update (end only on the end marker at exactly its delay). So a play_and_delete unit vanishes only when its
  recording really ends -- unless the decoded control data is wrong (decoders 0.8-0.9).
  Runtime check for the user: is the cryo tech driven by a recording (recorded_animations data array 0x6b0a10) when he
  vanishes, and is his object datum freed (deleted) at that moment?
- OPEN: actor_update_look_target 0x415480 (0.25, 3896 bytes; AI look/aim control every tick).
- unit_detach_reposition_and_nudge 0x56ca40 REWRITTEN (0.2 -> 0.9; from unit_release_transient_state): push direction
  from the PARENT's position (the draft read the unit twice), scenario_structure_bsp_locate_point_nudge_up gets a point
  copy (the draft passed the unit index as the point pointer -- a garbage write -- and ignored the nudge), light
  attachments (0, 1).
- object_nudge_position_by_velocity reviewed: network-prediction only, matches.
- NEW scan scratchpad/kindscan.py (extern parameter kinds vs the definition: int passed where a pointer is expected
  and vice versa). Fixed: object_get_position was declared (index, out) in encounter_redistribute_squads_toward_targets
  and hs_damage_apply_with_sound -- the callee wrote 12 bytes through the object index (crash / corruption).
  Checked benign: effect_new_with_color (typedef only), render_objects_collect -> structure_bsp_collect_visible_objects
  (callbacks and bounds order verified against objdump, frustum test ECX/EDX/stack).
- unit_compute_marker_offset_position: object_get_position (out, index) -- the unprototyped call wrote through the index.
- object_start_animation: animation lookup gets the graph TAG (EAX) and the name (EBX), print gets its arguments.
- NEW scan scratchpad/unproto_order.py (calls through unprototyped externs with arguments): only biped_update_facing's
  cross products remain; its ordinary (non-flying) path was checked against objdump and matches.
- OPEN (UI / MP, not a10): string_format_wide_va_bounded is (count EDX, dest, format, ...) but 9 files declare it
  (dest, format, ...) and write into their format literal: game_engine_build_end_game_result_text,
  game_engine_build_kill_feed_message_text, game_time_format_minutes_seconds, input_print_bound_controls,
  player_profile_1wide_list_update, server_list_menu_update, ui_network_host_setup_refresh,
  game_variant_list_matching_substring, server_browser_list_row_populate (counts per call in scratchpad/fmtcount.py).
- biped_update_animation_frame_trigger 0x55eaa0 REWRITTEN (0.2 -> 0.9; jump / landing timing from the biped movement
  integrators): +0x4d1 = trunc(tag +0x3d4 / +0x3d8 * 30 * clamped fraction); the draft stored trunc(span).
- unit_recompute_seat_occupants, unit_refresh_targeting_flag_and_weapons verified (0.9).
- actor_should_hold_position 0x4105c0 (0.2 -> 0.9): takes the actor definition (EDX) and sets the hold timer +0x5f4 to
  a random span of def +0x80..+0x84 seconds (the draft stored 0); actor_update_firing_state passes it.
- actor_reseed_movement_pause_timer header updated (already a faithful rewrite).
- unit_update_autoaim_interaction (0x570720): REWRITTEN from objdump. Clears object +0x107 bit 3 (draft cleared +0x106 bit 3); damage_data source is the unit's +0x410 object (draft looked up the damage effect index as an object); added the location_cluster/material -1 sentinels. 0.3 -> 0.9.
- unit_throw_grenade_move_to_hand (0x56e280): placement block REWRITTEN. Full object_placement_data (draft used a 0x60 buffer, overflowing into the stack); position = left-hand marker world position, forward = aiming vector, up = normalized perpendicular. The draft spawned grenades at the unit origin with garbage orientation. 0.2 -> 0.85.
- actor_update_grenade_eligibility_state header marked verified (0.9).
- object_delete_teardown (0x4edc80): FIXED. For network_role 0 (single player) the binary runs object_delete_unparented and then FALLS THROUGH into object_delete_recursive(object, 0); the C skipped the recursive delete, so torn-down objects (units destroyed after dying, objects with body vitality below the destroyed threshold) stayed alive. All 3 callers were checked against objdump (animation state machine death state 0x19, stance lookup -1 when forced, and body damage below the destroyed threshold); they gate correctly, so this does not add new deletions. 0.5 -> 0.9.
- actor_release_from_cluster_or_delete, actor_delete and actor_remove_from_unit_cluster: VERIFIED against objdump (0.9). actor_delete's extern for 0x436990 is now single-argument (EDI).
- object_delete_unparented (0x4f5aa0): network-only (delete message plus index cache); skipped per the no-MP rule.
- object_unlink_cluster_or_notify_parent (0x4f5de0) and object_set_cluster_and_parent (0x4f5c30): VERIFIED against objdump (0.9).
- Vanishing-NPC lead ruled out: object flag 0x80000 means "deleted when deactivated". objects_update (0x4f4f73) and object_set_cluster_and_parent (0x4f5db7) object_delete an active object that leaves the PVS while it is set. In both the binary and the C, only garbage_new and projectile_new set it (object_new preserves it, verified at 0x4f57dc..0x4f589c), so a unit is never deleted this way unless something writes object +0x10 wholesale. object_delete (0x4f5bd0) and object_mark_pending_delete (0x4f50f0, which in fact activates the object) were re-checked and are faithful.
- **VANISHING NPC, prime suspect FIXED: hs ai_erase erased every actor in the level.** a10's scripts call ai_erase 8 times. hs_evaluate_ai_erase -> ai_reference_notify_squad_index (0x432c20) -> ai_release_actors_filtered (0x42ab00). The binary calls it with EAX = encounter (ref & 0xffff), EDI = platoon (kind 1 sub-index, else -1), stack = squad (kind 2 sub-index, else -1), BL = 0. The C passed a single argument (the squad value, -1 for a plain encounter reference), which ai_release_actors_filtered took as encounter = -1 and so ran its "release every actor" path. Any ai_erase of an encounter therefore deleted the units of ALL actors, the cryo tech included. Both functions were rewritten from objdump (0.9). ai_release_actors_filtered also now builds its all-actors iterator with actor_iterator_new(&it, 0) (the draft used active = 1), passes the cursor to init_cursor, and forwards is_dead (BL). ai_release_inactive_encounters now passes (encounter, -1, -1, 1) and AL = 1 (it also released everything).
- ai_kill chain: ai_reference_notify_actors passed actor_mark_units_and_release its flag in the wrong slot (the binary uses AL = flag, stack = (actor, 0)). ai_kill (flag 0) was unaffected; the flag-1 variant (0x47d290) was wrong. Fixed (0.9).
- ai_migrate chain (12 uses in a10): ai_squads_merge (0x433590) REWRITTEN from objdump (0.2 -> 0.85). The draft walked the unassigned list instead of the source encounter's members, called the squad move without actor/encounter (EAX/EBX), masked the reference passed to find_best_matching_member, and passed the wrong squad field to encounter_add_actor. ai_squad_find_best_matching_member now returns a variant match before an actor match, as the binary does (0.9). Verified: actor_reset_squad_link_for_type_change and the squad iterator new/next.
- New scanner scratchpad/argless.py finds calls with no arguments to externs declared (void)/() whose definition takes parameters: 163 hits, output in scratchpad/argless.out. Fixed so far:
  - AI: actor_movement_action_cancel, actor_dispatch_perception_reset x2, actor_update_target_lead_position, team_pair_override_refresh/clear_flag, and actor_target_update_active_flag's missing prop argument;
  - items: weapon_trigger_begin_reload -> weapon_action_notify_for_weapon(item, 9/10);
  - interface: first_person_weapon_process_action REWRITTEN (zoom-invalidate unit argument; state 0xd/0xf/message stage instead of always 0).
- Verified (0.9): effect_new_on_object, effect_rebuild_markers, effect_first_person_screen_timer_active.
- a10 script-worker sweep (scratchpad/a10deps.py):
  - actor_movement_action_cancel (0x428650) REWRITTEN: it tests and clears active_movement.type (+0x46c), not secondary_action (+0x418), and passes the actor to the mode's +0x24 handler (the draft called it with no argument). 0.3 -> 0.9.
  - effect_new_on_object_with_node_table: first-person check now gets its object (ECX).
  - VERIFIED (0.9): actor_mark_units_and_release (ai_kill worker), ai_reference_flee_if_ready, the ai_reference actor iterator new/next/init_cursor, unit_update_vitality_fractions (unit_set_current_vitality), effect_new_with_color, actor_squad_action_status_broadcast.
- Script object-name chain VERIFIED against objdump (object_destroy x163, object_create_anew x119 and object_destroy_containing x21 in a10): hs_object_hierarchy_test, hs_object_name_cache_validate, object_new_from_scenario_name, object_new_from_scenario_placement, the object-name table reserve and release (0x4f9ac0/0x4f9b00; the release is called from object_delete_recursive as in the binary), object_lookup_table_get and hs_object_names_for_each. No stale-name path deletes a reused object slot.
- Argless-call fixes (AI):
  - encounter_update_platoon_defending_flag: evaluate_platoon_condition now gets (encounter, platoon +0x3c / +0x30 condition) (0.9).
  - encounter_redistribute_squads_toward_targets: ai_reference actor iterator new/next now get their iterator; unlink gets its actor.
  - actor_reject_firing_position_unreachable: flying steering and direct-reachability calls now get (actor, firing position, local) and (position, actor body position, 0, bsp, 0).
  - actor_score_firing_positions_by_history: evaluate_flank_offset(&hazard.direction, 0, candidate position, &hazard.position).
  - actor_update_aim_wander: weapon_trigger_get_average_damage(weapon tag, &rate), and the variant rate now caps the weapon rate (the draft compared the variant rate with itself).
  - actor_update_flee_response: resolve_flee_source_point(actor+0x3ec, actor+0x524, actor).
  - ai_object_list_max_flee_grade: morale grade gets (mode-data word, component+0x1c).
  - actor_spawn_additional_units (0.15 -> 0.85): spawn loop REWRITTEN (random heading, 0.3 offset, placement start point, attach args 2/-1, impulse vector). Its caller unit_pick_random_spawned_actor_count now passes (variant, count, unit, tag+0x260/30).
- **VANISHING NPC, second route FIXED: garbage collection plus the emergency actor release.** objects_garbage_collection (0x4f9c60, 0.2) runs on every ai_place (actor_place_new_unit) and on scenario object placement. It was REWRITTEN from objdump (0.85):
  - The draft's slot test read data_array.maximum_count (+0x20, always 0x800) instead of actual_count (+0x30), so every call ran in mode 2 (memory-critical), and its mode-2 stop test was inverted.
  - In mode 2 the emergency pass walks the {prepare, cleanup} table at 0x65ddd0: ai_release_inactive_swarms, then ai_build_priority_target_list with ai_release_inactive_encounters.
  - ai_build_priority_target_list stored encounter_handle (always none) instead of the iterator's current encounter, so the release pass released "encounter none" = every actor. Fixed (0.9).
  - ai_release_inactive_swarms iterated active actors only (binary: all) and passed is_dead 0/0xff (binary: 1). Fixed (0.9).
- object_test_in_atmosphere_zone (really "can any player see this object", used by garbage collection) walked the player array with argless datum_next calls. Now datum_next(prev, player_data) (0.85).
- Garbage-list membership VERIFIED: object_list_membership_set (0.9). Its only adders are item_update and unit_release_transient_state(unit, 0). The three callers of the latter (damage effects kill/knock-down, knock-down timeout with no health, the MP player kill) match the binary, so a living unit never becomes garbage.
- Argless fixes:
  - items: weapon_put_away/ready/become_charged -> hud_play_pickup_notification(weapon, 0xb/0xc/0xe); weapon_reset_triggers -> animation index 7 and weapon_notify_reload_cancel(weapon, magazine) (0.9).
  - units: unit_can_see_point -> device_machine_melee_attacked(hit object) and breakable_surface_apply_damage(&dd, ...); unit_try_give_grenade / unit_try_select_equipment -> equipment_pickup_play_sound(object).
  - physics: object_physics_resolve_mass_point_overlap -> unit_any_flagged_seat_occupied(self/other); object_physics_check_impact_damage -> unit_apply_impulse(candidate, &impulse).
- OPEN: the vehicle lean functions (vehicle_calculate_ground_contact_lean[_alt], vehicle_calculate_ground_lean_controls, vehicle_calculate_lean_controls, vehicle_calculate_wing_flex_controls) call every math helper without arguments and need full rewrites (not in a10).
- player_check_vehicle_boarding_interaction_lightweight (0x478c40; really the weapon-pickup prompt check) REWRITTEN from objdump (0.25 -> 0.9):
  - four helpers had been called without arguments;
  - the current weapon was requested with slot -1;
  - the candidate was re-fetched where the binary fetches the current weapon.
- hs_damage_apply_at_location (damage_new, used in a10) and hs_damage_apply_with_sound (damage_object): bsp3d_node_find_leaf now gets (0, bsp, point); the leaf is stored at +0x14 and the material at +0x4c = -1. damage_object now calls object_apply_damage(&dd, object, -1, -1, -1, 0); the draft passed only &dd, so the damage hit a garbage object index.
- Argless fixes: unit_get_current_weapon_autoaim_cone -> weapon_get_zoom_magnification(weapon, zoom); main_switch_structure_bsp -> hud_display_loading_message(1); first_person_weapon_set_state -> sound_impulse_fade_out(fp+0x1e98); hud_update_player -> camera_get_type_for_player(local player); projectile_response -> vector3d_angle_between(&hit->plane.normal, velocity).
- New scanner scratchpad/fewer.py lists callers whose extern declares FEWER parameters than the definition (out/prototype_mismatches.json): 351 SP-module pairs, in scratchpad/fewer.out. AI fixes so far:
  - ai_clear_object_references: actor_release_from_cluster_or_delete(swarm actor, unit); the draft passed the unit as the actor.
  - ai_unit_remap_actor_to_squad (0.9), encounter_propagate_platoon_state_to_actors and ai_unit_set_squad_reference: actor_reset_squad_link_for_type_change now gets (actor, EBX = target encounter, squad) and the notify gets (actor, CL, flag). The drafts passed only the squad, so moved actors landed in a garbage encounter. ai_unit_set_squad_reference also walks the unit's OLD encounter (+0x334) with a real cursor.
  - encounter_squad_spawn_reinforcement: spawn_actor(encounter, squad, 0, 1). The third argument feeds actor_place_new_unit, so this is on the ai_place path via ai_reference_resolve_squad_datum.
  - ai_unit_flee_if_ready: the status-broadcast record is the flee mode data (0.9). ai_reference_for_each_squad: clear_spawn_delay(encounter, cursor) (0.9).
  - encounter_set_team: actor_propagate_unit_field(actor, team); the "encounter none" test compares the full handle (0.9).
  - ai_recompute_all_relationship_flags: active flag gets the prop (EDI). actor_target_update_tracking_speed: mark_engaged(prop, actor, 0) x2. encounter_gather_occupied_clusters: group mask (actor, 0, 0).
- actor_reset_perception_scratch (0x428f40; really "reset a unit's controls to neutral") REWRITTEN (0.2 -> 0.9). It builds the 0x40-byte control block (facing from the unit, aim +0x23c, look +0x260) and applies it with unit_apply_control_block(unit, block, -1); the draft passed no unit or block. Its caller actor_dispatch_perception_reset (reached from ai_braindead / ai_magically_see / wake) passed the ACTOR where the binary passes the unit (actor +0x18, or each swarm component unit). Both fixed.
- More partial-argument fixes:
  - actor_squad_action_execute random_int_range(p1, p2 + 1);
  - actor_get_ranged_attack_vector cached wander (prop owner actor, out);
  - actor_movement_action_resolve path_find_validate_and_record_goal(&self+0x4a8, bsp, &body, 0, &dest);
  - actor_movement_test_obstacle_ray: ray_intersects_cylinder gets all 7 arguments (the draft passed 5 in the wrong order, so obstacle avoidance tested garbage);
  - actor_score_firing_positions_by_threat: point3d_distance_squared_to_segment was declared (point, origin, segment) but is (start, direction, point), so both calls were wrong; segment3d_distance_squared_to_segment now gets its 4 arguments.
- More AI argument fixes:
  - ai_unit_dispatch_actor_event_d: vocalization(actor, 0xd, 1, {6, ECX}).
  - encounter_redistribute: object_try_and_get(self+0x64, 3).
  - actor_reject_firing_position_by_pursuit: ai_pursuit_check_object / note_object now get (actor, encounter, firing position, query+0xc, ...). The real 0x436b90 convention is EBX object, EAX min tick, CX type, stack (encounter, out_count, out_last_tick).
  - path_find_test_direct_reachability: the BSP segment query now gets (1, &result, bsp, 0, 0, point_b, a - b, FLT_MAX); the draft passed four unrelated values (0.3 -> 0.85).
- **biped_update_facing: the vector helpers were declared without prototypes** (extern void f();), so vector3d_rotate_about_axis(v, axis, sin, cos) had its float angles promoted to double: every biped turn rotated the forward vector by garbage, and a garbage or NaN orientation can make an object stop rendering (another vanishing candidate). Real prototypes added. New scanner scratchpad/knr_float.py finds no other gameplay K&R float calls (only antenna_render_wire build_sprite). player_effect_build_camera_shake_matrix declared periodic_function_evaluate's phase as float; the definition takes a double. Fixed.
- Units argument fixes:
  - unit_get_look_origin_and_direction: unit_get_crouch_height_offset(out_origin, unit, &h, &r); the draft left the look origin unset on the non-marker path.
  - object_physics_check_impact_damage: crouch height (&sample[2], candidate, &sample[0], &sample[1]).
  - unit_detach_from_parent: unit_try_ready_weapon(unit, 1, 0).
  - unit_build_seat_occupant_zone_list: object_list_reference_add(list, child).
  - unit_find_best_seat_to_enter: actor_check_vehicle_target_available(unit, occupant actor, 0).
- object_physics_handle_nearby_object_impacts (0x508a10) REWRITTEN (0.3 -> 0.9). Impact damage now gets (&self collision context, biped) and the mass-point overlap gets (&self physics context, &other context). The draft passed throwaway scratch buffers, a crash path when vehicles touch bipeds.
- projectile_response: the two on-object impact effects now get (projectile, effect tag, hit object, node, 5, names, positions, frames, scale, fade, 0, 0); the draft dropped the three register arguments, so bullet-impact effects on objects spawned from garbage. vector3d_randomize_direction(velocity, velocity, &seed, 0, noise); the draft passed (0, noise).
- contrail_advance callers (object_delete_attachments, projectile_detonate) now pass the contrail handle (EDI). weapon_transfer_ammunition plays the pickup sound (EDX = tag +0x49c).
- **objects_initialize: the object memory pool had a garbage size.** The binary calls game_state_new_pool("objects") with EBX = 0x200000; the C passed no size, so pool->size and free_bytes came from a stack slot and game_state_cursor advanced by that garbage. Every later game-state allocation (object globals, the object-name table, cluster partitions, and everything initialized afterwards) was placed at a garbage offset. The garbage collector's memory-critical test (which can release actors) reads the same size. Fixed.
- player_effect_set_camera_impulse (0x4579b0) REWRITTEN (0.15 -> 0.85):
  - the impulse direction comes from normalized COPIES (the draft normalized the caller's damage direction in place);
  - the random magnitude uses min/max at +0x60/+0x64;
  - the cross product is up x direction and the rotation is about the direction;
  - the damage look nudge now calls player_compute_view_forward_vector(player, &yaw_pitch, &fwd) and game_engine_update_local_player_look(player, left.dir * s, fwd.dir * s); both had been called with no arguments, so getting hit could jerk the player's view by garbage.
- effect_new_at_texture_coordinate (local_player_index_for_object gets its object), weapon_stop_object_effect (effect_new_at_texture_coordinate(tag, item, -1, -1, -1)) and game_stop_current_map (decal_clear_flags(1)): arguments fixed. hs_effect_spawn_at_location (effect_new x99 in a10) VERIFIED (0.9).
- BOOT NOTE (user-requested boot):
  - The first crash was not a code bug: scratchpad/cdb_child.txt still held hard-coded breakpoints from the vanish-tracing session, which after the relinks landed mid-instruction (privileged instruction in update_client_queue_apply_tick). They are removed (the old copy is kept as scratchpad/cdb_child_trace_old.txt).
  - The real crash was vehicle_update -> vehicle_calculate_ground_lean_controls (type 3, a vehicle in the menu scene), whose math helpers were all called without arguments. Game-state cursor at the crash: 0x3fd68c of 0x400000, so the pool fix does not overflow.
- vehicle_calculate_ground_lean_controls (0x573100) REWRITTEN from objdump (0.1 -> 0.85): lean easing, desired basis from the facing, side-slip rotation, the axis-angle torque from the basis difference, forward/up force, then object_physics_tick(obj, 0, contacts, &F, &T) and the thruster effects.
- Vehicle solvers, types 0..2:
  - turret and steering_wheel: VERIFIED except that the EDI powered-mass-point buffer (vehicle_update [esp+0x88]) was a NULL stand-in written through and passed as the contact buffer. It is now a real parameter, and object_physics_tick gets (unit, buffer, contacts, 0, 0) (0.9).
  - vehicle_calculate_lean_controls (0x572df0, type 2): REWRITTEN (0.15 -> 0.85). Drive/steer entries in the ESI buffer, then the roll-correcting torque about forward.
  - vehicle_update passes node_output as the third argument.
- unit_update_marker_skid_effects (0x575460) REWRITTEN 0.25 -> 0.85: material_effects_play_at_marker now gets EAX = tag +0x3dc, the stack args in binary order (9/10, contact +0x70, &obj +0x98, intensity) and the EDX position / EDI offset locals. The draft passed the effect index as the tag and left the position/offset unset.
- Collision-result stack overflows fixed (collision_test_movement_segment writes a 0x50-byte collision_result):
  - vehicle_create_hover_thruster_effects (0x574900) REWRITTEN 0.2 -> 0.85 (20-byte buffer, wrong normal offsets, randomize args unset).
  - vehicle_create_hover_thruster_midpoint_effects (0x574bc0) REWRITTEN 0.15 -> 0.85 (cast from an uninitialized array, 20-byte buffer, wrong point/vector blocks).
  - unit_update_marker_traction_effects (0x575170) REWRITTEN 0.15 -> 0.85 (runs every vehicle tick: 20-byte buffer, wrong mass-point fields, t never read).
  - actor_choose_random_point_near (0x40faf0) 0.2 -> 0.85 (first cast origin is the input point, 20-byte buffer, t at +0x14).
  - actor_grenade_parabolic_path_clear: scratch 64 -> 0x50 bytes.
- game_engine_update_local_player_look (0x472160) REWRITTEN 0.15 -> 0.85: the unit/seat camera block (camera + 8) now drives the pitch limits, the auto-level target and the tilt adjustment. Autolevelling and seat pitch limits were dead code before. The 0x6c-byte seat marker had been written into a 60-byte buffer, overflowing the stack whenever the player sat in a seat with a yaw range.
- actor_target_data_acquire: scratch 0x30 -> 0x38 (actor_get_firing_positions writes 0x38; the frame slot is [esp+0x20] to the end).
- chimera__spectate_fp_camera_position verified 0.85.
- particle_impact_response_dispatch (0x4565a0) REWRITTEN 0.15 -> 0.85: takes the particle (EAX), fourcc (ECX), tag index (ESI) and intensity (stack). The 'effe' path spawns with {velocity, gravity} vectors at the particle position; the 'snd!' path plays at a full sound_placement. The draft spawned an effect with definition -1 at a NULL position and played a sound with a NULL placement.
  - Callers fixed: particle_impact passes death_effect (+0x58/+0x64); particle_update_motion passes collision_effect (+0x48/+0x54), where it previously passed fourcc 0, a no-op.
- particle_update_motion (0x4561a0) verified, 0.3 -> 0.85. The material_effects call is now gated on tag id != -1 (it was != 0, so particles without material effects played tag -1 near the player) and passes the collision normal as the offset (it was zero).
- particle_impact verified 0.9.
- Weapon fixes:
  - weapon_trigger_continue_burst fires trigger_index + 1 (it refired the same trigger).
  - weapon_reload_recovery_finish plays the tag's overheat_detonation (+0x390) instead of -1 before deleting.
  - Verified 0.9: weapon_magazine_begin_chamber, weapon_trigger_become_charged, weapon_stop_object_effect.
- Verified 0.9-0.95: unit_get_primary/secondary_eye_marker_position, object_new, unit_reset_light_effect, object_dispatch_effect_notify, physics_scalar_step_to_target_clamped, physics_scalar_move_toward_target (blam-cc: range is in ESI).
- effect_compute_spawn_basis (0x451930) rewritten to the real (AX index, EBX context, stack out) convention. It duplicates effect_marker_from_node_table.c (same address, 0.9), so both resolutions are now correct.
- object_collision_test_cluster_group: added the 0x7fffffff leaf mask; verified 0.9. Note: 0x746f98 and 0x746f90 are always written with the same collision BSP, so the C files that use 0x746f98 are fine.
- Duplicate-address files verified equivalent (0.95): object_resolve_collideable_reference, object_disconnect_from_map.
- Noted, not fixed: object_collect_local_player_relevant_objects (0x4fa1a0) is really one function spanning 0x4fa1a0..0x4fa39e (EDX point; stack filter, out, max, context), and 0x4fa280 is not a separate function. Its only caller is the MP nameplate path (gated on current_game_engine), so it's skipped.
- effect_spawn_particles (0x451f90, 2600 bytes) REWRITTEN 0.2 -> 0.85 from objdump. The particle_new creation record now carries the direction (+0x1c), initial angle (+0x40), the angular velocity roll (bit 3, +0x90) at +0x44, the radius roll (bit 9, +0xa0) as the scale, the first person marker flag and the create == 2 / == 1 bytes. The draft rolled both properties with bit 0 and the wrong ranges, dropped the direction/angle/flags, and called a bare __ftol() for the debug halving.
- player_effect_build_camera_shake_matrix (0x457390) REWRITTEN 0.2 -> 0.85 from objdump:
  - A finished scripted shake now clears bit 0 (the draft cleared bit 1, so the shake never ended).
  - Transition/periodic calls use their real type fields (+0x54/+0x88 in CX, +0xa0 in AX) and phases.
  - The impulse magnitudes come from +0x8c/+0x90, and the second random offset gets the raw magnitudes.
  - The placed rotation keeps its position (the draft overwrote it).
- particle_new verified 0.9 (leaf mask added); material_effects_play_at_marker verified 0.9; player_effect_random_shake_offset verified 0.9.
- Screen flash chain fixed:
  - player_effect_set_screen_flash_for_player takes (EAX player, descriptor, falloff) and forwards duration 1.0. Its four callers (shield recharge, full health, kill streak, teleporter) now pass the player; they had dropped it and passed 1.0f as an integer.
  - player_effect_set_screen_flash: the weight is descriptor +0x24 and the maximum +0x20 (the draft swapped them).
- effect_random_direction_vector: effect_property_random_value now gets bit 3 plus the caller's EBX/ESI/EDI (self and the a/b bits); the draft passed bit 0 and zeros. Its caller effect_event_apply is updated.
- effect_property_random_value verified 0.9.
- player_effect_apply_generic_damage_feedback (0x4569d0) verified and fixed:
  - flash is {type 1, +2 = 2, duration 1, max = fraction, weight 0, white}; shake is {duration 1, +8 = fraction * 0.01}.
  - Its caller main_switch_structure_bsp now passes the player in EDX; it had passed only the fraction, so the player index was garbage.
- player_effect_set_camera_shake verified 0.9.
- player_effect_build_screen_flash (0x457000) verified 0.9. Scripted fades use transition type 5, player flashes use their own type (flash +0x14), flash ticks count down by the tick length, and scripted ticks reset only for a real local player.
- Verified 0.9: player_effect_apply_at_object (blam-cc corrected: stack tag, ESI origin, player index always 0), effect_new_at_texture_coordinate.
- Effect lifecycle fixes:
  - effect_stop starts loop_stop_event + 1 (the draft replayed the loop stop index itself).
  - effect_start_event picks the global seed only for tag flag 4, otherwise the effect seed.
  - effect_random_velocity_vector passes its a/b bitsets into the property roll (they were zeros).
- Verified 0.9: effect_new, effect_marker_new.
- particle_next_sequence (0x455e60) FIXED: leaving the initial state falls straight into the looping pick in the same call. The draft's `else if` skipped it, so any particle without initial sequences got sequence -1 and was impacted (deleted) at its first pick, meaning such particles vanished at spawn.
- Verified 0.9: particle_advance_animation, particle_advance_frame.
- Footstep and material effect chain:
  - effect_marker_environment_probe (0x4533b0) had its effect-type and surface-material indices swapped (ESI is the first stack argument).
  - unit_update_footstep_and_idle_triggers now resets the idle counter after the idle trigger fires; it had retriggered every tick.
  - Verified 0.9: unit_fire_animation_sound_trigger, biped_advance_frame_counter_trigger, biped_trigger_on_velocity_threshold.
- Verified 0.9: particles_update, particle_update_physics_default.
- unit_seat_index_is_valid tests the entering object's labels (EAX), not the vehicle's. unit_check_weapon_use_permission now calls the game engine permission callback.
- Verified 0.9-0.95: unit_state_allows_control, unit_state_is_scripted_animation, unit_animation_state_is_compatible (jump tables decoded), unit_get_tag_flag_bit7, unit_is_look_target_valid, unit_get_active_weapon_scale, unit_current_weapon_is_type, unit_local_player_weapon_flag_check, unit_all_seats_unoccupied, unit_set_custom_animation_frame, unit_get_custom_animation_time_remaining.
- BOOT CRASH FIXED (bsp3d_node_find_leaf NULL BSP, via vehicle_calculate_ground_lean_controls -> object_physics_tick -> object_physics_tick_single_pass). The single pass's two find_leaf calls now pass the collision BSP [0x746f90] and mask the leaf, and the powered mass point quaternion comes from entry +0x1c (it was NULL). The vehicle solver rewrites made this path live.
- User report: "NPCs are looking upwards". unit_clamp_direction_to_aim_or_look_bounds (0x5697a0) REWRITTEN (0.25 -> 0.9). The draft projected the direction into the unit frame with scrambled components (e.g. up.k*dx + forward.j*dz), so aim/look pitch was garbage and pinned to the bound. It is used by unit_update (aim and look with zero turn rate) and by actor_get_aim_from_position.
- vector3d_rotate_toward_bounded (0x564ae0, the rate-limited aim/look/facing servo) verified end to end against objdump (0.4 -> 0.9, no change).
- OPEN (user report): "can't get out of this cryo tube". Exit path unit_exit_vehicle -> unit_try_exit_controlled_seat (0.85) plays seat exit animation slot 8 (state 0x1b); the unit_state_is_scripted_animation gate matches. Unknown whether the tube opens or the tutorial stalls first.
- actor_update_look_target (0x415480, 3896 B) REWRITTEN from objdump (0.25 -> 0.85). This is the per-tick AI facing/aim/look selection; its caches become the unit's desired facing/aiming/looking vectors. The draft drove the first block with the vocalization priority and the switch with the flee/reason kind (the binary does the reverse), wrote the vocalization point into the flee point, and tested local copies of the side flags where the binary reads actor +0x58d/+0x58e. AI look vectors therefore came from the wrong sources (user report: NPCs look upward). The idle-look search, body-turn check, horizontal facing flatten, facing hold and aiming-speed switch are all decoded.
- unit_apply_control_block verified 0.9 (control +0x34 look -> unit +0x254).
- AI look helpers (for the "NPCs look upward" report):
  - actor_resolve_look_target (0x414d00): the facing target is written into the actor's record +0x56c (was a local), and the aim search direction is flattened (z = 0) before normalizing, as at 0x414dff.
  - actor_look_randomize_direction (0x414f50): the target goes into record +0x57c (was a local), and the cone pick passes BL = 0.
  - actor_point_in_directional_lane (0x414990): normalizes a COPY of the 2D forward. The draft normalized cone_axis in place, rewriting the actor's facing cache on every call. Comparisons are now <=.
  - Verified 0.9-0.95: actor_look_pick_random_point_in_cone, point3d_within_horizontal_cone, actor_get_idle_facing_range, actor_reset_queued_look_vector.
- actor_update_facing_change_timer: clamps +0x354 UP to 1.8 (the draft stored 0.9); verified 0.9.
- Verified 0.9: actor_select_facing_target_prop (writes {1, prop} into the caller's record), actor_resolve_flee_source_point (all 7 kinds).
- actor_update_aim_wander (0x40fcb0, 2092 B) REWRITTEN from objdump (0.2 -> 0.85). Fixes: the burst length +0x5f4 is a random time from the burst block (+0x14..+0x18, scaled, x0.6 when +0x1ca) in ticks; the draft truncated +0x458. The bombardment randomizes a copy of the target, not +0x62c. The burst block (EDI out_a) and scale block (ESI out_b) were mixed. The wander offset (side/up plane, radius A), the per-tick return (radius B / ticks), the burst clamp against the tangent of the burst sweep at +0x638, and the firing-line broadcast are added.
- Verified 0.9: weapon_get_zoom_fov (really the difficulty table lookup), weapon_get_zoom_fov_resolved, actor_get_actor_definition.

## Static loop: verifications + ray chain + marker direction (no boot)
- Verified 0.9-0.95 against the binary: actor_select_stance_offset_pair, actor_target_is_visible_or_object_count_ok, actor_score_blast_area_clear, actor_target_data_refresh, actor_target_scan_potential_targets (end to end); weapon_get_zoom_fov(_resolved) is really the difficulty table lookup.
- actor_target_relationship_think: all 25 call sites agree with the binary; 0.7 (inner logic not re-derived).
- encounter_redistribute_squads_toward_targets: FIXED the unassigned-list insert, which resets firing_position_index and a type 3/4 active movement a second time before the mode callback (0x439d05); 0.7.
- object_collision_test_ray_nearby_chain (0x5055b0): decoded end to end; FIXED the vehicle mass-point path, which now also stores permutation_index = -1 (0x5056fa); 0.85.
- unit_get_average_active_marker_direction (0x575e30): REWRITTEN (0.15 -> 0.85). The old C passed a pointer-sized local as the 0x3c-byte object_physics_context (stack overrun inside the build call), transformed through a zero matrix and threw the result away, and returned position - average (the wrong sign). The binary returns normalize(ctx matrix * average(active mass point positions) - unit position). actor_movement_update, movement style 4, stores -direction into +0x5a4, so AI vehicle facing in that style was reversed/garbage before.
- Relink: left unresolved 1, traps 128.

## Static loop: charge trigger + grenade aim (no boot)
- actor_evaluate_custom_charge_trigger (0x424090): REWRITTEN (0.15 -> 0.85, raw offsets). The old C set the retry timer +0x364 with __ftol(0.0), dropping the random_real_range result, so it was always 0 and wrapped to -1 on the next decrement, freezing the decision for ~65k ticks. The binary stores max(random * 30, 31) truncated. The ally split in the new-roll path also read +0x378 where the binary reads +0x358.
- actor_compute_grenade_aim_direction (0x40f7e0): REWRITTEN clamp (0.2 -> 0.85). When the throw direction is >= 30 degrees from the actor's aim forward, the binary returns the forward turned 30 degrees about normalize(D x F). The old C returned the cross product rotated about the forward, i.e. grenades thrown sideways.
- Relink: left unresolved 1, traps 128.

## Static loop: actor_rate_potential_target (no boot)
- actor_rate_potential_target (0x41fd50): verified end to end (0.25 -> 0.85). Two fixes. No threat weapon, same team: the binary scores 2 beyond the melee threshold and 3 within it; the old C left 0/5 for the far case. Threat path with a NULL threat weapon definition: it falls through to the team/range checks instead of scoring 2. The score is now summed in the binary's order.

## Static loop: swarm leap offset (no boot)
- actor_compute_swarm_avoidance_offset (0x425c70): REWRITTEN leap branch (0.25 -> 0.85). The old C called projectile_solve_ballistic_arc without its register arguments (EAX = target prop +0xc8, ECX = component +4, ESI = out leap, EDI = 0) and with the wrong stack layout, then always steered along the actor facing. The binary uses the solved leap direction (2D-normalized, falling back to facing +0x174 and then the global forward) times the horizontal speed. z is the half-gravity term, capped at 0.075 unless prop +0x130, and the result is clamped to length max(radius, 0.12).

## Static loop: danger register + communication target (no boot)
- actor_danger_register_stationary_object (0x41ea60): verified end to end, 0.9, no change. The EAX `reference` is listed in the header blam-cc, which gen_hooks merges.
- ai_select_communication_target (0x42ec90): three fixes (0.2 -> 0.85).
  - Kind 3 looks up param_b's object (+0x1f4), not param_a's.
  - The 11-argument ai_communication_select_speaker_by_team was called with 10 arguments. Its DI team (param_a object +0xb8) was stack garbage, so the team filter picked random speakers.
  - actor_classify_communication_object_type is given the selected speaker (EAX = result), not the line kind.
- Relink: left unresolved 1, traps 128.

## 2026-09-27 (user testing) -- a10 cryo tube exit: NOT A BUG
- User: "can't get out of the cryo tube when the NPC says you can come out". tutorial_action shows the help text, enables input, resets the action test, then sleep_until player_action_test_action (bit 0x1 = control_flags 0x40 = raw button 2, the action key). Only after that does it fade out and call unit_exit_vehicle player0.
- The cdb trace (scratchpad/cdb_action_trace.txt) showed the script waiting at the test with jump/look/move bits set but no action bit, i.e. the user had not pressed the action key. The user then pressed E and the exit works.
- Statically verified on the way (no change): game_engine_digitize_control_input (0x472760) bit mapping and edge tail, hs player_action_test_action/accept/reset, hs player_enable_input, the button suppression and copy loop in game_engine_build_local_player_control_input, and the widget_close suppression.

## 2026-09-27 -- a10 "press E" help text never shown (OPEN, static checks done)
- User: the game never showed the "press E" prompt. tutorial_action calls show_hud_help_text / enable_hud_help_flash / hud_set_help_text before the action wait.
- Verified against the binary, no difference found:
  - the three hs evaluators and hud_set_help_text (0x4adb30);
  - hud_update_player (always ends in hud_messaging_update) and its caller first_person_weapon_update_screen_effects -> render_window;
  - hud_messaging_update: setup, help-shown gate, help colour, line rectangle, element loop, button icon (bound key name in quotes), text span (extents EBX/ESI/EDI, -3 cursor, draw);
  - game_engine_local_player_score_is_nonpositive (1 in SP); hud_state_reset (map start only);
  - current/render_local_player_index both resolve to 0x7c3108 in resolve.asm.
- The 20 struct offsets used (scenario hud_messages 0x5a0, HUDMessageText, message 0x40 stride, hud_messaging_globals 0x460..0x474, player record 0x460 stride, HUDGlobals 0xd0/0xd4/0xe2/0xfc, hud_flags +1) were checked with compile-time asserts (scratchpad/offchk.c, scratchpad/cchk.py).
- hud_anchor_offset_to_screen_position (0x4ab690): REWRITTEN (0.3 -> 0.85). The ECX child-placement jump table (0x4ab8b8) was an argument-less function-pointer call. All callers pass ECX = 0, so it's not the cause.
- OPEN, needs a runtime check (user present): break in hud_messaging_update at the tutorial_action prompt. Check help_shown (hud_flags +1, hud_messaging +0x46c), the message panel_count, and whether chimera__draw_16_bit_text (0.2) is reached with the tutorial text. Menus draw through the same text call, so a HUD-context render state (font or colour alpha) is the next suspect.
- Relink: left unresolved 1, traps 128.

## 2026-09-27 (user testing) -- HUD help text invisible: FIXED (chimera__draw_16/8_bit_text glyph state)
- A cdb trace (scratchpad/cdb_hud_trace.txt) showed hud_messaging_update drawing the help text correctly: "Self-Diagnostic Software Enabled", rect top 68 / left 8 / bottom 89 / right 304, colour a 0.50, rgb (0.46, 0.73, 1.0). The user's YouTube reference shows exactly that light-blue top-left help text ("Look around"). So the HUD logic was right and the draw was invisible.
- chimera__draw_16_bit_text (0x514ab0) and chimera__draw_8_bit_text (0x5148b0) left four glyph-state fields zero that the binary sets before rasterizer_draw_text_begin:
  - +0x28 / +0x2c = 1.0;
  - +0x40 / +0x44 = 1 / atlas width, 1 / atlas height.
  rasterizer_draw_text_begin copies +0x40/+0x44 into c17.xy, the scale from the glyph callback's pixel texture coordinates to 0..1, so every glyph sampled a single texel. Both fixed (0.2 -> 0.85). The rest of both functions was verified.
- OPEN: why menu text rendered despite this. Possibly a different vertex shader or constant path in the UI pass; worth a look if menu text shifts after the fix.
- Relink: left unresolved 1, traps 128.

## 2026-09-27 (user testing) -- a10 crash in cluster_flood_fill_within_radius: FIXED (pack(1) padding)
- The crash came after the cryo exit: the hs damage_new -> hs_damage_apply_at_location -> damage_apply_area_effect -> object_find_in_sphere -> cluster_flood_fill_within_radius chain read through a garbage cluster index (0xfde4). Log: scratchpad/cdb_crash_textfix.txt.
- Cause: types/hs.h is #pragma pack(1) (every header is), and hs_damage_request relied on implicit alignment. After uint16 unknown_10 the int32 leaf landed at 0x12, not 0x14, and after uint16 sound_index the position landed at 0x1a, not 0x1c. damage_apply_area_effect reads the binary layout (+0x14 leaf/cluster, +0x1c epicentre), so it got garbage. Explicit pads (unknown_12, unknown_1a) were added; this also fixes hs_damage_apply_with_sound, which uses the same record.
- New tool scratchpad/packaudit.py compiles an offsetof assert for every commented field offset (relative to the struct's first commented field) and every '// size' in types/*.h: 673 structs, 5889 checks. Besides the above:
  - actor_placement_request unknown_13[2] -> [3]: unknown_16/_18/_1a/_1c sat one byte low. The readers use raw offsets, so there's no behaviour change.
  - Harmless: flexible-array sizes (hs_function_definition, heap), actor_iterator_state and ai_communication_record trailing size, and networking (MP).
- Also: chimera__draw_16/8_bit_text glyph texel scale (previous entry).
- Relink: left unresolved 1, traps 128.

## Static loop: a10 tutorial panel lights (user report: "shouldn't the lights be flashing")
- Script (scratchpad/hsdecomp.py, a new s-expression dumper for scenario scripts): tutorial_looking_targeted / test_looking_cycle turn on all five panel flags at once. Each test_looking_cycle_<dir> swaps looking_panel_<dir>_success for looking_panel_<dir> and waits for objects_can_see_object. So all five lit at once is expected.
- scratchpad/objnames.py: the panels are scenery "scenery\light lens flare\light lens flare yellow" (green for _success), placement flag 1 (not automatic). scratchpad/tagatt.py: each has one light attachment at marker "smoker" whose primary scale is object function 1, and the light carries the lens flare. So the flash is a lens flare driven by the object function.
- Verified along the way, no change: scenario_objects_place and object_new_from_scenario_placement (the not-automatic flag and the 0x6b8cbc gate in game_start_new_map); object_update (gating, light attachment refresh, flag/offset asserts); object_for_each_light_attachment; object_light_recompute_transform (transform only); object_create_attachments (0.45 -> 0.9, all five types).
- lens_flare_render_all (0x513cf0): REWRITTEN (0.25 -> 0.85, raw offsets). The draft discarded each reflection's colour and specular, never passed the scale pair, colour or rotation, called rasterizer_lens_flare_quad_add with 2 of 5 arguments, and called set_current_key / set_vertex_specular without prototypes. Every lens flare in the game drew with garbage.
- OPEN (runtime): do the panel flares now show and pulse? If not, next are the light's function-driven intensity (object_lights_update_all 0.8) and the flare visibility and occlusion path (lens_flare_update_visibility 0.45, lens_flare_update_samples 0.4). Also OPEN: the cryo tech standing off to the right instead of in front of the panel during the look test (user screenshot).
- Relink: left unresolved 1, traps 128.
- Cryo tech position during the targeted look test (user screenshot: he stands off to the right, not in front of the panel). The command lists come from scratchpad/cmdlist.py, a new dumper.
  - looking_targeted_1 does not move him. His spot comes from moving_1_3: atom 1 go_to point 5 (the list has only points 0..4), then atom 3 go_to_and_face point 0 (-57.73, -1.11, 0.05), right in front of the panel lights (x ~ -57.3), facing point 1.
  - Verified against the binary: the out-of-range go_to returns 0 in both the binary (0x405624 jge -> result 0) and the C, and actor_squad_action_list_process then runs the next atom immediately, so atom 3 should run. actor_movement_set_destination_point (0x417610) matches, as does actor_movement_check_arrival (0x416700, 0.5 -> 0.9).
  - FIXED on the way: path_find_validate_and_record_goal (0x43a190, 0.3 -> 0.9, flying actors only) called path_find_test_direct_reachability with two NULL points. It is now EAX goal, ECX the body position (second argument, not unused), ESI local out, with a success-only copy.
  - OPEN: why the tech doesn't reach point 0. Next suspects are actor_movement_action_resolve's ground path (path_find_set_avoid_sphere 0.55, path_find_set_goal 0.6, path_find_context_init 0.7) or the go_to_and_face completion. A runtime check would show whether active_movement (+0x46c) holds point 0 after moving_1_3 starts.
- Relink: left unresolved 1, traps 128.

## Static loop: movement resolver, look cone, small AI verifications
- actor_movement_action_resolve (0x41a460): the ground-path branch (request, extra override, avoid sphere, context init, goal, run, reconstruct, completion checks) and the destination gate match the binary. 19 scenario/actor offsets were asserted (scratchpad/offchk5.c). The moving_1_3 point 0 surface index is 0x484 (valid), so the cryo tech's go_to_and_face reaches path_find_run. The tech-position OPEN item stays OPEN (runtime).
- unit_point_within_look_cone (0x56c100, hooked/stable, proven different): FIXED (0.25 -> 0.9). The binary normalizes (point - eye marker world position) before the dot with the unit look vector. The draft normalized an unused zero vector and dotted the raw difference, so the cone widened with distance. This affects objects_can_see_object, which gates the a10 look-tutorial panels, and the conversation participant checks.
- VERIFIED: actor_get_aim_from_position (0x40f9b0, 0.9) and actor_handle_death (0x40dd50, 0.9).
- Skipped: unit_spawn_with_starting_weapons (only caller is network_game_action_apply, MP).
- Relink: left unresolved 1, traps 128.
- VERIFIED (no change): hs_object_list_any_angle_match_gated (0x487ad0, 0.9), ai_conversation_resolve_participants (0x430fc0, 0.3 -> 0.85, end to end), actor_find_grenade_landing_spot (0x410c90, 0.9), object_nudge_position_by_velocity (0x4f7c40, network extrapolation, 0.85), unit_update_ground_contact_counter (0x575640, 0.9, offsets asserted).
- unit_set_facing_from_index_table (0x570de0): FIXED (0.25 -> 0.85). object_set_position_and_orientation gets EDI = the spawn record's position (the vehicle placement +0x08, or the scenario +0x37c entry in game-engine mode 5). The draft passed the vehicle's current position, so a vehicle reset only turned it in place instead of moving it back to its spawn.
- Skipped (MP): object_apply_shield_charge_and_notify (network message-delta handler).
- Relink: left unresolved 1, traps 128.
- actor_targets_share_descriptor (0x40e380): FIXED (0.25 -> 0.9). In the type-0 case the binary compares the last known positions of each actor's current target prop (+0x270), within 0.7 m. The draft read a datum out of the mode data (+0xa8).
- object_gather_light_list (0x4f2430): VERIFIED (0.85).
- Relink: left unresolved 1, traps 128.
- actor_check_step_obstruction (0x417bb0): verified end to end (0.25 -> 0.85). FIXED the inverted second probe. After the forward midpoint segment misses (obstructed, used_point_check), the binary casts step_up downward from the far end and clears the obstruction only when that misses; the draft cleared it on a hit. This affects actor_probe_step_direction and actor_find_danger_escape (AI stepping and escaping), and is a candidate for AI failing to walk around obstacles.
- path_find_compute_heuristic (0x43a310): VERIFIED (0.9).
- Relink: left unresolved 1, traps 128.
- actor_probe_step_direction (0x417e50): FIXED (0.4 -> 0.9). Variant 4 had the random side swapped: a roll <= 0x8000 takes (j, -i) with index 1.
- network_index_cache_remove (0x4e9d40, EAX container, ESI key) callers: the container is the ADDRESS 0x6870d8 (mov eax,imm; its dword is 0xd, followed by the name "object_index"). object_delete_by_pooled_node_id, object_delete_unparented, projectile_detonation_message_apply and projectile_send_detonation passed the dword's value (0xd, so the callee would read [0x65]). unit_exit_vehicle_seat (0x568120, really the player unit release) passed the unit as the container. All five fixed; unit_apply_damage_effects already used the array (address) form.
- Relink: left unresolved 1, traps 128.
- New scanner scratchpad/addrscan.py finds pointer-typed externs (extern T *x; // 0xADDR) whose owning function only uses the address as an immediate (push / mov reg,imm), meaning a static object whose ADDRESS is the argument, while the C loads the dword stored there. Real hits, all FIXED (declared as arrays):
  - player_help_screen_select_by_name: the 10 player_help_name_* strings. strstr(name, <first 4 string bytes as a pointer>) could crash or mis-select, reached through display_scenario_help, which the a10 look tutorial calls.
  - out_of_objects_error_prefix (object_new_with_datum_role_control) and console_error_category_objects (objects_garbage_collection): console error-log category strings.
  - s_no_weapon_label (unit_check_weapon_use_permission, unit_pickup_weapon).
  - projectile_create_from_network: network_index_cache_insert_if_free container (MP).
  False positives: actor_data (a coincidental 'or eax,imm'). Networking-module hits were left alone (MP).
- Relink: left unresolved 1, traps 128.
- scratchpad/addrscan2.py generalizes the scan to non-pointer externs. After filtering struct-typed hits (field access), no new real bugs: controls_gamepad_list_find loads through the immediate (same as reading the value), and the other hits are MP or coincidental immediates.
- unit_record_recent_damage_and_react (0x568230): REWRITTEN attacker/callout section (0.25 -> 0.85). The recent-damage bookkeeping (4 records at +0x430; a free slot, else the oldest that isn't the largest-damage record) and the hostility gate were verified. FIXED:
  - the attacker link (+0x324 for response 9, else +0x328) is read from the attacker OBJECT, not its tag definition;
  - the callout counter (+0x42a / +0x42c, 120-tick window) and the 3/5 threshold (+0x218 controlling player) live on the attacker, not the damaged unit;
  - ai_communication_broadcast gets the attacker HANDLE (the draft float-converted the raw parameter).
  This drives "taking fire" AI callouts.
- Relink: left unresolved 1, traps 128.
- unit_find_best_seat_to_enter (0x566560): VERIFIED end to end (0.25 -> 0.9).
- player_check_vehicle_interaction (0x478600): FIXED (0.4 -> 0.9). The upside-down vehicle's "flip" interaction (action 0xb) passes EBX = the vehicle; the draft passed -1, so the flip prompt had no target.
- Relink: left unresolved 1, traps 128.
- object_start_animation (0x4fa8d0): VERIFIED (0.3 -> 0.9). Its two callers were hs evaluators with no C: scenery_animation_start (0x47b760, 11 campaign uses) and scenery_animation_start_at_frame (0x47b7b0, 3 uses). NEW src/hs files written from objdump; each would have trapped (unlisted callback).
- Coverage check: all 195 hs functions a10 uses have C. Across the whole campaign (scratchpad/campaign_hs.txt), 73 hs evaluators still have no C and trap when a later level calls them; the list is in scratchpad/hs_missing_campaign.txt. The biggest are vehicle_unload 145 uses, ai_go_to_vehicle 143, vehicle_test_seat_list 87, ai_spawn_actor 82, ai_automatic_migration_target 72, ai_vehicle_encounter 57, recording_play_and_hover 49, ai_status 44, ai_disregard 39, unit_set_enterable_by_player 35, damage_object 34, object_set_permutation 32, object_create_containing 30. OPEN: next static work item (beyond a10).
- Relink: left unresolved 1, traps 128.
- Missing campaign hs evaluators, first 17 written from objdump (scratchpad/gen_hs_wrappers.py and gen_hs_wrappers2.py generate the thin typed-argument wrappers; callee prototypes checked with scratchpad/protos.py):
  - vehicle_unload, ai_go_to_vehicle, vehicle_test_seat_list, ai_spawn_actor, ai_automatic_migration_target, ai_vehicle_encounter, recording_play_and_hover, ai_status;
  - ai_disregard, unit_set_enterable_by_player (inline +0x204 bit 0x10000, set when false), damage_object (stack effect, EBX object), object_set_permutation (stack object / permutation, EBX region), object_create_containing plus its callback hs_object_create_name_index_if_absent (0x487cf0, NEW), ai_vehicle_enterable_distance (inline attention record +0x4), ai_attach, ai_exit_vehicle.
  Unlisted (trapping) callbacks drop from 609 to 591.
- Noted for later: hs_object_set_permutation_by_name calls object_set_permutation_by_name(param_2, region index, 1) without the object; check it against 0x488670.
- Relink: left unresolved 1, traps 128.
- hs_object_set_permutation_by_name (0x488670): FIXED (0.5 -> 0.85). object_set_permutation_by_name gets EAX = the object plus stack (permutation name, region index, 1); the draft dropped the object. The documented convention is corrected too: object and permutation on the stack, EBX = region name.
- 18 more missing campaign hs evaluators (scratchpad/gen_hs_wrappers3.py): device_group_get, ai_nonswarm_count, ai_living_fraction, device_one_sided_set, ai_vehicle_enterable_disable, ai_vehicle_enterable_actor_type, object_set_collideable, device_group_set_immediate, volume_test_object, unit_impervious, ai_renew, ai_maneuver_enable, ai_allow_dormant, vehicle_riders, ai_erase_all, garbage_collect_now, cls, game_time. Unlisted callbacks now 573 (from 609).
- Still missing (campaign): inspect (hs internal), fast_setup_network_server (MP host), and the lower-use tail in scratchpad/hs_missing_campaign.txt.
- Relink: left unresolved 1, traps 128.
- 19 more campaign hs evaluators (scratchpad/gen_hs_wrappers4.py): device_set_never_appears_locked, device_get_power, ai_migrate_by_unit, ai_migrate_and_speak ("advance" selects the platoon merge), ai_free_units, ai_attack, ai_set_blind / ai_set_deaf (encounter record +0x40 / +0x41), ai_prefer_target, ai_braindead_by_unit, ai_kill_silent, camera_set_first_person, ai_teleport_to_starting_location_if_unsupported, ai_going_to_vehicle, units_set_desired_flashlight_state, rasterizer_model_ambient_reflection_tint, ai_follow_target_ai, sound_enable_eax, breakable_surfaces_enable. Unlisted callbacks now 554 (from 609).
- ai_object_list_remap_units_and_children (0x433a70): FIXED (0.3 -> 0.8). The remap calls pass notify = 0: the binary pushes (reference, 0, 0), and EDX is overwritten before use, so the guessed EDX parameter was never real.
- Relink: left unresolved 1, traps 128.
- The final 19 missing campaign hs evaluators (scratchpad/gen_hs_wrappers5.py, 6.py): unit_has_weapon_readied, script_screen_effect_set_value, set_gamma, rasterizer_fixed_function_ambient (grey 0xFFvvvvvv), object_set_ranged_attack_inhibited (+0x107 bit 1), object_set_melee_attack_inhibited (+0x106 bit 0x80), numeric_countdown_timer_set / _stop, device_group_change_only_once_more_set, camera_set_dead (director mode 3), ai_teleport_to_starting_location, ai_follow_distance, ai_conversation_line, ai_allegiance_broken, unit_solo_player_integrated_night_vision_is_active, breakable_surfaces_reset, player_action_test_jump, game_safe_to_save, fast_setup_network_server.
- Campaign hs coverage: every hs function the campaign scripts call now has C, except inspect (a debug print of a value, hs internal) and play_update_history (MP debug; its callee player_update_history_play_local_player's C lacks the stack byte argument). Unlisted callbacks 609 -> 535 this session.
- Relink: left unresolved 1, traps 128.

## 2026-09-27 AI pathfinding fix (runtime trace, user present)
- FIXED: the a10 cryo tech never walked to the panel. Chain: moving_1_3 atom 3 -> actor_movement_set_destination_point
  -> action_resolve -> path_find_run = 1 but path_find_reconstruct_path = 0 -> ai_navigate_around_obstacles = 0 ->
  ai_search_step: the edge result reached the origin on the target surface, but ai_search_add_node returned -1. Its
  ancestor walk rejected same point + SAME side; the binary (0x43b5fc jne) rejects same point + DIFFERENT side.
  Every AI path in the game failed (actors completed go_to orders on the spot).
- Verified against the binary on the way: path_find_reconstruct_path, path_find_hash_lookup_vertex,
  ai_navigate_around_obstacles, ai_search_step, ai_search_evaluate_edge_cost (its 'distance'/'base_cost' params are
  really radius/length), path_find_trace_cluster_boundary_from_vertex, collision_bsp_surface_clip_line_2d,
  path_find_heights_are_close, decal_plane_solve_third_axis, actor_build_path_find_request, path_find_push_start_node.
- OPEN: a10 panel lens flares still not flashing (next).
- CONFIRMED by the user: AI pathfinding fixed (cryo tech walks to the panel).
- OPEN (runtime, a10 panel lights not flashing): the script does object_create looking_panel_* (name indices 0..4);
  object_new succeeds (active 1, query_0x28 1), object_create_attachments runs and light_new_attached is called
  with the yellow light tag 0xe9850811, owner = the panel, marker 0, function index 0, change colour -1. Yet no
  lens flare is ever queued for them (the five flares seen in object_lights_update_all are door locklights,
  definition levels\b30\devices\doors\door small\locklight). The yellow light colours are never black
  (0.62/0.40/0 .. 0.94/0.57/0), so the light must be missing from light_active_list (visibility collection).
  Verified against the binary on the way: object_lights_update_all flare section, light_new_attached,
  object_create_attachments, object_new gate, object_new_from_scenario_name/_placement, objects_update,
  object_update, object_update_functions, periodic/transition_function_evaluate, periodic tables init,
  object_light_recompute_transform (attenuation/cone/cluster insert). Next: snapshot the light record
  (+0x30 position, +0x10 cluster link, flags) and light_active_list after creation; suspects
  cluster_reference_add_within_radius (0.6) and structure_bsp_collect_visible_objects (0.5).
- Tooling note: cdb poi() sign-extends 32-bit values on this target, so compare with (poi(x) & 0xffffffff);
  a bp command that runs $$< must end with the filename (put the rest in the script file).

## 2026-09-27 lens flare occlusion fix (static; the user will not run more traces)
- Correction to the earlier panel note: the object-flare trace armed at list 13 and stopped after 400 hits (~2.7 s),
  before the script created the panel objects, so it never showed the panel lights lacking flares.
- FIXED lens_flare_update_samples (0x513ba0, was 0.4 -> 0.85): it moved each flare instance itself every frame
  (the binary builds the occlusion sample point in a stack local), used the flare normal for offset type 0 (the binary
  uses the camera forward 0x7c1234, scale -radius), and called 0x512190 with only the radius. 0x512190
  (render_rasterizer_dispatch_537800) returned void. So every flare's sample count (+0x24, which lens_flare_render_all
  requires > 0 at 0x513dad) was garbage: flares were skipped, or occlusion-tested at a garbage point, which also broke
  lens_flare_update_visibility's fade. Now: EDI slot = instance index, ECX = &sample point, stack radius, EAX returned.
- FIXED rasterizer_lens_flare_occlusion_query_get_result (0.6 -> 0.85): result starts at -1 (the draft returned the
  query pointer bits on a failed GetData).
- FIXED structure_bsp_portal_sphere_test (0.45 -> 0.85): the 2D tolerance is sqrt(tol^2 - distance * projected.x)
  as in the binary ([esp+0x14] at 0x554c6a), not distance^2. polygon2d_point_inside_tolerance rounds tol^2 first.
- VERIFIED: cluster_reference_add_within_radius, cluster_flood_fill_within_radius, lens_flare_update_visibility.
- The panel pulse chain (object function A cosine period 1 -> light t -> flare intensity +0x23 -> reflection
  brightness/radius lerp in lens_flare_render_all) was verified against the binary end to end.
- OPEN (runtime, when the user next plays): do the a10 panel flares now show and pulse?

## 2026-09-27 lens flares drawn again (CONFIRMED by the user: the a10 panel lights now flash)
- ROOT CAUSE: rasterizer_lens_flare_set_current_key (0x5120f0) ends with xor al,al and both callers test only AL
  (lens_flare_render_all 0x5143f8, light_volume_render_procedure 0x4fed19). The C returned the whole EAX (the
  bitmap tag id with its low byte cleared), so every real flare took the skip branch: no lens flare and no light
  volume was ever drawn. Now returns uint8_t 0.
- The user's screenshot showed the five panel squares with no glow at all, which pointed at the draw path.
- Verified on the way: rasterizer_lens_flare_quad_add (all 6 vertices), batch_find_slot, batch_draw_slot,
  batch_apply_material (inverted: non-zero from the binder = failure), rasterizer_validate_and_rebind_texture,
  rasterizer_lens_flare_occlusion_test_issue, occlusion_queries_create, lens_flare_get_visibility_byte,
  the reflection colour animation (the yellow panel flare pulses its tint orange <-> dark red, cosine 0.75 s),
  periodic_function_build_table. rasterizer_lens_flare_batching_select_mode mode 5 ALPHAARG1 fixed (0 = diffuse).
- New scanner scratchpad/alret_scan.py (+ alret_review.py): C functions typed 32-bit whose binary returns only AL.
  148 candidates, mostly networking; the gameplay ones reviewed return clean 0/1 or keep the right low byte.
  encounter_squad_spawn_reinforcement returns AL = 0 in the binary too and its callers cast to int8_t: fine.
- Crashes found while the user tested, all FIXED: rasterizer_sun_glow_render passed (target, rect) swapped to
  rasterizer_sun_glow_capture (crash with the sun on screen); chimera__draw_16/8_bit_text called with 3 args by
  the HUD text queue, in-game score, scoreboard row, teammate nameplate, world-relative text and the console
  overlay (text landed in the position slot; crash in the HUD text queue). OPEN: network_stats_overlay_draw has the
  same 3-arg draw call (networking debug overlay, skipped).
- NEW C: the five missing firing-position rule procs, trapped once the AI started choosing positions:
  rejection rows 0x4124c0, 0x412570 and scoring rows 0x411840, 0x411980, 0x411b60 (unlisted pointers 535 -> 530).

## 2026-09-27 play-test fixes (user present)
- Crashes fixed as the user reached them: contrail_age_points / flag_cloth_update dropped point_physics_tick's ESI
  velocity (every argument shifted); ui_widget_text_from_hud_objective (0x4a6770) was a missing UI data input
  (21 more entries of the table 0x00692b18 are still MISSING, mostly multiplayer/settings screens -- OPEN);
  decal_place called texture_cache_get(0) (now the real bitmap; decals are SKIPPED after the texture check until
  decal_place, 0.15, is rewritten -- OPEN); first_person_weapon_interface_tick_reset used Weapon +0 instead of the
  predicted resources at +0x4e4; first_person_weapon_update_animation_controls had animation/model swapped;
  first_person_weapon_update_lighting passed 0x7f7fffff as the lighting object; camera_observer_find_best_target
  called bsp3d_node_find_leaf with no arguments and read leaves through the scenario pointer.
- Friendly marines attacking the player, two causes, both FIXED: team_pair_override_add passed BL = 1 to
  team_pair_set (BL = 0 allies, 1 breaks) so (ai_allegiance player human) made the teams enemies, and
  team_pair_override_remove had the inverse; actor_init_prop_from_object wrote the tracked object's team to
  prop +0x16 instead of +0x12 (types/ai.h's object_type), so actor_target_relationship_think re-judged the player
  hostile every tick and alliance changes never reached the player prop.
- First-person gun drawn at the eye / off screen, FIXED: first_person_weapon_interface_initialize built both node
  match tables from (weapon tag, animation graph); the binary maps the animation graph (+0x478) onto the hands
  model (globals +0x180 -> +0xc) and the weapon first-person model (+0x468). Verified on the way: set_state (plus a
  blend test fix), update_state, update_active_state, the animation_controls pose/overlay/build chain,
  hud_meter_permute_node_records, hud_meter_find_matching_elements, animation_overlay_frame_orientations.
- core_save / core_load wired through main_loop (core.bin); launch with -console (scratchpad/cdb_play_console.sh).
- New checkers: scratchpad/callcount.py (call sites whose argument count differs from the definition; 279 sites,
  mostly networking -- OPEN queue for the static loop), scratchpad/alret_scan.py (byte-returning functions typed
  32-bit), scratchpad/protofilter.py.

## 2026-09-27 play-test diagnostics (TEMPORARY logging: src/interface/debug_play_diagnostics.c + a log line in team_pair_set)
- Marines still hostile after both fixes: the diagnostic shows the (player 1, human 2) override with active=1,
  status=0, no allied bits (+0xa4) and no secondary bits, and NO team_pair_set call this session. That is exactly
  the state the OLD build left (team_pair_override_add set active=1 then called team_pair_set(entry, 1, 0), which
  early-returns because active already equals 1). The user is playing from a checkpoint saved by an older build, so
  the saved game state carries the broken team table; ai_allegiance does not run again. Workaround for the user:
  console `ai_allegiance player human` after loading (the add finds the existing entry and re-runs team_pair_set
  with BL 0), or start a10 fresh. The code fix stands.
- First-person gun invisible after the node-table fix: the weapon is attached (weapon_hud=1, device_hud=1),
  state 0, animation 1 with the frame advancing, and node 0 sits at world coordinates near the player (e.g.
  -43.4 21.4 0.55). So the pose is built; the draw fails. OPEN: first_person_weapon_update_lighting (0.55) /
  render_model path.
- REMOVE the temporary logging (debug_play_diagnostics.c, its call in first_person_weapon_update and the
  standalone_log line in team_pair_set) once these are settled.

## 2026-09-27 static loop (no boots): register-argument and arity fixes
- AI/units: actor_build_order_grenade_or_melee failure path cleared order +7 instead of +0xe;
  unit_has_must_be_readied_weapon read weapon slots at object +0x4ec (binary +0x2f8). Verified at 0.85:
  ai_pursuit_note/check_object, squad_recent_object_get_or_create, actor_check_pain_reaction,
  actor_consider_target_candidate, try_give_grenade, player_apply_pickup_effect, physics_shape_add_surface/edge_proxy.
- game_engine_compute_local_player_look_vector (0x471f40) takes EAX = the OUT vector and forwards it in ESI to
  player_compute_view_forward_vector; the C dropped it (12-byte write through a stale pointer every time the HUD
  nameplate search ran).
- object_collect_local_player_relevant_objects (0x4fa1a0) rewritten: EDX point + stack (filter, context, max, out),
  collects objects in the point cluster's PVS; merged with the 0x4fa280 fragment (marked FRAGMENT).
  hud_find_nearby_teammate_for_nameplate rewritten; new hud_nameplate_candidate_filter (0x45e2e0, was a trap).
- select_players_to_display returned nothing (MSVC C4716); binary returns min(max_count, total).
- object_physics_check_impact_damage called object_collision_context_test_point(point) -- the binary passes EBX =
  context + stack point; test_point also skipped permutation 0xff, which the binary cannot (movzx byte vs 0xffff).
  Impact damage verified end to end (0.85).
- savegame_index_write_slot / append_slot never wrote the 0x206-byte entry (seek/write/close had no arguments; the
  entry parameter was missing / named unused) -- profile rename / create never updated the index.
- ui_build_level_select_list, ui_build_level_select_list_coop, player_profile_details_widget_refresh called
  game_state_read_checkpoint_summary / player_profile_scan_campaign_progress with no arguments (both write through
  register pointers: memory corruption on the campaign menu); the unlock test used 0x712f00 instead of the scan's
  last level; the co-op list lost the "next level" tests; profile details rewritten (missed the ninth widget).
- New checker scratchpad/livein.py: definitions with fewer C parameters than the binary's live-in registers
  (linear sweep to the first call). 9 left, all networking/startup/vehicle-hover.
- Skipped as networking: object_apply_shield_charge_and_notify, object_apply_linked_impulse (OPEN: its
  item_accelerate call passes 2 of 3), unit_spawn_with_starting_weapons.
- First-person gun (OPEN), static pass: the whole draw path matches the binary -- first_person_weapon_update_lighting
  (both render_model calls: EAX model, ECX permuted nodes, 11 stack args incl. flags 8), render_objects (0x6893ee = 1:
  FP drawn before objects), render_model's cutoff test (+0x8 vs pixels 0), chimera__rasterizer_set_frustum_z_func,
  render_camera_projection_zrange_push_pop_set, animation_graph_nodes_build_matrices' call (camera position /
  forward / up at 0x7c3114 / 0x7c3120 / 0x7c312c, bound correctly), rasterizer_set_shader_stage_config (stencil:
  mode 1 = FP writes 1, mode 2 = world only where 0; 0x6893ef = 1), D24S8 depth-stencil. The tick-time
  "camera=(0,0,0)" in the old DIAG is a red herring (the render camera is set inside render_window before the FP
  draw). Added TEMPORARY "DIAG fpdraw" lines (render_model, flags == 8, every 90th call): cutoff / early-out, LOD,
  node0, bounding centre, and per region the geometry and part shader types. Read them after the next normal play.

## 2026-09-27 (loop, static only) -- vanishing cryo tech: prime suspect FIXED (degenerate up vector)
- object_placement_data_initialize (0x4f53a0, STABLE but proven different) copied the POINTER bits at 0x696718 /
  0x696720 as the default forward / up (the binary loads the pointer and copies [ptr]..[ptr+8] = (1,0,0) /
  (0,0,1)); object_reset_velocity_and_wake had the same defect for the zero vector at 0x696714. Found by the new
  scratchpad/ptrform.py (same address declared as a pointer in some files and as an object in others).
- actor_place_new_unit (the ai_place path that creates the a10 crewmen e3f80001 / e3f90003) sets placement.forward
  from the yaw but never the up vector, and object_new copies placement->up into the object. So every AI-placed unit
  started with up ~ (9e-39, 9e-39, 9e-39): the root matrix (left = up x forward) collapses and the model is drawn
  degenerate -- invisible without being deleted, which is exactly what the traces showed. Moving bipeds presumably
  get up rebuilt by physics; the scripted, standing cryo techs would not. Fixed in commit a9a8bc2.
- OPEN (runtime, next user session): confirm the cryo tech stays visible. This also affects every other object
  created without an explicit orientation (weapons, items, garbage).
- Also this session: scratchpad/equcheck.py (extern address vs the linker's EQU; fixed actor_mode_definitions
  combat grade 0x65524c/0x655254, the (0,0,-1) vector in engagement reachability, camera_position_z),
  scratchpad/fnaddrcheck.py (9150 prototype address comments all agree with their definitions), scratchpad/livein.py
  and scratchpad/valptr.py. OPEN: the water draw procedure (0x7bf050 -> 0x535fd0 for ps_1_1) points into original
  code (no C), would fault on any map with water; a10 has none.
- First-person gun (OPEN): runtime log shows render_model called every frame for the FP model (lod 4, 8 parts: 2
  soso + schi), node matrices at the camera with orthonormal axes. HALO_FP_PLAIN (draw without the FP depth range)
  was inconclusive (the normal near plane clips it). Diagnostics now also log clip-space position and the effect
  type (DIAG fpclip) at the draw; read them after the next session.

## 2026-09-27 (loop, static only, user out) -- AI / weapon fixes from new checkers
New scratchpad tools: offprobe.py (compile-time struct offset checks: `offprobe.py ai.h actor.field=0x46c ...`,
comma-separated headers), constcheck.py (x87 constants a function uses that its C never mentions), bump.py (set a
file's rewrite confidence with a note). All checkers now split on a line-start `#if 0` (a comment mentioning
"#if 0" had truncated some files).
FIXED (each proven against objdump):
- actor_clear_target_state: the mode's target-cleared proc (+0x28: alert / guard) is tail-jumped with the actor on the
  stack; C called it with no argument.
- actor_replace_object_reference: tested/cleared secondary_action (+0x418) instead of active_movement.type (+0x46c),
  so dropping a reference wiped the queued secondary action; the mode replace-reference proc (+0x20: flee, guard,
  CONVERSE) takes (actor, old, new), C passed only the actor.
- object_find_nearest_squad_member: swarm unit_index (+0x18) and component_index (+0x58) swapped.
- actor_build_order_look: look duration missing the x30 seconds-to-ticks scale; direction never normalized in place.
- weapon_trigger_fire_or_reload: charge / overload timers missing x30 (1 s charge became 1 tick).
- ai_communication_record_line_played: speech stamp is the UNIT's +0x3f0 / +0x3fa (C wrote into ai_globals), tier
  slots and line timers use the stamp, second timer = ftol(delay*30 + stamp).
- encounter_squad_spawn_reinforcement: respawn timers (r*(max-min)+min)*30 from encounter +0x2c/+0x30 and squad
  +0x8c/+0x90 (C: r+30).
- unit_get_move_speed_for_range far branch out_b; unit_weapon_is_best_of_type returns 0 with no current weapon.
VERIFIED at 0.85: speaker selection chain (rate_speaker, select_in_reference, select_by_team), placement flags,
scripted action animation test, command status report, search-wait order, squad reinforcement processing, platoon
propagation, engagement range, ranged attack vector, weapon zoom magnification, player busy test, seat occupancy,
world-to-screen projection, actor type dispatchers 0x10 / 0x18.
OPEN: the actor mode table +0x1c slot (get_look_weights(actor, out)) is reached through a computed base; its caller
was not located. constcheck leftovers: hud_text_message_queue (0.08), console overlay, camera_debug (x30), vehicle
hover / wing flex (not in a10), decal_place (stopgap).

## 2026-09-27 (loop, static only) -- aim assist / target selection chain rewritten
The player's aim-assist target (game_engine_build_local_player_control_input -> camera_observer_get_target_angles
-> find_best_target -> generate_target_candidates -> cluster flood fill -> collect_target_candidates -> target_score
-> closest point on the target's pelvis-head segment) was a chain of low-confidence drafts. Fixed from objdump:
- camera_observer_get_target_angles (0x4596f0) REWRITTEN: camera type takes the slot (CX); the autoaim cone gets the
  player's desired_zoom_level (was 0, so "aim assist only when zoomed" weapons assisted unzoomed); the yaw / pitch
  rates use the ROOT-OBJECT VELOCITIES of the target and the player's unit (was camera-row positions).
- cluster_flood_fill_with_predicate (0x554e30) REWRITTEN: position / facing were never used and the 7-argument band
  test got 4 arguments; the caller dropped AX = start cluster.
- camera_observer_collect_target_candidates: team test is (object +0xb8 team, observer team); the draft used the
  controlling PLAYER's team (-1 for every AI) with the arguments swapped.
- camera_observer_target_score: EAX is a separate FACING vector (closest-point aux + angle dot product), the cone and
  reference are on the stack; the draft merged facing and cone.
- vector3d_closest_point_on_segment: two stack args; the nudge clamp is the target tag's autoaim width, delivered
  through unit_get_look_origin_and_direction's second argument (callers had passed 0.0 / a cone distance).
- Also this batch: object_physics_mass_point_update_orientation rotates the SOURCE vectors and renormalizes forward
  (OPEN: its caller object_physics_integrate_and_test_at_rest, 0.3, writes the rotated orientation straight into the
  mass point while the binary rotates into locals); unit_get_move_speed_for_range far branch.
VERIFIED at 0.85: look origin / direction, target direction, autoaim cone, best-target search, falloff, comparator,
clamp length, spring clamp, grenade decision chain (can_throw, facing commit, trace, ally candidate), weapon pickup
reachability, ranged attack vector, platoon propagation, engagement range, squad reinforcement processing.
## static loop: weapon fire trigger + effect player (2026-09-27)
- weapon_fire_trigger (0x4c3f10) 0.3 -> 0.85. Fixes vs objdump:
  - The FIRING effect was never played: the final call passed the damage tag and the misfire chance. It now passes
    (EDI = firing-effect effect tag +0x30+0x10*variant, stack firing_rate or 1.0 when empty, heat/overheated_threshold).
  - The chamber-pending check now runs whenever rounds remain after the shot, not only with infinite ammo.
  - The camo depower test uses current_game_engine (0x6f1d20), not network_game_mode.
  - Ejection port recovery requires trigger flag byte0 bit 0x80 clear.
  - Holder self-damage: dd random_blend / multiplier = 1.0 (the draft had 0, so no damage).
- weapon_play_trigger_tag_effect (0x4c47d0) verified -> 0.85; the signature now takes (real scale_a, real scale_b); externs updated.
  All six callers' EDI tag fields checked (ready/reloading/chambering/overheat_detonation).
- Relinked: unresolved 1, traps 127. hooks.txt == stable (only CRLF differs).
- weapon fp animation lengths: weapon_get_first_person_animation_time (0x4c2f80) verified -> 0.85. Callers passed a
  placeholder CX: weapon_ready now passes 0xa (was 0 -> the ready cooldown used the wrong animation's frame count),
  weapon_trigger_begin_reload passes 7 (was the magazine index -> reload length wrong). weapon_ready and
  weapon_trigger_begin_reload verified -> 0.85. Relinked: unresolved 1, traps 127.
## static loop: the decal pipeline (2026-09-27)
decal_place (0x44edc0, 6111 bytes) was a 0.15 skeleton with a STOPGAP that returned before any geometry, so no
bullet holes, blood or scorch decals ever appeared. The whole chain is rewritten from objdump and set to 0.85:
- decal_new (0x44dd90) now takes EAX requested_handle, the geometry cache block handle; the decal datum shares its
  index and salt. The draft passed -1, which always fails. The eviction scan now restarts the iterator when it runs
  dry (up to 100 times). The insert-before `self->previous = self` quirk is preserved.
- decal_build_projection (0x44e460): only the |i| == |j| major-axis tie-break was wrong.
- decal_flood_surfaces (0x44e730): the surface angle is measured against the decal normal (+0x44), not the box (+0x34).
- 0x44db30 (named structure_lightmap_uv_rect_build, really the DECAL SPRITE rect): two outputs, EDX = the raw
  sprite rectangle and the stack pointer = the world extent box. It is typed against Decal / Bitmap now.
- decal_place: tangent basis (random / no_random_rotation / sapien_snap_to_axis), sequence and radius rolls, sprite
  box, texture preload, flood, fold-over-edge wrapping of steep fallback surfaces, 1/256 lift, uv bytes, 6-vertex fan
  blocks into the vertex cache (64 bytes/block allocated, locked x1.5), record fields, Unlock (vtable +0x30),
  next_decal_in_chain with geometry inheritance.
- Callers' externs updated (decal_spawn_for_response verified 0.85; structure_decals_update_switch_transitions passes -1).
- OPEN (runtime): decals are now live. If shooting walls crashes, set a breakpoint in decal_place / the lock.
Relinked: unresolved 1, traps 127.
- Decal support functions (now live) verified -> 0.85: decal_link, decal_delete, decal_evict_object_decals,
  decal_rehash_object_decals; decal_update_fade (fade byte = fistp, not +0.5 truncation; expiry !(age < lifetime)),
  decals_update_fade and decal_clear_flags (iterator index starts at -1), rasterizer_decal_pass_begin (0x6893e4 WORD
  compare), rasterizer_decals_draw_cluster (1/255 is a double), decal_and_font_system_reset (warning fixed).
  Relinked: unresolved 1, traps 127.
- cache_allocate_block (0x4d1840) rewritten cleanly from objdump -> 0.85 (ring of gap windows, LRU, overlap eviction, link); cache_evict_entry verified -> 0.85. Both now carry every decal allocation. Relinked OK.
- hud_text_message_queue_update_and_draw (0x4a3e30, scrolling text widget) rewritten -> 0.85: new lines are queued at the running bottom (EBX), not at y = string index; elapsed = (uint32)dms * 0.08. hud_text_message_queue_add verified -> 0.85.
## static loop: a10-coverage pass (build/_called.txt x confidence; scratchpad/a10low.py)
- 413 functions that ran in the recorded a10 session are below 0.8; working lowest-first (skipping networking).
- game_engine_digitize_control_input, real_seek_toward_clamped (exact NaN orderings), unit_choose_dialogue_variant,
  unit_pick_random_dialogue_variant, unit_update_ik_detail_nodes verified -> 0.85.
- object_solve_two_bone_ik_to_marker FIXED -> 0.85: the IK target is marker_b.node_transform (+0x38, world) *
  inverse(marker_a.transform), in place; the draft used marker_b.transform (+0x04), so every unit's hand/parent IK
  aimed at a node-relative matrix.
- 2026-09-28 play-test fixes: death never reverted -- the per-tick players update (0x4749a0, misnamed main_switch_structure_bsp) tested player_globals +0x16 instead of +0x10 no_player_has_a_unit, so lost_map was never set; fade-stage counter moved after the loop; 0x71973c is a byte. unit_drop_inventory_weapons tested/deleted the cleared slot (-1) -> crash on death. decal_clip_buffers had no address (the long-standing 1 unresolved; crash in the decal flood). Decals now render in game.
- FP gun: draws succeed (hr 0, sane z/stencil/cull/cw); root 4-36 deg off the view forward; alpha test on (ref 127 GREATER) -- TEMPORARY experiment forces it off for FP draws.
- STOPGAP/OPEN: water draw procedures 0x535fd0 (pixel shader, ~3 KB) and 0x5358b0 (fixed function) have no C; the original pointer crashed in render_sky. rasterizer_select_hardware_codepaths now installs a logging no-op. TODO: rewrite 0x535fd0 and remove the stopgap.
- 2026-09-28 play-test: marines confirmed allied (allied(p,2)=1). actor_type_mounted_weapon_update (0x4263f0) written (trap). Vehicle-physics crash: object_physics_compute_mass_point_forces wrote a real_matrix4x3 into float[9] (stack overflow) and read it one float early -- fixed. FP gun: alpha-test state matches the binary; the gun shows with alpha test forced off (TEMPORARY), so the low output alpha comes from the pixel shader side (constants match except c4). OPEN.
## 2026-09-28 user play-test reports (a30): grunts miss, Banshee flip does nothing
- Grunts "mostly miss, sometimes hit": FIXED a real aim bug -- projectile_solve_straight_line (0x4bee20) returned the
  raw target-origin delta; the binary returns the NORMALIZED vector (actor +0x63c / +0x68c firing direction, used as a
  unit vector). projectile_get_aiming_vector verified. actor_compute_accuracy_scale is misnamed (a movement arrival
  radius) -- verified. The user was also on Easy, which legitimately lowers AI accuracy. OPEN (runtime): re-check.
- Banshee flip: the whole chain is verified against the binary with no difference found:
  player_update_nearby_interactions_primary (type table), player_check_vehicle_interaction (0.9, pending type 0xb),
  player_set_pending_interaction_action, game_engine_players_update_server (action bit 0x40 -> 0x4793a0),
  player_execute_pending_interaction case 11 (+0x4cc |= 0x10, +0x4d1 direction, +0x4d2 = 0), vehicle_update's flip
  block (0x571105..0x5712cc, all constants). OPEN (runtime): does the HUD offer the flip prompt? If yes, the spin is set
  but something later cancels it (object_physics_tick / mass point forces on the Banshee, or the at-rest sleep); if no,
  check the vitality_flags / +0x324 gates in player_check_vehicle_interaction.
- Also written: hs_evaluate_vehicle_hover (trapped in a30), three nav-point evaluators; scratchpad/hsmissing.py shows
  the campaign's scripts need only 'inspect' and 'play_update_history' beyond what exists.
## 2026-09-28 (loop, static only) -- a10-coverage pass continued
- FIXED: actor_recompute_grenade_eligibility (0x42f260) rewritten -- the recheck delay is lerp(actor tag +0x400..+0x404
  when eligible, else +0x3f8..+0x3fc) * 30 + the unit's +0x3fa ticks (when its +0x388 > 0); the draft's formula was a
  placeholder.
- FIXED: prop +0x122 is a signed byte (all ten ordered compares in the binary are jg/jle); types/ai.h changed, which
  also corrects actor_target_relationship_think and actor_target_update_tracking_speed.
- CORRECTED header: segment3d_within_radius_of_segment takes a_start on the stack (not EAX).
- VERIFIED -> 0.85: physics_shape_pill_test_ray (biped capsule: rules out bullets passing through the player),
  player_compute_view_forward_vector, structure_leaf_faces_for_each, path_find_push_start_node,
  object_get_root_parent_placement, hud_update_dispatch, actor_scan_backup_and_panic_reaction and ~20 small AI helpers
  (burst, target engagement, flank grading, grenade throw / trajectory, mode transition loop, orders, comms, search).

## 2026-09-28 static loop (cont.) -- commits ea0c3eb..e3e8f06, relinked (0 unresolved, 127 traps)
- vehicle_calculate_animation_controls REWRITTEN: selectors 0x20/0x21/0x22 (slide, ground lean, ground contact)
  are clamped to [0,1] in the original; the draft stored them raw (negative lean leaked into vehicle anims).
- object_lights_gather_nearest: own-object exclusion reads the LIGHT tag flags byte +0 bit 2, not Object.flags.
- detail_objects_update_render_list: search key z seeded with the camera cell (was an uninitialized local).
- ai_communication_rate_player_proximity: unknown cluster skips only the PVS test, the trace still runs.
- BUG CLASS (fixed, audited tree-wide with scratchpad/unitcast.py + extcast.py): unit_data/weapon_data field
  offsets are absolute (struct starts at object +0x1f4 / +0x22c) but 11 sites cast the object pointer itself:
  player_weapon_locality_for_object, ai_communication_rate_player_proximity (aim vector), 
  player_check_assassination_opportunity (device interaction aim vector read object +0x48!),
  player_kill_streak_begin/continue/set_max/tick + player_add_kill_streak (ORed 0x10 into OBJECT flags +0x10,
  wrote +0x22e), game_engine_apply_player_spawn_loadout_message, game_engine_send_unit_weapon_loadout,
  player_spawn_starting_profile_weapon (magazine counts at +0x8a instead of +0x2b6).
- verified -> 0.85: unit_is_seat_control_available, any_local_player_within_10_units, player_profile_get_flag_by_id,
  actor_target_has_conflicting_neighbor, actor_update_awareness_level, actor_update_idle_stagger,
  actor_update_target_combat_status, device_compute_function_values, encounter_evaluate_platoon_condition,
  object_sum_attached_light_luminance, object_collision_context_build.
- OPEN (runtime, user): banshee flip prompt; FP gun alpha; cryo tech; grunt accuracy. Kill-streak object-flag
  corruption fix may matter if those helpers run in campaign (check a10 coverage).

## 2026-09-28 banshee flip (user: pressing E doesn't flip it) -- static, commit 2649a5b, relinked (0 unresolved, 127 traps)
- Chain verified against the binary: player_execute_pending_interaction case 11 (vehicle +0x4cc |= 0x10, +0x4d1 dir,
  +0x4d2 = 0; no wake call in the original either) -> vehicle_update 0x571105..0x5712d3 flip block (angular velocity
  = axis * clamp(-2 up.k, tag +0x340..+0x344) * +-0.3 for 30 ticks, clears at-rest 0x20) -> type 5 (banshee) ->
  0x573ee0 dispatch -> 0x573f60 hover stabiliser (radius -1) -> object_physics_tick general path.
  Banshee physics (scratchpad/bansheephys.py): radius -1, mass 4000, 22 mass points, 2 powered.
- object_physics_compute_mass_point_forces (0.3 -> 0.85, full decode) had FOUR real bugs:
  torque.j = (offset x force).j NEGATED (every vehicle's per-mass-point torque about world y inverted; also in
  object_physics_tick_single_pass); ground normal force used the mass point's mass instead of Physics.mass;
  material friction scale gate inverted (binary: applies when Physics.mass <= 7500 -- banshee/warthog yes,
  scorpion/pelican no); leaf lookup passed 0x746f98 instead of the collision BSP 0x746f90 (wrong leaf/cluster ->
  wrong water depth). Plus k,j,i sum orders.
- VERIFIED: object_physics_context_build, object_get_node_local_transform, object_update (dispatch path),
  vehicle_calculate_mounted_controls_dispatch call roles.
- OPEN (runtime, user): banshee flip; general vehicle handling should now be closer to retail (torque sign).

## retail-independence loop
### iteration 1 (2026-09-28) -- commits e2e418f..b110bac, relinked (0 unresolved, 127 direct traps)
- New tool: scripts/DecompileAt.java + scratchpad/ghidra_decompile_at.sh (headless, read-only) decompiles functions
  Ghidra never created (table-only callbacks). All 622 missing addresses -> scratchpad/missing_decomp.c (0 failures).
  Use it as the skeleton, then recover register args / x87 orders from the disassembly.
- Written: 8 AI dialogue conditions (0x42f4f0..0x42f7f0), actor type updates hunter/sentinel/engineer/flood/
  flood carrier/infection (b30+ blockers), infection swarm update 0x424c20 (4 KB).
- Missing functions: 639 -> 624 (stored pointers 512 -> 497; AI now 0).
- NEXT: breakable-surface shatter 0x500090 (decompiled skeleton in scratchpad/bsce_decomp.c, asm in
  scratchpad/bsce.asm; CRT floor 0x623e40 / ceil 0x6267f0), water 0x5358b0/0x535fd0, rasterizer LABs, then UI.
- Build-time independence: not started. Run-time independence: not started.
### iteration 2 (2026-09-28) -- commits 0d78ed9..4d99983, relinked (0 unresolved, 125 direct traps)
- breakable_surface_shatter (0x500090, 4.8 KB): glass shatter particles + sound. Its callers declared it as
  physics_point_spawn_contact_effect with the address on a continuation line (never bound) -> renamed and bound.
- Water: rasterizer_water_draw_pixel_shader (0x535fd0), rasterizer_water_update_ripple_texture (0x534f80),
  rasterizer_water_draw_fixed_function (0x5358b0); the water stopgap in rasterizer_select_hardware_codepaths is
  GONE. Traps 127 -> 125.
- SMOKE: plain standalone_boot dies on the known Keystone.dll thread AV (only cdb swallows it) -> use
  scratchpad/cdb_trace.sh for unattended boots. a10 under cdb for 120 s: no trap/exception, but the player unit
  never appeared (still in the opening), so the new water code was not exercised. OPEN (runtime): water in play.
- NEXT: rasterizer LAB_0051e570/0051e8f0/0051fad0/0051fd80 + FUN_005276c0/00527ae0, units callbacks, then UI.
### iteration 3 (2026-09-28) -- commits ..a726eda, 37fe76b, relinked (0 unresolved, 119 direct traps)
- Rasterizer: environment lightmap single/two stream (0x51e8f0/0x51e570), self-illumination two/single stream
  (0x51fad0/0x51fd80), environment draw single stream / fixed function (0x5276c0/0x527ae0); wired into
  select_hardware_codepaths / select_draw_functions. Rasterizer traps now 0. Direct traps 125 -> 119.
- gen_standalone.py: NOT_CODE_POINTER_RANGES [(0x679600, 0x67d000)] -- a data table there produced a false
  code-pointer (commit 115260f).
- Units: biped/vehicle is_old_enough (0x55b780/0x572a30), biped/vehicle network_baseline_take (0x55b3d0/0x572410).
  DEFERRED to networking: vehicle 0x571f20 (send creation) and 0x5726e0 (apply update).
- Interface: 42 small ui_event handlers (ui_event_function_table 0x6927d0) + 6 game_data_input callbacks
  (0x692b18: 0x4a4c70/0x4a6e50/0x4a7300/0x4a7340/0x4a7350/0x4a73d0), all < 64 bytes, from objdump
  (scratchpad/dump_group.py MODULE MAXSIZE [MINSIZE], scratchpad/gen_ui_small.py). Interface stored pointers
  142 -> 94. Note: 0x4a10f0 passes a zero-extended word to network_client_rejoin_check, whose C takes int8_t.
- Stored pointers without C: hs 142, interface 94, game 90, networking 86, shell 28, units 2, main 1, memory 1.
- NEXT: interface handlers 64..160 bytes (46), then the larger ones (50), then the hs parse procs.
- Build-time independence: not started. Run-time independence: not started.
- (iteration 3, continued) commits 922df93, 99900dc: 43 mid-size (64..160 byte) interface handlers + the list row
  formatter ui_list_default_item_format (0x4a8310), which resolved two unbound '?' names. Direct traps 119 -> 117
  (interface direct traps 0). Interface stored pointers 94 -> 51. Tools: scratchpad/who.py ADDR... (C definition
  or extern for an address), scratchpad/uigen_lib.py (shared generator for ui_event / game_data_input files).
- OPEN (networking phase): network_staged_message_commit (0x4da250) C ignores the AX word that 0x4a1570/0x4a15e0
  pass (declared there as a second argument). 0x4a44f0 (checkpoint list open) needs its row callback 0x4a4280
  (0x266 bytes, GetDateFormatA/GetTimeFormatA, only referenced from 0x4a44f0) -- do both together.
- NEXT: the 50 large interface handlers (>= 160 bytes, incl. 0x4a44f0 + 0x4a4280), then hs parse procs.
### iteration 4 (2026-09-28) -- commits ..c5e923d.., relinked (0 unresolved, 117 direct traps)
- 33 large (165..471 byte) interface handlers written from objdump via scratchpad/gen_ui_big{A..F}.py (shared
  scratchpad/uigen_lib.py: first_list_child, list_item_id, PROFILE/VARIANT selectors, load_ext). Interface stored
  pointers 51 -> 18. Findings: both ui_event dispatchers (0x497c9a, 0x49a4be) load ECX with the table index, so a
  handler returning the entry ECX's high byte (0x4a2f10) always returns 0; 0x4a3b70 is game_data_input[58].
- Remaining interface: 0x49d8b0 0x49e300 0x49e5d0 0x49e7e0 0x49ea50 0x49edc0 0x49f030 0x49f680 0x49fad0 0x49fd30
  0x4a02a0 0x4a1dc0 0x4a3540 0x4a44f0(+0x4a4280) 0x4a5740 0x4a6fa0 0x4a7880 0x4bb360 (477..906 bytes).
- (iteration 4, continued) commits ..HEAD: the last 18 interface handlers (477..906 bytes) incl. the checkpoint list
  (0x4a44f0 + its hidden row callback checkpoint_list_add_row 0x4a4280), the variant option view/commit pairs, the
  video settings commit, the scrolling list row click (0x4a3540) and the MP lobby update (0x4a5740).
  MILESTONE: interface stored pointers 0 (from 142). Direct traps still 117.
  Notes: 0x4a5740 writes L"" over the .rdata literal L"?" at 0x66a80c (would fault in retail); it indexes a stack
  array by the local player byte (only player 0 modelled). 0x4a44f0's callback was invisible to missing_by_module
  because its only reference is an immediate inside a function with no C -- other such hidden callbacks may exist.
- Remaining stored pointers: hs 142, game 90, networking 86, shell 28, units 2, main 1, memory 1.
- NEXT: hs parse procs (0x487530, 0x484ea0, 0x484600, 0x484db0, 0x484b40, 0x485280, 0x485310, 0x4853c0, 0x4854f0),
  then hs evaluators.
- (iteration 4, continued) hs: the 9 parse procs (begin, cond, and/or, + - * / min max, sleep, sleep_until, wake,
  unit, the rcon/sv_* string family) and all 132 missing evaluate handlers (scratchpad/hsgen_lib.py +
  gen_hs_eval1/2.py; the three orphan records ai_attach_units / ai_detach_units / ai_magically_see_units are not in
  hs_function_definitions but still had stored evaluate pointers). hs stored pointers 142 -> 1 (0x48b150, the
  scenario script syntax byte-swap callback -- needs 0x4cfd50 too; cache work).
  FIX: ai_object_list_initialize_shield_stun_thresholds (0x561ab0) takes its two overrides by value (objdump
  0x561b05..0x561b3c), not as pointers.
  Tools: scratchpad/annot.py (annotates a dump with repo names/signatures and hs record signatures),
  scratchpad/condense.py, scratchpad/hsprocs.py.
- Stored pointers now: game 90, networking 86, shell 28, units 2, main 1, hs 1, memory 1. Direct traps 117.
- NEXT: shell / cseries / saved_games (shell 28 stored + 26 direct, cseries 7, saved_games 2), then the '?' names.
- (iteration 4, continued) shell/saved_games/cseries round, commits ..HEAD. Direct traps 117 -> 95:
  * 17 config.txt presence-flag setters (0x57d120..0x57d220).
  * hwreq C++ helpers: tree_splice_insert (0x57c390, std::_Tree::_Insert) and tree_erase_one (0x57c820, erase),
    hwreq_map_find, hwreq_property_set_map_index (operator[]), hwreq_property_set_apply, std exception what() and
    the out_of_range / length_error copy constructors (__fastcall = the CRT's thiscall; gen_standalone_link now
    binds __fastcall rewrites as @name@N), CRT std::exception copy-ctor / dtor (0x627dd2 / 0x627e1c).
    FIXES: tree_splice_insert also takes the tree (EDI) and parent (ECX) -- tree_hint_insert_unique /
    tree_insert_unique pass them now, and the hinted insert's predecessor case tests predecessor->right (was
    hint->left); tree_insert_unique's parent is the undecremented descent node.
  * gen_standalone: DIDATAFORMAT slot 0x64dff0 is data (DirectInput object formats in .text 0x613400); unsplit
    table entries inherit a library module (D3DX vtable entry 0x5c0ba0, zlib deflate 0x4d2870, 3 CRT labels);
    DEAD_CODE_POINTER_SLOTS lists the six catch(...) funclets of the retail hwreq C++ frames.
  * gen_standalone_link: only objects whose source exists are linked (12 stale objects -> nine console_window_*
    '?' traps gone).
  * saved_games path_split_components (0x556000) / path_build_full (0x5560d0); hs_rebuild_source rewritten with the
    file helpers' register arguments (it called them with none) and its comparator bound
    (file_reference_compare_full_path, was the unbound file_reference_compare_by_name).
  * objects_dump_memory opened its file with mode 0 instead of "a+b" (fixed); waypoint isnan = (x != x);
    bitmap_data_block delete proc (0x43f010); last three hs evaluators (unit_kill_silent, unit_set_emotion_animation,
    vehicle_driver). ai_object_list_initialize_shield_stun_thresholds takes floats by value (fixed).
  OPEN: hwreq_length_error_destruct / hwreq_out_of_range_destruct (ThrowInfo unwind procs) are cdecl but the CRT
    would call them thiscall (only on a thrown exception, which is fatal in the C build anyway).
  OPEN: '?' names left are networking C with lost register arguments (item_add_ammunition in
    network_game_action_apply, network_df0e0_broadcast, network_player_entry_is_valid, network_player_table_remove,
    sig__setup_master_server_connection_sig) -- networking phase. GameSpy SDK (0x614000..0x61e000) traps also go
    there (they were counted as shell/cseries).
- Stored pointers now: game 87 (the five MP engines' callbacks, mapped by scratchpad/engine_slots.py), networking 83,
  units 2, hs 1 (0x48b150). Direct traps 95.
- NEXT: the 87 MP game engine callbacks (ctf, king, oddball, race, slayer; cdecl, signatures from the slot callers).
### iteration 5 (2026-09-28) -- commits 764710d..HEAD, relinked (0 unresolved, 95 direct traps)
- Multiplayer game engine callbacks: 60 of 87 written (scratchpad/engine_lib.py + gen_engine1..3.py; slots mapped by
  scratchpad/engine_slots.py; all cdecl since they are only called through game_engine_definition slots): scores,
  team scores, player/team/header score texts, round resets, new-life / round-reset hooks, GameSpy query-report hooks
  (they call GameSpy 0x616640 / 0x615590, already counted traps -- networking phase), King reset/new life, Oddball
  init, Race is_winner / get_score / changed object / tick hook.
- FIX: game_time_format_minutes_seconds (0x466530) passes the character count in EDX to every bounded wide format
  (0x40 for the parts, its second argument -- 0x100 at every caller -- for the result) and divides signed; the old C
  dropped the count. New: its ASCII twin 0x466600 (game_time_format_minutes_seconds_ascii).
- OPEN: game_engine_queue_multiplayer_sound's C models one argument (the sound); the binary also takes the target
  player (EDI) and a broadcast flag (stack). The new callers pass the true sound index.
- Stored pointers: networking 83, game 27, units 2, hs 1. Direct traps 95.
- NEXT: the 27 large engine callbacks (0x4684a0 886, 0x469300 1079, 0x46cac0 / 0x46e480 message-text switches, ...;
  race allow_grenade_counts 0x46e980 needs 0x46d6f0 too).
### iteration 6 (2026-09-28) -- commits dbd9214..c396eba, relinked (0 unresolved, 95 direct traps)
- Multiplayer game engine callbacks: the remaining 27 written (gen_engine4..14.py) -- every engine slot now has C:
  inits, reset_objects, updates, unknown_48/68/70 hooks, the five profile_post_update decoders (shared end-of-message
  seek as a static), profiles_updated encoders, the five build_message_text overrides, race vehicle spawn
  (0x46e980 + 0x46d6f0 + tag picker 0x46d5d0 as statics), oddball carrier helpers (0x46c8e0/0x46c910/0x46cef0).
- FIXED (callers passed wrong/made-up arguments, each checked against every binary call site):
  * game_engine_queue_multiplayer_sound (0x46be40) takes ESI sound, EDI player, stack broadcast: 22 C callers mostly
    passed the broadcast flag as the sound; all now pass the binary's values (kill-feed arms 0x0e..0x12 get
    0x10/0x0f/0x0e/0x11/0x12; the 7/9..0x0c group queues none).
  * game_engine_send_end_game_notification (EAX reason): callers pass 1 / 3 / 2 (they passed none).
  * on_player_death calls slot +0x68 with (killer, death_object, victim, is_suicide) (it passed none).
  * ctf reset_team_return_credit (0x468840), player_drop_flag (0x4688b0), relocate_object_hill (0x46c1a0): EBX/EDI
    are loaded inside (the object and its stand / found position), not forwarded; flag tick's missing 0x469080 call
    added; player flag tick resets the flag, not the player.
  * kill feed by relationship (0x460c10) forwards BL and the recipient; kill feed to team (0x460ba0) is ESI message,
    BL broadcast, stack team; notify_both_teams (0x468460) takes only EAX; touch_flag commits the cache with EBX = 1.
  * relocate_hill_marker (0x46bfe0) takes ESI = ball index (owner team + marker type filter); callers pass i.
  * place text 0x4633f0 takes the rank from game_engine_compare_score_to_others (it returned string 0).
- Stored pointers without C: networking 83, units 2 (0x571f20, 0x5726e0), hs 1 (0x48b150); game 0.
  Direct traps 95 (networking 61, shell 22 + cseries 7 = GameSpy SDK and friends).
- NEXT: networking (83 stored pointers, 61 direct traps, the '?' names, GameSpy 0x614000..0x61e000), then units
  0x571f20 / 0x5726e0 and hs 0x48b150; then step 2 (build-time independence).
### iteration 6b (2026-09-28) -- commits c54a87b..8bfca59, relinked (0 unresolved, traps 95 -> 93)
- hs: syntax data byte-swap proc 0x48b150 (tag data definition "hs_syntax_data_definition"; 0x4cfd50 inlined) -- hs
  stored pointers 0.
- Traps: the CRT _CIpow (0x6283c0) and wcslen (0x625b7a) traps are gone. Three callers called _CIpow with no
  operands (ally bonus pow(1 - (d - 1) * 0.2, 0.6); nameplate scale pow(min(n, 10) * 0.1, 1.9) * 0.5; nameplate fade
  pow(opacity, 1.9)); game_engine_rasterize_in_game_score (0x465690) takes (player, opacity) on the stack (the C had
  the player in EAX and the text alpha from a third argument passed as 0). The server browser row's wcslen was the
  inlined copy's unused length; its bounded formats take EDX = 0x1f / 7 (the C passed the text as the count).
- Left: networking 83 stored pointers + units 2 (0x571f20 / 0x5726e0, network object-create message builders);
  direct traps 93 = GameSpy SDK (0x614540..0x61e550 and thunks 0x61c3e0.. / 0x622050), server browser comparators
  0x4b6cd0 / 0x4b6e70 / 0x4b6fb0 (they call GameSpy SB getters 0x617490 / 0x617c10), autopatch callbacks
  0x576a70 / 0x5777d0, network_channel_gap_* (0x441020..0x441f30, 0x4ba660), the five '?' names.
- NEXT: networking -- start with the GameSpy SDK leaf functions (query/report and server browser getters), then the
  comparators, autopatch callbacks and channel gaps.
### iteration 7 (2026-09-28) -- commits 226c8ce..aa49239, relinked (0 unresolved, traps 93 -> 61)
- New module src/gamespy (header gamespy.h, generator scratchpad/gs_lib.py): the GameSpy SDK linked into halo.exe
  (0x614540..0x623142). Its call closure from the traps is ~390 functions / 60 KB (scratchpad/closure.py stops at the
  CRT, 0x623142); written bottom-up so no new traps appear. Done so far (95 functions): darray, hashtable, nonport
  socket helpers, GT2 accessors + gt2AddressToString / gt2StringToAddress, serverbrowsing server accessors, key
  tables, ref strings (__declspec(thread) table, as the SDK's TLS slot), key parsers, SBServerList basics, qr2
  buffers. The SDK's Winsock (WS2_32/WSOCK32 delay imports) and CRT calls go to ws2_32.lib / libcmt by name.
- LINK: gen_standalone_link binds an unresolved name whose address is a rewritten cdecl C function to it
  ("rewritten function by address"), so callers still declaring FUN_006147a0 reach gt2GetConnectionState. Every
  newly bound caller's arity was checked against the binary call sites.
- FIXED callers: network_listen_start passes the queue to gt2SetSocketData (0x442179 pushes ESI); the server browser
  player list calls SBServerGetPlayerStringValue as (server, index, key, default) (0x4b7450); network_session_host_start
  registers the natneg callback 0x578160 (it passed NULL) -- written with its completion callback 0x578120.
- Also written: the server browser comparators 0x4b6cd0 / 0x4b6e70 / 0x4b6fb0.
- NOTE: the linker folded identical GameSpy functions (ICF): 0x6175f0 is ArrayLength and also the SBServer public-ip /
  GT2Socket SOCKET getter; 0x6147d0 is gt2GetRemotePort and the raw SBServer port getter.
- NEXT: natneg (NNBeginNegotiationWithSocket 0x614f30 and its engine), qr2 core (0x616340 qr2_init..), SB query
  engine / serverbrowsing (0x61e5b0..0x6202b0), GT2 core (0x619b90 think, 0x61c3e0..), gcdkey (0x61aa50), ghttp
  (0x620520..0x623110), then game-side network_channel_gap_*, autopatch callbacks, the '?' names.
### iteration 8 (2026-09-28) -- commits f7ef7c6..7b2e3ff, relinked (0 unresolved, traps 61 -> 41)
- gamespy: qr2_buffer_add_int, gt2Listen, gt2SetReceiveDump, gcd_getkeyhash, gcd_compute_response (MD5 already in
  cseries), ServerBrowserState/Count/GetServer/public ip, ghttpSetProxy; the whole gcdkey server (16 functions).
- networking callbacks written: 0x441f30 (GT2 connection callback), 0x5777d0 (autopatch check result), 0x4ba660
  (server browser list callback), 0x5760a0 (CD key result).
- REWRITTEN (register-lost end to end, found while wiring gcd_authenticate_user): the host join path -- the message
  dispatcher's join cases, join request handler 0x4e21d0 (reason codes 0/6/1/2/3 and the accept packet), join
  confirm 0x4e2400, network_join_request_reset_state 0x4e0ab0 (really: the CD key check of a joining machine),
  0x575ff0 (really: gcd_authenticate_user + ban check), network_server_notify_or_resend_challenge 0x4e0af0 (its stack
  server was dropped; payload = the reason).
- OPEN (important): src/networking has many C-to-C calls with lost or invented register arguments (its README even
  accepts "signature disagreements"); e.g. network_game_process_incoming_message still calls most handlers with no
  arguments. Multiplayer needs a networking call audit (every extern's arity/order vs the callee's definition and the
  binary call site) -- after the traps are gone.
- NEXT: natneg (0x614ab0..0x615360), qr2 core (0x615600..0x616340), SB query engine, GT2 core, ghttp/autopatch
  (0x61bd00..0x61c260, 0x620520..0x623110), then 0x441020/0x441040/0x441060/0x4410b0/0x441200 and the '?' names.
### iteration 9 (2026-09-28) -- commits e177fe7..HEAD, relinked (0 unresolved, traps 41 -> 34)
- gamespy: NAT negotiation (18 functions: negotiator list, INIT/PING/CONNECT_ACK, NNBeginNegotiationWithSocket,
  NNCancel, packet handlers, NNProcessData, per-negotiator think, getlocalhost, IsPrivateIP); qr2 query handling
  (15: B64Encode, GameSpy RC4, challenge response, new/old query replies, parse, client messages, qr2_parse_queryA,
  keep-alive, qr2_init_socketA, socket drain).
- networking: GT2 dump callbacks 0x441020 / 0x441040, the query socket's unrecognized-message callback 0x441200.
- FOUND: network_session_host_start calls qr2_init_socketA with NULL for five callbacks the binary passes
  (0x5779c0 server key, 0x577f40 team key, 0x577fb0 key list, 0x5780c0 count, 0x578100 add error) -- none has C
  (so they never counted as traps); 0x577e40 (the player key callback, C name network_session_host_dispatch_message)
  models 3 of its 4 arguments. NEXT: write those five, fix host_start and 0x577e40; then qr2 heartbeat/think
  (0x616680..0x616ff0), SB query engine, GT2 core, ghttp/autopatch, 0x441060/0x4410b0, the '?' names.
### iteration 10 (2026-09-28) -- commits 5cab844..ec67403, relinked (0 unresolved, traps 34 -> 29)
- networking: the host's six qr2 callbacks from the binary (server key 0x5779c0 with the Halo key set, player key
  0x577e40 rewritten with 4 args, team key 0x577f40, key list 0x577fb0, count 0x5780c0, add error 0x578100);
  network_session_host_start rewritten to pass them (it passed NULL for five). Game socket query callback 0x4410b0.
- gamespy: qr2 heartbeat / scheduler / state change / shutdown / think; gt2CreateSocket with its connection table
  callbacks, gti2FreeConnection, gti2FreeSocket.
- Left (29): GT2 core (think 0x614540, connect 0x6145a0, send 0x6146b0, close 0x614710, close socket 0x614860,
  accept/reject 0x61ce80/0x61cee0 and their graph 0x618xxx..0x61dxxx), ServerBrowser API + SB query engine
  (0x616eb0..0x617290, 0x61e5b0..0x6202b0), ghttp/autopatch (0x61bd00..0x61c260, 0x620520..0x623110, 0x576a70),
  0x441060 (needs gt2CloseAllConnections), and the five '?' names.
### iteration 11 (2026-09-28) -- commits b3e36a2..f673f35, relinked (0 unresolved, traps 29 -> 8)
- gamespy: the GT2 transport (~90 functions, src/gamespy/gt2.h): Halo's GT2 is modified -- the handshake swaps
  Diffie-Hellman style keys (16-byte big numbers, generator "3", modulus "10001", random 16-hex-digit private
  exponent; gt2_bignum_*), and every data message gets a CRC32 appended and is TEA-encrypted with the shared key
  (tea_encrypt_block/buffer written next to the existing decrypt pair in src/cseries). Sockets, connections,
  callbacks, reliable delivery (serials, acks, nacks, resends, out-of-order buffering), gt2Connect/Send/Think/
  Accept/Reject/Close*. NOTE: gti2Send appends 4 bytes PAST the caller's message and encrypts in place.
- gamespy: the ServerBrowser (~50 functions, src/gamespy/sb.h): browser API, query engine, master list protocol
  (crypt header, key list, popular values, server records), push/ad-hoc messages, LAN broadcast, GOA card cipher.
  Master server string in this build: "s1.ms01.hosthpc.com":28910. FIXED SBIsNullServer (compared the address of
  the null-server pointer 0x6a27f8 instead of its value).
- networking: socket error callback 0x441060; the master-server thread 0x4b5f80 (sig__setup_master_server_connection
  _sig, __stdcall; its creator's extern fixed); '?' names bound: item_add_ammunition -> weapon_add_ammunition(EAX
  action entry), network_player_entry_is_valid -> network_player_entry_validate 0x4de9f0, network_df0e0_broadcast ->
  network_game_settings_broadcast_send 0x4df0e0 (server, entry), network_player_table_remove ->
  network_player_entry_remove 0x4de640 (EBX = server session, then client session per call site).
- NAMING NOTE: 0x614850 (our gt2SetSendDump) stores +0x28 = the RECEIVE dump, 0x61e550 (gt2SetReceiveDump) +0x24 =
  the SEND dump (and is ICF-shared with SBEngineSetPublicIP). Behaviour is right; only the names are swapped.
- Left (8): ghttp / autopatch (0x61bd00, 0x61bd40, 0x61bd80, 0x61bef0, 0x61c020, 0x61c030, 0x61c260 and their graph
  0x6208a0..0x622e10, ~69 functions) and 0x576a70.
- OPEN (unchanged): the networking C-to-C call audit (e.g. network_game_action_apply calls ~45 handlers with no args;
  the binary passes EAX = the action entry and ESI; network_channel_attempt_connect calls gt2Connect with 1 of 8 args).
### iteration 12 (2026-09-28) -- commits 1aee489..c7766ab, relinked (0 unresolved, traps 8 -> 0)
- MILESTONE: 0 direct traps (build/standalone/traps.txt is empty; unresolved 0).
- gamespy: ghttp (~60 functions, src/gamespy/ghttp.h: connection table + lock, the 8-state request machine, status /
  headers / redirects / chunked bodies, save-to-file, url-encoded and multipart posting, buffers) and the patch
  tracker ptCheckForPatch (hpcup.bungie.net/motd/vercheck.asp) with its completion callback.
- networking: autopatch progress callback 0x576a70; FIXED autopatch_download_start's save path -- the binary passes
  ghttpSaveEx 9 arguments with EDX as the file name (the C passed only the URL).
- Still missing (scratchpad/missing_functions.txt): 85 functions reached only through stored code pointers in .data --
  networking 83: the message-delta field type table at 0x69a2f8.. (0x4e89c0..0x4ebc20, ~75 encode/decode/compare
  handlers) and server-browser UI handlers in the ui event table (0x4b61a0, 0x4b6370, 0x4b63c0, 0x4b6570, 0x4b7920,
  0x4b7a80, 0x4b7c40); units 2: 0x571f20 / 0x5726e0 (table at 0x69b61c). NEXT: write those (they would execute image
  code at run time), then step 2.
### iteration 13 (2026-09-28) -- commits aeacbac..HEAD, relinked (0 unresolved, traps 0)
- MILESTONE: STEP 1 DONE -- scratchpad/missing_functions.txt is empty (no stored code pointer and no direct call
  lacks C); traps 0, unresolved 0.
- networking: all 28 message-delta field kinds' table functions and codecs (0x4e89c0..0x4ebc20; shared
  src/networking/message_delta_codec.h with the inlined absolute bit-stream seek): integers, reals, booleans,
  strings, wide strings, structure arrays, compound records, pointers, the index cache (with hash_table_initialize /
  dispose written in src/objects), flags, grenade counts, quantized reals, unit normals (vector3d_to_angles),
  locality positions, digital throttle, weapon/grenade indices, velocities through the waypoint table, item
  placement. FIXED: vector3d_quantize picks its level count by connection mode inside (the C took it as an argument)
  and waypoint_table_quantize_initialize uses integer level counts (the C treated them as floats).
- interface: the 7 server-browser widget events (filter panel open/apply/cancel/back, the button row, list rows
  with double-click join, browser close saving the filters into the profile).
- units: vehicle_encode_network_create 0x571f20 and vehicle_apply_network_update 0x5726e0.
- OPEN: (a) the networking call audit (network_game_action_apply's ~45 handlers get no args; gt2Connect called with
  1 of 8; the biped siblings unit_build_network_update / unit_apply_network_health_update model their
  message_delta calls and object index wrongly -- the vehicle versions written today show the real shapes).
- NEXT: step 2 (build-time independence).

## Iteration 14 (2026-09-28) -- retail-independence STEP 2 DONE (build-time independence)
- traps before/after: 0 / 0; unresolved 0.
- tools/retail_guard.py: HALO_NO_RETAIL=1 wraps open()/io.open()/subprocess so any read of halo.exe or
  out/functions.json aborts the build. Without it the generators re-extract from bin/halo.exe and refresh
  standalone/frozen/ (committed): layout.json, imports.json (315 slots), code_pointer_slots.json (3064 slots with
  retail name + module; C symbols attached at build time from src/ headers), game_crt.json, code_address_ret.json,
  d3dx_sizes.json.
- gen_standalone.py split into extract_retail() + a frozen-only main(); gen_standalone_link.py reads the CRT map,
  ret bytes and D3DX sizes from frozen/.
- Verified: HALO_NO_RETAIL=1 gen_standalone + gen_standalone_link -> 0 unresolved, 0 traps; layout/imports/
  code_pointers/code_entries JSON, report, code_pointers.asm, resolve.asm, standalone_tables.c and link_report
  identical to the retail run; halo_rebuilt.exe differs only in the 2 PE timestamp bytes. msvc_build units under
  the guard: 260 ok, 0 failed.
- Caveats: a NEW code_address_ thunk needs one retail run (or a hand entry in code_address_ret.json); a new C
  function at an address not in code_pointer_slots.json gets no data pointer until a retail refresh.
- NEXT: step 3 (image source replacing halo_image.bin). OPEN: networking call audit (iteration 13).

## Iteration 15 (2026-09-28) -- retail-independence STEP 3 core DONE (run-time independence)
- traps before/after: 0 / 0; unresolved 0 (HALO_NO_RETAIL=1 build + link).
- tools/gen_image_source.py (retail-only, one-time) wrote standalone/image/*.asm + pieces.json (1.3 MB): .rdata,
  initialised .data, .tls, .rsrc, and from .text only the two ranges read as data (DIOBJECTDATAFORMAT[256] at
  0x613400; the switch table at 0x4a0268). Every code-pointer slot is `dd halo_code_<addr>`, bound by
  /ALTERNATENAME to the C rewrite (2397 targets) or the existing named cp_trap_ stub (667 dead retail D3DX/CRT
  library slots -- same as the old loader patching).
- Verified statically BEFORE switching (tools/verify_image_source.py, commit bac03e1): identical to halo_image.bin
  apart from the 3064 relocated pointers, each equal to what the loader's table patched.
- Switched (ffbf996): loader memcpy's the linked pieces; the halo_image.bin read, fix_code_pointers and the
  standalone_code_pointers table are removed. Re-verified after the switch against the map (C symbol addresses).
  The exe no longer contains "halo_image.bin". NOT RUN (static only) -- the user should smoke-test a boot.
- Dropped from the image: .text original code (2.33 MB, never read as data; still reserved, zero, non-executable
  so a stray jump still faults as before).
- OPEN: ui_controls_options_populate_from_profile (src/interface) dispatches through the retail switch table at
  0x4a0268 -- its 7 entries are case labels inside retail code (0x4a013e..0x4a0162), which were never runnable in
  the standalone (DEP fault, no code_entries match). Rewrite it as a C switch.
- OPEN: networking call audit (iteration 13). Long tail: promote image regions to typed C.

## Iteration 16 (2026-09-28) -- step 3 clean-up: no retail .text, no self-redirects
- traps before/after: 0 / 0; unresolved 0 (HALO_NO_RETAIL=1).
- FIXED ui_controls_options_populate_from_profile (0x4a0050): the third row is a switch compiled to a .text jump
  table (index bytes 0x4a0284 by value-1, not 0x4a0283): 3/5/10/15/25 -> selection 1..5, else 0, then rows 4-5.
  The C called the retail case labels as functions and returned early (would have faulted in the standalone).
- Image: both .text pieces dropped (the switch table; DIOBJECTDATAFORMAT 0x613400, reachable only through the retail
  c_dfDIKeyboard 0x64dfdc -- the C links c_dfDIKeyboard / c_dfDIMouse2 from dinput8.lib). The image is .rdata,
  initialised .data, .tls, .rsrc; verify_image_source: identical apart from 3064 relocated pointers;
  gen_image_source reproduces the committed files.
- Link: code entries now require the object to define its function -- 9 fragment/clone files (objects 0x4efea0..
  0x4f9540, ranges inside functions whose C covers them) had entries resolving to their own retail address (a
  jump there would redirect to itself forever). 5533 entries; no symbol resolves into retail .text any more.
- Checked: the 667 cp_trap_ stubs are all slots in tables only the retail runtime walks (D3DX vtables/tables 366
  targets, CRT/EH scope and unwind tables, __xi/__xc init tables, the unused zlib deflate configuration_table at
  0x65c70c, gamespy's GOAGetUniqueID pointer 0x683dac -- no retail or C reference to any of them). Kept as named
  diagnostic stubs.
- OPEN: networking call audit (iteration 13). Long tail: promote image regions to typed C.

## Iteration 17 (2026-09-28) -- networking call audit, part 1 (receive path)
- traps before/after: 0 / 0; unresolved 0 (HALO_NO_RETAIL=1); image verify identical; hooks.txt == stable (content;
  the two differ only in CRLF, both untouched since 2026-09-26).
- Tools: scratchpad/audit_calls.py (call-site setup vs the callee's C signature/notes), scratchpad/livein.py
  (registers a function reads before writing), scratchpad/nga_cases.py (jump table -> handler).
- FIXED decode prototypes: message_delta_decode_compound_field (EAX context, ECX destination), _forced (+EDX
  baseline, stack force), _staged (EAX context) had the context missing or swapped with the destination in 12
  item/projectile handlers + object_delete_by_pooled_node_id.
- REWRITTEN from the disassembly (decoded into nothing / phantom parameters before): unit_dispatch_seat_exit_message
  0x56c400, unit_scripting_set_or_drop_weapon 0x56ddb0, unit_spawn_with_starting_weapons 0x572110 (really the vehicle
  creation receiver), unit_network_create_update_apply 0x55b110 (biped creation receiver),
  unit_apply_network_control_update 0x566c90.
- FIXED game handlers: spawn-loadout 0x477c70 and interaction 0x478f10 (player from the message, key tables through
  +0x28, unit_pickup_weapon / player_swap_to_weapon args), sound-status 0x46bca0 and end-game 0x467230 (decoded
  value, not an ECX parameter; sound tag to sound_start_unspatialized).
- REWRITTEN the receive chain: network_game_process_incoming_messages 0x4db180 (real bit stream per item; read_item's
  6th arg), network_incoming_item_dispatch 0x4db630, network_game_action_queue_drain 0x4db870 (client, stream,
  sender; builds the decode context), network_game_message_decode_dispatch 0x4db6b0 (client, record, length,
  sender to all 18 handlers), network_game_action_apply 0x4da320 (context + client to all ~48 handlers).
- OPEN (audit part 2): decode(void) still in chat_server_relay_incoming_message and unit_apply_network_health_update;
  message handlers 0x4dc190/0x4dc240/0x4dc2e0/0x4dc410 declare (client, uint8_t*, int16_t*, int32_t*) but receive
  (client, record, length, sender); gt2Connect called with 1 of 8 args; unit_build_network_update; the 0x614850 /
  0x61e550 dump-setter names.

## Iteration 18 (2026-09-28) -- networking call audit, part 2 (message handlers, host path)
- traps before/after: 0 / 0; unresolved 0 (HALO_NO_RETAIL=1); image verify identical.
- Client message handlers (behind network_game_message_decode_dispatch): 0x4dc190/0x4dc240/0x4dc2e0/0x4dc410 and
  0x4dbc00/0x4dbd40/0x4dbe50 took the length as a pointer or declared data_packet_group_decode_packet with 8 / 6
  arguments (it takes 7, remaining length first in EAX) -- expected class came out 0 or every argument shifted.
  Now: length reduced by 2 in place and passed by address; join_finalize / table_index_apply / disconnect_notify /
  timer get their real arguments; settings request reads the engine-version byte from the body.
- Server path: network_game_process_incoming_message 0x4e1c60 and its 13 handlers (0x4e24d0..0x4e2930) rewritten
  from the disassembly (the dispatcher passed nothing to eleven of them); network_game_broadcast_player_set_changed
  takes the one argument its callers pass and broadcasts via network_session.
- Host receive chain rewritten: network_channel_drain_bitstream 0x4e1290 (real bit stream per item),
  network_channel_dispatch_bitstream_unit 0x4e18b0 (ECX stream, ESI machine), network_client_drain_queued_updates
  0x4e1f40 (decode context; types 0xd/0xf/0x1a/0x34/0x36 with their registers), chat_server_relay_incoming_message
  0x4aabd0 (decodes into locals, relays by scope).
- unit_apply_network_health_update 0x55b5f0: its own object and decode into the health block copy.
- OPEN: gt2Connect called with 1 of 8 args; unit_build_network_update; the 0x614850 / 0x61e550 dump-setter names;
  network_game_server_handle_client_join's register pass-throughs (object_count / BL); several callees' C carry
  parameter names that do not match what the binary passes (network_game_settings_broadcast_send's "round" is the
  server) -- behaviour kept positional.

## Iteration 19 (2026-09-28) -- code-immediate callbacks; audit OPEN items closed
- traps before/after: 0 / 0; unresolved 0 (HALO_NO_RETAIL=1); image verify identical.
- NEW CLASS of missing functions: callbacks whose address the original passes as an immediate from code
  (scratchpad/imm_code_refs.py + imm_code_refs_c.py; the step-1 scan only read data tables). Written from the
  disassembly: network_channel_connected_callback 0x441e00 (GT2 connected), input_enumerate_gamepad_callback 0x491d70
  and input_enumerate_gamepad_object_callback 0x491c50 (DirectInput EnumDevices / EnumObjects),
  network_session_reject_pending_connection_callback 0x4e1410. input_system_initialize and
  network_game_server_host_new stored the literal retail addresses (jumps into original code); now by name.
  scratchpad/text_literals.py: no hex literal in C code points into retail .text (2 left are a size and 'FPS').
  The 6 GameSpy statics the scan also lists already have C; 0x639340 is a retail EH funclet.
- gt2Connect: network_channel_attempt_connect passes all 8 arguments (was 1), with the C callback table.
- unit_build_network_update 0x55aed0 rewritten: the biped creation encoder, exact inverse of 0x55b110, with buffer /
  budget / bit-count return (player_respawn updated).
- gt2SetSendDump (0x61e550, +0x24) / gt2SetReceiveDump (0x614850, +0x28) names swapped back.
- Networking call audit: all OPEN items from iterations 13/17/18 closed except the register pass-through of
  network_game_server_handle_client_join (passed 0, UNSURE).
- NEXT: long tail -- promote image regions to typed C; re-audit other modules with scratchpad/audit_calls.py.

## Iteration 20 (2026-09-28) -- send path; prototype re-audit started
- traps before/after: 0 / 0; unresolved 0 (HALO_NO_RETAIL=1); image verify identical.
- tools/check_prototypes.py re-run: 525 extern declarations disagree with their definition (scratchpad/
  proto_by_callee.py groups them by callee); now 502. Biggest remaining: networking 200, game 132, interface 39.
- Send path fixed from the disassembly (16 senders + update_server_send_update + rcon): each writes a 1-bit item flag
  (1 game action / 0 message record) and the encoded bits into the channel's outgoing stream (channel +0x10) with
  bit_stream_write_bits_chunked(EAX stream, ECX values, stack bits) -- the C passed placeholders in the wrong order;
  network_channel_stream_flush takes the stream (ESI) too; update_server wrote into the retransmit stream (+0x544).
- rcon_send_request is 304 bytes (0x4e4e30 "network_game_server_send_message_to_all_machines" is its copy-loop
  label -> documented fragment, no code entry); rcon and team-allegiance encode one contiguous record.
- NEXT: continue the prototype mismatches -- data_packet_group_encode_packet (9 callers declare 4 params, the
  definition has 10: check 0x4d0ae0's real convention), network_session_send_to_machine (7 vs 9),
  gamespy_array_length, tag_lookup, then game/ and interface/.

## 2026-09-28 -- HALO_NO_RETAIL removed (user request: always no retail)
- The build tools always call retail_guard.forbid_retail(); there is no retail mode. Build commands are now plain:
  python tools/msvc_build.py; python tools/gen_standalone.py > /dev/null && python tools/gen_standalone_link.py
- Retail extraction moved to the maintenance tool tools/freeze_retail_inputs.py (never part of a build; reproduces
  standalone/frozen/ byte for byte and writes build/standalone/halo_image.bin for verify_image_source.py).
  tools/gen_image_source.py is the other maintenance tool (regenerates standalone/image/*.asm).
- Boot test on user request: loads the saved campaign checkpoint (mission "Halo"), 60 s, no crash besides the known
  Keystone thread AV.

## Ghidra types loop (job c8dd7b0d; tools/ghidra_residue.py, gate: tools/objdiff.py 0 differ)
Start totals (live code): 7135 offsets, 896 Ghidra names, 74 Ghidra types.
- iter 1 cutscene + render (+rasterizer, shaders): render 1/1/0 -> 0/0/0; cutscene 1/0/0 kept (recorded_animation_compressed_update reads a 16-bit delay from a byte stream, not a struct -- DONE). shader_effect.unknown_98 -> average_particle_radius; param_9 -> lighting (rasterizer_transparent_geometry_group_new). 339 objects identical.
- iter 2 saved_games (+ai, game, hs, objects, projectiles, units): new types/game.h game_main_globals (0x006b0b80: map_loaded, active, players_are_double_speed, map_loading_in_progress, map_load_progress, difficulty, random_seed); the two names cache_file_slot_table / main_game_globals unified as main_game_globals (globals.asm entry dropped), 23 files. Offsets 7135 -> 7115. objdiff: all identical except 6 game objects where p[N] became p->field -- /Od computed the constant index at runtime (mov r,1; shl/imul) vs a displacement; same byte, same width, same extension, registers renumbered: kept. Boot OK (cdb 45s, in level). objdiff now ignores $SG/$LN label numbers and takes old=new renames. saved_games left: game_state_save_thread_proc +0x126 (write buffer = game state header), saved_game_create_custom_variant param_1 (unused).
- iter 3 saved_games: done (0/0/0). game_state_write_buffer accesses -> game_state_header fields (file_checksum, scenario_name, difficulty); param_1 -> unused. 119 identical.
