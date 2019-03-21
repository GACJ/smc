// SMC32 File-handling routines Copyright Mark B Davies 1998-2000

#include "filer.h"
#include "smc.h"
#include <limits.h>
#include <stdlib.h>
#include <time.h>

int isMMXsupported();

constexpr auto DELIMITERS = " ,\t";
constexpr auto MATCHWILD = "x?-.";

constexpr auto TOKEN_ROUNDS = "round";
constexpr auto TOKEN_LEADHEAD = "leadhead";
constexpr auto TOKEN_INTERNAL = "internal";
constexpr auto TOKEN_CALLFROM = "callfrom";
constexpr auto TOKEN_STARTFROM = "startfrom";
constexpr auto TOKEN_ENDIN = "endin";
constexpr auto TOKEN_PLAIN = "plain";
constexpr auto TOKEN_BOB = "bob";
constexpr auto TOKEN_SINGLE = "single";
constexpr auto TOKEN_EXTREME = "extreme";
constexpr auto TOKEN_PN = "pn";
constexpr auto TOKEN_PARTS = "part";
constexpr auto TOKEN_MAKELIB = "makelib";
constexpr auto TOKEN_USELIB = "uselib";
constexpr auto TOKEN_MUSICMIN = "min";
constexpr auto TOKEN_MUSTHAVE = "musthave";
constexpr auto TOKEN_SHOULDHAVE = "shouldhave";
// Flags
constexpr auto TOKEN_ALLROTS = "allrots";
constexpr auto TOKEN_LONGESTYET = "longestyet";
constexpr auto TOKEN_BESTYET = "bestyet";
constexpr auto TOKEN_PALINDROMIC = "palindromic";
// "Undocumented" flags
constexpr auto TOKEN_NOMMX = "nommx";
constexpr auto TOKEN_NONODES = "nonodes";
constexpr auto TOKEN_NOREGEN = "noregen";
constexpr auto TOKEN_BITTRUTH = "bittruth";

const char NOCALLINGBELL = '-';             // Sets coursestructured to FALSE
const char LIMITCALLCHAR = '<';             // Followed by max # calls allowed
const char ROWMATCHLEADHEADCHAR = 'l';      // Match leadheads only
const char ROWMATCHCOURSINGORDERCHAR = 'c'; // Match all leadheads in course
const char ROWMATCHHANDSTROKECHAR = 'h';    // Match handstrokes only
const char ROWMATCHBACKSTROKECHAR = 'b';    // Match backstrokes only
const char ROWMATCHWRAPCHAR = '/';
const char ROWMATCHPOSCHAR = '+'; // Match only positive rows
const char ROWMATCHNEGCHAR = '-'; // Match only negative rows

extern int maxnparts[];

// If there are any buffered compositions, write them out to disk
// Also write a 'checkpoint' with the current state of the engine
int Composer::flushcompbuffer(int checkpoint)
{
    if (!outfile.resetpos())
        return (FALSE);
    lastcheckpoint = clock(); //stats.lastdisplaytime;
                              // stats.nodesgenerated+= stats.nodecount;
                              // stats.nodecount = 0;
    if (ncompsinbuffer)
    {
        if (!outfile.multiwrite(compbuffer))
            return (FALSE);
        stats.ncompsoutput += ncompsinbuffer;
        ncompsinbuffer = 0;
        compbufptr = compbuffer;
    }
    outfile.markpos();
    if (checkpoint)
    {
        stats.save();
        *compbufptr++ = CHECKPOINT_SYMBOL;
        // !! Outputcomp() COULD re-call flushcompbuffer(), generating recursive loop
        // However this should only happen if the compbuffer is too small
        nparts = 1;
        nodesperpart = ncompnodes;
        comprot = 0;
        outputcomp();
        if (!outfile.multiwrite(compbuffer))
            return (FALSE);
        // Output checkpoint stats - use values saved when last comp was produced
        sprintf(outfile.buffer, "%u %u %u %u %u %u %u %d ", (int)stats.save_ncompsfound, ((int*)&stats.save_ncompsfound)[1], (int)stats.save_nrotsfound, ((int*)&stats.save_nrotsfound)[1], (int)stats.save_nodesgenerated, ((int*)&stats.save_nodesgenerated)[1], (int)stats.save_elapsed, ((int*)&stats.save_elapsed)[1]);
        if (!outfile.multiwrite(outfile.buffer))
            return (FALSE);
    }
    ncompsinbuffer = 0;
    compbufptr = compbuffer;
    outfile.close();
    return (TRUE);
}

void Composer::finaloutput()
{
    time_t systime;

    flushcompbuffer(FALSE); // Flush buffer, but don't output checkpoint
    if (outfile.open())
        outfile.resetpos();
    outfile.writeline("");
    sprintf(outfile.buffer, "%s search complete on ", TOKEN_END);
    systime = time(nullptr);
    strcat(outfile.buffer, asctime(localtime(&systime)));
    outfile.multiwrite(outfile.buffer); // asctime adds \n for us
    sprintf(outfile.buffer, "%d.%03d million nodes generated", int(stats.nodesgenerated / 1000000), (int)(stats.nodesgenerated % 1000000) / 1000);
    printf("%s\n", outfile.buffer);
    outfile.writeline();
    if (((int*)&stats.ncompsfound)[1])
        sprintf(outfile.buffer, "%llu million compositions found, ", stats.ncompsfound / 1000000);
    else
        sprintf(outfile.buffer, "%llu Compositions found, ", stats.ncompsfound);
    if (rotationalsort)
    {
        if (((int*)&stats.nrotsfound)[1])
            sprintf(outfile.buffer + strlen(outfile.buffer), "%lld million rotations, ", stats.nrotsfound / 1000000);
        else
            sprintf(outfile.buffer + strlen(outfile.buffer), "%lld rotations, ", stats.nrotsfound);
    }
    sprintf(outfile.buffer + strlen(outfile.buffer), "%d output", stats.ncompsoutput);
    printf("%s\n", outfile.buffer);
    outfile.writeline();
    sprintf(outfile.buffer, "Best score %d, longest length %d", stats.bestscore, stats.longestlength);
    printf("%s\n", outfile.buffer);
    outfile.writeline();
    sprintf(outfile.buffer, "Time taken: ");
    // Print elapsed time to thousandth of a second accuracy
    printelapsed(outfile.buffer + strlen(outfile.buffer), FALSE);
    printf("%s\n", outfile.buffer);
    outfile.writeline();
    // Print library stats
    if (usefraglib)
    {
        sprintf(outfile.buffer, "%d fragments pruned", stats.prunecount);
        printf("%s\n", outfile.buffer);
        outfile.writeline();
    }
    if (makefraglib)
    {
        sprintf(outfile.buffer, "%d comps in hashtable; %d out of %d entries used", comphasher.nitems, comphasher.entriesused, comphasher.gettablesize());
        printf("%s\n", outfile.buffer);
        outfile.writeline();
        sprintf(outfile.buffer, "%d fragments written to %s", stats.nfragsfound, fraglib.getname());
        printf("%s\n", outfile.buffer);
        outfile.writeline();
    }
    outfile.close();
    printf("Output file %s complete\n", outfile.getname());
}

