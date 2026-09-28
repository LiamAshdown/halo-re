"""for each sender, the instructions before each call to bit_stream_write_bits_chunked (0x4cf8f0) and to
message_delta_encode_message (0x4ec940) / data_packet_group_encode_packet (0x4d0ae0)"""
import re, subprocess
R = "C:\\Users\\Liam-\\halo-re\\"
FILES = """src/game/game_engine_send_team_allegiance_message.c src/interface/chat_queue_team_message.c src/interface/chimera__chat_out.c src/networking/network_connection_finalize_join.c src/networking/network_game_record_message_send.c src/networking/network_game_server_send_message_to_all_machines.c src/networking/network_game_settings_ack_send.c src/networking/network_game_settings_packet_send.c src/networking/network_host_presence_broadcast_tick.c src/networking/network_send_join_request_packet.c src/networking/network_server_build_full_game_info_packet.c src/networking/network_server_build_game_info_packet.c src/networking/network_session_info_packet_send.c src/networking/network_session_player_join_notify.c src/networking/network_staged_message_commit.c src/networking/rcon_send_request.c src/networking/update_server_send_update.c""".split()
for f in FILES:
    t = open(R + f.replace("/", "\\"), encoding="utf-8").read(3000)
    m = re.search(r"address 0x([0-9a-f]+), size (\d+)", t)
    a, n = int(m.group(1), 16), int(m.group(2))
    out = subprocess.run(["objdump", "-d", "-M", "intel", "--no-show-raw-insn", "--start-address=0x%x" % a,
                          "--stop-address=0x%x" % (a + n), R + "bin\\halo.exe"], capture_output=True, text=True).stdout
    lines = [l.strip() for l in out.split("\n") if re.match(r"\s*[0-9a-f]+:", l)]
    print("=== %s (0x%x)" % (f.split("/")[-1][:-2], a))
    for i, l in enumerate(lines):
        if re.search(r"call\s+0x(4cf8f0|4ddb60)", l):
            print("   " + " ; ".join(x.split("\t", 1)[-1] if "\t" in x else x for x in lines[max(0, i - 5):i + 1]))
