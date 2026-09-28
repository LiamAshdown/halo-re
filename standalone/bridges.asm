; standalone/bridges.asm -- the few symbols whose meaning differs between the standalone exe and the hook harness
; (harness/, where the rewritten C runs inside the retail game).
;
; D3DXCreateEffect: the game creates its effects through the 2003 D3DX statically linked into the original exe; the
;   standalone uses the June 2010 D3DX instead, through standalone/d3dx_compat.c, which wraps the effect in the 2003
;   interface layout the C calls.
; code_address_<fn>: C that hands a function's address to Windows (an APC, a window procedure) or to the I/O queue
;   declares code_address_<fn>; the harness binds it to the original function's address, the standalone to the C
;   function -- through a stdcall adapter where the C is cdecl but the caller pops like the original (ret 12).

.386
.model flat
option casemap:none

EXTERN _standalone_d3dx_create_effect:PROC
EXTERN _cache_io_completion_routine:PROC
EXTERN _cache_io_sound_decode_thunk:PROC
EXTERN _shell_window_procedure@16:PROC

.code

PUBLIC _D3DXCreateEffect
_D3DXCreateEffect:
    jmp _standalone_d3dx_create_effect

; an I/O completion routine (0x443b00 ends ret 12): the C is cdecl
PUBLIC _code_address_cache_io_completion_routine
_code_address_cache_io_completion_routine:
    push ebp
    mov ebp, esp
    push dword ptr [ebp+16]
    push dword ptr [ebp+12]
    push dword ptr [ebp+8]
    call _cache_io_completion_routine
    add esp, 12
    pop ebp
    ret 12

PUBLIC _code_address_cache_io_sound_decode_thunk
_code_address_cache_io_sound_decode_thunk:
    jmp _cache_io_sound_decode_thunk

PUBLIC _code_address_shell_window_procedure
_code_address_shell_window_procedure:
    jmp _shell_window_procedure@16

END