void Composer::printelapsed(char* buf, int nearestsecond)
{
    int seconds, minutes = 0, hours = 0;

    seconds = (int)(stats.elapsed / CLOCKS_PER_SEC);
    if (seconds >= 60)
    {
        minutes = seconds / 60;
        seconds = seconds % 60;
        if (minutes >= 60)
        {
            hours = minutes / 60;
            minutes = minutes % 60;
        }
    }
    if (nearestsecond)
        sprintf(buf, "%d:%02d:%02d", hours, minutes, seconds);
    else
        sprintf(buf, "%d:%02d:%02d.%03lld", hours, minutes, seconds, stats.elapsed % CLOCKS_PER_SEC);
}

const char* calltokens[NDIFFCALLS] = { TOKEN_PLAIN, TOKEN_BOB, TOKEN_SINGLE, TOKEN_EXTREME };

// For required format see readinputfile() below
int Composer::writefileheader(LineFile& f)
{
    time_t systime;
    int i, j;

    if (!f.incextension(OUTEXT))
        return (FALSE);
    f.setmode("w");
    sprintf(f.buffer, "/ %s (c) 1998 Mark B. Davies & Graham A.C. John", VERSION);
    if (!f.writeline())
        return (FALSE);
    if (!f.writeline("/ This is a machine-generated composition output file - do not alter"))
        return (FALSE);
    if (!f.writeline("/ Scroll down to the 'SMC32 search started' line to see the compositions"))
        return (FALSE);
    if (!f.writeline("/ If there is no 'search complete' at the end, the search may be restarted"))
        return (FALSE);
    // Write out method, lengths, min score, calling positions, calling bell
    if (!writeheaderessential(f))
        return (FALSE);
    // Write out allowed parts
    if (!exclude.allpartsallowed)
    {
        strcpy(f.buffer, TOKEN_PARTS);
        for (i = 1; i <= MAXNPARTS; i++)
            if (exclude.allowedparts[i])
                sprintf(f.buffer + strlen(f.buffer), " %d", i);
        if (!f.writeline())
            return (FALSE);
    }
    // Write out any rounds specifiers
    if (!exclude.nointernalrounds)
    {
        sprintf(f.buffer, "%s %s", TOKEN_ROUNDS, TOKEN_INTERNAL);
        if (!exclude.noleadheadrounds)
            sprintf(f.buffer + strlen(f.buffer), " %s", TOKEN_LEADHEAD);
        if (!f.writeline())
            return (FALSE);
    }
    // Write out library controls
    if (makefraglib)
    {
        sprintf(f.buffer, "%s %d", TOKEN_MAKELIB, ncompstostore);
        if (!f.writeline())
            return (FALSE);
    }
    // Write out "must-have" blocks
    if (musthaveblocks != nullptr)
        for (i = 0; i < musthaveblocks->listsize(); i++)
        {
            Block* b = (Block*)musthaveblocks->getitem(i);
            if (b->essential)
                sprintf(f.buffer, "%s ", TOKEN_MUSTHAVE);
            else
                sprintf(f.buffer, "%s ", TOKEN_SHOULDHAVE);
            writerow(b->entrylh, f.buffer + strlen(f.buffer));
            sprintf(f.buffer + strlen(f.buffer), " ");
            writerow(b->exitlh, f.buffer + strlen(f.buffer));
            sprintf(f.buffer + strlen(f.buffer), " %s", b->calling);
            if (!f.writeline())
                return (FALSE);
        }
    // Write out flags
    f.buffer[0] = 0;
    if (showallrots)
        sprintf(f.buffer + strlen(f.buffer), "%s ", TOKEN_ALLROTS);
    if (showlongestyet)
        sprintf(f.buffer + strlen(f.buffer), "%s ", TOKEN_LONGESTYET);
    if (showbestyet)
        sprintf(f.buffer + strlen(f.buffer), "%s ", TOKEN_BESTYET);
    if (palindromic)
        sprintf(f.buffer + strlen(f.buffer), "%s ", TOKEN_PALINDROMIC);
    if (singleleadnodes)
        sprintf(f.buffer + strlen(f.buffer), "%s ", TOKEN_NONODES);
    if (!useMMX && isMMXsupported())
        sprintf(f.buffer + strlen(f.buffer), "%s ", TOKEN_NOMMX);
    if (disableregen)
        sprintf(f.buffer + strlen(f.buffer), "%s ", TOKEN_NOREGEN);
    if (bitwisetruthflags)
        sprintf(f.buffer + strlen(f.buffer), "%s ", TOKEN_BITTRUTH);
    if (!f.writeline())
        return (FALSE);
    // Write out music
    if (!f.writeline(TOKEN_MUSIC))
        return (FALSE);
    for (i = 0; i < nmusicdefs; i++)
    {
        if (!f.writeline(musicdefs[i].name))
            return (FALSE);
        if (musicdefs[i].weighting)
            sprintf(f.buffer, "%d %d", musicdefs[i].weighting, musicdefs[i].minscore / abs(musicdefs[i].weighting));
        else
            sprintf(f.buffer, "%d %d", musicdefs[i].weighting, musicdefs[i].minscore);
        for (j = 0; j < musicdefs[i].nrows; j++)
            writerowmatch(f.buffer + strlen(f.buffer), musicdefs[i].matches[j]);
        if (!f.writeline())
            return (FALSE);
    }
    // Write out excluded rows
    if (exclude.nrows)
    {
        if (!f.writeline("exclusions"))
            return (FALSE);
        sprintf(f.buffer, "-1 0");
        for (i = 0; i < exclude.nrows; i++)
            writerowmatch(f.buffer + strlen(f.buffer), exclude.rows[i]);
        if (!f.writeline())
            return (FALSE);
    }
    // Write start time
    if (!f.writeline(""))
        return (FALSE);
    sprintf(f.buffer, "%s search started on ", TOKEN_START);
    systime = time(nullptr);
    strcat(f.buffer, asctime(localtime(&systime)));
    if (!outfile.multiwrite(outfile.buffer)) // asctime adds \n for us
        return (FALSE);
    f.markpos();
    f.close();
    f.setmode("r+");
    return (TRUE);
}

int FragmentLibrary::writelibheader(Composer* ring)
{
    f.newfile(ring->outfile.getname());
    f.changeexttype(LIBEXT);
    f.setmode("w");
    sprintf(f.buffer, "/ %s (c) 1998 Mark B. Davies & Graham A.C. John", VERSION);
    if (!f.writeline())
        return (FALSE);
    if (!f.writeline("/ Fragment Library file. Corresponds to search file:"))
        return (FALSE);
    // Write out name of search file originally used for library generation
    sprintf(f.buffer, "%s", ring->outfile.getname());
    if (!f.writeline())
        return (FALSE);
    // Write out method, lengths, min score, calling positions
    if (!ring->writeheaderessential(f))
        return (FALSE);
    if (!f.writeline(""))
        return (FALSE);
    if (!f.writeline(TOKEN_FRAGMENTLIB))
        return (FALSE);
    f.close();
    return (TRUE);
}

