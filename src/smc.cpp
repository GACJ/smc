// SMC - Single Method Composer
// Copyright Graham A C John 1991-98 All rights reserved
// SMC32 32-bit conversion
// Copyright Mark B Davies 1998-2000

#include "smc.h"
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <cpuid.h>
#elif defined(_MSC_VER) && (_MSC_VER >= 1500) && (defined(_M_X64) || defined(_M_IX86)) // VS2008
#include <excpt.h>
#include <intrin.h>
#include <nmmintrin.h>
#endif

int badusage()
{
    printf("Usage:\n");
    printf(" Composing:  SMC32 <method-file>.%s\n", INEXT);
    printf(" Restarting: SMC32 <output-file>.%s\n", OUTEXT);
    printf(" Analysing:  SMC32 <output-file>.%s <music-file>.%s [maxn]\n", OUTEXT, MUSEXT);
    return (1);
}

#if defined(_MSC_VER)
int __cdecl main(int argc, char** argv)
#else
int main(int argc, char** argv)
#endif
{
    ExtMethod method;
    Composer ring(&method);
    int n;

    printf("\n%s: raw composing power\n", VERSION);
    printf("%s\n", COPYRIGHT);
    if (argc < 2)
        return badusage();
    ring.redomusic = FALSE;
    ring.outfile.newfile(argv[1]);
    auto ext = ring.outfile.getextension();
    // Starting a new search?
    if (strcmpi(ext, INEXT) == 0)
    {
        if (argc != 2)
            return badusage();
        else if (!ring.newsearch())
            return (10);
        else
            return (0);
    }

    // Restarting a previous .sf0 file?
    if (strncmpi(ext, OUTEXT, 2) != 0)
        return badusage();
    if (!isdigit(ext[2]))
        return badusage();
    if (argc > 2)
    {
        if (argc > 4)
            return badusage();
        ring.printcourseendsfirst = TRUE;
        if (strcmpi(argv[2], "redo") == 0)
        {
            ring.redomusic = TRUE;
            printf("Re-using original music defs\n");
        }
        else
        {
            ring.redomusic = FALSE;
            ring.musicfile.newfile(argv[2]);
            if (strcmpi(ring.musicfile.getextension(), MUSEXT))
                return badusage();
        }
        if (argc == 4)
            n = atoi(argv[3]);
        else
            n = 10;
        if (!ring.musicsort(n))
            return (12);
    }
    else if (!ring.restartsearch())
        return (11);
    return (0);
}

int Composer::newsearch()
{
    char lhcode[10];

    if (!readinputfile(outfile))
        return (FALSE);
    // If a music include line was specified, read in that file
    if (musicinclude)
    {
        musicfile.newfile(musicinclude);
        if (strcmpi(musicfile.getextension(), MUSEXT))
        {
            printf("ERROR: bad music-include filename %s\n", musicinclude);
            return (FALSE);
        }
        if (!readmusicfile(musicfile))
            return (FALSE);
        musicfile.close();
        if (!readmusicminoverrides(outfile)) // Read any musicdef minimum overrides
            return (FALSE);
    }
    // Otherwise, music definitions can be in the method file
    else if (!readmusicfile(outfile))
        return (FALSE);
    outfile.close();
    m->setcompletename(methodname, FALSE);
    m->showleadcode(lhcode);
    printf("%s %s (%s)\n", methodname, m->leadhead, lhcode);
    if (!writefileheader(outfile))
        return (FALSE);
    if (makefraglib)
    {
        if (!fraglib.writelibheader(this))
            return (FALSE);
    }
    if (!((ExtMethod*)m)->buildtables(*this))
        return (FALSE);
    if (!newcomp())
        return (FALSE);
    if (usefraglib)
    {
        if (!fraglib.read(this, nullptr)) // Filename is set up in readinputfile()
            return (FALSE);
        if (!fraglib.compress())
            return (FALSE);
    }
    if (makefraglib)
    {
        printf("Fragments are being written to %s\n", fraglib.getname());
        printf("Restart file is %s\n", outfile.getname());
    }
    else
        printf("Compositions are being written to %s\n", outfile.getname());
    if (palindromic)
    {
        // Initialise palindromic node - just a plain Home for now
        comp[0].palinode = comp[0].node->prevnode[internalcallnums[BOB]];
    }
    compose();
    return (TRUE);
}

// Returns true if an mmx processor is present
int isMMXsupported()
{
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    return __builtin_cpu_supports("mmx");
#elif defined(_MSC_VER) && (_MSC_VER >= 1500) && (defined(_M_X64) || defined(_M_IX86)) // VS2008
    __try
    {
        // the cpuid instruction sets bit 23 if an mmx processor is present
        int regs[4];
        __cpuid(regs, 1);
        return (regs[3] & (1 << 23)) != 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }
#else
    return FALSE;
#endif
}

