import re
R = "C:\\Users\\Liam-\\halo-re\\src\\"


def edit(rel, pairs, count=1):
    p = R + rel
    t = open(p, encoding="utf-8").read()
    for old, new in pairs:
        if isinstance(old, re.Pattern):
            t, n = old.subn(new, t, count=count)
            assert n >= 1, (rel, old.pattern[:60])
        else:
            assert t.count(old) >= 1, (rel, old[:60])
            t = t.replace(old, new, count)
    open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("edited", rel)


# a thread routine is __stdcall and takes the thread parameter: 0x5771c0 ends ret 4
edit("networking\\autopatch_proxy_initialize.c", [
    ('''uint32_t autopatch_proxy_initialize(void)
{''', '''// FIXED 2026-09-28: a CreateThread routine (network_initialize), __stdcall with the unused thread parameter --
// the original ends ret 4 (0x5771d7).
uint32_t __stdcall autopatch_proxy_initialize(void *parameter)
{
    (void)parameter;'''),
])
# network_initialize starts the proxy thread on autopatch_proxy_initialize (0x4416ad push 0x5771c0), not on
# join_game_server_browser_tick
edit("networking\\network_initialize.c", [
    ("extern void join_game_server_browser_tick(void); // foreign module (autopatch, > 0x4b80f0)",
     "extern uint32_t __stdcall autopatch_proxy_initialize(void *parameter); // 0x5771c0, the proxy thread"),
    ("        CreateThread(0, 0x10400, join_game_server_browser_tick, 0, 0, &thread_id);",
     "        CreateThread(0, 0x10400, autopatch_proxy_initialize, 0, 0, &thread_id); // FIXED 2026-09-28: 0x4416ad pushes\n"
     "            // 0x5771c0 (autopatch_proxy_initialize); the C started join_game_server_browser_tick on the thread"),
])
# these two thread routines end in ExitThread and never return, so their convention does not matter: cast
edit("main\\network_hostname_resolve_with_timeout.c", [
    ("                                  network_hostname_resolve_thread_proc, hostname, 0, &thread_id);",
     "                                  (LPTHREAD_START_ROUTINE)network_hostname_resolve_thread_proc, hostname, 0,\n"
     "                                  &thread_id); // the routine ends in ExitThread (never returns)"),
])
edit("networking\\network_local_hostent_get.c", [
    ("    thread_handle = CreateThread(0, 0x10400, network_hostname_thread_proc,",
     "    thread_handle = CreateThread(0, 0x10400, (LPTHREAD_START_ROUTINE)network_hostname_thread_proc, // ends in ExitThread"),
])
# inet_ntoa takes a struct in_addr by value
edit("game\\game_engine_rasterize_in_game_score.c", [
    (re.compile(r"address_text = inet_ntoa\((\(\(raw << 0x10 \| \(raw & 0xff00\) \| \(raw >> 0x10 & 0xff\)\) << 8\) \|\s*\(raw >> 0x18\))\);"),
     r"{ struct in_addr in; in.s_addr = \1; address_text = inet_ntoa(in); }"),
])
edit("interface\\ui_network_host_setup_refresh.c", [
    ("        char *text = inet_ntoa(swapped);",
     "        struct in_addr swapped_address;\n        char *text;\n\n        swapped_address.s_addr = swapped;\n"
     "        text = inet_ntoa(swapped_address);"),
])
edit("main\\network_game_client_connect_by_hostname.c", [
    ("            address_text = inet_ntoa(**(uint32_t **)((char *)host + 0xc));",
     "            address_text = inet_ntoa(**(struct in_addr **)((char *)host + 0xc));"),
])
# _tolower is a <ctype.h> macro once the SDK headers are in; the CRT function it replaces does the same
edit("networking\\autopatch_get_proxy_settings.c", [
    ("extern int _tolower(int c);                                  // 0x624687, CRT\n", ""),
    ('#include "win32.h"\n', '#include "win32.h"\n#include <ctype.h>\n'),
])