// If filename==nullptr, assume already set up
// Otherwise, filename imported and changed to LIBEXT, preserving number
int FragmentLibrary::read(Composer* ring, char* filename)
{
    CompStore dupcalling, primcalling;
    Fragment frag;
    Node* startnode;
    char *p, *q;
    int endplacebell, ret;
    int i;

    if (filename)
    {
        f.newfile(filename);
        f.changeexttype(LIBEXT);
    }
    f.setmode("r");
    if (!f.open())
    {
        printf("ERROR: Failed to open library file %s\n", f.getname());
        return (FALSE);
    }
    // Read search-file name (not used at present)
    if (!f.readline())
    {
        printf("ERROR: Failed to read search-file name from %s\n", f.getname());
        return (FALSE);
    }
    // Read method name (ignored)
    if (!f.readline())
    {
        printf("ERROR: Failed to read method name from %s\n", f.getname());
        return (FALSE);
    }
    // Read PN, check it's the same as current Composer method
    if (!f.readline())
    {
        printf("ERROR: Failed to read place notation from %s\n", f.getname());
        return (FALSE);
    }
    if (!ring->m->issamepn(f.buffer))
    {
        printf("ERROR: Cannot use this library - different method!\n");
        return (FALSE);
    }
    // Read minimum and maximum lengths (ignored)
    if (!f.readline())
        return (TRUE);
    // Read minimum score (ignored)
    if (!f.readline())
        return (TRUE);
    // Read calling positions
    while (f.readline())
    {
        p = strtok(f.buffer, DELIMITERS);
        // Stop when a FRAGMENT section is reached
        if (strcmpi(p, TOKEN_FRAGMENTLIB) == 0)
            break;
        else if (strncmpi(p, TOKEN_PLAIN, strlen(TOKEN_PLAIN)) == 0)
        {
            // Should check the same as Composer!!
            continue;
        }
        else if (strncmpi(p, TOKEN_BOB, strlen(TOKEN_BOB)) == 0)
        {
            // Should check the same as Composer!!
            continue;
        }
        else if (strncmpi(p, TOKEN_SINGLE, strlen(TOKEN_SINGLE)) == 0)
        {
            // Should check the same as Composer!!
            continue;
        }
        else if (strncmpi(p, TOKEN_EXTREME, strlen(TOKEN_EXTREME)) == 0)
        {
            // Should check the same as Composer!!
            continue;
        }
        else
        {
            printf("ERROR: unrecognised line in fragment library %s\n", f.getname());
            printf("%s\n", p);
            return (FALSE);
        }
    }
    // Finally, read fragments
    while (f.readline())
    {
        frag.clear();
        // Read fragment length
        p = strtok(f.buffer, DELIMITERS);
        if (p == nullptr)
        {
            printf("ERROR: incomplete fragment line in %s\n", f.getname());
            return (FALSE);
        }
        frag.length = atoi(p);
        if (frag.length <= 0 || frag.length > MAXFRAGLENGTH)
        {
            printf("ERROR: bad fragment length %s\n", p);
            return (FALSE);
        }
        // Read fragment start and end placebell
        p = strtok(nullptr, DELIMITERS);
        if (p == nullptr)
        {
            printf("ERROR: incomplete fragment line in %s\n", f.getname());
            return (FALSE);
        }
        q = strchr(rounds, *p);
        if (q == nullptr)
        {
            printf("ERROR: bad fragment start place bell %s\n", p);
            return (FALSE);
        }
        frag.startplacebell = (char)(q - rounds);
        p = strtok(nullptr, DELIMITERS);
        if (p == nullptr)
        {
            printf("ERROR: incomplete fragment line in %s\n", f.getname());
            return (FALSE);
        }
        q = strchr(rounds, *p);
        if (q == nullptr)
        {
            printf("ERROR: bad fragment final place bell %s\n", p);
            return (FALSE);
        }
        endplacebell = (int)(q - rounds);
        // Find node which starts at this placebell
        startnode = ring->findstartingnode(frag.startplacebell);
        if (startnode == nullptr)
            return (FALSE);
        // Read fragment duplicate calling
        p = strtok(nullptr, "\t");
        if (p == nullptr)
        {
            printf("ERROR: incomplete fragment line in %s\n", f.getname());
            return (FALSE);
        }
        dupcalling.nparts = 1;
        if (!ring->readcalling(p, dupcalling, startnode, frag.length))
            return (FALSE);
        // Read fragment primary calling
        p = strtok(nullptr, "\t");
        if (p == nullptr)
        {
            printf("ERROR: incomplete fragment line in %s\n", f.getname());
            return (FALSE);
        }
        primcalling.nparts = 1;
        if (!ring->readcalling(p, primcalling, startnode, frag.length))
            return (FALSE);
        // Collapse callings into fragment patterns
        for (i = 0; i < frag.length; i++)
        {
            frag.shiftupduplicate();
            frag.shiftupprimary();
            frag.primary[0] |= primcalling.calling[i] & 3;
            frag.duplicate[0] |= dupcalling.calling[i] & 3;
        }
        // Finally, add to fragment library
        ret = add(frag, endplacebell);
        if (ret < 0)
            return (FALSE);
        if (ret > 0)
            ring->stats.nfragsfound++;
    }
    f.close();
    printf("%d fragments read from library %s\n", ring->stats.nfragsfound, f.getname());
    return (TRUE);
}

int Composer::writeheaderessential(LineFile& f)
{
    char tmprow[MAXNBELLS];
    int i, j;

    if (!f.writeline(m->name))
        return (FALSE);
    if (!f.writeline(m->getpn()))
        return (FALSE);
    sprintf(f.buffer, "%d %d", minlength, maxlength);
    if (!f.writeline())
        return (FALSE);
    sprintf(f.buffer, "%d", minscore);
    if (!f.writeline())
        return (FALSE);
    for (i = 0; i < nbells; i++)
        if (i < 7 - 1)
            tmprow[i] = -1;
        else
            tmprow[i] = i;
    if (!coursestructured || callingbell != nbells - 1 || courseend.nrows > 1 || !samerow(courseend.matches[0].row, tmprow))
    {
        sprintf(f.buffer, "%s ", TOKEN_CALLFROM);
        if (!coursestructured)
            sprintf(f.buffer + strlen(f.buffer), "%c", NOCALLINGBELL);
        else
        {
            sprintf(f.buffer + strlen(f.buffer), "%c", rounds[callingbell]);
            for (i = 0; i < courseend.nrows; i++)
                writerowmatch(f.buffer + strlen(f.buffer), courseend.matches[i]);
        }
        if (!f.writeline())
            return (FALSE);
    }
    if (!startinrounds)
    {
        sprintf(f.buffer, "%s ", TOKEN_STARTFROM);
        char* p = f.buffer + strlen(f.buffer);
        for (i = 0; i < nbells; i++)
            *p++ = rounds[startrow[i]];
        *p++ = 0;
        if (!f.writeline())
            return (FALSE);
    }
    if (!endinrounds)
    {
        sprintf(f.buffer, "%s ", TOKEN_ENDIN);
        char* p = f.buffer + strlen(f.buffer);
        for (i = 0; i < nbells; i++)
            *p++ = rounds[finishrow[i]];
        *p++ = 0;
        if (!f.writeline())
            return (FALSE);
    }
    if (!exclude.defaultcalls[PLAIN])
    {
        strcpy(f.buffer, calltokens[PLAIN]);
        i = internalcallnums[PLAIN];
        if (maxcalls[i] < INT_MAX)
            sprintf(f.buffer + strlen(f.buffer), " %c%d", LIMITCALLCHAR, maxcalls[i]);
        for (j = 1; j < nbells; j++)
        {
            if (maxcallsperpos[j][i] < INT_MAX)
                sprintf(f.buffer + strlen(f.buffer), " %c%c%d", rounds[j], LIMITCALLCHAR, maxcallsperpos[j][i]);
        }
        if (!f.writeline())
            return (FALSE);
    }
    for (i = 0; i <= ncalltypes; i++)
        if (calltypes[i] != PLAIN)
        {
            strcpy(f.buffer, calltokens[calltypes[i]]);
            sprintf(f.buffer + strlen(f.buffer), " %s%s", TOKEN_PN, m->getcallpn(calltypes[i]));
            if (maxcalls[i] < INT_MAX)
                sprintf(f.buffer + strlen(f.buffer), " %c%d", LIMITCALLCHAR, maxcalls[i]);
            for (j = 1; j < nbells; j++)
            {
                if (exclude.allowedcalls[i][j])
                    sprintf(f.buffer + strlen(f.buffer), " %c%s", rounds[j], callposnames[i][j]);
                if (maxcallsperpos[j][i] < INT_MAX)
                    sprintf(f.buffer + strlen(f.buffer), "%c%d", LIMITCALLCHAR, maxcallsperpos[j][i]);
            }
            if (!f.writeline())
                return (FALSE);
        }
    return (TRUE);
}

