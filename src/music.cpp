// SMC32 music.cpp Copyright Mark B Davies 1998-2000

#include "smc.h"
#include <limits.h>

// Uncomment this to prevent TVs being removed from the ouput list
#define TVS

//#define UNIQUECHECK
#ifdef UNIQUECHECK
//#define UNIQUEOUT
#endif

// Parameter 'maxncomps' is the number of 'best' comps we want to keep.
int Composer::musicsort(int maxncomps)
{
    CompMusicStore topcomp;
    Node* node;
    Block* block;
    char titleline[MAXLINEBUF];
    int rotationalsortsave;
    int ncomps, noutput;

    outfile.setmode("r");
    if (!readinputfile(outfile))
        return (FALSE);
    m->setcompletename(methodname, FALSE);
    if (redomusic)
    {
        if (!readmusicfile(outfile))
            return (FALSE);
    }
    else
    {
        if (!skipmusicfile(outfile))
            return (FALSE);
        if (!readmusicfile(musicfile))
            return (FALSE);
        musicfile.close();
    }
    // Relax some of the exclusions - in particular, want to be able to analyse both
    // round-block and rounds-internal compositions
    exclude.nointernalrounds = FALSE;
    exclude.noleadheadrounds = FALSE;
    exclude.allpartsallowed = TRUE;
    exclude.minnparts = 1;
    // Turn off bitwise truth tables - music analysis routines use ordinary false node tables
    bitwisetruthflags = false;
    // Turn off music optimisation, so each type of music gets counted separately
    optimisemusic = FALSE;
    // Turn on rotational sort, so that nodes with excluded changes still get processed
    // (This also stops the nobbling of the false-node tables by musthave blocks)
    rotationalsortsave = rotationalsort;
    rotationalsort = TRUE;
    if (!((ExtMethod*)m)->buildtables(*this))
        return (FALSE);
    if (!newcomp())
        return (FALSE);
    rotationalsort = rotationalsortsave;
    stats.elapsed = clock();
    printf("Analysing music...\n");
    if (nmusicdefs == 0)
    {
        printf("Analysis abandoned because no music defs were specified\n");
        return (TRUE);
    }
    if (!compsorter.init(maxncomps))
    {
        printf("ERROR: failed to alloc composition storage\n");
        return (FALSE);
    }
    safedelete(specialcomps);
    // One extra "special" comp for "max musthave blocks"
    specialcomps = new CompMusicStore[nmusicdefs + 1];
    if (specialcomps == nullptr)
    {
        printf("ERROR: failed to allocate 'special' composition storage\n");
        return (FALSE);
    }
    for (int i = 0; i < nmusicdefs + 1; i++)
    {
        specialcomps[i].score = -1;
        specialcomps[i].nmusthaveblocks = 0;
        if (!specialcomps[i].music.alloc(nmusicdefs))
        {
            printf("ERROR: failed to allocate 'special' composition storage\n");
            return (FALSE);
        }
        if (i < nmusicdefs)
            specialcomps[i].music.score[i] = -1;
    }
    topcomp.calling = nullptr;
    topcomp.allocsize = 0;
    ncomps = 0;
    minscore = 0; // Ignore minscore in smc file
    while (outfile.readline())
    {
        if (*outfile.buffer == CHECKPOINT_SYMBOL)
            break;
        if (strncmp(outfile.buffer, TOKEN_END, strlen(TOKEN_END)) == 0)
            break;
        ncomps++;
        if ((ncomps & 0x3F) == 0)
            printf("\r%d", ncomps);
        if (!inputcomp(outfile.buffer))
        {
            printf("Analysis aborted at composition number %d\n", ncomps);
            return (FALSE);
        }
        // Analyse comp
        topcomp.rot = -1;
        comprot = 0;
        countcalls();
        topcomp.nmusthaveblocks = 0;
        for (int i = 0; i < musthaveblocks->listsize(); i++)
        {
            block = (Block*)musthaveblocks->getitem(i);
            if (block == nullptr)
            {
                printf("ERROR: null returned from musthaveblocks::getitem\n");
                return (FALSE);
            }
            if (block->entrynode->node->included)
                topcomp.nmusthaveblocks++;
        }
        topcomp.nTVs = 0;
#ifdef UNIQUECHECK
        evalcomp();
        topcomp.rot = 0;
        if (!topcomp.music.set(music))
            return (FALSE);
        topcomp.score = calcnodehash();
        if (!storecomp2(topcomp))
            return (FALSE);
        if (!compsorter.adduniquecomp(topcomp, this))
            return (FALSE);
#else
        do
        {
            int oldscore = score;
            if (evalcomp())
            {
                // Won't have been a score in the composition file if Author was given
                if (oldscore == 0 && author != nullptr)
                    oldscore = score;
                if (redomusic && oldscore != score)
                    printf("Incorrect score %d - should be %d\n", oldscore, score);
                // If comp achieved minimums, add to CompSorter
                topcomp.rot = comprot;
                topcomp.score = score;
                if (!topcomp.music.set(music))
                    return (FALSE);
                if (!storecomp2(topcomp))
                    return (FALSE);
                if (!compsorter.addcomp(topcomp))
                    return (FALSE);
                if (topcomp.nmusthaveblocks > specialcomps[nmusicdefs].nmusthaveblocks
                    || (topcomp.nmusthaveblocks == specialcomps[nmusicdefs].nmusthaveblocks && score > specialcomps[nmusicdefs].score))
                {
                    specialcomps[nmusicdefs].rot = comprot;
                    specialcomps[nmusicdefs].score = score;
                    specialcomps[nmusicdefs].nmusthaveblocks = topcomp.nmusthaveblocks;
                    if (!specialcomps[nmusicdefs].music.set(music))
                        return (FALSE);
                    if (!storecomp2(specialcomps[nmusicdefs]))
                        return (FALSE);
                }
                for (int i = 0; i < nmusicdefs; i++)
                {
                    if (musicdefs[i].weighting > 0 && (music.score[i] > specialcomps[i].music.score[i] || (music.score[i] == specialcomps[i].music.score[i] && score > specialcomps[i].score)))
                    {
                        specialcomps[i].rot = comprot;
                        specialcomps[i].score = score;
                        specialcomps[i].nmusthaveblocks = topcomp.nmusthaveblocks;
                        if (!specialcomps[i].music.set(music))
                            return (FALSE);
                        if (!storecomp2(specialcomps[i]))
                            return (FALSE);
                    }
                }
            }
            if (!rotationalsort)
                break;
            // Move on to next rotation
            if (coursestructured)
            {
                node = comp[0].node; // Find next course end - start from rounds!
                while (comprot < nodesperpart && node && node->nodex->callingbellpos[comp[comprot].call] != callingbell)
                {
                    node = node->nextnode[comp[comprot].call];
                    comprot++;
                }
                if (node == nullptr) // Bad comp!
                    break;
            }
            comprot++;
        } while (!redomusic && comprot < nodesperpart);
#endif
    }
    printf("\r");
    outfile.close();
    stats.elapsed = clock() - stats.elapsed;
    printelapsed(titleline);
    printf(" Time taken: %s\n", titleline);
    if (ncomps == 0)
    {
        printf("\nNo compositions reached music minimums\n");
        return (TRUE);
    }
    printf("Saving compositions...");
    sprintf(titleline, "/ Analysis of compositions from %s", outfile.getname());
    outfile.changeexttype(COMPEXT); // Leaves number in place
    outfile.setmode("w");
    if (!outfile.open())
    {
        printf("\nERROR: failed to create %s\n", outfile.getname());
        return (FALSE);
    }
    sprintf(outfile.buffer, "/ %s (c) 1998 Mark B. Davies & Graham A.C. John", VERSION);
    if (!outfile.writeline())
        return (FALSE);
    if (!outfile.writeline(titleline))
        return (FALSE);
#ifdef UNIQUEOUT
    resetcompbuffer();
    outfile.markpos();
    outfile.close();
    outfile.setmode("r+");
    for (int i = 0; i < compsorter.ncompslisted(); i++)
    {
        compptr = compsorter.get(i);
        loadcomposition(*compptr);
        outputcomp();
    }
    flushcompbuffer(FALSE);
#else
    noutput = 0;
    for (int i = 0; i < compsorter.ncompslisted(); i++)
    {
        displaycomp(i + 1, compsorter.get(i), outfile);
        noutput++;
    }
    if (nmusicdefs > 1)
    {
        *outfile.buffer = 0;
        if (!outfile.writeline())
            return (FALSE);
        sprintf(outfile.buffer, "/ ************* Special selection ****************");
        if (!outfile.writeline())
            return (FALSE);
        if (specialcomps[nmusicdefs].nmusthaveblocks > 0)
        {
            displaycomp(-nmusicdefs, &specialcomps[nmusicdefs], outfile);
            noutput++;
        }
        for (int i = 0; i < nmusicdefs; i++)
        {
            if (musicdefs[i].weighting > 0 && specialcomps[i].score > 0 && specialcomps[i].music.score[i] > 0)
            {
                displaycomp(-i, &specialcomps[i], outfile);
                noutput++;
            }
        }
    }
#endif
    outfile.close();
    printf("\n%d compositions found, %d output\n", ncomps, noutput);
    printf("%s written\n", outfile.getname());
    return (TRUE);
}

