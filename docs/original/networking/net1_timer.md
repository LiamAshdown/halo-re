# Original notes: networking `net1_timer`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_timer sources.

## network_timer_advance.c

```
// network_timer_advance  (Ghidra: FUN_004deb50; named per this rewrite)
// address 0x4deb50, size 93 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Advances a millisecond timer pair
// (unaff_ESI[0..1]) using the high-resolution performance counter, subtracting elapsed time
// from a remaining-time field." A small, generic 2-dword [remaining_ms, last_tick_ms] record,
// used by network_timer_start.c (0x4debf0) and this batch's other timer helpers; not tied to any
// header-declared struct.
// register convention: timer in ESI (unaff_ESI). blam-cc: ESI -> timer
```

```
#if 0
Original Ghidra decompilation (0x4deb50):

void FUN_004deb50(void)

{
  int iVar1;
  int iVar2;
  int *unaff_ESI;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar2 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  iVar1 = unaff_ESI[1];
  unaff_ESI[1] = iVar2;
  if (iVar1 < iVar2) {
    iVar2 = iVar2 - iVar1;
    if (iVar2 < *unaff_ESI) {
      *unaff_ESI = *unaff_ESI - iVar2;
      return;
    }
    *unaff_ESI = 0;
  }
  return;
}
#endif
```

## network_timer_decrement_floored.c

```
// network_timer_decrement_floored  (Ghidra: FUN_004debd0; named per this rewrite)
// address 0x4debd0, size 28 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Register-based helper that advances the shared
// timer via network_timer_advance then decrements *in_EAX by unaff_EDI, floored at zero." Uses the same
// [remaining_ms, last_tick_ms] layout as network_timer_advance.c.
// register convention: timer in EAX (in_EAX), decrement in EDI (unaff_EDI). blam-cc: EAX ->
// timer, EDI -> decrement
```

```
#if 0
Original Ghidra decompilation (0x4debd0):

void FUN_004debd0(void)

{
  int *in_EAX;
  int unaff_EDI;

  FUN_004deb50();
  if (unaff_EDI < *in_EAX) {
    *in_EAX = *in_EAX - unaff_EDI;
    return;
  }
  *in_EAX = 0;
  return;
}
#endif
```

## network_timer_increment_clamped.c

```
// network_timer_increment_clamped  (Ghidra: FUN_004debb0; named per this rewrite)
// address 0x4debb0, size 32 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Register-based helper that advances the shared
// timer via network_timer_advance then adds an increment to *in_EAX, clamping the result to an upper
// bound." Uses the same [remaining_ms, last_tick_ms] layout as network_timer_advance.c; `timer`
// is passed straight through to that call.
// register convention: timer in EAX (in_EAX), increment in EDI (unaff_EDI), upper bound in EBX
// (unaff_EBX). blam-cc: EAX -> timer, EBX -> upper_bound, EDI -> increment
```

```
#if 0
Original Ghidra decompilation (0x4debb0):

void FUN_004debb0(void)

{
  int *in_EAX;
  int iVar1;
  int unaff_EBX;
  int unaff_EDI;

  FUN_004deb50();
  iVar1 = *in_EAX + unaff_EDI;
  if (iVar1 < unaff_EDI) {
    *in_EAX = unaff_EBX;
    return;
  }
  *in_EAX = iVar1;
  if (unaff_EBX < iVar1) {
    iVar1 = unaff_EBX;
  }
  *in_EAX = iVar1;
  return;
}
#endif
```

## network_timer_start.c

```
// network_timer_start  (Ghidra: FUN_004debf0; named per this rewrite)
// address 0x4debf0, size 68 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Initialises a timer pair pointed to by
// unaff_ESI with the current performance-counter time and the requested duration parameter."
// Uses the same [remaining_ms, last_tick_ms] layout as network_timer_advance.c.
// register convention: timer in ESI (unaff_ESI). blam-cc: ESI -> timer, stack -> duration_ms
```

```
#if 0
Original Ghidra decompilation (0x4debf0):

void FUN_004debf0(undefined4 param_1)

{
  undefined4 uVar1;
  undefined4 *unaff_ESI;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  *unaff_ESI = param_1;
  unaff_ESI[1] = uVar1;
  return;
}
#endif
```
