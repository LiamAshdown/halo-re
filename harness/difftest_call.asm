; difftest_call(target, regs[6], stack_words, n_words, float_return, out[4])
; Loads eax ecx edx ebx esi edi from regs, pushes n_words stack words (stack_words[0] ends up at [esp+4] of the
; callee), calls target, and stores eax, edx and (if float_return) ST0 as a double into out. Works for callee- and
; caller-cleaned functions alike (esp is restored from ebp).
.386
.model flat, c
option casemap:none
.data
dt_target DD 0
dt_regs   DD 6 dup(0)
.code
PUBLIC difftest_call
difftest_call PROC
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    mov eax, [ebp+8]
    mov dt_target, eax
    mov esi, [ebp+12]
    mov ecx, 6
    lea edi, dt_regs
    rep movsd
    mov ecx, [ebp+20]
    mov edx, [ebp+16]
pushloop:
    test ecx, ecx
    jz pushed
    dec ecx
    push dword ptr [edx+ecx*4]
    jmp pushloop
pushed:
    mov eax, dt_regs[0]
    mov ecx, dt_regs[4]
    mov edx, dt_regs[8]
    mov ebx, dt_regs[12]
    mov esi, dt_regs[16]
    mov edi, dt_regs[20]
    call dword ptr [dt_target]
    lea esp, [ebp-12]
    mov ecx, [ebp+28]
    mov [ecx], eax
    mov [ecx+4], edx
    cmp dword ptr [ebp+24], 0
    je nofloat
    fstp qword ptr [ecx+8]
nofloat:
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
difftest_call ENDP
END