// Copies one MusicCount into another. Scores are copied, not unlinked
int MusicCount::set(MusicCount& m)
{
    if (!ensurespace(m.nmusicdefs))
    {
        printf("ERROR: failed to allocate music storage\n");
        return (FALSE);
    }
    for (int i = 0; i < m.nmusicdefs; i++)
        score[i] = m.score[i];
    return (TRUE);
}

int MusicCount::ensurespace(int n)
{
    if (n > nmusicdefs)
    {
        nmusicdefs = 0;
        if (!alloc(n))
            return (FALSE);
    }
    return (TRUE);
}

int MusicDef::ensurespace(int n)
{
    if (n > nrows)
    {
        safedelete(matches);
        nrows = 0;
        matches = new MusicRow[n];
        if (matches == nullptr)
            return (FALSE);
        nrows = n;
    }
    return (TRUE);
}

// Ensure space and copy in composition
int CompStore::copyincomp(int nodesperpart, Composition* composition)
{
    this->nodesperpart = nodesperpart;
    if (!ensurespace(nodesperpart))
        return (FALSE);
    for (int i = 0; i < nodesperpart; i++)
        calling[i] = composition[i].call;
    return (TRUE);
}

int CompStore::ensurespace(int length)
{
    if (allocsize < length)
    {
        safedelete(calling);
        allocsize = 0;
        calling = new char[length];
        if (calling == nullptr)
        {
            printf("ERROR: failed to alloc composition buffer\n");
            return (FALSE);
        }
        allocsize = length;
    }
    return (TRUE);
}

