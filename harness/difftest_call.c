/* difftest_call(target, regs[6], stack_words, n_words, float_return, out[4])
 *
 * Loads eax ecx edx ebx esi edi from regs, pushes n_words stack words (stack_words[0] ends up at [esp+4] of the callee),
 * calls target, and stores eax, edx and (if float_return) ST0 as a double into out. Works for callee- and caller-cleaned
 * functions alike (esp is restored from ebp). The register-exact call needs inline assembly; this is the only place. */

static unsigned long dt_target;
static unsigned long dt_regs[6];

__declspec(naked) void difftest_call(void *target, unsigned long regs[6], unsigned long *stack, int n, int float_return,
                                     unsigned long out[4])
{
    __asm {
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
        mov eax, dword ptr [dt_regs+0]
        mov ecx, dword ptr [dt_regs+4]
        mov edx, dword ptr [dt_regs+8]
        mov ebx, dword ptr [dt_regs+12]
        mov esi, dword ptr [dt_regs+16]
        mov edi, dword ptr [dt_regs+20]
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
    }
}
