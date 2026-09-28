p = "C:\\Users\\Liam-\\halo-re\\src\\game\\game_engine_handle_sound_status_event.c"
t = open(p, encoding="utf-8").read()


def sub(old, new):
    global t
    assert t.count(old) == 1, old[:70]
    t = t.replace(old, new)


sub("extern void sound_start_unspatialized(float volume);                      // 0x543dd0, UNSURE exact identity",
    "extern datum_index sound_start_unspatialized(datum_index definition_index, float scale); // 0x543dd0, EDX definition, stack scale")
sub('''void game_engine_handle_sound_status_event(void *event, int32_t sound_index)
{
    if (*(int32_t *)*(void **)event == 0) {
        int32_t scratch;
        if (message_delta_decode_compound_field(event, &scratch) != 0) {''',
    '''// FIXED 2026-09-28 (networking call audit, from the disassembly 0x46bca0..0x46bcf4): the sound index is the decoded
// value itself (there is no ECX argument), and the sound's tag (+0xc of its 0x10-byte entry) goes to
// sound_start_unspatialized in EDX.
void game_engine_handle_sound_status_event(void *event)
{
    if (*(int32_t *)*(void **)event == 0) {
        int32_t sound_index;
        if (message_delta_decode_compound_field(event, &sound_index) != 0) {''')
sub("sound_start_unspatialized(1.0f);", "sound_start_unspatialized(*(datum_index *)(sound + 0xc), 1.0f);")
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