// Returns FALSE if this CompMusicStore contains different music to storedcomp.
// Otherwise, if it the same, -1 is returned if the storedcomp is better, or +1
// if this CompMusicStore is better. 'Better' means shorter, or fewer calls
int CompMusicStore::isTV(CompMusicStore& storedcomp)
{
    int i;

    if (score != storedcomp.score)
        return (FALSE);
    for (i = 0; i < music.nmusicdefs; i++)
        if (music.score[i] != storedcomp.music.score[i])
            return (FALSE);
    // Compositions are musically identical.
    // Return -1 if storedcomp has more parts, +1 if this has more parts
    if (storedcomp.nparts != nparts)
    {
        if (storedcomp.nparts > nparts)
            return (-1);
        else
            return (1);
    }
    // Return -1 if storedcomp is shorter, +1 if this is shorter
    if (storedcomp.length != length)
    {
        if (storedcomp.length < length)
            return (-1);
        else
            return (1);
    }
    // Return -1 if storedcomp has fewer calls, +1 if this has fewer calls
    if (storedcomp.ncalls != ncalls)
    {
        if (storedcomp.ncalls < ncalls)
            return (-1);
        else
            return (1);
    }
    // Return -1 if storedcomp has more bobs (i.e. fewer singles and extremes)
    if (storedcomp.callcount[1] > callcount[1])
        return (-1);
    return (1);
}

