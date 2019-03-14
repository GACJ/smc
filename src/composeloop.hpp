// Include file used to create various versions of the composing loop
// Copyright Mark B Davies 1998-2000

#ifndef __ASM_IMPL__

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
struct composeloop
{
    static void run(Composer& composer)
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
            int edx;
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
            ebx.stats.nodesgenerated += (int)mm0;
        else
            ebx.stats.nodesgenerated += ebx.stats.nodecount;
        if (MMXCOUNTER)
        {
            // emms
        }
        return;

    lbl_loadcallnextcall:
        ecx = edi->call;
        if (CALLCOUNT)
        {
            auto edx = ecx;
            ebx.ncallsleft[ecx]++;
            edx += esi->callcountposindex;
            ebx.ncallsleftperpos[0][edx]++;
        }

    lbl_sublennextcall:
        ebp += esi->nrows;

    lbl_nextcall:
        esi = edi->node;

        if (!MULTIPART || ebp >= ebx.maxpartlength)
        {
#ifdef REVERSECALLS
            ecx--;
            if (ecx >= 0)
#else
            ecx++;
            if (ecx <= ebx.ncalltypes)
#endif
            {
                goto regen;
            }
        }

    backtrack:
        if (backtrack_sub(ebx, edi, esi, ecx, ebp))
            goto asmdone;

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
            ebx.regenptr += (ebx.comp - edi) + esi->regenoffset[ecx];
        }
        if (MMXFRAGOPT)
        {
#ifdef STOREPATT
            mm2 = edi->pattern;
#endif
        }

    composeloop:
        STARTTIMEC;
        esi = esi->nextnode[ecx];
        edi->call = ecx; // Add call to composition
        if (esi == nullptr)
        {
            goto lbl_nextcall;
        }
        if (MMXFRAGOPT)
        {
            mm7 = ecx;
        }
        if (MMXCOUNTER)
        {
            // paddd mm0,mm1
        }
        else
        {
            ebx.stats.nodecount++;
        }
        if (!FALSEBITS)
        {
#ifdef UNVISITABLE
            if (esi->unvisitable != 0) // Node unvisitable?
#else
            if (esi->included != 0) // Already had that node?
#endif
            {
                goto lbl_nextcall;
            }
        }
        ebp -= esi->nrows;
        if (ebp < 0)
        {
            goto lbl_sublennextcall; // Composition too long?
        }

        if (CALLCOUNT)
        {
            eax = ebx.ncallsleft[ecx] - 1;
            if (eax < 0)
                goto lbl_sublennextcall;
            auto edx = ecx + esi->callcountposindex;
            ebx.ncallsleft[ecx] = eax;
            eax = ebx.ncallsleftperpos[edx] - 1;
            if (eax < 0)
            {
                ebx.ncallsleft[ecx]++;
                goto lbl_sublennextcall;
            }
            ebx.ncallsleftperpos[edx] = eax;
        }

#ifndef CLEANPROOF
        STARTTIMEF;
        if (FALSEBITS)
        {
            ecx = esi->nfalsenodes;
            eax = *esi->falsebits[ecx].tableptr & esi->falsebits[ecx].tablemask;
            if (eax != 0)
                goto lbl_loadcallnextcall;
            ecx--;
            while (ecx != 0)
            {
                eax = *esi->falsebits[ecx].tableptr & esi->falsebits[ecx].tablemask;
                if (eax != 0)
                    goto lbl_loadcallnextcall;
                ecx--;
            }
        }
        else
        {
            auto edx = esi->falsenodes[0];
            if (edx != 0)
            {
#ifdef EXPANDFALSELOOP
                ecx = 1;
                if (esi->falsenodes[0]->included != 0)
                {
                    goto lbl_loadcallnextcall;
                }
#else
                ecx = 0;
#endif
                while (ecx != edx)
                {
                    eax = esi->falsenodes[ecx];
                    ecx++;
#ifdef UNVISITABLE
                    esi->falsenodes[ecx]->unvisitable++;
#else
                    if (esi->falsenodes[ecx]->included != 0)
                        goto lbl_loadcallnextcall;
#endif
                }
            }
        }
