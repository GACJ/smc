#ifdef __TESTBED__

#include "smc.h"

static short cmpflh[6];
static short cmpmus8 = 0;
static Node testnode;
static char extranodespace[6 * 4];

int testbed(int test)
{
    int cyclelo, ncycles = 0;

    if (test == 0)
    {
        // clang-format off
        __asm
        {
            lea esi,[testnode]
            mov ecx,6
            mov [esi]Node.nfalsenodes,ecx
            mov [esi]Node.included,0
        fill0:  mov [esi+ecx*4-4]Node.falsenodes,esi
            loop    fill0
            mov edi,6
            mov ebx,1000
        tloop0: mov ecx,0
            rdtsc
            mov [cyclelo],eax
            mov eax,esi
        floop0:
        //  cmp ecx,[esi]Node.nfalsenodes
            cmp ecx,edi
            je  fnext0
            mov eax,[esi+ecx*4]Node.falsenodes
            inc ecx
            cmp [eax]Node.included,0
        //  je  floop
            jmp floop0
        fnext0: rdtsc
            sub eax,[cyclelo]
            mov [ncycles],eax
            dec ebx
            jnz tloop0
        }
        // clang-format on
    }
    else
    {
        // clang-format off
        __asm
        {
            push    es
            push    gs
            mov ax,ds
            mov gs,ax
            mov ecx,6
        fill1:  mov [cmpflh+ecx*2-2],ax
            loop    fill1
            mov ebx,1000
        tloop1: mov ecx,6
            rdtsc
            mov [cyclelo],eax
        floop1: mov es,gs:[cmpflh+ecx*2-2]
            test    es:[cmpmus8],1
            jnz tdone1
            loop    floop1
            rdtsc
            sub eax,[cyclelo]
            mov [ncycles],eax
            dec ebx
            jnz tloop1
        tdone1: pop gs
            pop es
            pop ebx
            pop ebp
        }
        // clang-format on
    }
    return (ncycles);
}

#endif // __TESTBED__
