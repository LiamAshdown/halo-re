// config_set_decal_z_bias  (Ghidra: FUN_0057d2b0; renamed per out/phase4/shell_types_notes.md)
// address 0x57d2b0, size 36 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/shell_types_notes.md: "FUN_0057d2b0 / 2e0 / 310 / 340 are the
//   DecalZBias / DecalSlopeZBias / TransparentDecalZBias / TransparentDecalSlopeZBias setters
//   (floats at 0x00722b80 / 88 / 84 / 8c)." This one writes 0x00722b80 = config_decal_z_bias.
// register convention: config.txt property setter; value string is a plain stack parameter.
// blam-cc: (value on the stack, matching Ghidra's recognized parameter)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t sscanf(const char *buffer, const char *format, ...); // 0x626572 CRT
extern float config_decal_z_bias; // 0x00722b80

// config.txt "DecalZBiasValue" setter: parses a float and stores it unconditionally.
uint8_t config_set_decal_z_bias(char *value)
{
    sscanf(value, "%f", &config_decal_z_bias);
    return 1;
}

#if 0
Original Ghidra decompilation (0x57d2b0):

undefined4 FUN_0057d2b0(char *param_1)

{
  undefined4 local_4;

  _sscanf(param_1,"%f",&local_4);
  DAT_00722b80 = local_4;
  return 1;
}
#endif
