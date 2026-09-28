p = "C:\\Users\\Liam-\\halo-re\\src\\game\\game_engine_send_team_allegiance_message.c"
t = open(p, encoding="utf-8").read()


def sub(old, new):
    global t
    assert t.count(old) == 1, old[:60]
    t = t.replace(old, new)


sub('''    uint8_t team_index_desired = 0xff;
    uint8_t local_team_byte;      // Ghidra's local_1c: the field message_delta_encode_message reads
    uint8_t local_broadcast_byte; // Ghidra's local_1b, immediately after it in memory
    uint8_t *fields_ptr;          // Ghidra's local_18 = &local_team_byte
    int32_t fields_pad;           // Ghidra's local_14 = 0
''', '''    uint8_t team_index_desired = 0xff;
    struct {
        uint8_t team;             // 0x00 the local player's desired team (0x47055d: BL)
        uint8_t broadcast;        // 0x01 the argument (0x470548: AL)
    } record;                     // one 2-byte record: the encoder reads both from one address
    void *fields_ptr[2];          // [0] = &record, [1] = 0
''')
sub('''    local_broadcast_byte = (uint8_t)broadcast;
    fields_ptr = &local_team_byte;
    fields_pad = 0;
    local_team_byte = team_index_desired;
    (void)local_broadcast_byte;
    (void)fields_pad;
''', '''    record.broadcast = (uint8_t)broadcast;
    record.team = team_index_desired;
    fields_ptr[0] = &record;
    fields_ptr[1] = 0;
''')
sub("(void **)&fields_ptr, 0, 1, 0);", "fields_ptr, 0, 1, 0);")
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