int Composer::setdefaults()
{
    int i;

    showlongestyet = FALSE;
    showbestyet = FALSE;
    showallrots = FALSE; // Only shows best rotation
    singleleadnodes = FALSE;
    rotationalsort = TRUE;
    disableregen = FALSE;
    palindromic = FALSE;
    bitwisetruthflags = FALSE;
    useMMX = isMMXsupported();
    makefraglib = FALSE;
    usefraglib = FALSE;
    storecomps = FALSE;
    countingcalls = FALSE;
    startinrounds = TRUE;
    endinrounds = TRUE;
    samestartandend = TRUE;
    for (i = 0; i < MAXNBELLS; i++)
    {
        startrow[i] = i;
        finishrow[i] = i;
    }
    ncompstostore = 0;
    ncalltypes = 1;                  // Does NOT include PLAIN
    exclude.nointernalrounds = TRUE; // Must be set if rotationalsort is on!
    exclude.noleadheadrounds = FALSE;
    exclude.allpartsallowed = TRUE; // Allow all numbers of parts
    exclude.minnparts = 1;
    for (i = 1; i <= MAXNPARTS; i++)
        exclude.allowedparts[i] = TRUE;
    nmusicdefs = 0;
    // If there are no music minimums, can add music scores for each node into
    // a single combined score, to improve evalcomp() speed
    optimisemusic = TRUE;

    nbells = m->nbells;
    /*
// Got rid of this for build 0.961
 if (nbells<=6)
  coursestructured = FALSE;
 else
*/
    coursestructured = TRUE;
    callingbell = nbells - 1;
    if (!courseend.ensurespace(1))
    {
        printf("ERROR: failed to allocate courseend storage\n");
        return (FALSE);
    }
    courseend.matches[0].type = MUSICANYROW;
    courseend.matches[0].sign = 0;
    for (i = 0; i < nbells; i++)
        if (i < 7 - 1)
            courseend.matches[0].row[i] = -1;
        else
            courseend.matches[0].row[i] = i;
    courseend.matches[0].row[callingbell] = callingbell;

    // 'internalcallnums' are now set in Composer::setup()
    for (i = 0; i <= ncalltypes; i++)
    {
#ifdef SWAPCALLORDER
        calltypes[ncalltypes - i] = (Call)i;
#else
        calltypes[i] = (Call)i;
#endif
    }
    for (i = 0; i <= ncalltypes; i++)
        defaultcallingpositions(i);
    return (TRUE);
}

void Composer::defaultcallingpositions(int call)
{
    int i;

    exclude.defaultcalls[calltypes[call]] = TRUE;
    if (calltypes[call] == PLAIN)
    {
        for (i = 0; i < nbells; i++)
            exclude.allowedcalls[call][i] = TRUE; // All positions allowed at plain!
    }
    else
    {
        if (nbells <= 6)
        {
            for (i = 0; i < nbells; i++)
                exclude.allowedcalls[call][i] = TRUE;
        }
        else
        {
            for (i = 0; i < nbells; i++)
                exclude.allowedcalls[call][i] = FALSE;
        }
        if (m->fourthsplacebobs())
        {
            if (nbells <= 8 && calltypes[call] == BOB)
                exclude.allowedcalls[call][2] = TRUE;      // Before
            exclude.allowedcalls[call][nbells - 3] = TRUE; // Middle
            exclude.allowedcalls[call][nbells - 2] = TRUE; // Wrong
            exclude.allowedcalls[call][nbells - 1] = TRUE; // Home
        }
        else
        {
            exclude.allowedcalls[call][1] = TRUE; // In
            exclude.allowedcalls[call][2] = TRUE; // Out
            exclude.allowedcalls[call][4] = TRUE; // V
            if (nbells <= 8 && calltypes[call] == BOB)
                exclude.allowedcalls[call][nbells - 1] = TRUE; // Home
        }
    }
    maxcalls[call] = INT_MAX;
    for (i = 0; i < nbells; i++)
    {
        maxcallsperpos[i][call] = INT_MAX;
        strcpy(callposnames[call][i], "@");
    }
    strcpy(callposnames[call][1], "I");
    strcpy(callposnames[call][2], "B");
    strcpy(callposnames[call][3], "F");
    strcpy(callposnames[call][4], "V");
    strcpy(callposnames[call][nbells - 3], "M");
    strcpy(callposnames[call][nbells - 2], "W");
    strcpy(callposnames[call][nbells - 1], "H");
}

// Sets up various required tables and fields
// Must call after setdefaults() but before table-building
int Composer::setup()
{
    char tmprow[MAXNBELLS];
    int call;
    int i, j, b, c;
    int index1;

    calcfactorials();
    for (i = 0; i < MAXNBELLS; i++)
        binrounds[i] = i;
    for (i = 0; i <= NDIFFCALLS; i++)
        internalcallnums[i] = -1;
    for (i = 0; i <= ncalltypes; i++)
        internalcallnums[calltypes[i]] = i;
    courselen = m->courselength() / m->leadlen;
    findcalltranspositions();
    // Find calling position order
    copyrow(binrounds, row);
    i = 0;
    do
    {
        // Transpose by all possible calls to see where calling bell goes
        // For each possible call, check if it gives the same calling position
        //  as a previous call in this loop. (index1 = index of first call added)
        // If so, update callposcallmasks[], otherwise add new calling position
        index1 = i;
        for (call = BOB; call <= EXTREME; call++)
        {
            c = internalcallnums[call];
            if (c >= 0)
            {
                transpose(row, calltrans[c], tmprow);
                b = findcallingbell(tmprow);
                if (exclude.allowedcalls[c][b])
                {
                    for (j = index1; j < i; j++)
                        if (callposorder[j] == b)
                        {
                            callposcallmasks[j] |= 1 << c;
                            break;
                        }
                    if (j >= i)
                    {
                        callposcallmasks[i] = 1 << c;
                        callposorder[i++] = b;
                    }
                }
            }
        }
        // Transpose by plain lead (or first possible call if excluded)
        for (call = PLAIN; call <= EXTREME; call++)
            if (internalcallnums[call] >= 0)
            {
                transpose(row, calltrans[internalcallnums[call]], tmprow);
                copyrow(tmprow, row);
                break;
            }
        if (call > EXTREME)
            break;
    } while (findcallingbell(row) != callingbell);
    callposorder[i++] = -1;
    return (TRUE);
}
