"""message_delta_decode_compound_field / _forced / _staged: canonical prototypes and argument order in the item and
projectile network handlers (the binary: EAX decode context, ECX destination, EDX changed baseline, stack force)"""
import re
R = "C:\\Users\\Liam-\\halo-re\\src\\"

DECODE = ("extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); "
          "// 0x4ec590, EAX context, ECX destination")
FORCED = ("extern uint8_t message_delta_decode_compound_field_forced(void *decode_context, void *destination,\n"
          "    int32_t changed_offset, uint8_t force); // 0x4ec600, EAX context, ECX destination, EDX baseline, stack force")
STAGED = ("extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); "
          "// 0x4ec670, EAX context: rejects (skips) the message")

FILES = {
    "items\\equipment_apply_network_update.c": "update_record",
    "projectiles\\projectile_apply_network_update.c": "update_record",
    "items\\weapon_apply_network_update.c": "update_record",
    "projectiles\\projectile_attach_apply.c": "incoming_record",
    "projectiles\\projectile_create_from_network.c": "incoming_record",
    "projectiles\\projectile_detonation_message_apply.c": "incoming_record",
    "items\\equipment_create_from_creation_message.c": "incoming_record",
    "items\\weapon_create_from_creation_message.c": "incoming_record",
    "items\\weapon_add_ammunition.c": "message_record",
    "items\\weapon_apply_ammo_correction.c": "message_record",
    "items\\weapon_apply_ammo_correction_and_resync.c": "message_record",
    "items\\weapon_predict_ammo.c": "message_record",
    "objects\\object_delete_by_pooled_node_id.c": "record",
}

NOTE = ("// FIXED 2026-09-28 (networking call audit): message_delta_decode_compound_field / _forced / _staged take the\n"
        "// decode context first (EAX) and the destination second (ECX); the calls here had the context missing or the two\n"
        "// swapped.\n")

for rel, ctx in FILES.items():
    p = R + rel
    t = open(p, encoding="utf-8").read()
    cut = t.find("\n#if 0")
    head, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
    n0 = head
    # prototypes (whole declaration through the end of its line)
    head = re.sub(r"extern [^;]*?\bmessage_delta_decode_compound_field_forced\s*\([^;]*\);[^\n]*", lambda m: FORCED, head, flags=re.S)
    head = re.sub(r"extern [^;]*?\bmessage_delta_decode_compound_field_staged\s*\([^;]*\);[^\n]*", lambda m: STAGED, head, flags=re.S)
    head = re.sub(r"extern [^;(]*?\bmessage_delta_decode_compound_field\s*\([^;]*\);[^\n]*", lambda m: DECODE, head, flags=re.S)
    # calls
    head = re.sub(r"message_delta_decode_compound_field_staged\(\s*0?\s*\)", "message_delta_decode_compound_field_staged(%s)" % ctx, head)
    head = re.sub(r"message_delta_decode_compound_field_forced\(&(\w+), &([\w\->.]+), %s, 0\)" % ctx,
                  r"message_delta_decode_compound_field_forced(%s, &\1, (int32_t)&\2, 0)" % ctx, head)
    head = re.sub(r"message_delta_decode_compound_field\(&(\w+), %s\)" % ctx,
                  r"message_delta_decode_compound_field(%s, &\1)" % ctx, head)
    head = re.sub(r"message_delta_decode_compound_field\(&(\w+)\)", r"message_delta_decode_compound_field(%s, &\1)" % ctx, head)
    if rel.endswith("weapon_apply_network_update.c"):
        head = head.replace("message_delta_decode_compound_field_forced(0)",
                            "message_delta_decode_compound_field_forced(update_record, &snapshot, (int32_t)&wd->network_state, 0)")
        head = head.replace("message_delta_decode_compound_field(0)",
                            "message_delta_decode_compound_field(update_record, &snapshot)")
    if head == n0:
        print("UNCHANGED", rel)
        continue
    # the note goes before the first #include
    i = head.find("#include")
    head = head[:i] + NOTE + "\n" + head[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(head + tail)
    calls = re.findall(r"message_delta_decode_compound_field\w*\([^;]*?\)(?=[;)\s=!|])", head.split(NOTE)[1])
    print(rel)
    for c in calls:
        if "extern" not in c:
            print("   ", " ".join(c.split())[:140])
