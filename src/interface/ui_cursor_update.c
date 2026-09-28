// ui_cursor_update  (Ghidra: FUN_004972c0, unnamed)
// address 0x4972c0, size 189 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: reads the win32 OS cursor position (or, when ui_use_os_cursor is off, a raw
// device-delta pair run through a quadratic sensitivity curve), turns it into a frame-to-frame
// delta, and feeds the new position to interface_update_for_resolution_change @0x497250, which
// clamps and stores it into ui_cursor_x/ui_cursor_y (0x00718f84/88, named by types/interface.h).
// Disassembled directly (objdump 0x4972c0..0x497380) because Ghidra drops the EAX/ECX arguments
// to the tail call entirely and the two bare __ftol() calls hide their FPU-stack operands.
// register convention: no register-passed arguments; feeds EAX/ECX to the tail call by
// blam-cc's own convention (first two register slots).
// UNSURE: DAT_006b1804/006b15f9/006b1828/006b180c select and hold a raw two-int32 delta pair
// (device polling result, not otherwise identified) and DAT_0068e674/0068e678/00672da8/00672af8
// are the per-axis sensitivity curve coefficients; none of these are named anywhere else in this
// module or in types/devices.h, so they are declared here with placeholder names.
// UNSURE: cursor X moves opposite to the raw OS/device delta while Y moves with it (the OS-cursor
// branch does `ui_cursor_x + (previous_mouse_x - new_mouse_x)` but
// `ui_cursor_y + (new_mouse_y - previous_mouse_y)`); preserved exactly as disassembled, not
// symmetrized.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint8_t ui_use_os_cursor;      // 0x00718f81
extern int32_t previous_mouse_x;      // 0x006b2f24, last frame's OS cursor x
extern int32_t previous_mouse_y;      // 0x006b2f20, last frame's OS cursor y
extern int32_t ui_cursor_x;           // 0x00718f84
extern int32_t ui_cursor_y;           // 0x00718f88

extern int32_t cursor_delta_source_active;    // 0x006b1804, UNSURE: nonzero selects a raw delta source
extern uint8_t cursor_delta_source_variant;   // 0x006b15f9, UNSURE: picks between the two records below
extern int32_t cursor_delta_record_a[2];      // 0x006b1828, UNSURE: raw {dx, dy} device delta
extern int32_t cursor_delta_record_b[2];      // 0x006b180c, UNSURE: raw {dx, dy} device delta
extern float cursor_sensitivity_x;            // 0x0068e674, UNSURE: X-axis curve coefficient
extern float cursor_sensitivity_y;            // 0x0068e678, UNSURE: Y-axis curve coefficient
extern double cursor_sensitivity_curve_scale; // 0x00672da8, UNSURE: shared quadratic coefficient
extern double cursor_sensitivity_curve_bias;  // 0x00672af8, UNSURE: shared additive constant

extern void interface_update_for_resolution_change(int32_t new_cursor_x, int32_t new_cursor_y); // 0x497250
extern int32_t __ftol(double x); // 0x6391b4, MSVC float-to-long (truncating)
extern int32_t __stdcall GetCursorPos(win32_point *point);

// blam-cc: no register-passed arguments
// Per-frame cursor position update: either reads the real OS cursor and turns it into a delta
// against last frame's OS position, or (when the OS cursor is not in use) reads a raw two-axis
// device delta and runs each axis through a quadratic sensitivity curve
// (`raw * (|raw| * sensitivity * curve_scale + curve_bias)`), then applies the resulting delta
// to ui_cursor_x/ui_cursor_y via interface_update_for_resolution_change.
void ui_cursor_update(void)
{
    int32_t delta_x, delta_y;

    if (ui_use_os_cursor != 0) {
        win32_point point;

        GetCursorPos(&point);
        delta_x = previous_mouse_x - point.x;
        delta_y = previous_mouse_y - point.y;
        previous_mouse_x = point.x;
        previous_mouse_y = point.y;
    } else {
        int32_t *record;
        float raw_x, raw_y, curve_x, curve_y;
        double scaled_x, scaled_y;

        record = cursor_delta_record_b;
        if (cursor_delta_source_active != 0) {
            record = cursor_delta_record_a;
            if (cursor_delta_source_variant == 0) {
                record = cursor_delta_record_b;
            }
        }
        raw_x = (float)record[0];
        raw_y = (float)record[1];
        curve_x = cursor_sensitivity_x * (raw_x < 0.0f ? -raw_x : raw_x);
        scaled_x = (double)curve_x * cursor_sensitivity_curve_scale + cursor_sensitivity_curve_bias;
        delta_x = __ftol(scaled_x * raw_x);
        curve_y = cursor_sensitivity_y * (raw_y < 0.0f ? -raw_y : raw_y);
        scaled_y = (double)curve_y * cursor_sensitivity_curve_scale + cursor_sensitivity_curve_bias;
        delta_y = __ftol(scaled_y * raw_y);
    }
    interface_update_for_resolution_change(ui_cursor_x + delta_x, ui_cursor_y - delta_y);
}

#if 0
Original Ghidra decompilation (0x4972c0):

void FUN_004972c0(void)

{
  tagPOINT local_8;

  if (DAT_00718f81 == '\0') {
    __ftol();
    __ftol();
  }
  else {
    GetCursorPos(&local_8);
    DAT_006b2f24 = local_8.x;
    DAT_006b2f20 = local_8.y;
  }
  interface_update_for_resolution_change();
  return;
}

Disassembly (objdump -d -M intel, 0x4972c0..0x497380):

004972c0:
  mov    al,ds:0x718f81
  sub    esp,0x8
  test   al,al
  push   esi
  je     0x4972fd
  lea    eax,[esp+0x4]
  push   eax
  call   DWORD PTR ds:0x63a410        ; GetCursorPos
  mov    esi,DWORD PTR ds:0x6b2f24
  mov    ecx,DWORD PTR [esp+0x4]
  mov    eax,DWORD PTR ds:0x6b2f20
  mov    edx,DWORD PTR [esp+0x8]
  sub    esi,ecx
  sub    eax,edx
  mov    DWORD PTR ds:0x6b2f24,ecx
  mov    DWORD PTR ds:0x6b2f20,edx
  jmp    0x497362
  mov    eax,ds:0x6b1804
  push   edi
  xor    edi,edi
  test   eax,eax
  je     0x49731c
  mov    al,ds:0x6b15f9
  test   al,al
  mov    edi,0x6b1828
  jne    0x49731c
  mov    edi,0x6b180c
  fild   DWORD PTR [edi]
  fld    st(0)
  fabs
  fmul   DWORD PTR ds:0x68e674
  fmul   QWORD PTR ds:0x672da8
  fadd   QWORD PTR ds:0x672af8
  fmul   st,st(1)
  call   0x6391b4                     ; __ftol
  fstp   st(0)
  fild   DWORD PTR [edi+0x4]
  mov    esi,eax
  fld    st(0)
  fabs
  fmul   DWORD PTR ds:0x68e678
  fmul   QWORD PTR ds:0x672da8
  fadd   QWORD PTR ds:0x672af8
  fmul   st,st(1)
  call   0x6391b4                     ; __ftol
  fstp   st(0)
  pop    edi
  mov    ecx,DWORD PTR ds:0x718f88
  mov    edx,DWORD PTR ds:0x718f84
  sub    ecx,eax
  lea    eax,[edx+esi*1]
  call   0x497250                     ; interface_update_for_resolution_change(EAX=x, ECX=y)
  pop    esi
  add    esp,0x8
  ret
#endif
