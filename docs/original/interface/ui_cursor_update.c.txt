// ui_cursor_update  (Ghidra: FUN_004972c0, unnamed)
// address 0x4972c0, size 189 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: reads the win32 OS cursor position (or, when ui_use_os_cursor is off, a raw
// device-delta pair run through a quadratic sensitivity curve), turns it into a frame-to-frame
// delta, and feeds the new position to interface_update_for_resolution_change @0x497250, which
// clamps and stores it into ui_cursor_x/ui_cursor_y (0x00718f84/88, named by types/interface.h).
// Disassembled directly (objdump 0x4972c0..0x497380) because Ghidra drops the EAX/ECX arguments
// to the tail call entirely and the two bare __ftol() calls hide their FPU-stack operands.
// register convention: no register-passed arguments; feeds EAX/ECX to the tail call by
// blam-cc's own convention (first two register slots).
// The device records / curve coefficients keep provisional names (their owning module is not established).
// Cursor X moves opposite to the raw OS delta while Y moves with it, exactly as disassembled:
// new_x = x + (prev_x - new_x), new_y = y - (prev_y - new_y) (0x49736e..0x497370).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t ui_use_os_cursor;      // 0x00718f81
extern int32_t previous_mouse_x;      // 0x006b2f24, last frame's OS cursor x
extern int32_t previous_mouse_y;      // 0x006b2f20, last frame's OS cursor y
extern int32_t ui_cursor_x;           // 0x00718f84
extern int32_t ui_cursor_y;           // 0x00718f88

extern int32_t mouse_device;    // 0x006b1804, UNSURE: nonzero selects a raw delta source
extern uint8_t input_suppressed;   // 0x006b15f9, UNSURE: picks between the two records below
extern int32_t mouse_neutral_state[2];      // 0x006b1828, UNSURE: raw {dx, dy} device delta
extern int32_t live_mouse_state[2];      // 0x006b180c, UNSURE: raw {dx, dy} device delta
extern float cursor_sensitivity_x;            // 0x0068e674, UNSURE: X-axis curve coefficient
extern float cursor_sensitivity_y;            // 0x0068e678, UNSURE: Y-axis curve coefficient
extern double cursor_sensitivity_curve_scale; // 0x00672da8, UNSURE: shared quadratic coefficient
extern double cursor_sensitivity_curve_bias;  // 0x00672af8, UNSURE: shared additive constant

extern void interface_update_for_resolution_change(int32_t new_cursor_x, int32_t new_cursor_y); // 0x497250
extern int32_t __ftol(double x); // 0x6391b4, MSVC float-to-long (truncating)

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
        double raw_x, raw_y, scaled_x, scaled_y;

        // 0x4972fd..0x497317: the original starts EDI at 0 and only loads a record when mouse_device is nonzero
        // (a null read otherwise); the live record is kept as the default here so the rewrite cannot fault.
        record = live_mouse_state;
        if (mouse_device != 0) {
            record = mouse_neutral_state;
            if (input_suppressed == 0) {
                record = live_mouse_state;
            }
        }
        // Everything stays in x87 extended precision in the original (fild / fabs / fmul / fadd / fmul), so double is used.
        raw_x = (double)record[0];
        scaled_x = (raw_x < 0.0 ? -raw_x : raw_x) * (double)cursor_sensitivity_x * cursor_sensitivity_curve_scale +
            cursor_sensitivity_curve_bias;
        delta_x = __ftol(scaled_x * raw_x);
        raw_y = (double)record[1];
        scaled_y = (raw_y < 0.0 ? -raw_y : raw_y) * (double)cursor_sensitivity_y * cursor_sensitivity_curve_scale +
            cursor_sensitivity_curve_bias;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