const char* Block::blocktype()
{
    if (essential)
        return TOKEN_MUSTHAVE;
    return TOKEN_SHOULDHAVE;
}

// Format of input file is (on separate lines):
//  Name
//  PN
//  Minlength maxlength
//  Min score
// These are optional:
//  Flags: longestyet bestyet allrots nonodes nommx noregen
//  PARTS n1 n2 n2 ...
//  CALLFROM b courseend1 courseend2 ...
//  STARTFROM row
//  ENDIN row
// (zero or more of) MUSTHAVE entrylh exitlh calling
//  MAKELIB maxncompstostore
//  USELIB <library-file>.lf0
//  PLAIN b1 b2 ...
//  BOB callpos1 callpos2 ...
//  SINGLE callpos1 callpos2 ...
// Each calling position should be specified as the tenor position immediately
//  followed by a descriptive name, with no spaces. E.g. 3B
// This must be at the very end of the file:
//  MUSIC <music-file>.mus
// A '/' as the first character can be used to comment out a line

// !! Any changes must be reflected in writefileheader() above
int Composer::readinputfile(LineFile& f)
{
    char* buf = f.buffer;
    char buf2[MAXLINEBUF];
    char *p, *b;
    int i, call, leadheadrounds;
    int musthave = 0, shouldhave = 0;

    safedelete(musthaveblocks);
    musthaveblocks = new BulkList(5, sizeof(Block));

    musicinclude = nullptr;
    if (!f.open())
    {
        printf("ERROR: Failed to open %s\n", f.getname());
        return (FALSE);
    }
    // Read name and place notation
    if (!f.readline())
    {
        printf("ERROR: Failed to read method name from %s\n", f.getname());
        return (FALSE);
    }
    strcpy(buf2, buf);
    if (!f.readline())
    {
        printf("ERROR: Failed to read place notation from %s\n", f.getname());
        return (FALSE);
    }
    // Pass name and pn to method library code
    if (!m->newmethod(buf2, buf))
    {
        printf("ERROR: failed to parse method %s PN %s\n", buf2, buf);
        return (FALSE);
    }
    // Set various composing flags and default calling positions
    if (!setdefaults())
        return (FALSE);
    // Read minimum and maximum lengths
    if (!f.readline())
        return (FALSE);
    sscanf(buf, " %d %d", &minlength, &maxlength);
    // Read minimum score
    if (!f.readline())
        return (FALSE);
    sscanf(buf, " %d", &minscore);
    // All other fields are optional
    // Read flags and calling positions (if any)
    while (f.readline())
    {
        p = strtok(buf, DELIMITERS);
        // Stop when a MUSIC section is reached
        if (strcmpi(p, TOKEN_MUSIC) == 0)
        {
            musicinclude = strtok(nullptr, DELIMITERS);
            break;
        }

        // Read MAKELIBRARY line
        else if (strncmpi(p, TOKEN_MAKELIB, strlen(TOKEN_MAKELIB)) == 0)
        {
            makefraglib = TRUE;
            //   storecomps = TRUE;
            ncompstostore = 10000; // default CompHasher/CompSorter size
            p = strtok(nullptr, DELIMITERS);
            if (p)
                ncompstostore = atoi(p);
        }

        // Read USELIBRARY line
        else if (strncmpi(p, TOKEN_USELIB, strlen(TOKEN_USELIB)) == 0)
        {
            usefraglib = TRUE;
            p = strtok(nullptr, DELIMITERS);
            if (p == nullptr)
            {
                printf("ERROR: need library filename\n");
                return (FALSE);
            }
            if (!fraglib.newfile(p))
                return (FALSE);
        }

        // Read PARTS line
        else if (strncmpi(p, TOKEN_PARTS, strlen(TOKEN_PARTS)) == 0)
        {
            p = strtok(nullptr, DELIMITERS);
            if (p)
            {
                exclude.allpartsallowed = FALSE;
                exclude.minnparts = INT_MAX;
                for (i = 1; i <= MAXNPARTS; i++)
                    exclude.allowedparts[i] = FALSE;
                while (p)
                {
                    i = atoi(p);
                    if (i < 1 || i > maxnparts[nbells])
                    {
                        printf("ERROR: bad number of parts %s\n", p);
                        return (FALSE);
                    }
                    exclude.allowedparts[i] = TRUE;
                    if (i < exclude.minnparts)
                        exclude.minnparts = i;
                    p = strtok(nullptr, DELIMITERS);
                }
                if (exclude.minnparts == INT_MAX)
                {
                    exclude.allpartsallowed = TRUE;
                    for (i = 1; i <= MAXNPARTS; i++)
                        exclude.allowedparts[i] = TRUE;
                }
            }
        }

        // Read ROUNDS line
        else if (strncmpi(p, TOKEN_ROUNDS, strlen(TOKEN_ROUNDS)) == 0)
        {
            leadheadrounds = FALSE;
            exclude.noleadheadrounds = FALSE;
            exclude.nointernalrounds = TRUE;
            rotationalsort = TRUE;
            p = strtok(nullptr, DELIMITERS);
            while (p)
            {
                if (strcmpi(p, TOKEN_LEADHEAD) == 0)
                    leadheadrounds = TRUE;
                else if (strcmpi(p, TOKEN_INTERNAL) == 0)
                {
                    exclude.nointernalrounds = FALSE;
                    rotationalsort = FALSE; // Can't have regen if internal rounds wanted!
                }
                else
                    printf("WARNING: unrecognised flag on ROUNDS line %s\n", p);
                p = strtok(nullptr, DELIMITERS);
            }
            // Turn off leadheadrounds if ONLY internal rounds was specified
            if (exclude.nointernalrounds == FALSE && leadheadrounds == FALSE)
                exclude.noleadheadrounds = TRUE;
        }

        // Read CALLFROM line
        else if (strncmpi(p, TOKEN_CALLFROM, strlen(TOKEN_CALLFROM)) == 0)
        {
            p = strtok(nullptr, DELIMITERS);
            if (p == nullptr)
            {
                printf("ERROR: calling bell not specified\n");
                return (FALSE);
            }
            if (p[0] == NOCALLINGBELL)
            {
                // No calling bell - means non course-structured composition
                coursestructured = FALSE;
                p = strtok(nullptr, DELIMITERS);
                if (p != nullptr)
                    printf("WARNING: if non course-structured calling set, cannot specify course ends\n");
            }
            else
            {
                b = strchr(rounds, p[0]);
                if (b == nullptr)
                {
                    printf("ERROR: unrecognised calling bell %s\n", p);
                    return (FALSE);
                }
                callingbell = (int)(b - rounds);
                // Copy into buf2, count how many courseend row matches
                p = strtok(nullptr, "");
                if (p == nullptr)
                {
                    printf("ERROR: no course-end rows specified for CALLFROM line\n");
                    return (FALSE);
                }
                strcpy(buf2, p);
                i = 0;
                p = strtok(p, DELIMITERS);
                while (p)
                {
                    i++;
                    p = strtok(nullptr, DELIMITERS);
                }
                if (i == 0)
                {
                    printf("ERROR: no course-end rows specified for CALLFROM line\n");
                    return (FALSE);
                }
                if (!courseend.ensurespace(i))
                {
                    printf("ERROR: course-end row match allocation failed\n");
                    return (FALSE);
                }
                // Load each courseend row
                p = strtok(buf2, DELIMITERS);
                for (i = 0; i < courseend.nrows && p; i++)
                {
                    if (!readrowmatch(p, courseend.matches[i]))
                        return (FALSE);
                    if (courseend.matches[i].type != MUSICANYROW)
                    {
                        printf("ERROR: course ends cannot have musicdef types: %s\n", p);
                        return (FALSE);
                    }
                    p = strtok(nullptr, DELIMITERS);
                }
                if (i != courseend.nrows)
                {
                    printf("INTERNAL ERROR: unexpected number of rows in CALLFROM courseends\n");
                    return (FALSE);
                }
            }
        }

        // Read STARTFROM line
        else if (strncmpi(p, TOKEN_STARTFROM, strlen(TOKEN_STARTFROM)) == 0)
        {
            p = strtok(nullptr, DELIMITERS);
            if (!readlh(p, startrow, "ERROR in STARTFROM line"))
                return (FALSE);
            startinrounds = FALSE;
            // If rounds has been set as the start row, ignore it
            if (isrounds(startrow))
                startinrounds = TRUE;
        }

        // Read ENDIN line
        else if (strncmpi(p, TOKEN_ENDIN, strlen(TOKEN_ENDIN)) == 0)
        {
            p = strtok(nullptr, DELIMITERS);
            if (!readlh(p, finishrow, "ERROR in ENDIN line"))
                return (FALSE);
            endinrounds = FALSE;
            // If rounds has been set as the end row, ignore it
            if (isrounds(finishrow))
                endinrounds = TRUE;
        }

        // Read MUSTHAVE/SHOULDHAVE blocks
        else if (strncmpi(p, TOKEN_MUSTHAVE, strlen(TOKEN_MUSTHAVE)) == 0 || strncmpi(p, TOKEN_SHOULDHAVE, strlen(TOKEN_SHOULDHAVE)) == 0)
        {
            Block* block = new Block();
            block->essential = (strncmpi(p, TOKEN_MUSTHAVE, strlen(TOKEN_MUSTHAVE)) == 0);
            if (block->essential)
                block->blocknumber = ++musthave;
            else
                block->blocknumber = ++shouldhave;
            p = strtok(nullptr, DELIMITERS);
            if (!readlh(p, block->entrylh, "ERROR in entry leadhead for MUST/SHOULDHAVE block"))
                return (FALSE);
            p = strtok(nullptr, DELIMITERS);
            if (!readlh(p, block->exitlh, "ERROR in exit leadhead for MUST/SHOULDHAVE block"))
                return (FALSE);
            p = strtok(nullptr, DELIMITERS);
            if (p == nullptr)
            {
                printf("ERROR in %s BLOCK: no calling specified\n", block->blocktype());
                return (FALSE);
            }
            b = block->calling;
            do
            {
                strcpy(b, p);
                b = b + strlen(b);
                *b++ = ' ';
                p = strtok(nullptr, DELIMITERS);
            } while (p != nullptr);
            *--b = 0;
            musthaveblocks->add(block);
        }

        // Read call lines
        else if (strncmpi(p, TOKEN_PLAIN, strlen(TOKEN_PLAIN)) == 0)
        {
            if (!readcall(addcalltype(PLAIN)))
                return (FALSE);
        }
        else if (strncmpi(p, TOKEN_BOB, strlen(TOKEN_BOB)) == 0)
        {
            if (!readcall(addcalltype(BOB)))
                return (FALSE);
        }
        else if (strncmpi(p, TOKEN_SINGLE, strlen(TOKEN_SINGLE)) == 0)
        {
            if (!readcall(addcalltype(SINGLE)))
                return (FALSE);
        }
        else if (strncmpi(p, TOKEN_EXTREME, strlen(TOKEN_EXTREME)) == 0)
        {
            if (!readcall(addcalltype(EXTREME)))
                return (FALSE);
        }

        // Read flags
        else
            while (p)
            {
                if (strcmpi(p, TOKEN_ALLROTS) == 0)
                    showallrots = TRUE;
                else if (strcmpi(p, TOKEN_LONGESTYET) == 0)
                    showlongestyet = TRUE;
                else if (strcmpi(p, TOKEN_BESTYET) == 0)
                    showbestyet = TRUE;
                else if (strcmpi(p, TOKEN_PALINDROMIC) == 0)
                    palindromic = TRUE;
                else if (strcmpi(p, TOKEN_NONODES) == 0)
                    singleleadnodes = TRUE;
                else if (strcmpi(p, TOKEN_NOMMX) == 0)
                    useMMX = FALSE;
                else if (strcmpi(p, TOKEN_NOREGEN) == 0)
                    disableregen = TRUE;
                else if (strcmpi(p, TOKEN_BITTRUTH) == 0)
                    bitwisetruthflags = TRUE;
                else
                    printf("WARNING: unrecognised flag %s\n", p);
                p = strtok(0, DELIMITERS);
            }
    }

    // Palindromic search disables regen and multipart searches
    if (palindromic)
    {
        rotationalsort = FALSE;
        exclude.allpartsallowed = TRUE;
        exclude.minnparts = 1;
        exclude.nointernalrounds = TRUE;
        exclude.noleadheadrounds = TRUE;
    }
    // If different start and finish rows, turn off rotational sort
    if (!samerow(startrow, finishrow))
    {
        samestartandend = FALSE;
        rotationalsort = FALSE;
    }
    // Or, if using "musthave" blocks, turn off rotational sort
    if (musthaveblocks->listsize() > 0)
    {
        rotationalsort = FALSE;
    }
    // If searching for multiparts, must have regen on!
    if (!exclude.allpartsallowed)
        if (!rotationalsort)
        {
            if (musthaveblocks->listsize() > 0)
            {
                printf("ERROR: cannot search for multiparts when using %s/%s blocks\n", TOKEN_MUSTHAVE, TOKEN_SHOULDHAVE);
                printf("       Comment out either the PARTS or the %s/%s blocks\n", TOKEN_MUSTHAVE, TOKEN_SHOULDHAVE);
            }
            if (!samestartandend)
            {
                printf("ERROR: cannot search for multiparts with different starting and ending rows\n");
                printf("       Comment out either the PARTS or the STARTFROM/ENDIN lines\n");
            }
            else
            {
                printf("ERROR: cannot search for multiparts with internal rounds\n");
                printf("       Comment out either the PARTS or the ROUNDS line\n");
            }
            return (FALSE);
        }
    // If debugging flag noregen is set, turn off rotational sort
    if (disableregen)
        rotationalsort = FALSE;
    if (makefraglib)
        rotationalsort = exclude.nointernalrounds = exclude.noleadheadrounds = FALSE;
    // If 'coursestructured' flag CLEAR, allow all calling positions for any position
    // All course ends will also be allowed by the table-builder
    if (!coursestructured)
    {
        countingcalls = FALSE;
        for (call = 0; call <= ncalltypes; call++)
        {
            if (calltypes[call] == PLAIN)
                exclude.defaultcalls[PLAIN] = TRUE;
            else
                exclude.defaultcalls[calltypes[call]] = FALSE;
            maxcalls[call] = INT_MAX;
            for (i = 0; i < nbells; i++)
            {
                maxcallsperpos[i][call] = INT_MAX;
                exclude.allowedcalls[call][i] = TRUE;
            }
        }
    }
    return (TRUE);
}