#endif

    leadok:
        STOPTIMEF;
        if (MMXFRAGOPT)
        {
            // psllq   mm2, 2
            // por mm2, mm7
            // mov edx, [esi]Node.fragmap
            // movd    eax, mm2
            // cmp edx, 0
            // je  notfrag
            // and eax, [ebx]Composer.fraglib.mask
            // mov edx, [edx + eax * 4]
            // cmp edx, 0
            // je  notfrag
            // // Check call pattern against fragment list
            // mov ecx, [edx]
            // add edx, 4 // size int
            // pattloop:   dec ecx
            // js  notfrag
            // movq    mm6, mm2
            // movq    mm7, [edx]
            // pand    mm6, [edx + 8]
            // pcmpeqd mm6, mm7
            // add edx, size CompressedFrag
            // packssdw    mm6, mm6
            // movd    eax, mm6
            // cmp eax, 0
            // je  pattloop
            // inc[ebx]Composer.stats.prunecount
            // jmp lbl_loadcallnextcall
            // notfrag
        }
        if (PALINDROMIC)
        {
            ecx = edi->call;
            if (esi == edi->palinode)
                goto reachednodeheadapex;
            auto eax2 = edi->palinode->prevnode[ecx];
            if (eax2 == nullptr)
                goto lbl_sublennextcall;
            (edi + 1)->palinode = eax2;
            if (esi == eax2)
                goto reachedmidnodeapex;
        }
        // Sub len was here
        if (PALINDROMIC)
        {
            auto eax2 = edi->palinode;
            if (FALSEBITS)
            {
                *eax2->falsebits[0].tableptr |= eax2->falsebits[0].tablemask;
            }
            else
            {
                eax2->included = 1; // Mark palindromic node included
            }
        }
        edi++; // Increment comp ptr
        if (FALSEBITS)
        {
            *esi->falsebits[0].tableptr |= esi->falsebits[0].tablemask;
        }
        else
        {
#ifdef UNVISITABLE
            esi->unvisitable = 1; // Mark node included, unvisitable
#else
            esi->included = 1;      // Mark node included
#endif
        }
        if (REGENERATION)
        {
            eax = ebx.regenptr;
        }
        edi->node = esi; // Add lead to composition
        if (MMXFRAGOPT)
        {
#ifdef STOREPATT
            edi->pattern = mm2;
#endif
        }
        if (REGENERATION)
        {
            ecx = edi + eax;
        }
        else
        {
#ifdef REVERSECALLS
            ecx = ebx.ncalltypes;
#else
            ecx = 0;
#endif
        }
        STOPTIMEC;
        if (esi->comesround == 0) // Has come round?
            goto composeloop;

        if (PALINDROMIC)
        {
            // Backtrack on normal rounds - unlinked palindromic 2-part of double length!
            goto backtrackfromrounds;
        reachedmidnodeapex:
            // Need to add on half the palindromic node's length
            eax = esi->nrows >> 1;
            ebx.midnodeapex = 1;
            eax += ebx.maxpalilength;
            goto inccomp;
        reachednodeheadapex:
            ebx.midnodeapex = 0;
            eax = ebx.maxpalilength;
        inccomp:
            edi++; // Increment comp ptr
            if (FALSEBITS)
            {
                *esi->falsebits[0].tableptr |= esi->falsebits[0].tablemask;
            }
            else
            {
                esi->included = 1; // Mark node included
            }
            edi->node = esi; // Add lead to composition
        }
        else
        {
            eax = ebx.maxpalilength;
        }
        eax -= ebp;
        if (PALINDROMIC)
        {
            esi->nrows -= ebp;
            eax *= 2;
        }
        if (eax < ebx.minlengthnow)
            goto backtrackfromrounds;
        ebx.complength = eax;
        ebx.lengthcountdown = ebp;
        if (MMXCOUNTER)
        {
            ebx.stats.nodecount = mm0;
        }
        compptr = edi;
        if (MMXCOUNTER)
        {
            // emms
        }

        ebx.ncompnodes = compptr - ebx.comp;

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
            ebx.nodesperpart = ebx.lastregen - ebx.comp;
            // Course end reached - can drop through to non course-structured case
            // Calculate number of parts
            ebx.nparts = ebx.ncompnodes / ebx.nodesperpart;
            // Tail-end regeneration detected if the last backtrack doesn't produce integral parts
            if (ebx.nparts * ebx.nodesperpart != ebx.ncompnodes)
                continue;
        }
        if (PALINDROMIC)
        {
            // Expand palindromic composition
            int i;
            for (i = 0; i < ebx.ncompnodes; i++)
                ebx.comp[ebx.ncompnodes - 1 + i + ebx.midnodeapex].call = ebx.comp[ebx.ncompnodes - 1 - i].call;
            for (i = 0; i < ebx.ncompnodes - 1 + ebx.midnodeapex; i++)
            {
                ebx.comp[ebx.ncompnodes + i + 1].node = ebx.comp[ebx.ncompnodes + i].node->nextnode[ebx.comp[ebx.ncompnodes + i].call];
                if (ebx.comp[ebx.ncompnodes + i + 1].node == nullptr)
                    break;
            }
            ebx.ncompnodes = ebx.ncompnodes * 2 - 1 + ebx.midnodeapex;
            if (i + ebx.ncompnodes < ebx.ncompnodes)
                continue;
        }
        ebx.stats.ncompsfound++;
        ebx.analysecomp();
    }

    static bool backtrack_sub(Composer& ebx, Composition*& edi, Node*& esi, int& ecx, int& ebp)
    {
        do
        {
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
                auto edx = esi->nfalsenodes;
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
            Node* eax;
            if (PALINDROMIC)
            {
                eax = edi->palinode;
            }
            if (edi < ebx.comp)
                return true;
            if (PALINDROMIC)
            {
                if (FALSEBITS)
                {
                    eax->falsebits->tableptr[0] ^= eax->falsebits->tablemask;
                }
                else
                {
                    eax->included = 0; // Reset palindromic included flag
                }
            }
            ecx = edi->call;
            ebp += esi->nrows; // Reduce composition length
            if (CALLCOUNT)
            {
                auto edx = ecx;
                ebx.ncallsleft[ecx]++;
                esi->callcountposindex += edx;
                ebx.ncallsleftperpos[0][edx]++;
            }
            esi = edi->node; // Load previous lead
        } while ((MULTIPART && ebx.maxpartlength == ebp) || lbl_nextcall2(esi, ecx));
        STOPTIMEBT;
        return false;
    }

    static bool lbl_nextcall2(const Node* esi, int& ecx)
    {
        while (true)
        {
#ifdef REVERSECALLS
            ecx--;
            if (ecx < 0)
#else
            ecx++;
            if (ecx > composer.ncalltypes)
#endif
            {
                // backtrack
                return true;
            }
            else
            {
                if (MULTIPART && CALLCOUNT)
                {
                    // Before re-entering the composing loop, check maximum number of calls per part not exceeded.
                    // Note that this test requires the next-node pointer, so we MUST test for allowed call first!
                    // (Normally done in composing loop)
                    if (esi->nextnode[ecx] == 0 || ebx.ncallsleft[ecx] < ebx.npartcalls[ecx])
                        continue;

                    edx = ecx + esi->nextnode[ecx]->callcountposindex;
                    if (ebx.ncallsleftperpos[edx] < ebx.npartcallsperpos[edx])
                        continue;
                }
                return false;
            }
        }
    }
};

#endif
