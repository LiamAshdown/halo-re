#include "tags.h"
#include "memory.h"
#include "dialogs.h"

#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

CHECK(logfont_size, sizeof(win32_logfonta) == k_dialog_logfont_size);
CHECK(logfont_weight, OFF(win32_logfonta, weight) == 0x10);
CHECK(logfont_underline, OFF(win32_logfonta, underline) == 0x15);
CHECK(logfont_face, OFF(win32_logfonta, face_name) == 0x1c);
CHECK(language, k_dialog_language_english == 0x409);
CHECK(ctl_color, k_dialog_message_ctl_color_static == 0x138);

int main(void) { return 0; }
