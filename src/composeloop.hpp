// Include file used to create various versions of the composing loop
// Copyright Mark B Davies 1998-2000

#ifndef __ASM_IMPL__

#include "smc.h"

// Both the following optimisations appear to make cycle performance in the inner
// loop on the Cyrix chip WORSE. However, overall performance is still improved!!
// MMXCOUNTER saves 2 cycles/loop on a Pentium
// !! Now moved to individual composing loops
//#define MMXCOUNTER
// EXPANDFALSELOOP save (?) cycles on a Pentium
#define EXPANDFALSELOOP

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
struct composeloop
{
    static void run(Composer& composer)
    {
        Composition *compptr, *edi;
        Node* esi;
        int eax, ecx, ebp;
        // long long mm0, mm1, mm2, mm7;

        auto& ebx = composer;

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
            goto reenter;
        }

        edi = compptr;
        // auto& ebx = composer;
        ecx = compptr->call;
        ebp = composer.lengthcountdown;
        if (REGENERATION)
        {
            composer.lastregen = edi;
        }
        esi = edi->node;
        goto composeloop;

    whileloop:
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
    reenter:
        edi = compptr;
        // ebx = composer;
        ebp = ebx.lengthcountdown;
        esi = edi->node;
    backtrackfromrounds:
        // Check if more than SHOWSTATSFREQ million nodes since last showstats()
        auto edx1 = ebx.stats.nodecount;
        if (edx1 < SHOWSTATSFREQ * 1000000)
        {
            goto backtrack;
        }
        // Drop out to showstats()
        eax = ebx.maxpalilength - ebp;
        ebx.complength = eax;
        ebx.lengthcountdown = ebp;
        ebx.stats.nodecount = edx1;
        compptr = edi;
        composer.ncompnodes = (int)(compptr - composer.comp);
        composer.showstats();
        goto reenter;

    asmdone:
        ebx.stats.nodesgenerated += ebx.stats.nodecount;
        goto alldone;

    lbl_loadcallnextcall:
        ecx = edi->call;

    lbl_sublennextcall:
        ebp += esi->nrows;

    lbl_nextcall:
        esi = edi->node;

        ecx++;
        if (ecx <= ebx.ncalltypes)
            goto regen;

    backtrack:
        STARTTIMEBT;
        esi->included = 0; // Reset included flag
        edi--;             // All compositions found?
        if (edi < ebx.comp)
            goto asmdone;
        ecx = edi->call;
        ebp += esi->nrows; // Reduce composition length
        esi = edi->node;   // Load previous lead

    lbl_nextcall2:
        ecx++;
        if (ecx > ebx.ncalltypes)
            goto backtrack;
        STOPTIMEBT;

    regen:
        if (REGENERATION)
        {
            // The regen pointer is set to point back to the start of the composition
            // i.e. edi - this->comp
            // This lead's regenoffset is also added. For the case where course-end
            // synchronisation is necessary (not all calling positions allowed) the regenoffset
            // for this lead points sufficiently far into the plain lead buffer (beyond the
            // START of the composition) to copy in plains up to the course end.
            ebx.lastregen = edi;
            ebx.regenptr = ((intptr_t)ebx.comp - (intptr_t)edi) + esi->regenoffset[ecx];
        }

    composeloop:
        STARTTIMEC;
        esi = esi->nextnode[ecx];
        edi->call = ecx; // Add call to composition
        if (esi == nullptr)
        {
            goto lbl_nextcall;
        }
        ebx.stats.nodecount++;
        if (esi->included != 0) // Already had that node?
        {
            goto lbl_nextcall;
        }
        ebp -= esi->nrows;
        if (ebp < 0)
        {
            goto lbl_sublennextcall; // Composition too long?
        }

        STARTTIMEF;
        auto edx2 = esi->nfalsenodes;
        if (edx2 != 0)
        {
            ecx = 1;
            if (esi->falsenodes[0]->included != 0)
            {
                goto lbl_loadcallnextcall;
            }
            while (ecx != edx2)
            {
                auto eax2 = esi->falsenodes[ecx];
                ecx++;
                if (eax2->included != 0)
                    goto lbl_loadcallnextcall;
            }
        }

    leadok:
        STOPTIMEF;
        edi++;             // Increment comp ptr
        esi->included = 1; // Mark node included
        if (REGENERATION)
        {
            eax = ebx.regenptr;
        }
        edi->node = esi; // Add lead to composition
        if (REGENERATION)
        {
            ecx = ((Composition*)((intptr_t)edi + (intptr_t)eax))->call;
        }
        STOPTIMEC;
        if (esi->comesround == 0) // Has come round?
            goto composeloop;

        eax = ebx.maxpalilength;
        eax -= ebp;
        if (eax < ebx.minlengthnow)
            goto backtrackfromrounds;
        ebx.complength = eax;
        ebx.lengthcountdown = ebp;
        compptr = edi;
        ebx.ncompnodes = (int)(compptr - ebx.comp);

        if (REGENERATION)
        {
            // Check composition hasn't got 'tail end regeneration'. This occurs when a small
            // amount (< part size and >=course length) of the initial call sequence is
            // regenerated at the end of the composition. If this final call sequence is
            // rotated to the start, it matches an earlier duplicate composition. E.g.
            //  H W W WH* H
            // can be produced when a backtrack at * causes the initial H course to be
            // regenerated, producing rounds. This composition is identical to the already-
            // produced
            //  H H W W WH
            // so can safely be rejected.
            // We can detect this condition when the last backtrack position DOESN'T split
            // the composition up into an integral number of parts. So, find # parts first.
            ebx.lastregen++;
            if (ebx.coursestructured)
            {
                // Move on to next course end
                while (ebx.lastregen < compptr)
                {
                    if (ebx.lastregen->node->nparts) // Nparts==0 -> not a course end
                        break;
                    ebx.lastregen++;
                }
            }
            ebx.nodesperpart = (int)(ebx.lastregen - ebx.comp);
            // Course end reached - can drop through to non course-structured case
            // Calculate number of parts
            ebx.nparts = ebx.ncompnodes / ebx.nodesperpart;
            // Tail-end regeneration detected if the last backtrack doesn't produce integral parts
            if (ebx.nparts * ebx.nodesperpart != ebx.ncompnodes)
                goto whileloop;
        }
        ebx.stats.ncompsfound++;
        ebx.analysecomp();
        goto whileloop;
    alldone:
        return;
    }
};

#endif
