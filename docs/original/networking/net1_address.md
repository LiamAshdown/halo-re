# Original notes: networking `net1_address`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_address sources.

## network_address_parse_port.c

```
// network_address_parse_port  (Ghidra: network_address_parse_port, already named)
// address 0x4dc560, size 119 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Parses and range-checks a numeric port
// substring (after a delimiter) out of an address string, optionally writing the value through
// an output pointer." Ghidra fully recovered the cdecl `port_out` stack parameter and the
// control flow; only the address-string input is an elided register argument.
// register convention: address string in EAX (in_EAX, elided from Ghidra's own signature).
// blam-cc: EAX -> address_string, stack -> port_out
```

```
#if 0
Original Ghidra decompilation (0x4dc560):

char __cdecl network_address_parse_port(long *port_out)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  long lVar5;
  char *pcVar6;

  bVar1 = true;
  iVar3 = FUN_006257e0();
  if (iVar3 == 0) {
    return '\x01';
  }
  pcVar6 = (char *)(iVar3 + 1);
  while (*pcVar6 != '\0') {
    iVar4 = _isdigit((int)*pcVar6);
    if (iVar4 == 0) {
      bVar1 = false;
    }
    pcVar6 = pcVar6 + 1;
    if (!bVar1) {
      return '\0';
    }
  }
  if (!bVar1) {
    return '\0';
  }
  lVar5 = _atol((char *)(iVar3 + 1));
  if ((lVar5 < 1) || (0xffff < lVar5)) {
    cVar2 = '\0';
  }
  else {
    cVar2 = '\x01';
  }
  if (port_out == (long *)0x0) {
    return cVar2;
  }
  *port_out = lVar5;
  return cVar2;
}
#endif
```

## network_address_string_is_valid.c

```
// network_address_string_is_valid  (Ghidra: FUN_004dc730; named per this rewrite)
// address 0x4dc730, size 91 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Validates that a user-supplied string looks
// like a legal IP address or hostname, optionally with a port suffix." Matches the code: if the
// string is non-empty and does NOT already parse as a normalized dotted address, every character
// must be '.', '-', ':' or alphanumeric.
// register convention: address string in EAX (in_EAX). blam-cc: EAX -> address_string
```

```
#if 0
Original Ghidra decompilation (0x4dc730):

char FUN_004dc730(void)

{
  char cVar1;
  char cVar2;
  char *in_EAX;
  int iVar3;
  char local_1c [28];

  cVar1 = '\0';
  if ((*in_EAX != '\0') &&
     (cVar1 = network_address_string_normalize(in_EAX,local_1c,(uchar *)0x0), cVar1 == '\0')) {
    cVar2 = '\x01';
    do {
      cVar1 = *in_EAX;
      if (cVar1 == '\0') {
        return cVar2;
      }
      if ((((cVar1 == '.') || (cVar1 == '-')) || (cVar1 == ':')) ||
         (iVar3 = _isalnum((int)cVar1), iVar3 != 0)) {
        cVar2 = '\x01';
      }
      else {
        cVar2 = '\0';
      }
      in_EAX = in_EAX + 1;
      cVar1 = '\0';
    } while (cVar2 != '\0');
  }
  return cVar1;
}
#endif
```

## network_address_string_normalize.c

```
// network_address_string_normalize  (Ghidra: network_address_string_normalize, already named)
// address 0x4dc5e0, size 327 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Parses a dotted-decimal IP address string (with
// an optional trailing port) and writes back a canonical 'a.b.c.d' or 'a.b.c.d:port' string,
// also reporting whether the address is the wildcard 0.0.0.0." Ghidra fully recovered the cdecl
// (address_string, out_buffer, out_is_any) signature and control flow.
// UNSURE: the sscanf format string additionally captures a 5th numeric field
// ("%d.%d.%d.%d.%d") that is never read back (local_4); this looks like it exists only so a
// dot-delimited 5th field does not make the match count exceed 4, and is preserved verbatim.
```

```
#if 0
Original Ghidra decompilation (0x4dc5e0):

char __cdecl
network_address_string_normalize(char *address_string,char *out_buffer,uchar *out_is_any)

{
  uchar uVar1;
  char cVar2;
  int iVar3;
  int local_18;
  int local_14;
  int local_10;
  int local_c;
  long local_8;
  undefined1 local_4 [4];

  cVar2 = '\0';
  local_8 = -1;
  iVar3 = _sscanf(address_string,"%d.%d.%d.%d.%d",&local_c,&local_10,&local_14,&local_18,local_4);
  if (((((iVar3 == 4) && (-1 < local_c)) && (local_c < 0x100)) &&
      (((-1 < local_10 && (local_10 < 0x100)) &&
       ((-1 < local_14 && ((local_14 < 0x100 && (-1 < local_18)))))))) && (local_18 < 0x100)) {
    uVar1 = '\0';
    if ((((local_c == 0) && (local_10 == 0)) && (local_14 == 0)) && (local_18 == 0)) {
      uVar1 = '\x01';
    }
    if (out_is_any != (uchar *)0x0) {
      *out_is_any = uVar1;
    }
    cVar2 = network_address_parse_port(&local_8);
    if (cVar2 == '\x01') {
      if (local_8 != -1) {
        ___snprintf(out_buffer,0x19,"%d.%d.%d.%d:%d",local_c,local_10,local_14,local_18,local_8);
        return '\x01';
      }
      ___snprintf(out_buffer,0x19,"%d.%d.%d.%d",local_c,local_10,local_14,local_18);
    }
  }
  return cVar2;
}
#endif
```

## network_address_to_string.c

```
// network_address_to_string  (Ghidra: network_address_to_string, already named)
// address 0x440570, size 149 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: out/phase4/networking_types_notes.md "s_network_address (0x14)" -- this function
// is the entire evidence base for that struct: it reads addr[8] (halfword) as `size` and, for
// the IPv4 case, prints bytes +3,+2,+1,+0 of the address as %hd.%hd.%hd.%hd (high byte
// first) and addr[9] as the port.
// register convention: EAX -> addr, no stack arguments.
// FIXED (register inputs, objdump): notes wording ("address pointer in EAX") didn't match the
// checker's alias for s_network_address*, so EAX (read at 0x440577) looked unclaimed. Body
// already used addr correctly; reworded the blam-cc line to the standard "REG -> name" form.
```

```
#if 0
Original Ghidra decompilation (0x440570):

undefined * network_address_to_string(void)

{
  ushort *in_EAX;

  DAT_006a3f38 = 0;
  if (in_EAX[8] == 4) {
    __snprintf(&DAT_006a3f38,0x100,"%hd.%hd.%hd.%hd:%hu",(uint)*(byte *)((int)in_EAX + 3),
               (uint)(byte)in_EAX[1],(uint)*(byte *)((int)in_EAX + 1),(uint)(byte)*in_EAX,
               (uint)in_EAX[9]);
    return &DAT_006a3f38;
  }
  if (in_EAX[8] == 0x10) {
    __snprintf(&DAT_006a3f38,0x100,"%4X.%4X.%4X.%4X.%4X.%4X.%4X.%4X:%hu",(uint)*in_EAX,
               (uint)in_EAX[1],(uint)in_EAX[2],(uint)in_EAX[3],(uint)in_EAX[4],(uint)in_EAX[5],
               (uint)in_EAX[6],(uint)in_EAX[7],(uint)in_EAX[9]);
  }
  return &DAT_006a3f38;
}
#endif
```
