// qr2_buffer_add_int  (GameSpy SDK in halo.exe; no C existed)
// address 0x616640, size 60 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616640..0x61667b: the value printed with "%d" (0x0065fb30), then
//   qr2_buffer_add.
// blam-cc: cdecl

#include "gamespy.h"

extern void qr2_buffer_add(char *buffer, const char *value);

void qr2_buffer_add_int(char *buffer, int value)
{
    char text[0x14];

    sprintf(text, "%d", value);
    qr2_buffer_add(buffer, text);
}