// Reads leadhead row from input *p, checking for right number of bells, good characters etc
int Composer::readlh(char* p, char* buf, const char* errprefix)
{
    int i;
    char* b;

    if (p == nullptr)
    {
        printf("%s:\nno row given\n", errprefix);
        return (FALSE);
    }
    if ((int)strlen(p) != nbells)
    {
        printf("%s:\nincorrect number of bells in row %s\n", errprefix, p);
        return (FALSE);
    }
    if (p[0] != '1')
    {
        printf("%s:\nrow %s not a leadhead\n", errprefix, p);
        return (FALSE);
    }
    for (i = 0; i < nbells; i++)
    {
        b = strchr(rounds, p[i]);
        if (b == nullptr)
        {
            printf("%s:\nbad character in %s\n", errprefix, p);
            return (FALSE);
        }
        buf[i] = (char)(b - rounds);
    }
    return (TRUE);
}

// Adds in new call type to arrays
// Returns internal number
int Composer::addcalltype(Call calltype)
{
    int i, j, cpos;

    // Is call already present in calltypes array?
    for (i = 0; i <= ncalltypes; i++)
        if (calltypes[i] == calltype)
            return (i);
            // No - have to add it in
            // First find where to insert
#ifdef SWAPCALLTYPES
    for (cpos = ncalltypes; cpos >= 0; cpos--)
#else
    for (cpos = 0; cpos <= ncalltypes; cpos++)
#endif
        if (calltype < calltypes[cpos])
            break;
    // Move other call types out of the way
    for (i = ncalltypes; i >= cpos; i--)
    {
        // Note calltrans[] and internalcallnums[] are set in Composer::setup(), later
        calltypes[i + 1] = calltypes[i];
        maxcalls[i + 1] = maxcalls[i];
        for (j = 0; j < nbells; j++)
        {
            exclude.allowedcalls[i + 1][j] = exclude.allowedcalls[i][j];
            maxcallsperpos[j][i + 1] = maxcallsperpos[j][i];
            strcpy(callposnames[i + 1][j], callposnames[i][j]);
        }
    }
    // Poke in the new call type
    ncalltypes++;
    calltypes[cpos] = calltype;
    defaultcallingpositions(cpos);
    return (cpos);
}

