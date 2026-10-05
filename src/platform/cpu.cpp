/**
 * @file src/platform/cpu.cpp
 * halo/platform/cpu.hpp: the x86 instructions on 32-bit x86 (MSVC or GCC-style inline assembly), portable stand-ins
 * elsewhere.
 */

#include "halo/platform/cpu.hpp"

#if defined(_M_IX86) && defined(_MSC_VER)
#include <float.h>
#endif
#if !defined(_M_IX86) && !defined(__i386__)
#include <chrono>
#endif

namespace halo::platform {

#if defined(_M_IX86) && defined(_MSC_VER)

void fpu_reset(uint16_t control_word)
{
    __asm { finit }
    __asm { fldcw control_word }
}

void fpu_control(uint32_t value, uint32_t mask)
{
    _control87(value, mask);
}

bool cpuid(uint32_t leaf, uint32_t registers[4])
{
    uint32_t original_flags;
    uint32_t readback_flags;
    uint32_t a, b, c, d;

    // CPUID exists when the EFLAGS.ID bit can be toggled
    __asm {
        pushfd
        pop eax
        mov original_flags, eax
        xor eax, 0x200000
        push eax
        popfd
        pushfd
        pop eax
        mov readback_flags, eax
        push original_flags
        popfd
    }
    if ((readback_flags ^ original_flags) == 0) {
        return false;
    }
    __asm {
        mov eax, leaf
        cpuid
        mov a, eax
        mov b, ebx
        mov c, ecx
        mov d, edx
    }
    registers[0] = a;
    registers[1] = b;
    registers[2] = c;
    registers[3] = d;
    return true;
}

uint64_t time_stamp_counter()
{
    uint32_t tsc_low, tsc_high;

    __asm {
        rdtsc
        mov tsc_low, eax
        mov tsc_high, edx
    }
    return (static_cast<uint64_t>(tsc_high) << 32) | tsc_low;
}

#elif defined(__i386__)

void fpu_reset(uint16_t control_word)
{
    __asm__ __volatile__("finit\n\tfldcw %0" : : "m"(control_word));
}

void fpu_control(uint32_t value, uint32_t mask)
{
    (void)value;  // ponytail: GCC x86 builds keep the default control word; add the fnstcw/fldcw mapping if one ships
    (void)mask;
}

bool cpuid(uint32_t leaf, uint32_t registers[4])
{
    __asm__ __volatile__("cpuid" : "=a"(registers[0]), "=b"(registers[1]), "=c"(registers[2]), "=d"(registers[3]) : "a"(leaf), "c"(0));
    return true;  // every i686 target has CPUID
}

uint64_t time_stamp_counter()
{
    uint32_t tsc_low, tsc_high;

    __asm__ __volatile__("rdtsc" : "=a"(tsc_low), "=d"(tsc_high));
    return (static_cast<uint64_t>(tsc_high) << 32) | tsc_low;
}

#else

void fpu_reset(uint16_t control_word)
{
    (void)control_word;
}

void fpu_control(uint32_t value, uint32_t mask)
{
    (void)value;
    (void)mask;
}

bool cpuid(uint32_t leaf, uint32_t registers[4])
{
    (void)leaf;
    (void)registers;
    return false;
}

uint64_t time_stamp_counter()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

#endif

}  // namespace halo::platform
