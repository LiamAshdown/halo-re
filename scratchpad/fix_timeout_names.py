import os, re
os.chdir(r'C:\Users\Liam-\halo-re')

p = 'src/networking/network_server_check_machine_timeout.c'
s = open(p, encoding='utf-8').read()
old_decl = s[s.index('extern char network_player_entry_is_valid(network_player_entry *entry);'):s.index('extern void network_channel_remove_child')]
new_decl = '''extern char network_player_entry_validate(network_player_entry *entry);
    // blam-cc: EAX -> entry; 0x4de9f0. The EAX convention is pinned by 0x4e0f80 `mov eax,esi` /
    // 0x4e102b `lea eax,[esp+0x20]`, both immediately before the call.
extern uint32_t network_game_settings_broadcast_send(uint32_t round, uint32_t *record);
    // 0x4df0e0; both arguments are pushed: (server, the 32-byte player entry)
extern uint32_t network_player_entry_remove(network_player_entry *key, network_game_session *session);
    // blam-cc: EAX -> key, EBX -> session; 0x4de640. 0x4e105f passes the server session (EBX = server + 8),
    // 0x4e108a the client's (EBX = network_client + 0xb14).
'''
s = s.replace(old_decl, new_decl)
s = s.replace('network_player_entry_is_valid(', 'network_player_entry_validate(')
s = s.replace('network_df0e0_broadcast(server, &copy)', 'network_game_settings_broadcast_send((uint32_t)server, (uint32_t *)&copy)')
s = s.replace('network_df0e0_broadcast(server, entry)', 'network_game_settings_broadcast_send((uint32_t)server, (uint32_t *)entry)')
a = '                        network_player_table_remove(&copy); // blam-cc: EAX -> &copy\n'
b = '                            network_player_table_remove(&copy); // blam-cc: EAX -> &copy, again\n'
assert a in s and b in s
s = s.replace(a, '                        network_player_entry_remove(&copy, &server->session); // EAX &copy, EBX server + 8\n')
s = s.replace(b, '                            network_player_entry_remove(&copy, &network_client->session); // EBX client + 0xb14\n')
open(p, 'w', encoding='utf-8').write(s)
print(s.count('network_df0e0_broadcast'), s.count('network_player_table_remove'), s.count('is_valid'))

# the other callers of network_player_entry_is_valid
for f in os.listdir('src/networking'):
    q = 'src/networking/' + f
    t = open(q, encoding='utf-8').read()
    if 'network_player_entry_is_valid' not in t:
        continue
    t = re.sub(r'extern char network_player_entry_is_valid\(network_player_entry \*entry\);',
               'extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, blam-cc: EAX -> entry', t)
    t = t.replace('network_player_entry_is_valid(', 'network_player_entry_validate(')
    open(q, 'w', encoding='utf-8').write(t)
    print('fixed', f)
