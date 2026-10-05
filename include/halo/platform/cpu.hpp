/**
 * @file include/halo/platform/cpu.hpp
 * Processor-level operations the engine performed with x86 instructions (FNINIT/FLDCW, CPUID, RDTSC). On x86
 * src/platform/cpu.cpp runs exactly those instructions; elsewhere (WebAssembly) they report no x87 unit and no CPUID.
 */
#pragma once

#include <cstdint>

namespace halo::platform {

/** Resets the x87 unit and loads control_word (FNINIT; FLDCW); does nothing where there is no x87 unit. */
void fpu_reset(uint16_t control_word);

/** Sets the x87 control word bits in mask to value, as the C library's _control87; does nothing where there is no x87 unit. */
void fpu_control(uint32_t value, uint32_t mask);

/** Executes CPUID for leaf into registers (EAX, EBX, ECX, EDX); false when the processor has no CPUID instruction. */
bool cpuid(uint32_t leaf, uint32_t registers[4]);

/** The processor's time-stamp counter (RDTSC); a nanosecond clock where there is none. */
uint64_t time_stamp_counter();

}  // namespace halo::platform