int readcalllimit(char* p)
{
    int max = INT_MAX;
    if (p)
    {
        if (p[0] == LIMITCALLCHAR)
            sscanf(p + 1, "%d", &max);
    }
    return max;
}

int Composer::readcall(int call)
{
    char *p, *b;
    int i, l;
    int calllimit;

    p = strtok(nullptr, DELIMITERS);
    // Check for max call limit before pn...
    calllimit = readcalllimit(p);
    if (calllimit < INT_MAX)
        p = strtok(nullptr, DELIMITERS);
    // See if call pn specified
    if (calltypes[call] != PLAIN)
        if (p && strncmpi(p, TOKEN_PN, strlen(TOKEN_PN)) == 0)
        {
            p += strlen(TOKEN_PN);
            if (!m->newcall(calltypes[call], p))
                return (FALSE);
            p = strtok(nullptr, DELIMITERS);
        }
    // ... and after.
    if (calllimit == INT_MAX)
    {
        calllimit = readcalllimit(p);
        if (calllimit < INT_MAX)
            p = strtok(nullptr, DELIMITERS);
    }
    if (calllimit < INT_MAX)
    {
        countingcalls = TRUE;
        maxcalls[call] = calllimit;
        // A call limit of 0 means the entire call is disabled
        if (calllimit <= 0)
        {
            exclude.defaultcalls[calltypes[call]] = FALSE;
            for (i = 0; i < nbells; i++)
                exclude.allowedcalls[call][i] = FALSE;
            return (TRUE);
        }
    }
    // Get rid of default calling positions
    if (p)
    {
        exclude.defaultcalls[calltypes[call]] = FALSE;
        // For non-Plain calls, assume no other calling position than those specified
        if (calltypes[call] != PLAIN)
            for (i = 0; i < nbells; i++)
                exclude.allowedcalls[call][i] = FALSE;
        // Remove default calling position names
        for (i = 0; i < nbells; i++)
            strcpy(callposnames[call][i], "@");
    }
    // Read in calling positions
    while (p)
    {
        b = strchr(rounds, p[0]);
        if (b == nullptr)
        {
            printf("ERROR: unrecognised calling position %s\n", p);
            return (FALSE);
        }
        i = (int)(b - rounds);
        exclude.allowedcalls[call][i] = TRUE;
        l = MAXCALLPOSNAME;
        // See if there is a call limit (e.g. "<12") immediately after call pos name
        b = strchr(p + 1, LIMITCALLCHAR);
        if (b != nullptr)
            l = (int)(b - (p + 1));
        if (calltypes[call] == PLAIN)
        {
            if (p[1] != 0 && p[1] != LIMITCALLCHAR)
            {
                printf("ERROR: cannot name plain calling positions!\n");
                return (FALSE);
            }
        }
        else
        {
            if (p[1] == 0)
            {
                printf("ERROR: unnamed calling position %s\n", p);
                return (FALSE);
            }
            strncpy(callposnames[call][i], p + 1, l);
            callposnames[call][i][l] = 0; // ensure null-terminated
        }
        // Parse call limit immediately after call pos name...
        calllimit = readcalllimit(b);
        p = strtok(nullptr, DELIMITERS);
        // ... as well as in the next token, when separated by a space.
        if (calllimit == INT_MAX)
        {
            calllimit = readcalllimit(p);
            if (calllimit < INT_MAX)
                p = strtok(nullptr, DELIMITERS);
        }
        if (calllimit < INT_MAX)
        {
            // A call limit of 0 is treated differently - we don't have to count
            // calls, the position is turned off completely (most useful for Plains)
            if (calllimit <= 0)
                exclude.allowedcalls[call][i] = FALSE;
            else
            {
                countingcalls = TRUE;
                maxcallsperpos[i][call] = calllimit;
            }
        }
        else if (calltypes[call] == PLAIN)
        {
            // There's now no reason to specify a Plain calling position, unless
            // a call count (possibly of 0) is applied to it.
            printf("WARNING: Plain position specified without call count.\n");
        }
    }
    return (TRUE);
}

int Composer::skipmusicfile(LineFile& f)
{
    if (!f.open())
    {
        printf("ERROR: Failed to open music file %s\n", f.getname());
        return (FALSE);
    }
    while (f.readline())
    {
        // Stop at "SMC32 start" line
        if (strncmpi(f.buffer, TOKEN_START, strlen(TOKEN_START)) == 0)
            break;
    }
    return (TRUE);
}

