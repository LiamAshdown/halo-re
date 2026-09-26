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
