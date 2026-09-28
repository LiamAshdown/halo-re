p = "C:\\Users\\Liam-\\halo-re\\src\\networking\\network_game_server_host_new.c"
t = open(p, encoding="utf-8").read()
old = '''        host->session.message_callback = (void *)0x4e1410; // matches header's "stores 0x004e1410 here"'''
assert t.count(old) == 1
t = t.replace(old, '''        host->session.message_callback = (void *)network_session_reject_pending_connection_callback; // 0x4e1410''')
i = t.index("#include")
j = t.index("\n\n", t.rindex("#include", 0, t.index("{")))
t = t[:j] + '''

// FIXED 2026-09-28 (retail-independence loop): the session's message callback is the C
// network_session_reject_pending_connection_callback, not the literal retail address 0x4e1410.
extern int32_t network_session_reject_pending_connection_callback(void *unused, int32_t reject_code); // 0x4e1410''' + t[j:]
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
