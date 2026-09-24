// physics_shape_forward_call_helper  (Ghidra: FUN_00501470, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x501470, size 47 bytes
// name confidence: 0.15   rewrite confidence: 0.3
// evidence: out/phase2/results/physics_00.json -- a two-line thunk with no globals of its own,
//   calling FUN_0044d860 (a projection helper belonging to a different module, see
//   FUN_0044d860's own header comment there) purely for its side effect and returning its
//   second argument unchanged. Seen live at 0x55ab30 as `FUN_00501470(1,&local_c)`, where the
//   second argument is the address of a 3-float scratch buffer.
// register convention: none -- both arguments are Ghidra-recognized stack parameters.
// UNSURE: param_1's role could not be determined from this file alone (it is never read here);
//   the exact type of param_2 is unresolved without following FUN_0044d860 into its own module.

#include "tags.h"
#include "memory.h"

extern void FUN_0044d860(void *param); // 0x44d860, not physics (module unresolved between
                                        // effects and math); UNSURE, see file header

void *physics_shape_forward_call_helper(uint32_t param_1, void *param_2)
{
    FUN_0044d860(param_2);
    return param_2;
}

#if 0
Original Ghidra decompilation (0x501470):

undefined4 FUN_00501470(undefined4 param_1,undefined4 param_2)

{
  FUN_0044d860(param_2);
  return param_2;
}
#endif
