p = "C:\\Users\\Liam-\\halo-re\\src\\game\\game_engine_apply_player_interaction_message.c"
t = open(p, encoding="utf-8").read()


def sub(old, new):
    global t
    assert t.count(old) == 1, old[:70]
    t = t.replace(old, new)


sub("extern uint8_t player_swap_to_weapon(uint32_t handle); // this batch, 0x479240",
    "extern uint8_t player_swap_to_weapon(uint32_t player_index, datum_index target_weapon); // 0x479240, EAX player, stack weapon")
sub("uint8_t game_engine_apply_player_interaction_message(void **envelope, datum_index join_key)\n{",
    '''// FIXED 2026-09-28 (networking call audit, from the disassembly 0x478f10..0x478fe8): there is no join_key argument
// -- the player is the message's first field through the player key table (0x687558 +0x28, EBP); the key tables
// are indexed through the pointer at +0x28 (the C added 0x28 to the table's own address); and the swap path passes
// the player too (player_swap_to_weapon: EAX player, stack weapon).
uint8_t game_engine_apply_player_interaction_message(void **envelope)
{''')
sub("primary_handle = *(uint32_t *)((uint8_t *)machine_table + 0x28 + message.machine_id * 4);",
    "primary_handle = (uint32_t)(*(int32_t **)(machine_table + 0x28))[message.machine_id];")
sub("player *p = (player *)datum_get(join_key, player_data); // UNSURE: array argument",
    "player *p = (player *)datum_get((datum_index)primary_handle, player_data); // 0x478f4d: EDX handle, ESI players")
sub('''                    interaction_object = *(datum_index *)((uint8_t *)object_pooled_node_globals + 0x28 +
                        message.interaction_pooled_id * 4);''', '''                    interaction_object = (datum_index)(*(int32_t **)(object_pooled_node_globals + 0x28))[
                        message.interaction_pooled_id];''')
sub('''                    secondary_handle = *(uint32_t *)((uint8_t *)object_pooled_node_globals + 0x28 +
                        message.secondary_pooled_id * 4);''', '''                    secondary_handle = (uint32_t)(*(int32_t **)(object_pooled_node_globals + 0x28))[
                        message.secondary_pooled_id];''')
sub("return player_swap_to_weapon(secondary_handle);",
    "return player_swap_to_weapon(primary_handle, (datum_index)secondary_handle);")
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
