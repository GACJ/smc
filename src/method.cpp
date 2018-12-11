// SMC - Single Method Composer
// Copyright Graham A C John 1991-98 All rights reserved
// SMC32 32-bit conversion
// Copyright Mark B Davies 1998-2000

// Finds false lead heads

#include "smc.h"
#include <string.h>

int Composer::findfalseLHs()
{
    char *rowptr, *p;
    int treblepos;
    int i, j;

    safedelete(workinglead);
    workinglead = new char[m->leadlen * nbells];
    if (workinglead == nullptr)
    {
        printf("ERROR: failed to alloc workinglead\n");
        return (FALSE);
    }
    nfalseLHs = 0;
    safedelete(falseLHs);
    falseLHs = new FalseLH[m->leadlen * (m->leadlen - 1)];
    if (falseLHs == nullptr)
    {
        printf("ERROR: failed to alloc false LHs\n");
        return (FALSE);
    }
    starttouch();
    // Use codes 0..nbells-1 instead of ASCII rounds
    copyrow(binrounds, row);
    calllist[0] = PLAIN;
    // Generate first lead, store in workinglead[]
    startlead();
    rowptr = workinglead;
    while (changen < m->leadlen)
    {
        for (i = 0; i < nbells; i++)
            *rowptr++ = row[i];
        change();
    }
    rowptr = workinglead;
    // Calculate false lead heads
    for (changen = 0; changen < m->leadlen; changen++)
    {
        // Find treble in row
        for (treblepos = 0; treblepos < nbells; treblepos++)
            if (rowptr[treblepos] == 0)
                break;
        // Compare this row with every other one in lead
        p = workinglead;
        for (i = 0; i < m->leadlen; i++)
        {
            // If treble in same position (but not the same change!) calculate false transposition
            if (i != changen && p[treblepos] == 0)
            {
                inversetrans(falseLHs[nfalseLHs].row, p, rowptr);
                // Check whether this is a new LH
                for (j = 0; j < nfalseLHs; j++)
                    if (samerow(falseLHs[j].row, falseLHs[nfalseLHs].row))
                        break;
                if (j >= nfalseLHs) // New false LH - keep it
                    falseLHs[nfalseLHs++].pos = changen;
            }
            p += nbells;
        }
        rowptr += nbells;
    }
    // for (j=0; j<nfalseLHs; j++)
    //  printrow(falseLHs[j].row);
    return (TRUE);
}

int Method::courselength()
{
    Ring ring(this);
    char lead1[MAXNBELLS], lh[MAXNBELLS], row[MAXNBELLS], binrounds[MAXNBELLS];
    int i, n;

    for (i = 0; i < nbells; i++)
    {
        lead1[i] = strchr(rounds, leadhead[i]) - rounds;
        binrounds[i] = i;
    }
    ring.copyrow(binrounds, lh);
    n = 0;
    do
    {
        n++;
        ring.transpose(lh, lead1, row);
        ring.copyrow(row, lh);
    } while (!ring.samerow(lh, binrounds));
    return n * leadlen;
}

// Calculate calltrans[] lead transposition for each call
void Composer::findcalltranspositions()
{
    int call;

    for (call = 0; call <= ncalltypes; call++)
    {
        starttouch();
        copyrow(binrounds, row);
        calllist[0] = calltypes[call];
        startlead();
        do
            change();
        while (changen < m->leadlen);
        copyrow(row, calltrans[call]);
    }
}

// For each place bell, find the number of plain leads to the course end
void Composer::findcourseenddists()
{
    int i, b, n;

    for (i = 1; i < nbells; i++)
    {
        b = i;
        n = 0;
        // Find how many leads before this place bell reaches callingbell position
        while (b != callingbell)
        {
            if (leadspernode[b])
                n++;
            b = strchr(m->leadhead, rounds[b]) - m->leadhead;
            // If it's stuck in a loop and won't reach callingbell, set courseenddist to
            // courselen. This will make the rotational sort insert an entire course
            // of plain leads, which should be enough to make the touch run false and require
            // another backtrack
            if (b == i)
            {
                n = courselen;
                break;
            }
        }
        courseenddist[i] = n;
    }
}

// Sets up two tables: leadspernode and leadstonodeend.
// Leadstonodeend counts from ANY starting position of the
// calling bell to the node end.
// Leadspernode is the same, except contains 0 where the
// calling bell is not at a proper node start.
void Composer::findnodes()
{
    char tmprow[MAXNBELLS];
    char binzero[MAXNBELLS];
    char nodestarts[MAXNBELLS];
    int call;
    int i, j;

    if (singleleadnodes)
    {
        for (i = 1; i < nbells; i++)
        {
            leadspernode[i] = 1;
            leadstonodeend[i] = 1;
        }
        nodespercourse = nbells - 1;
        return;
    }
    // Set up binzero[] row. The callingbell is poked into this.
    for (i = 0; i < nbells; i++)
        binzero[i] = 0;
    // Find starting point of each course segment, mark with TRUE
    for (i = 1; i < nbells; i++)
    {
        nodestarts[i] = FALSE;
        // Find leads which can be reached by a call.
        // These are treated as starting points of course segments.
        for (call = 0; call <= ncalltypes; call++)
            if (calltypes[call] != PLAIN && exclude.allowedcalls[call][i])
            {
                nodestarts[i] = TRUE;
                break;
            }
    }
    // Find leads where a call is allowed.
    // These are treated as the end of course segments, so the *next* plain lead
    // is marked as a starting point
    for (i = 1; i < nbells; i++)
        for (call = 0; call <= ncalltypes; call++)
            if (calltypes[call] != PLAIN)
            {
                copyrow(binzero, row);
                row[i] = callingbell;
                transpose(row, calltrans[call], tmprow);
                if (exclude.allowedcalls[call][findcallingbell(tmprow)])
                {
                    transpose(row, calltrans[internalcallnums[PLAIN]], tmprow);
                    nodestarts[findcallingbell(tmprow)] = TRUE;
                }
            }
    // If it isn't already, we must have the home position as the start of a
    // course segment, so we can get to and from rounds!
    //nodestarts[callingbell] = TRUE;
    nodestarts[findcallingbell(startrow)] = TRUE;
    nodestarts[findcallingbell(finishrow)] = TRUE;
    // For each course segment, find out how many plain leads long it is
    nodespercourse = 0;
    for (i = 1; i < nbells; i++)
    {
        copyrow(binzero, row);
        row[i] = callingbell;
        j = 0;
        do
        {
            transpose(row, calltrans[internalcallnums[PLAIN]], tmprow);
            copyrow(tmprow, row);
            j++;
        } while (!nodestarts[findcallingbell(row)]);
        leadstonodeend[i] = j;
        leadspernode[i] = 0;
        if (nodestarts[i])
        {
            nodespercourse++;
            leadspernode[i] = j;
        }
    }
}