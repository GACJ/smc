// Include file used to create various versions of the composing loop
// Copyright Mark B Davies 1998-2000

#include "smc.h"

// How many million nodes between each showstats() call
#define SHOWSTATSFREQ 2

#define STARTTIMEBT
#define STOPTIMEBT
#define STARTTIMEC
#define STOPTIMEC
#define STARTTIMEF
#define STOPTIMEF

template<
    bool PALINDROMIC,
    bool MMXCOUNTER,
    bool REGENERATION,
    bool MMXFRAGOPT,
    bool CALLCOUNT,
    bool MULTIPART,
    bool FALSEBITS>
void composeloop(Composer& composer)
{
    Composition *compptr, *edi;
    Node* esi;
    int eax, ecx, ebp;
    long long mm0, mm1, mm2;

    auto& ebx = composer;

#ifdef CYCLETIMER
    composer.ncycles = 10000;
#endif

    if (PALINDROMIC)
    {
        if (composer.ncompnodes & 1)
        {
            composer.midnodeapex = TRUE;
            composer.ncompnodes /= 2;
            // Subtract half node length
            composer.complength -= composer.comp[composer.ncompnodes].node->nrows / 2;
        }
        else
        {
            composer.midnodeapex = FALSE;
            composer.ncompnodes = (composer.ncompnodes + 1) / 2;
        }
    }

    composer.lengthcountdown = composer.maxpalilength - composer.complength;
    compptr = &composer.comp[composer.ncompnodes];
    if (composer.ncompnodes)
    {
        // Need to count up calls already used in the restarted composition
        if (composer.countingcalls)
        {
            for (int i = 0; i < composer.ncompnodes; i++)
            {
                composer.ncallsleft[composer.comp[i].call]--;
                composer.ncallsleftperpos[composer.comp[i + 1].node->callcountposindex / NDIFFCALLS][composer.comp[i].call]--;
            }
        }
    }
    else
    {
        // ASM block 1
        edi = compptr;
        if (MMXCOUNTER)
        {
            eax = 1;
            mm1 = eax;
            mm0 = 0;
        }

        // auto& ebx = composer;
        ecx = compptr->call;
        ebp = composer.lengthcountdown;
        if (REGENERATION)
        {
            composer.lastregen = edi;
        }
        esi = edi->node;
        if (MMXFRAGOPT)
        {
            // The call pattern mm2 is initially set to 0.
            // Entries beyond the start of the composition will all be Plain
            // It will not match with any duplicate, because all duplicates start with a call
            mm2 = 0;
#ifdef STOREPATT
            edi->pattern = mm2;
#endif
        }
        goto composeloop;
    }

    int edx;
    while (true)
    {
        //  eax = transient
        //  ebx = this
        //  ecx = call / falsenode index
        //  edx = transient
        //  esi = current node
        //  edi = compptr
        //  ebp = maxpalilength - current complength
        // For MMX routines:
        //  mm0 = nodecount
        //  mm1 = 1
        //  mm2 = current call pattern
        //  mm6 = transient
        //  mm7 = transient
        // reenter:
        edi = compptr;
        if (MMXCOUNTER)
        {
            eax = 1;
            mm1 = 1;
        }
        // ebx = composer;
        ebp = ebx.lengthcountdown;
        if (MMXCOUNTER)
        {
            mm0 = ebx.stats.nodecount;
        }
        esi = edi->node;
#ifdef STOREPATT
        if (MMXFRAGOPT)
        {
            mm2 = edi->pattern;
        }
#endif
    backtrackfromrounds:
        // Check if more than SHOWSTATSFREQ million nodes since last showstats()
        if (MMXCOUNTER)
        {
            edx = (int)mm0;
        }
        else
        {
            edx = ebx.stats.nodecount;
        }
        if (edx < SHOWSTATSFREQ * 1000000)
        {
            goto backtrack;
        }
        // Drop out to showstats()
        eax = ebx.maxpalilength - ebp;
        ebx.complength = eax;
        ebx.lengthcountdown = ebp;
        ebx.stats.nodecount = edx;
        // compptr = edi;
        if (MMXCOUNTER)
        {
            // emms
        }
        composer.ncompnodes = compptr - composer.comp;
        composer.showstats();
    }
asmdone:
    if (MMXCOUNTER)
        edx = (int)mm0;
    else
        edx = ebx.stats.nodecount;
    ebx.stats.nodesgenerated += edx;
    if (MMXCOUNTER)
    {
        // emms
    }
    return;

lbl_loadcallnextcall:
    ecx = edi->call;
    if (CALLCOUNT)
    {
        edx = ecx;
        ebx.ncallsleft[ecx]++;
        edx += esi->callcountposindex;
        ebx.ncallsleftperpos[0][edx]++;
    }

lbl_sublennextcall:
    ebp += esi->nrows;

lbl_nextcall:
    esi = edi->node;

    if (MULTIPART)
    {
        if (ebp < ebx.maxpartlength)
            goto backtrack;
    }
#ifdef REVERSECALLS
    ecx--;
    if (ecx >= 0)
        goto regen;
#else
    ecx++;
    if (ecx <= ebx.ncalltypes)
        goto regen;
#endif

backtrack:
    STARTTIMEBT;

    if (FALSEBITS)
    {
        esi->falsebits->tableptr[0] ^= esi->falsebits->tablemask;
    }
    else
    {
#ifdef UNVISITABLE
        esi->unvisitable = 0; // Reset unvisitable flag
        // Decremement unvisitability of false nodes
        edx = esi->nfalsenodes;
        if (edx != 0)
        {
            ecx = 0;
            while (ecx != edx)
            {
                esi->falsenodes[ecx]->unvisitable--;
                ecx++;
            }
        }
#else
        esi->included = 0; // Reset included flag
#endif
    }
    edi--; // All compositions found?
    Node* eax2;
    if (PALINDROMIC)
    {
        eax2 = edi->palinode;
    }
    if (edi < ebx.comp)
        return;
    if (PALINDROMIC)
    {
        if (FALSEBITS)
        {
            eax2->falsebits->tableptr[0] ^= eax2->falsebits->tablemask;
        }
        else
        {
            eax2->included = 0; // Reset palindromic included flag
        }
    }
    ecx = edi->call;
    ebp += esi->nrows; // Reduce composition length
    if (CALLCOUNT)
    {
        edx = ecx;
        ebx.ncallsleft[ecx]++;
        esi->callcountposindex += edx;
        ebx.ncallsleftperpos[0][edx]++;
    }
    esi = edi->node; // Load previous lead
    if (MULTIPART)
    {
        if (ebx.maxpartlength == ebp)
            goto backtrack;
    }

lbl_nextcall2:
#ifdef REVERSECALLS
    ecx--;
    if (ecx < 0)
        goto backtrack;
#else
    ecx++;
    if (ecx > composer.ncalltypes)
        goto backtrack;
#endif

    if (MULTIPART && CALLCOUNT)
    {
        // Before re-entering the composing loop, check maximum number of calls per part not exceeded.
        // Note that this test requires the next-node pointer, so we MUST test for allowed call first!
        // (Normally done in composing loop)
        if (esi->nextnode[ecx] == 0)
            goto lbl_nextcall2;

        if (ebx.ncallsleft[ecx] < ebx.npartcalls[ecx])
            goto lbl_nextcall2;

        edx = ecx + esi->nextnode[ecx]->callcountposindex;
        if (ebx.ncallsleftperpos[edx] < ebx.npartcallsperpos[edx])
            goto lbl_nextcall2;
    }

    STOPTIMEBT;

// TODO, line 257
regen:
composeloop:;
}

void test(Composer& c)
{
    composeloop<true, true, true, true, true, true, true>(c);
}