// Inserts a new composition into the 'best' list
// Checks for (musical) Trivial Variations
int CompSorter::addcomp(CompMusicStore& newcomp)
{
    CompMusicStore* replace;
    int whichTV;
    int i, j;

    // Find where to insert
    replace = nullptr;
    for (i = 0; i < n; i++)
#ifdef TVS
        if (newcomp.score == list[i]->score)
        {
            whichTV = list[i]->isTV(newcomp); // Is new composition a TV?
            if (whichTV)
            {
                if (whichTV > 0) // Yes, and old composition is better
                {
                    list[i]->nTVs++;
                    return (TRUE);
                }
                else // New composition is the better TV
                {
                    replace = list[i];
                    newcomp.nTVs = 1 + replace->nTVs;
                    goto copy;
                }
            }
        }
        else
#endif
            if (newcomp.score > list[i]->score)
        {
            j = n;
            if (j >= listsize)
            {
                j = listsize - 1;
                replace = list[j];
            }
            for (; j > i; j--)
                list[j] = list[j - 1];
            break;
        }
    // If there is space for it, copy comp in
    if (i < listsize)
    {
        n++;
        if (n > listsize)
            n = listsize;
        if (replace == nullptr)
        {
            if (!bulkalloc.add(&newcomp))
            {
                printf("ERROR: failed to allocate bulk composition storage\n");
                return (FALSE);
            }
            list[i] = (CompMusicStore*)bulkalloc.getitem(n - 1);
            goto resetnewcomp;
        }
        list[i] = replace;
    copy:
        safedelete(replace->calling);
        safedelete(replace->music.score);
        *replace = newcomp;
    resetnewcomp:
        newcomp.calling = nullptr; // Calling has been transferred - dereference in newcomp
        newcomp.allocsize = 0;
        newcomp.music.score = nullptr; // Similarly, unlink music array
        newcomp.music.nmusicdefs = 0;
    }
    return (TRUE);
}

#ifdef UNIQUECHECK
// Checks composition against stored comps, to see if node-unique
// If so, add to stored library
CompSorter::adduniquecomp(CompMusicStore& newcomp, Composer* ring)
{
    int whichTV;
    int i, j;

    newcomp.hashvalue = ring->calcnodehash();
#ifndef UNIQUEOUT
    for (i = 0; i < n; i++)
        if (ring->isnodeidentical(list[i], newcomp.hashvalue))
        {
            list[i]->nTVs++;
            return (TRUE);
        }
#endif
    // If there is space for it, copy comp in
    if (!bulkalloc.add(&newcomp))
    {
        printf("ERROR: failed to allocate bulk composition storage\n");
        return (FALSE);
    }
    list[n] = (CompMusicStore*)bulkalloc.getitem(n);
    n++;
    newcomp.calling = nullptr; // Calling has been transferred - dereference in newcomp
    newcomp.allocsize = 0;
    newcomp.music.score = nullptr; // Similarly, unlink music array
    newcomp.music.nmusicdefs = 0;
    return (TRUE);
}
#endif

extern char callchars[];

// Used for storing calling output when printing course ends first
char displaybuf[MAXLINEBUF];