// Format of each PAIR of lines in music file:
// Music-type name
// score-per-row min-changes row1 row2 ...
// Rows can have wildcard characters '?'
// A '/' as the first character can be used to comment out a line
// !! Any changes must be reflected in writefileheader() above
int Composer::readmusicfile(LineFile& f)
{
    char* buf = f.buffer;
    char buf2[MAXLINEBUF];
    char* p;
    int nrows;
    int weight, min;
    int i, j;

    if (!f.open())
    {
        printf("ERROR: Failed to open music file %s\n", f.getname());
        return (FALSE);
    }
    // First read through file, counting lines
    // Check for excluded rows (weighting<0, min>=0) and count seperately
    nmusicdefs = exclude.nrows = 0;
    safedelete(musicdefs);
    safedelete(exclude.rows);
    f.markpos();
    while (f.readline())
    {
        // Must stop at "SMC32 start" line, since this will be encountered in a restart
        // of a search file
        if (strncmpi(buf, TOKEN_START, strlen(TOKEN_START)) == 0)
            break;
        nmusicdefs++;
        if (!f.readline())
        {
            printf("ERROR: one of your musicdefs was probably missing a name line\n");
            return (FALSE);
        }
        p = strtok(buf, DELIMITERS);
        if (p)
        {
            weight = atoi(p);
            if (weight == 0) // Weighting = 0 -> ignore line
                nmusicdefs--;
            else if (weight < 0) // Weighting < 0?
            {
                p = strtok(nullptr, DELIMITERS);
                if (p && atoi(p) >= 0) // Minimum >= 0?
                {
                    nmusicdefs--;
                    while (strtok(nullptr, DELIMITERS)) // Count number of excluded rows
                        exclude.nrows++;
                }
            }
        }
    }
    if (nmusicdefs == 0 && exclude.nrows == 0)
        return (TRUE);

    // Now allocate musicdefs[], exclude.rows[], and music arrays
    if (nmusicdefs)
    {
        musicdefs = new MusicDef[nmusicdefs];
        if (musicdefs == 0)
        {
            printf("ERROR: failed to allocate musicdef array\n");
            return (FALSE);
        }
        if (!music.alloc(nmusicdefs))
        {
            printf("ERROR: failed to allocate music-count array\n");
            return (FALSE);
        }
    }
    if (exclude.nrows)
    {
        exclude.rows = new MusicRow[exclude.nrows];
        if (exclude.rows == 0)
        {
            printf("ERROR: failed to allocate excluded rows array\n");
            return (FALSE);
        }
    }
    if (!f.resetpos())
        return (FALSE);

    // Run through each pair of lines in the file, loading musicdefs
    i = j = 0;
    while (f.readline())
    {
        // Must stop at "SMC32 start" line, since this will be encountered in a restart
        // of a .sf0 file
        if (strncmpi(buf, TOKEN_START, strlen(TOKEN_START)) == 0)
            break;
        strcpy(buf2, buf);
        if (!f.readline())
        {
            printf("ERROR: musicdef %s did not have row line\n", buf2);
            return (FALSE);
        }
        // Read score-per-row (weighting)
        p = strtok(buf, DELIMITERS);
        if (p == nullptr)
        {
            printf("ERROR: musicdef %s did not have score-per-row field\n", buf2);
            return (FALSE);
        }
        weight = atoi(p);
        if (weight == 0) // No score - ignore
            continue;
        // Read min # changes
        p = strtok(nullptr, DELIMITERS);
        if (p == nullptr)
        {
            printf("ERROR: musicdef %s did not have min-changes field\n", buf2);
            return (FALSE);
        }
        min = atoi(p);

        // If weight<0 and min>=0 this is an exclusion - copy into exclude.rows[] array
        if (weight < 0 && min >= 0)
        {
            p = strtok(nullptr, DELIMITERS);
            while (p)
            {
                if (j >= exclude.nrows)
                {
                    printf("INTERNAL ERROR: too many exclude rows!\n");
                    return (FALSE);
                }
                if (!readrowmatch(p, exclude.rows[j]))
                    return (FALSE);
                j++;
                p = strtok(nullptr, DELIMITERS);
            }
        }

        // Otherwise, is a musicdef, not an exclusion
        else
        {
            if (i >= nmusicdefs)
            {
                printf("INTERNAL ERROR: too many musicdefs!\n");
                return (FALSE);
            }
            // Load musicdef name
            musicdefs[i].name = new char[strlen(buf2) + 1];
            if (musicdefs[i].name == 0)
            {
                printf("ERROR: failed to allocate musicdef name field for %s\n", buf2);
                return (FALSE);
            }
            strcpy(musicdefs[i].name, buf2);
            musicdefs[i].weighting = weight;
            // If any music minimums found, must treat separately and cannot optimise treatment
            // of music scores (i.e. cannot combine into one figure)
            if (weight < 0 || min > 0)
                optimisemusic = FALSE;
            if (weight)
                musicdefs[i].minscore = min * abs(weight);
            else
                musicdefs[i].minscore = min;
            // Copy remaining characters on line into buf2, count how many rows
            p = strtok(nullptr, "");
            if (p == nullptr)
            {
                printf("ERROR: musicdef %s did not have any rows\n", musicdefs[i].name);
                return (FALSE);
            }
            strcpy(buf2, p);
            nrows = 0;
            p = strtok(p, DELIMITERS);
            while (p)
            {
                nrows++;
                p = strtok(nullptr, DELIMITERS);
            }
            if (nrows == 0)
            {
                printf("ERROR: musicdef %s did not have any rows\n", musicdefs[i].name);
                return (FALSE);
            }
            // Allocate space for row matches
            if (!musicdefs[i].ensurespace(nrows))
            {
                printf("ERROR: musicdef %s rows allocation failed\n", musicdefs[i].name);
                return (FALSE);
            }
            // Go through each row, loading into musicdef
            p = strtok(buf2, DELIMITERS);
            for (nrows = 0; nrows < musicdefs[i].nrows && p; nrows++)
            {
                if (!readrowmatch(p, musicdefs[i].matches[nrows]))
                    return (FALSE);
                p = strtok(nullptr, DELIMITERS);
            }
            if (nrows != musicdefs[i].nrows)
            {
                printf("INTERNAL ERROR: unexpected number of rows in musicdef %s\n", musicdefs[i].name);
                return (FALSE);
            }
            i++;
        }
    }
    if (i != nmusicdefs || j != exclude.nrows)
    {
        printf("INTERNAL ERROR: unexpected number of musicdefs\n");
        return (FALSE);
    }
    if (nmusicdefs)
    {
        printf("%d music definitions ", nmusicdefs);
        if (exclude.nrows)
            printf("and ");
        else
            printf("read\n");
    }
    if (exclude.nrows)
    {
        printf("%d row exclusions read\n", exclude.nrows);
        // Can't use regeneration compose loop if any rows are excluded
        rotationalsort = FALSE;
    }
    return (TRUE);
}

