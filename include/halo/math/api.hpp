/**
 * @file include/halo/math/api.hpp
 * The public C++ API of the math module: every function other modules call (namespace halo::math, vectors and
 * planes by reference) plus the Globals service object.
 */
#pragma once

#include "halo/math/globals.hpp"
#include "halo/math/math.hpp"

namespace halo::math {

/** Evaluates a periodic function by the engine's integer function id (the tag field values). */
inline real periodic_function_evaluate(int type, double time)
{
    return periodic_function_evaluate(static_cast<periodic_function_type>(type), time);
}

/** Evaluates a transition function by the engine's integer function id (the tag field values). */
inline real transition_function_evaluate(int type, real phase)
{
    return transition_function_evaluate(static_cast<transition_function_type>(type), phase);
}

/** Builds a transition function lookup table by the engine's integer function id. */
inline void periodic_function_build_transition_table(int type, uint8_t *table)
{
    periodic_function_build_transition_table(static_cast<transition_function_type>(type), table);
}

/** Builds a periodic function lookup table by the engine's integer function id. */
inline void periodic_function_build_table(int type, uint8_t *out)
{
    periodic_function_build_table(static_cast<periodic_function_type>(type), out);
}

}  // namespace halo::math
