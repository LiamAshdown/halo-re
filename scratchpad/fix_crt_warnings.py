R = "C:\\Users\\Liam-\\halo-re\\src\\"


def edit(rel, pairs):
    p = R + rel
    t = open(p, encoding="utf-8").read()
    for old, new in pairs:
        assert t.count(old) == 1, (rel, old[:60])
        t = t.replace(old, new)
    open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("edited", rel)


# the game's CRT is 32-bit time: _time32 then the 32-bit localtime (the header's localtime is the 64-bit one)
edit("cseries\\write_to_error_file.c", [
    ("                local_time = localtime(&time_value);",
     "                local_time = _localtime32(&time_value); // 32-bit time, as _time32 wrote it"),
])
edit("interface\\video_resolution_list_build.c", [
    ("          (int (__cdecl *)())video_resolution_compare);",
     "          (int (__cdecl *)(const void *, const void *))video_resolution_compare);"),
    ("              sizeof(int32_t), (int (__cdecl *)())video_refresh_rate_compare);",
     "              sizeof(int32_t), (int (__cdecl *)(const void *, const void *))video_refresh_rate_compare);"),
])
edit("interface\\virtual_keyboard_character_is_legal.c", [
    ("                int32_t blocked = strchr(virtual_keyboard_blacklist_charset, character);",
     "                const char *blocked = strchr(virtual_keyboard_blacklist_charset, character);"),
])