// If compn<=0, means composition is a 'special' which maximises the
// single musicdef[-compn]
// If compn=-nmusicdefs, this is the "max musthave blocks' composition
int Composer::displaycomp(int compn, CompMusicStore* thiscomp, LineFile& f)
{
    Node *node, *nextnode = nullptr;
    char* buf;
    int firstworkingbell, lastworkingbell;
    int callpos, queuedcolumn, nbobs, realcourseend;
    int callingbellcourseendpos;
    int fixedbells[MAXNBELLS];
    Call queuedcall;
    int call, rot, columnchecker;
    int i, j;

    f.buffer[0] = 0;
    if (!f.writeline())
        return (FALSE);
    if (thiscomp == nullptr)
    {
        printf("INTERNAL ERROR: composition not in list\n");
        return (FALSE);
    }
    // Find first and last working bells
    if (coursestructured)
    {
        // First use the specified courseend rows to work out where
        // the calling bell falls at the course end.
        callingbellcourseendpos = -1;
        for (i = 0; i < courseend.nrows; i++)
        {
            j = findcallingbell(courseend.matches[i].row);
            if (j > 0 && (j == callingbell || callingbellcourseendpos < 0))
                callingbellcourseendpos = j;
        }
        if (callingbellcourseendpos < 0)
            callingbellcourseendpos = callingbell;
        // Next set up an array to indicate which bells are static
        // at every possible course end. Include start and end rows!
        for (j = 0; j < nbells; j++)
            fixedbells[j] = startrow[j];
        for (j = 0; j < nbells; j++)
            if (fixedbells[j] != finishrow[j])
                fixedbells[j] = -1;
        for (i = 0; i < courseend.nrows; i++)
            for (j = 1; j < nbells; j++)
                if (fixedbells[j] != courseend.matches[i].row[j])
                    fixedbells[j] = -1;
        // Finally can set first and last working bell variables
        firstworkingbell = 0;
        lastworkingbell = nbells - 1;
        for (j = 0; j < nbells; j++)
            if (fixedbells[j] < 0)
                break;
            else
                firstworkingbell++;
        for (j = nbells - 1; j >= 0; j--)
            if (fixedbells[j] < 0)
                break;
            else
                lastworkingbell--;
    }
    else
    {
        callingbellcourseendpos = callingbell;
        firstworkingbell = 1;
        lastworkingbell = nbells - 1;
    }

    // Print composition title (length, method name, comp number)
    if (thiscomp->author)
        sprintf(f.buffer, "%d %s, comp. %s", thiscomp->length, methodname, thiscomp->author);
    else
    {
        sprintf(f.buffer, "%d %s, gen. SMC32", thiscomp->length, methodname);
        if (compn > 0)
            sprintf(f.buffer + strlen(f.buffer), " (No.%d)", compn);
    }
    // If compn<=0, is a special comp maximising musicdefs[-compn]
    if (compn <= 0)
    {
        if (-compn >= nmusicdefs)
            sprintf(f.buffer + strlen(f.buffer), " (Max must/shouldhave blocks)");
        else
            sprintf(f.buffer + strlen(f.buffer), " (Max %s)", musicdefs[-compn].name);
    }
    if (!f.writeline())
        return (FALSE);
    // Print out column headings
    buf = displaybuf;
    for (i = 0; callposorder[i] >= 0; i++)
        // Find the first call which is capable of producing this calling position
        for (call = 1; call <= ncalltypes; call++)
            if (callposcallmasks[i] & (1 << call))
            {
                buf += sprintf(buf, "%3s", callposnames[call][callposorder[i]]);
                break;
            }
    if (printcourseendsfirst)
    {
        buf = f.buffer;
        *buf++ = ' ';
    }
    else
        buf += sprintf(buf, "  ");
    *buf++ = ' ';
    if (startinrounds)
    {
        for (j = firstworkingbell; j <= lastworkingbell; j++)
            *buf++ = rounds[j];
    }
    else
    {
        for (j = firstworkingbell; j <= lastworkingbell; j++)
            *buf++ = rounds[startrow[j]];
    }
    *buf++ = ' ';
    *buf = 0;
    if (!printcourseendsfirst)
        buf = f.buffer;
    strcpy(buf, displaybuf);
    if (!f.writeline())
        return (FALSE);
    buf = displaybuf;

    // Print composition
    node = comp[0].node; // Start from rounds regardless of rotation
    copyrow(node->nodex->nodehead, row);
    queuedcall = PLAIN;
    queuedcolumn = 0;
    nbobs = 0;
    realcourseend = FALSE;
    rot = thiscomp->rot;
    for (i = 0; i < thiscomp->nodesperpart; i++)
    {
        call = thiscomp->calling[rot];
        nextnode = node->nextnode[call];
        if (calltypes[call] != PLAIN)
        {
            // If there is a call outstanding from an earlier column, or the same column if
            // the current and previous calls aren't both bobs, print the old call
            callpos = node->nodex->callingbellpos[call];
            if (queuedcall != PLAIN && (callpos != callposorder[queuedcolumn] || queuedcall != BOB || calltypes[call] != BOB))
            {
                if (queuedcall == BOB && nbobs > 1)
                    buf += sprintf(buf, "%3d", nbobs);
                else
                    buf += sprintf(buf, "%3c", callchars[queuedcall]);
                queuedcolumn++;
                nbobs = 0;
                queuedcall = PLAIN;
            }
            // Move on to the new call's column - use callposcallmasks[] to check each calling
            // position can be reached by this type of call
            columnchecker = 0;
            while (callpos != callposorder[queuedcolumn] || (callposcallmasks[queuedcolumn] & (1 << call)) == 0)
                if (callposorder[queuedcolumn] < 0)
                {
                    // Check we don't wrap round to second course end without finding correct column!
                    if (++columnchecker > 1)
                    {
                        printf("INTERNAL ERROR: cannot find calling position %c for call %c\n", rounds[callpos], callchars[calltypes[call]]);
                        return (FALSE);
                    }
                    // Print course end
                    if (printcourseendsfirst)
                    {
                        buf = f.buffer;
                        *buf++ = ' ';
                    }
                    else
                    {
                        buf += sprintf(buf, "  ");
                    }
                    if (realcourseend)
                    {
                        *buf++ = ' ';
                        for (j = firstworkingbell; j <= lastworkingbell; j++)
                            *buf++ = rounds[row[j]];
                        *buf++ = ' ';
                    }
                    // If no 'real' courseend has been stored, must calculate a bracketted courseend
                    else if (findcourseend(node->nodex->nodehead, callingbellcourseendpos))
                    {
                        *buf++ = '(';
                        for (j = firstworkingbell; j <= lastworkingbell; j++)
                            *buf++ = rounds[row[j]];
                        *buf++ = ')';
                    }
                    *buf = 0;
                    if (!printcourseendsfirst)
                        buf = f.buffer;
                    strcpy(buf, displaybuf);
                    if (!f.writeline())
                        return (FALSE);
                    buf = displaybuf;
                    queuedcolumn = 0;
                }
                else
                {
                    buf += sprintf(buf, "   ");
                    queuedcolumn++;
                }
            realcourseend = FALSE;
        }
        // If we have reached a course end, store the row
        if (nextnode->nodex->nodehead[callingbellcourseendpos] == callingbell)
        {
            copyrow(nextnode->nodex->nodehead, row);
            realcourseend = TRUE;
        }
        // If we have reached a call, queue it up
        if (calltypes[call] != PLAIN)
        {
            queuedcall = calltypes[call];
            if (queuedcall == BOB)
                nbobs++;
        }
        node = nextnode;
        if (++rot >= thiscomp->nodesperpart)
            rot = 0;
    }
    // Finish last line of comp
    if (queuedcall == BOB && nbobs > 1)
        buf += sprintf(buf, "%3d", nbobs);
    else
        buf += sprintf(buf, "%3c", callchars[queuedcall]);
    queuedcolumn++;
    while (callposorder[queuedcolumn] > 0)
    {
        buf += sprintf(buf, "   ");
        queuedcolumn++;
    }
    if (printcourseendsfirst)
    {
        buf = f.buffer;
        *buf++ = ' ';
    }
    else
        buf += sprintf(buf, "  ");
    // Display the part-end row regardless of whether the calling bell is home
    // - this is important for non-course-structured compositions
    if (!realcourseend)
        copyrow(nextnode->nodex->nodehead, row);
    *buf++ = ' ';
    for (j = firstworkingbell; j <= lastworkingbell; j++)
        *buf++ = rounds[row[j]];
    *buf++ = ' ';
    *buf = 0;
    if (!printcourseendsfirst)
        buf = f.buffer;
    strcpy(buf, displaybuf);
    if (!f.writeline())
        return (FALSE);
    // Non-course structured compositions may need an extra line, in the case
    // where the last course brings up a "real course end" (calling bell home)
    // but extra plains are needed to bring up the actual part end.
    if (realcourseend && !coursestructured)
    {
        copyrow(nextnode->nodex->nodehead, row);
        if (row[callingbellcourseendpos] != callingbell)
        {
            buf = f.buffer;
            if (printcourseendsfirst)
                *buf++ = ' ';
            else
            {
                queuedcolumn = 0;
                while (callposorder[queuedcolumn] > 0)
                {
                    buf += sprintf(buf, "   ");
                    queuedcolumn++;
                }
                buf += sprintf(buf, "  ");
            }
            *buf++ = ' ';
            for (j = firstworkingbell; j <= lastworkingbell; j++)
                *buf++ = rounds[row[j]];
            *buf = 0;
            if (!f.writeline())
                return (FALSE);
        }
    }
    buf = f.buffer;
    // Print number of parts, if any
    if (thiscomp->nparts > 1)
    {
        sprintf(f.buffer, "%d part", thiscomp->nparts);
        if (!f.writeline())
            return (FALSE);
    }
    // Print number of TVs, if any
    if (thiscomp->nTVs)
    {
        if (thiscomp->nTVs == 1)
            sprintf(f.buffer, "Has one trivial variation");
        else
            sprintf(f.buffer, "Has %d trivial variations", thiscomp->nTVs);
        if (!f.writeline())
            return (FALSE);
    }
    // Print music content
    sprintf(f.buffer, "Score: %d", thiscomp->score);
    if (!f.writeline())
        return (FALSE);
    if (musthaveblocks->listsize() > 0)
    {
        j = 0;
        sprintf(f.buffer, "  Musthave blocks");
        for (i = 0; i < musthaveblocks->listsize(); i++)
        {
            Block* b = (Block*)musthaveblocks->getitem(i);
            if (b == nullptr)
            {
                printf("ERROR: null returned from musthaveblocks::getitem\n");
                return (FALSE);
            }
            if (b->essential && b->entrynode->node->included)
            {
                sprintf(f.buffer + strlen(f.buffer), " %d", b->blocknumber);
                j++;
            }
        }
        if (j > 0)
            if (!f.writeline())
                return (FALSE);
        j = 0;
        sprintf(f.buffer, "  Shouldhave blocks");
        for (i = 0; i < musthaveblocks->listsize(); i++)
        {
            Block* b = (Block*)musthaveblocks->getitem(i);
            if (b == nullptr)
            {
                printf("ERROR: null returned from musthaveblocks::getitem\n");
                return (FALSE);
            }
            if (!b->essential && b->entrynode->node->included)
            {
                sprintf(f.buffer + strlen(f.buffer), " %d", b->blocknumber);
                j++;
            }
        }
        if (j > 0)
            if (!f.writeline())
                return (FALSE);
    }
    for (i = 0; i < nmusicdefs; i++)
        if (thiscomp->music.score[i])
        {
            sprintf(f.buffer, "  %d %s", thiscomp->music.score[i] / musicdefs[i].weighting, musicdefs[i].name);
            if (!f.writeline())
                return (FALSE);
        }
    return (TRUE);
}

