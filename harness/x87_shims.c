/* Single x87 instructions that Ghidra shows as calls (fsin, fcos, ...) and that the rewritten files declare as
   externs. Each is the one instruction, so results match the original bit for bit (same 80-bit unit, same rounding). */
#include <stdint.h>

double fsin(double x)  { double r; __asm { fld x } __asm { fsin } __asm { fstp r } return r; }
double fcos(double x)  { double r; __asm { fld x } __asm { fcos } __asm { fstp r } return r; }
double ftan(double x)  { double r; __asm { fld x } __asm { fptan } __asm { fstp st(0) } __asm { fstp r } return r; }
/* FPATAN: atan2(ST1 = y, ST0 = x) */
double fpatan(double y, double x) { double r; __asm { fld y } __asm { fld x } __asm { fpatan } __asm { fstp r } return r; }
float fabsf(float x) { float r; __asm { fld x } __asm { fabs } __asm { fstp r } return r; }
/* ROUND / fistp_round: FISTP in the current rounding mode (the engine leaves it at round-to-nearest) */
int32_t ROUND(float x) { int32_t r; __asm { fld x } __asm { fistp r } return r; }
int32_t fistp_round(float x) { int32_t r; __asm { fld x } __asm { fistp r } return r; }
