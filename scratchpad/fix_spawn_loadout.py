p = "C:\\Users\\Liam-\\halo-re\\src\\game\\game_engine_apply_player_spawn_loadout_message.c"
t = open(p, encoding="utf-8").read()


def sub(old, new):
    global t
    assert t.count(old) == 1, old[:70]
    t = t.replace(old, new)


sub('''extern void unit_pickup_weapon(datum_index unit_handle, uint8_t is_primary); // 0x56d400, units module;''',
    '''extern uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index); // 0x56d400, stack mode, EAX weapon, ECX unit''')
sub('''// blam-cc: EAX -> envelope, stack -> player_handle
void game_engine_apply_player_spawn_loadout_message(void **envelope, uint32_t player_handle)
{''', '''// blam-cc: EAX -> envelope
// FIXED 2026-09-28 (networking call audit, from the disassembly 0x477c70..0x477e95): there is no player_handle
// argument -- the player is the message's first field looked up in the player key table (0x687558 +0x28, EBX),
// the same handle the unit's owner fields, the grenade counts (0x4613c0, EAX) and the kill streaks (0x479ba0, EBX)
// get. The key tables are indexed through the pointer at +0x28 (the C added 0x28 to the table's own address), and
// each known weapon is picked up with unit_pickup_weapon(0, weapon, unit) (0x477ded: EAX weapon, ECX unit, stack 0).
void game_engine_apply_player_spawn_loadout_message(void **envelope)
{''')
sub('''        datum_index owner_handle = (datum_index)0xffffffff;
        if (message.machine_id != 0) {
            owner_handle = *(datum_index *)((uint8_t *)machine_table + 0x28 + message.machine_id * 4);
            // UNSURE: offset arithmetic mirrors machine_table's own established +0x28 array
            // shape; see header note.
        }
''', '''        datum_index owner_handle = (datum_index)0xffffffff;
        uint32_t player_handle;
        if (message.machine_id != 0) {
            owner_handle = (datum_index)(*(int32_t **)(machine_table + 0x28))[message.machine_id];
        }
        player_handle = (uint32_t)owner_handle;
''')
sub('''            datum_index new_unit = *(datum_index *)((uint8_t *)object_pooled_node_globals + 0x28 +
                    message.unit_pooled_id * 4);''', '''            datum_index new_unit = (datum_index)(*(int32_t **)(object_pooled_node_globals + 0x28))[
                    message.unit_pooled_id];''')
sub('''                        game_engine_apply_player_grenade_counts(player_handle); // UNSURE:
                            // could plausibly be owner_handle instead; Ghidra shows no visible
                            // argument at all for this call
''', '''                        game_engine_apply_player_grenade_counts(player_handle); // 0x477db2: EAX = the player handle
''')
sub('''                                if (message.weapon_pooled_ids[i] == 0 ||
                                    *(datum_index *)((uint8_t *)object_pooled_node_globals + 0x28 +
                                        message.weapon_pooled_ids[i] * 4) == (datum_index)0xffffffff) {
                                    unit->weapons[i] = (datum_index)0xffffffff;
                                } else {
                                    unit_pickup_weapon(new_unit, 0); // UNSURE: no explicit weapons[i]
                                        // store in this branch, see header note
                                }''', '''                                int32_t weapon = message.weapon_pooled_ids[i] != 0
                                    ? (*(int32_t **)(object_pooled_node_globals + 0x28))[message.weapon_pooled_ids[i]]
                                    : -1;
                                if (weapon == -1) {
                                    unit->weapons[i] = (datum_index)0xffffffff;
                                } else {
                                    unit_pickup_weapon(0, (uint32_t)weapon, new_unit);
                                }''')
sub('''                            datum_index vehicle = *(datum_index *)((uint8_t *)object_pooled_node_globals +
                                0x28 + message.seat_vehicle_pooled_id * 4);''', '''                            datum_index vehicle = (datum_index)(*(int32_t **)(object_pooled_node_globals + 0x28))[
                                message.seat_vehicle_pooled_id];''')
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
