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

/* MSVC 7.1 runtime helpers that some rewrites call by name (they were visible in Ghidra's output). The real ones
   take their input on the x87 stack (__ftol) or use a stdcall-like convention (__alldiv/__allmul), so a C call
   would pass garbage; these take the arguments the rewrites declare and compute the same result. */
int32_t __ftol(double x) { return (int32_t)(long long)x; }                     /* chop toward zero, low dword */
long long __allmul(int32_t a_low, int32_t a_high, int32_t b_low, int32_t b_high)
{
    return (long long)(((unsigned long long)(unsigned long)a_high << 32) | (unsigned long)a_low) *
           (long long)(((unsigned long long)(unsigned long)b_high << 32) | (unsigned long)b_low);
}
int32_t __alldiv(long long a, int32_t b_low, int32_t b_high)
{
    long long b = (long long)(((unsigned long long)(unsigned long)b_high << 32) | (unsigned long)b_low);
    return b ? (int32_t)(a / b) : 0;
}