// Music-minimum overrides are only ever present in a .smc file, never a search file
// Format of each line:
// min-changes music-name
int Composer::readmusicminoverrides(LineFile& f)
{
    char* p;
    int i, min;

    while (f.readline())
    {
        p = strtok(f.buffer, DELIMITERS);
        if (p == nullptr)
            return (TRUE);
        min = atoi(p);
        p = strtok(nullptr, "\n");
        if (p == nullptr)
        {
            printf("ERROR: musicdef override not given\n");
            return (FALSE);
        }
        for (i = 0; i < nmusicdefs; i++)
            if (strcmpi(f.buffer, musicdefs[i].name) == 0)
                break;
        if (i == nmusicdefs)
        {
            printf("ERROR: unknown music def %s!\n", p);
            return (FALSE);
        }
        if (musicdefs[i].weighting)
            musicdefs[i].minscore = min * abs(musicdefs[i].weighting);
        else
            musicdefs[i].minscore = min;
    }
    return (TRUE);
}

int Composer::readrowmatch(char* buffer, MusicRow& m)
{
    char* b;
    int i, j;

    m.type = MUSICANYROW;
    m.sign = 0;
    for (i = 0; i < nbells; i++)
        m.row[i] = -1;
    for (i = 0; i < nbells && buffer[i]; i++)
        if (strchr(MATCHWILD, buffer[i]) == nullptr)
        {
            b = strchr(rounds, buffer[i]);
            if (b == nullptr)
            {
                printf("ERROR: row %s contains bad character\n", buffer);
                return (FALSE);
            }
            m.row[i] = (char)(b - rounds);
        }
    switch (buffer[i])
    {
        case (ROWMATCHWRAPCHAR):
            if (this->m->leadlen & 1)
            {
                printf("ERROR: method has odd lead length - can't count wrap music!\n");
                return (FALSE);
            }
            // For wrap rows, the backstroke change must be read into wrapbackstroke
            m.type = MUSICWRAP;
            for (j = 0; j < nbells; j++)
                m.wrapbackstroke[j] = -1;
            m.handstrokewrapmatch = FALSE;
            for (j = 0; j < nbells && buffer[i + j + 1]; j++)
                if (strchr(MATCHWILD, buffer[i + j + 1]) == nullptr)
                {
                    b = strchr(rounds, buffer[i + j + 1]);
                    if (b == nullptr)
                    {
                        printf("ERROR: musicdef row %s contains bad character\n", buffer);
                        return (FALSE);
                    }
                    m.wrapbackstroke[j] = (char)(b - rounds);
                }
            i++;
            break;
        case (ROWMATCHCOURSINGORDERCHAR):
            // These types of row must be leadheads. They are permuted to every lead in a plain
            // course while matching.
            m.type = MUSICCOURSINGORDER;
            i++;
            break;
        case (ROWMATCHLEADHEADCHAR):
            // This type of row is only compared against leadheads
            m.type = MUSICLEADHEAD;
            i++;
            break;
        case (ROWMATCHBACKSTROKECHAR):
            // This type of row only matches backstrokes (assumes even lead length!!!)
            m.type = MUSICBACKSTROKE;
            i++;
            break;
        case (ROWMATCHHANDSTROKECHAR):
            // This type of row only matches handstrokes (assumes even lead length!!!)
            m.type = MUSICHANDSTROKE;
            break;
            i++;
        default:
            break;
    }
    if (buffer[i] == ROWMATCHNEGCHAR)
        m.sign = -1;
    else if (buffer[i] == ROWMATCHPOSCHAR)
        m.sign = +1;
    return (TRUE);
}

void Composer::writerowmatch(char* buffer, MusicRow& m)
{
    int i;

    *buffer++ = ' ';
    for (i = 0; i < nbells; i++)
        if (m.row[i] < 0)
            *buffer++ = MATCHWILD[0];
        else
            *buffer++ = rounds[m.row[i]];
    switch (m.type)
    {
        case (MUSICWRAP):
            *buffer++ = ROWMATCHWRAPCHAR;
            for (i = 0; i < nbells; i++)
                if (m.wrapbackstroke[i] < 0)
                    *buffer++ = MATCHWILD[0];
                else
                    *buffer++ = rounds[m.wrapbackstroke[i]];
            break;
        case (MUSICCOURSINGORDER):
            *buffer++ = ROWMATCHCOURSINGORDERCHAR;
            break;
        case (MUSICLEADHEAD):
            *buffer++ = ROWMATCHLEADHEADCHAR;
            break;
        case (MUSICBACKSTROKE):
            *buffer++ = ROWMATCHBACKSTROKECHAR;
            break;
        case (MUSICHANDSTROKE):
            *buffer++ = ROWMATCHHANDSTROKECHAR;
            break;
        default:
            break;
    }
    if (m.sign < 0)
        *buffer++ = ROWMATCHNEGCHAR;
    else if (m.sign > 0)
        *buffer++ = ROWMATCHPOSCHAR;
    *buffer = 0;
}

int Composer::restartsearch()
{
    int searchcomplete = FALSE;

    outfile.setmode("r+");
    if (!readinputfile(outfile))
        return (FALSE);
    m->setcompletename(methodname, FALSE);
    if (!readmusicfile(outfile))
        return (FALSE);
    if (!((ExtMethod*)m)->buildtables(*this))
        return (FALSE);
    if (!newcomp())
        return (FALSE);
    if (makefraglib)
    {
        printf("Restarting fragment library build...\n");
        if (!fraglib.read(this, outfile.getname()))
            return (FALSE);
    }
    else
    {
        printf("Restarting search...\n");
        if (usefraglib)
        {
            if (!fraglib.read(this, nullptr)) // Filename is set up in readinputfile()
                return (FALSE);
            if (!fraglib.compress())
                return (FALSE);
        }
    }
    outfile.markpos();
    // Read through previous composition list
    while (outfile.readline())
    {
        if (*outfile.buffer == CHECKPOINT_SYMBOL)
            break;
        if (strncmp(outfile.buffer, TOKEN_END, strlen(TOKEN_END)) == 0)
        {
            searchcomplete = TRUE;
            break;
        }
        if (sscanf(outfile.buffer, "%d %d", &complength, &score) != 2)
        {
            printf("ERROR: invalid composition line\n%s\n", outfile.buffer);
            return (FALSE);
        }
        if (complength > stats.longestlength)
            stats.longestlength = complength;
        if (showlongestyet && complength > minlengthnow)
        {
            minlengthnow = complength + 1;
            if (minlengthnow > minlength)
                minlengthnow = minlength;
        }
        if (score > stats.bestscore)
            stats.bestscore = score;
        stats.ncompsoutput++;
        outfile.markpos();
    }
    if (searchcomplete)
    {
        printf("Search %s is complete!\n", outfile.getname());
        return (TRUE);
    }
    // Parse checkpoint composition
    if (inputcomp(outfile.buffer + 1) == FALSE)
        return (FALSE);
    // Read restart stats values
    if (!outfile.readline())
    {
        printf("ERROR: failed to read %s\n", outfile.getname());
        return (FALSE);
    }
    if (8 != sscanf(outfile.buffer, "%u %u %u %u %u %u %d %u", (int*)&stats.ncompsfound, ((int*)&stats.ncompsfound) + 1, (int*)&stats.nrotsfound, ((int*)&stats.nrotsfound) + 1, (int*)&stats.nodesgenerated, ((int*)&stats.nodesgenerated) + 1, (int*)&stats.elapsed, ((int*)&stats.elapsed) + 1))
    {
        printf("ERROR: bad restart values in file %s\n", outfile.getname());
        return (FALSE);
    }
    if (!outfile.resetpos())
        return (FALSE);
    compose();
    return (TRUE);
}