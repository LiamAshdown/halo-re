R = "C:\\Users\\Liam-\\halo-re\\src\\"


def edit(rel, pairs):
    p = R + rel
    t = open(p, encoding="utf-8").read()
    for old, new in pairs:
        assert t.count(old) == 1, (rel, old[:70])
        t = t.replace(old, new)
    open(p, "w", encoding="utf-8", newline="\n").write(t)


edit("game\\game_engine_dispatch_end_game_notification.c", [
    ('''void game_engine_dispatch_end_game_notification(void *event, int32_t stage)
{''', '''// FIXED 2026-09-28 (networking call audit): the stage is the decoded value (a stack slot, 0x467230 push ecx), not
// an ECX argument -- the dispatcher's ECX there is only its own scratch.
void game_engine_dispatch_end_game_notification(void *event)
{
    int32_t stage;
'''),
])
edit("objects\\object_delete_by_pooled_node_id.c", [
    ('''void object_delete_by_pooled_node_id(int32_t **record, uint32_t pooled_node_id)
    // blam-cc: EAX -> record, ECX -> pooled_node_id
{''', '''// FIXED 2026-09-28 (networking call audit): the pooled node id is the decoded value (the slot 0x4f5b50 push ecx
// reserves), not an ECX argument.
void object_delete_by_pooled_node_id(int32_t **record)
    // blam-cc: EAX -> record
{
    uint32_t pooled_node_id;'''),
])
print("ok")