// Given a leadhead, applies Plains until courseend appears
// Returns FALSE if courseend not found
int Composer::findcourseend(char* leadhead, int callingbellcourseendpos)
{
    char tmprow[MAXNBELLS];
    int i = 0;

    copyrow(leadhead, row);
    while (row[callingbellcourseendpos] != callingbell)
    {
        transpose(row, calltrans[internalcallnums[PLAIN]], tmprow);
        copyrow(tmprow, row);
        if (++i > courselen)
            return (FALSE);
    }
    return (TRUE);
}

// Checks the current row (and changen in lead!) to see whether it matches any of
// the rows in MusicDef m
int Composer::ismusicmatch(MusicDef& m)
{
    int i;

    for (i = 0; i < m.nrows; i++)
        if (isrowmatch(m.matches[i]))
            return (TRUE);
    return (FALSE);
}

// Checks the current row (and changen in lead!) to see whether it matches this
// MusicRow
int Composer::isrowmatch(MusicRow& m)
{
    MusicRow tempmus;
    char tmprow[MAXNBELLS];
    int i;

    // Check sign
    if (m.sign)
    {
        if (isnegative(row))
        {
            if (m.sign > 0)
                return (FALSE);
        }
        else
        {
            if (m.sign < 0)
                return (FALSE);
        }
    }
    // MUSICCOURSINGORDER is a special type. The row must be a leadhead.
    // It is permuted via plains to every position in the plain course, to see if
    // there are any matches. This is especially useful for counting handbell positions.
    if (m.type == MUSICCOURSINGORDER)
    {
        if (changen > 0)
            return (FALSE);
        tempmus.type = MUSICLEADHEAD;
        tempmus.sign = m.sign;
        copyrow(m.row, tempmus.row);
        do
        {
            if (isrowmatch(tempmus))
                return (TRUE);
            transpose(tempmus.row, calltrans[internalcallnums[PLAIN]], tmprow);
            copyrow(tmprow, tempmus.row);
        } while (!samerow(m.row, tempmus.row));
        return (FALSE);
    }
    // MUSICLEADHEAD row types only match leadheads (changen==0)
    if (m.type == MUSICLEADHEAD && changen > 0)
        return (FALSE);
    // MUSICBACK/HANDSTROKE row types only match rows at back/handstroke
    if (m.type == MUSICBACKSTROKE && (changen & 1))
        return (FALSE);
    if (m.type == MUSICHANDSTROKE && (changen & 1) == 0)
        return (FALSE);
    // MUSCWRAP row types are processed in two passes: handstroke then backstroke.
    // If a handstroke matches, the 'handstrokewrapmatch' flag is set, but FALSE returned.
    // The backstroke won't match unless the immediately preceeding h/stroke set the flag
    // !!! Relies on even lead length
    if (m.type == MUSICWRAP)
    {
        if (changen & 1) // Clear the flag before every handstroke
            m.handstrokewrapmatch = FALSE;
        else
        {
            if (!m.handstrokewrapmatch) // Backstrokes don't match unless flag set
                return (FALSE);
            for (i = 0; i < nbells; i++) // Check b/stroke against wrapbackstroke match
                if (m.wrapbackstroke[i] >= 0 && row[i] != m.wrapbackstroke[i])
                    return (FALSE);
            return (TRUE); // Yes! Both strokes of wrap match
        }
    }
    // Normal row type matches at any position in lead
    for (i = 0; i < nbells; i++)
        if (m.row[i] >= 0 && row[i] != m.row[i])
            return (FALSE);
    if (m.type == MUSICWRAP)
    {
        m.handstrokewrapmatch = TRUE; // Handstroke of wrap passed - set flag
        return (FALSE);               // But return FALSE - b/stroke must match too
    }
    return (TRUE);
}
