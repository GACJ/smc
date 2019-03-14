// SMC - Single Method Composer
// Copyright Graham A C John 1991-98 All rights reserved
// SMC32 32-bit conversion
// Copyright Mark B Davies 1998-2000

#include "smc.h"
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

// How many clock ticks between checkpoints (given no other comp output)
const int CHECKPOINTFREQ = 60 * CLOCKS_PER_SEC;
//const CHECKPOINTFREQ = 4*CLOCKS_PER_SEC;

char callchars[] = "p-sxu";

int Composer::newcomp()
{
    int i, j;

    // If 'showlongestyet' set, minlengthnow is initially set to 0
    // As longer and longer compositions are produced, it will work up to minlength
    if (showlongestyet)
        minlengthnow = 0;
    else
        minlengthnow = minlength;
    // Maxpartlength is only used for rotational multipart sorts
    // It is held as maxlength-maxpartlength for comparison to ebp in composing loop
    if (rotationalsort)
        maxpartlength = maxlength - maxlength / exclude.minnparts;
    else
        maxpartlength = 0;
    ncompnodes = 0;
    safedelete(compalloc);
    i = maxlength;
    if (palindromic)
        i = i * 2 + 2;
    compalloc = new char[ALIGNMENT + sizeof(Composition) * (i / m->leadlen + 1 + courselen * 2)];
    if (compalloc == nullptr)
    {
        printf("ERROR: failed to alloc composition array!\n");
        return (FALSE);
    }
    comp = (Composition*)((uintptr_t)compalloc + ALIGNMENT - 1 & ~(ALIGNMENT - 1));
    // The first courselen entries in comp[] are reserved for copying by the regeneration
    // (rotational sort) code, in the case where not all calling positions are allowed
    for (i = 0; i <= courselen; i++)
#ifdef REVERSECALLS
        comp[i].call = ncalltypes;
#else
        comp[i].call = 0;
#endif
    comp += courselen;
    regenptr = -courselen * sizeof(Composition);
    // Correct starting node already determined by gennodetable()
    comp[0].node = startnode;
    // Calculate percent0 and percentrange for percentage-complete calculations
    if (!findpercentrange())
        return (FALSE);
    // Clear all stats counters and timers
    stats.clear();
    // Reset included leads
    Node* node;
    for (i = 0; i < nodesincluded; i++)
    {
        node = (Node*)(nodes + i * nodesize);
        node->included = FALSE;
        node->unvisitable = 0;
        if (rotationalsort)
            for (j = 0; j <= ncalltypes; j++)
            {
                // Set up regenoffsets in each lead
                node->regenoffset[j] = -((int)sizeof(Composition));
                if (coursestructured)
                    node->regenoffset[j] -= sizeof(Composition) * courseenddist[nodeextra[i].callingbellpos[j]];
            }
    }
    complength = comp[0].node->nrows;
    // Set first node to be included
    if (bitwisetruthflags)
    {
        unsigned int* p = comp[0].node->falsebits[0].tableptr;
        *p |= comp[0].node->falsebits[0].tablemask;
    }
    else
        comp[0].node->included = TRUE;
    // Reset call counts
    for (i = 0; i <= ncalltypes; i++)
    {
        ncallsleft[i] = maxcalls[i];
        npartcalls[i] = ncallsleft[i] - maxcalls[i] / exclude.minnparts;
        for (j = 0; j < nbells; j++)
        {
            ncallsleftperpos[j][i] = maxcallsperpos[j][i];
            npartcallsperpos[j][i] = ncallsleftperpos[j][i] - maxcallsperpos[j][i] / exclude.minnparts;
        }
    }
    // Initialise composition storage, if 'storecomps' set
    // !! If search is restarted, this will be empty
    if (storecomps)
        if (!compsorter.init(ncompstostore))
        {
            printf("ERROR: failed to alloc composition storage\n");
            return (FALSE);
        }
    if (makefraglib)
    {
        i = 1;
        while (i < ncompstostore / COMPSPERBULKHASH)
            i <<= 2;
        if (!comphasher.init(i))
            return (FALSE);
        printf("Composition hash table allocated, size %d\n", i);
    }
    return (TRUE);
}

void Composer::showstats()
{
    char timebuf[100];
    float interval, nodespeed, evalspeed;
    int noutput;

    stats.elapsed = clock();
    interval = float(stats.elapsed - stats.lastdisplaytime) / CLOCKS_PER_SEC;
    // Number of million nodes checked per second
    nodespeed = float(stats.nodecount / (interval * 1e6));
    // Number of comps evaluated (including rotations) per second
    evalspeed = float(stats.evalcount) / interval;
    stats.lastdisplaytime = (clock_t)stats.elapsed;
    stats.elapsed -= stats.starttime;
    stats.nodesgenerated += stats.nodecount;
    stats.nodecount = stats.evalcount = 0;
    // If there are buffered compositions waiting to be written, flush them
    // Even if there are no comps, call flushcompbuffer() to write a checkpoint if
    //  sufficient interval has passed
    if (ncompsinbuffer > 0 || stats.lastdisplaytime - lastcheckpoint > CHECKPOINTFREQ)
        flushcompbuffer();
    printelapsed(timebuf);
    if (makefraglib)
        noutput = stats.nfragsfound;
    else
        noutput = stats.ncompsoutput;
    printf("\r%-9d%-10d%-10d%-9.2f%-9.0f%-12s%.3f", stats.bestscore, noutput, int(stats.nodesgenerated / 1000000), nodespeed, evalspeed, timebuf, 100.0 * calcpercentcomplete());
}

// Probably still very inaccurate for rotation sort - based on percent^(1/e)
// Return range 0.0-1.0
double Composer::calcpercentcomplete()
{
    Node* node = comp[0].node;
    double percent = -percent0;
    double treefraction = percentrange;
    int i, c, call, ncallpos;

    for (i = 0; i < ncompnodes; i++)
    {
        ncallpos = 0;
        for (call = 0; call <= ncalltypes; call++)
            if (node->nextnode[call])
                ncallpos++;
        c = 0;
        for (call = 0; call < comp[i].call; call++)
            if (node->nextnode[call])
                c++;
        treefraction /= ncallpos;
#ifdef REVERSECALLS
        percent += treefraction * (ncalltypes - c);
#else
        percent += treefraction * c;
#endif
        node = node->nextnode[call];
    }
#if 1
    if (rotationalsort)
        percent = pow(percent, 0.5); //1.0/M_E);
#endif
    return percent;
}

// This routine calculates two numbers:
// percent0 - the starting percentage-complete value for the search tree
//      (i.e. percentage for a composition using plains wherever possible)
// percentrange - 100.0 / %age range in the search tree from plains-wherever-possible
//      to bobs-wherever possible. Range 100.0 up.
int Composer::findpercentrange()
{
    Node* node;
    double p0, p1;
    char rotsortsave;
    int i, call, len, minlen;

    percent0 = 0.0;
    percentrange = 1.0;
    // Must clear rotationalsort to get unadjusted numbers from calcpercentcomplete()
    rotsortsave = rotationalsort;
    rotationalsort = FALSE;

    // Search a good depth in to get good accuracy and to make sure we are past
    // any forced-path opening sections (e.g. "musthave" block from rounds).
    minlen = courselen * m->leadlen;
    if (minlen < minlength)
        minlen = minlength;
    node = comp[0].node;
    i = len = 0;
    while (len < minlen)
    {
        len += node->nrows;
        for (call = 0; call <= ncalltypes; call++)
            if (node->nextnode[call] && !node->nextnode[call]->comesround)
                break;
        if (call > ncalltypes)
            break;
        node = node->nextnode[call];
        comp[i++].call = call;
    }
    ncompnodes = i;
    p0 = calcpercentcomplete();

    // Next do the search with bobs wherever possible, to get the ending value.
    node = comp[0].node;
    i = len = 0;
    while (len < minlen)
    {
        len += node->nrows;
        for (call = ncalltypes; call >= 0; call--)
            if (node->nextnode[call] && !node->nextnode[call]->comesround)
                break;
        if (call < 0)
            break;
        node = node->nextnode[call];
        comp[i++].call = call;
    }
    ncompnodes = i;
    p1 = calcpercentcomplete();
    // Cope with call types being swapped
    if (p0 < p1)
    {
        percentrange = 1.0 / (p1 - p0);
        percent0 = p0 * percentrange;
    }
    else
    {
        percentrange = 1.0 / (p0 - p1);
        percent0 = p1 * percentrange;
    }
    printf("R, P0 = %f, %f, %f\n", percentrange, percent0, calcpercentcomplete());
    // Must reset values for main composing loop
    rotationalsort = rotsortsave;
    ncompnodes = 0;
#ifdef REVERSECALLS
    comp[0].call = ncalltypes;
#else
    comp[0].call = 0;
#endif
    return (TRUE);
}

void Composer::analysecomp()
{
    Node* node;
    int bestscore = -INT_MAX, bestrot = -1;

    comprot = 0;
    if (!rotationalsort || (nmusicdefs == 0 && !showallrots))
    {
        if (evalcomp())
        {
            if (!rotationalsort)
                countparts();
            // Don't bother with short comps with too few parts
            if (nparts >= exclude.minnparts)
            {
                if (makefraglib)
                {
                    if (!comphasher.addcomp(this))
                        exit(100);
                    return;
                }
                outputcomp();
            }
        }
    }
    else
    {
        // Don't bother with short comps with too few parts
        if (nparts < exclude.minnparts)
            return;
        do
        {
            stats.nrotsfound++;
            if (evalcomp())
            {
                if (score > bestscore)
                {
                    bestscore = score;
                    bestrot = comprot;
                }
                if (showallrots)
                    outputcomp();
            }
            // Move on to next rotation
            if (coursestructured)
            {
                node = comp[0].node; // Find next course end - !!! ASSUME START FROM ROUNDS
                while (comprot < nodesperpart && node && node->nodex->callingbellpos[comp[comprot].call] != callingbell)
                {
                    node = node->nextnode[comp[comprot].call];
                    comprot++;
                }
                if (node == nullptr) // Bad comp!
                    break;
            }
            comprot++;
        } while (comprot < nodesperpart);
        if (!showallrots)
            if (bestrot >= 0)
            {
                score = bestscore;
                comprot = bestrot;
                outputcomp();
            }
    }
}

// Maximum number of parts for each stage, assuming Principle!
int maxnparts[MAXNBELLS + 1] = { 0, 1, 2, 3, 3, 2 * 3, // 0-5 bells
                                 2 * 3,
                                 3 * 4,
                                 3 * 5,
                                 4 * 5,
                                 2 * 3 * 5, // 6-10
                                 2 * 3 * 5,
                                 3 * 4 * 5,
                                 3 * 4 * 5,
                                 3 * 5 * 6,
                                 3 * 5 * 7, // 11-15
                                 4 * 5 * 7,
                                 2 * 3 * 5 * 7,
                                 2 * 3 * 5 * 7,
                                 3 * 4 * 5 * 7,
                                 3 * 4 * 5 * 7 }; // 16-20

// Only needed if rotationalsort off
void Composer::countparts()
{
    int i, j;

    nparts = 1;
    // Must end in rounds
    if (comp[ncompnodes].node->nrows == 0)
        for (nparts = maxnparts[nbells]; nparts > 1; nparts--)
        {
            nodesperpart = ncompnodes / nparts;
            // Composition length must be integrally divisible
            if (nodesperpart * nparts != ncompnodes)
                continue;
            // Nodehead must generate exactly this number of parts
            if (comp[nodesperpart].node->nparts != nparts)
                continue;
            // Check calling is the same in each part
            for (i = 0; i < nodesperpart; i++)
                for (j = 1; j < nparts; j++)
                    if (comp[i].call != comp[i + j * nodesperpart].call)
                    {
                        i = INT_MAX - 1;
                        break;
                    }
            if (i != INT_MAX)
                break;
        }
    if (nparts == 1)
        nodesperpart = ncompnodes;
}

//#define ASMEVAL

// Evaluate one particular rotation
int Composer::evalcomp()
{
    Node* node = comp[0].node; // Start from rounds regardless of rotation
    int goodenough = TRUE;
    int achievedminimums = TRUE;
    int rot = comprot;
#ifdef ASMEVAL
    int nmus;
    int *rotstart, *compend;
#endif
    int i, j;

    stats.evalcount++;

    // Count up music
    score = 0;
    if (nmusicdefs)
    {
#ifdef ASMEVAL
        // clang-format off
        __asm
        {
          push    ebx
          mov ebx,[this]
          cmp [ebx]Composer.optimisemusic,0
          je  notoptimised
      // If music optimisation on, count up combined score, not invidual music scores
      // Run through nodes, counting scores
          mov edx,0       // edx = running score
          mov esi,[rot]
          mov edi,[node]
          mov eax,[ebx]Composer.ncompnodes
          imul    esi,sizeof(Composition)
          mov ecx,[ebx]Composer.comp
          imul    eax,sizeof(Composition)
          add esi,ecx
          add eax,ecx
          mov [rotstart],esi
          mov [compend],eax
      // Do first half: rot to compend
      optnode1:   mov ecx,[edi]Node.nodex
          mov eax,[esi]Composition.call
          add edx,[ecx]NodeExtra.combinedscore
          mov edi,[edi+eax*4]Node.nextnode
          add esi,sizeof(Composition)
          cmp edi,0
          je  nullnode
          cmp esi,[compend]
          jb  optnode1
          mov esi,[ebx]Composer.comp  // !! Relies on call field being 1st
          cmp esi,[rotstart]
          jae scoringdone
      optnode2:   cmp edi,0
          je  nullnode
          mov ecx,[edi]Node.nodex
          mov eax,[esi]Composition.call
          add edx,[ecx]NodeExtra.combinedscore
          add esi,sizeof(Composition)
          mov edi,[edi+eax*4]Node.nextnode
          cmp esi,[rotstart]
          jb  optnode2
          jmp scoringdone
  
      // Clear scores
      notoptimised:
          mov eax,0
          mov ecx,[ebx]Composer.nmusicdefs
          mov edx,[ebx]Composer.music.score
          mov [nmus],ecx
      clear:  mov [edx+ecx*4-4],eax
          dec ecx
          jg  clear
      // Run through nodes, counting scores
          mov esi,[rot]
          mov edi,[node]
          mov eax,[ebx]Composer.ncompnodes
          imul    esi,sizeof(Composition)
          mov ecx,[ebx]Composer.comp
          imul    eax,sizeof(Composition)
          add esi,ecx
          add eax,ecx
          mov [rotstart],esi
          mov [compend],eax
      // Do first half: rot to compend
      node1:  mov ebx,[edi]Node.nodex
          mov ecx,0
      // Check exclusions
          cmp [ebx]NodeExtra.excluded,0
          jne nullnode
          mov ebx,[ebx]NodeExtra.music
      music1: mov eax,[ebx+ecx*4]
          inc ecx
          add [edx+ecx*4-4],eax
          cmp ecx,[nmus]
          jl  music1
          mov eax,[esi]Composition.call
          add esi,sizeof(Composition)
          mov edi,[edi+eax*4]Node.nextnode
          cmp esi,[compend]
          jae secondhalf
          cmp edi,0
          jne node1
      nullnode:   pop ebx
        }
        return (FALSE); // Can't do this rotation (nextnode==nullptr)
        __asm
        {
      // Do second half: compstart to rot
      secondhalf:
          mov ebx,[this]
          mov esi,[ebx]Composer.comp  // !! Relies on call field being 1st
          cmp esi,[rotstart]
          jae totalscore
      node2:  cmp edi,0
          je  nullnode
          mov ebx,[edi]Node.nodex
          mov ecx,0
      // Check exclusions
          cmp [ebx]NodeExtra.excluded,0
          jne nullnode
          mov ebx,[ebx]NodeExtra.music
      music2: mov eax,[ebx+ecx*4]
          inc ecx
          add [edx+ecx*4-4],eax
          cmp ecx,[nmus]
          jl  music2
          mov eax,[esi]Composition.call
          add esi,sizeof(Composition)
          mov edi,[edi+eax*4]Node.nextnode
          cmp esi,[rotstart]
          jb  node2
      // Calculate total score, and check minimums
      totalscore:
          mov ebx,[this]
          mov ecx,0
          mov esi,0
          mov edi,[ebx]Composer.musicdefs
      total:  mov eax,[edx+ecx*4]
          inc ecx
          add esi,eax
          cmp eax,[edi]MusicDef.minscore
          jge nextmus
          mov [achievedminimums],FALSE
      nextmus:    add edi,sizeof(MusicDef)
          cmp ecx,[nmus]
          jl  total
          mov edx,esi
      scoringdone:
          mov [ebx]Composer.score,edx
          pop ebx
        }
        // clang-format on
#else
        // If music optimisation on, count up combined score, not invidual music scores;
        // there will be no music minimums to check
        if (optimisemusic)
            for (i = 0; i < ncompnodes; i++)
            {
                score += node->nodex->combinedscore;
                node = node->nextnode[comp[rot].call];
                if (node == nullptr) // Can't do this rotation
                    return (FALSE);
                if (++rot >= ncompnodes)
                    rot = 0;
            }
        else
        {
            for (j = 0; j < nmusicdefs; j++)
                music.score[j] = 0;
            for (i = 0; i < ncompnodes; i++)
            {
                if (node->nodex->excluded) // Check exclusions
                    return (FALSE);
                for (j = 0; j < nmusicdefs; j++)
                    music.score[j] += node->nodex->music[j];
                node = node->nextnode[comp[rot].call];
                if (node == nullptr) // Can't do this rotation
                    return (FALSE);
                if (++rot >= ncompnodes)
                    rot = 0;
            }
            for (j = 0; j < nmusicdefs; j++)
            {
                score += music.score[j];
                if (music.score[j] < musicdefs[j].minscore)
                    achievedminimums = FALSE;
            }
        }
#endif
    }
    if (!achievedminimums || score < minscore)
        goodenough = FALSE;
    if (score >= stats.bestscore)
    {
        stats.bestscore = score;
        if (showbestyet && achievedminimums)
            goodenough = TRUE;
    }
    if (complength > stats.longestlength)
    {
        stats.longestlength = complength;
        if (showlongestyet)
        {
            minlengthnow = complength + 1; // Must have a longer comp next
            if (minlengthnow > minlength)
                minlengthnow = minlength;
            goodenough = TRUE;
        }
    }
    return goodenough;
}

// Print composition to compbuffer
// !! Must match with inputcomp() below
int Composer::outputcomp()
{
    Node* node = comp[0].node; // Start from rounds regardless of rotation
    int rot = comprot;
    int i;

#if 0
 if (storecomps)
 {
  CompMusicStore newcomp;
  if (!storecomposition(newcomp))
   return(FALSE);
  if (!compsorter.addcomp(newcomp,nmusicdefs))
   return(FALSE);
  return(TRUE);
 }
#endif
    compbufptr += sprintf(compbufptr, "%d\t%d\t%d\t", complength, score, nparts);
    if (coursestructured)
        for (i = 0; i < nodesperpart; i++)
        {
            compbufptr = printcall(compbufptr, comp[rot].call, node->nodex->callingbellpos[comp[rot].call]);
            if (compbufptr == nullptr)
                return (FALSE);
            node = node->nextnode[comp[rot].call];
            if (node == nullptr)
                return (FALSE);
            if (++rot >= nodesperpart)
                rot = 0;
        }
    else
    {
        for (i = 0; i < nodesperpart; i++)
        {
            *compbufptr++ = callchars[calltypes[comp[rot].call]];
            if (++rot >= nodesperpart)
                rot = 0;
        }
    }
    compbufptr = stpcpy(compbufptr, "\n");
    ncompsinbuffer++;
    // Have to save a copy of certain stats values. This is so that we can write out
    // contemporary stats for the composition when it is (eventually) stored on disk.
    stats.save();
    // If composition buffer is nearly full, write it out to disk
    if (compbuffer + COMPBUFFERSIZE - compbufptr < MAXCOMPSPACE)
        return flushcompbuffer();
    return (TRUE);
}

// Returns updated buffer pointer
// !! Doesn't necessarily null-terminate
char* Composer::printcall(char* buf, int c, int pos)
{
    Call call = calltypes[c];

    if (call)
    {
        if (call > 1)
            *buf++ = callchars[call];
        buf = stpcpy(buf, callposnames[c][pos]);
    }
    if (pos == callingbell && (call == PLAIN || !extendinglead))
        *buf++ = ' ';
    return (buf);
}

// !! Must match with outputcomp() above
int Composer::inputcomp(char* compbuf)
{
    CompMusicStore storedcomp;
    char* p;
    int inputlength;

    // First read length, score and number of parts
    p = strtok(compbuf, " \t");
    if (p == nullptr)
    {
        printf("ERROR: failed to parse composition length\n");
        return (FALSE);
    }
    inputlength = atoi(p);
    p = strtok(nullptr, " \t");
    if (p == nullptr)
    {
        printf("ERROR: failed to parse composition score\n");
        return (FALSE);
    }
    // For non-SMC32 compositions, author's name is put in score field
    if (isalpha(*p))
    {
        storedcomp.score = 0;
        storedcomp.author = new char[strlen(p) + 1];
        if (storedcomp.author)
            strcpy(storedcomp.author, p);
    }
    else
        storedcomp.score = atoi(p);
    p = strtok(nullptr, " \t");
    if (p == nullptr)
    {
        printf("ERROR: failed to parse number of parts\n");
        return (FALSE);
    }
    storedcomp.nparts = atoi(p);
    p = strtok(nullptr, "\n");
    if (p == nullptr)
    {
        printf("ERROR: blank composition parsed\n");
        return (FALSE);
    }
    storedcomp.rot = 0;
    if (!readcalling(p, storedcomp, comp[0].node))
        return (FALSE);
    storedcomp.length *= storedcomp.nparts;
    if (storedcomp.length != inputlength)
    {
        printf("ERROR: composition %d changes long, not %d.\n", storedcomp.length, inputlength);
        return (FALSE);
    }
// Count calls - not used?
#if 0
 storedcomp.ncalls = 0;
 for (i=0; i<NDIFFCALLS; i++)
  storedcomp.callcount[i] = 0;
 for (i=0; i<storedcomp.nodesperpart; i++)
  if (storedcomp.calling[i])
  {
   storedcomp.ncalls++;
   storedcomp.callcount[storedcomp.calling[i]]++;
  }
#endif
    if (!loadcomposition(storedcomp))
        return (FALSE);
    return (TRUE);
}

// If nnodes<=0, assume reading complete composition & extend to part end/rounds
// On entry, storedcomp.nparts should already be set!
// The Node parameter gives the starting node for call conversion - normally rounds
int Composer::readcalling(char* buf, CompStore& storedcomp, Node* node, int nnodes)
{
    char *c, *p = buf;
    char ch;
    int maxnodes;
    int i, j, k, call;
    Node* tmpnode;

    storedcomp.length = 0;
    // Ensure space in storedcomp for calling
    maxnodes = nnodes;
    if (maxnodes <= 0)
        maxnodes = maxlength / m->leadlen + 1 + courselen;
    if (!storedcomp.ensurespace(maxnodes))
        return (FALSE);

    // Now read calling
    if (coursestructured)
    {
        i = 0;
        while (*p)
        {
            if (*p == ' ') // Skip spaces
            {
                p++;
                continue;
            }
            c = strchr(callchars, *p); // See if there is a single prefix
            if (c)
            {
                call = (int)(c - callchars);
                if (call > 1)
                {
                    call = internalcallnums[c - callchars];
                    p++;
                }
                else
                    call = internalcallnums[BOB];
            }
            else
                call = internalcallnums[BOB]; // If not, call is set to Bob
                                              // Run through all calling position names, find out which one this is
            for (j = 0; j < nbells; j++)
                if (strncmp(p, callposnames[call][j], strlen(callposnames[call][j])) == 0)
                    break;
            if (j == nbells)
            {
                printf("ERROR: failed to parse calling\n%s\n", p);
                return (FALSE);
            }
            // Add plains to composition until calling position comes up
            k = 0;
            while (node->nodex->callingbellpos[call] != j)
            {
                storedcomp.calling[i] = internalcallnums[PLAIN];
                storedcomp.length += node->nrows;
                tmpnode = node->nextnode[internalcallnums[PLAIN]];
                if (tmpnode == nullptr)
                {
                    ch = *++p;
                    *p = 0;
                    printf("ERROR: invalid composition parsed: call %c%s not allowed at this node!\n", callchars[call], callposnames[call][j]);
                    printrow(node->nodex->nodehead);
                    printf("%s ( %s)\n", buf, p + 1);
                    *p = ch;
                    return (FALSE);
                }
                node = tmpnode;
                ++i;
                if (++k > courselen) // Check for infinite loop
                {
                    ch = *++p;
                    *p = 0;
                    printf("ERROR: call %c%s not reached in parsed composition!\n", callchars[call], callposnames[call][j]);
                    printrow(node->nodex->nodehead);
                    printf("%s ( %s)\n", buf, p + 1);
                    *p = ch;
                    return (FALSE);
                }
            }
            // Add call to composition
            storedcomp.calling[i] = call;
            storedcomp.length += node->nrows;

            /*
   printf("%c%s ",callchars[call],callposnames[call][j]);
   printrow(node->nodex->nodehead);
   if (node->nextnode[call]==nullptr)
   {
     printf("ERROR: null node from call %c%s!\n",callchars[call],callposnames[call][j]);
   }
*/

            node = node->nextnode[call];
            if (++i > maxnodes)
            {
                printf("ERROR: parsed composition too long!\n%s\n", buf);
                return (FALSE);
            }
            if (node == nullptr)
                // Don't bother testing final nullptr node if we're reading a fragment
                // This is because there can be fragments ending in a snap finish - these
                // should really be eliminated from roundblock searches, but aren't that
                // harmful, and certainly shouldn't cause an error.
                if (nnodes <= 0 || i < maxnodes)
                {
                    printf("ERROR: invalid composition parsed - final node null!\n%s\n", buf);
                    return (FALSE);
                }
            p += strlen(callposnames[call][j]);
        }
        // If 'nnodes' parameter set, add sufficient plains to reach this length
        // Otherwise, add plains to end of composition (rounds or part end if multipart)
        if (nnodes > 0)
            while (i < nnodes)
            {
                // Check node first to avoid producing error on final nullptr - see comment above
                if (node == nullptr)
                {
                    printf("ERROR: invalid composition parsed - final node null!\n%s\n", buf);
                    return (FALSE);
                }
                storedcomp.calling[i] = internalcallnums[PLAIN];
                storedcomp.length += node->nrows;
                node = node->nextnode[internalcallnums[PLAIN]];
                ++i;
            }
        else
        {
            k = 0;
            while (!node->comesround && (storedcomp.nparts == 1 || node->nparts == 0))
            {
                storedcomp.calling[i] = internalcallnums[PLAIN];
                storedcomp.length += node->nrows;
                node = node->nextnode[internalcallnums[PLAIN]];
                if (node == nullptr)
                    break;
                if (++k > courselen)
                {
                    printf("ERROR: parsed composition did not reach part end/rounds!\n");
                    return (FALSE);
                }
                ++i;
            }
            if (node != nullptr && node->comesround)
                storedcomp.length += node->nrows;
        }
    }
    else
    {
        i = 0;
        while (*p)
        {
            c = strchr(callchars, *p++);
            if (c == nullptr)
                continue;
            call = internalcallnums[c - callchars];
            storedcomp.calling[i] = call;
            storedcomp.length += node->nrows;
            node = node->nextnode[call];
            if (++i > maxnodes)
            {
                printf("ERROR: parsed composition too long!\n%s\n", buf);
                return (FALSE);
            }
            if (node == nullptr)
                // Don't bother testing final nullptr node if we're reading a fragment
                // This is because there can be fragments ending in a snap finish - these
                // should really be eliminated from roundblock searches, but aren't that
                // harmful, and certainly shouldn't cause an error.
                if (nnodes <= 0 || i < maxnodes)
                {
                    printf("ERROR: invalid composition parsed - final node null!\n%s\n", buf);
                    return (FALSE);
                }
        }
        // Add on number of rows to rounds in final node.
        if (node != nullptr && node->comesround)
            storedcomp.length += node->nrows;
    }
    storedcomp.nodesperpart = i;
    return (TRUE);
}

// Run through "must-have" block calling, removing any nextnode pointers other than
// those which follow the calling. Set "inmusthaveblock" flag for all nodes within
// the block other than entry node. Note that if we discover an inmusthaveblock flag
// already set, that is an error - blocks must be independent.
// This code has been taken from Composer::readcalling() above.
int Composer::readblockcalling(Block* block)
{
    char *c, *p = block->calling;
    NodeExtra* nodex = block->entrynode;
    NodeExtra* tmpnode;
    int j, call;

    nodex->blockentry = TRUE;
    if (coursestructured)
    {
        while (*p)
        {
            if (*p == ' ') // Skip spaces
            {
                p++;
                continue;
            }
            c = strchr(callchars, *p); // See if there is a single prefix
            if (c)
            {
                call = (int)(c - callchars);
                if (call > 1)
                {
                    call = internalcallnums[c - callchars];
                    p++;
                }
                else
                    call = internalcallnums[BOB];
            }
            else
                call = internalcallnums[BOB]; // If not, call is set to Bob
                                              // Run through all calling position names, find out which one this is
            for (j = 0; j < nbells; j++)
                if (strncmp(p, callposnames[call][j], strlen(callposnames[call][j])) == 0)
                    break;
            if (j == nbells)
                return musthaveblockerror(block, "Failed to parse calling");
            p += strlen(callposnames[call][j]);
            // Go through plains until calling position comes up
            tmpnode = nodex;
            while (nodex->callingbellpos[call] != j)
            {
                nodex->essential = block->essential;
                nodex->forcenextnodeto(internalcallnums[PLAIN]);
                nodex = &nodeextra[nodex->nextnode[internalcallnums[PLAIN]]];
                if (nodex == nullptr)
                    return musthaveblockerror(block, "Invalid calling!");
                if (nodex == tmpnode) // Check for infinite loop
                    return musthaveblockerror(block, "Calling position not reached!");
                if (nodex->inmusthaveblock)
                    return musthaveblockerror(block, "Overlapping blocks not allowed!");
                nodex->inmusthaveblock = TRUE;
            }
            // Reached call
            nodex->essential = block->essential;
            nodex->forcenextnodeto(call);
            nodex = &nodeextra[nodex->nextnode[call]];
            if (nodex == nullptr)
                return musthaveblockerror(block, "Invalid calling!");
            if (nodex->inmusthaveblock)
                return musthaveblockerror(block, "Overlapping blocks not allowed!");
            nodex->inmusthaveblock = TRUE;
        }
        // Add plains to reach exit point
        tmpnode = nodex;
        while (nodex != block->exitnode)
        {
            nodex->essential = block->essential;
            nodex->forcenextnodeto(internalcallnums[PLAIN]);
            nodex = &nodeextra[nodex->nextnode[internalcallnums[PLAIN]]];
            if (nodex == nullptr || nodex == tmpnode) // Check for infinite loop
                return musthaveblockerror(block, "Exit leadhead not reached!");
            if (nodex->inmusthaveblock)
                return musthaveblockerror(block, "Overlapping blocks not allowed!");
            nodex->inmusthaveblock = TRUE;
        }
        nodex->essential = block->essential;
    }
    else // Non course-structured
    {
        while (*p)
        {
            c = strchr(callchars, *p++);
            if (c == nullptr)
                continue;
            call = internalcallnums[c - callchars];
            nodex->essential = block->essential;
            nodex->forcenextnodeto(call);
            nodex = &nodeextra[nodex->nextnode[call]];
            if (nodex == nullptr)
                return musthaveblockerror(block, "Invalid calling!");
            if (nodex->inmusthaveblock)
                return musthaveblockerror(block, "Overlapping blocks not allowed!");
            nodex->inmusthaveblock = TRUE;
        }
        if (nodex != block->exitnode)
            return musthaveblockerror(block, "Exit leadhead not reached!");
        nodex->essential = block->essential;
    }
    return (TRUE);
}

// The main dispatcher for the various composing loops
void Composer::compose()
{
#ifdef FRAGPTRINNODE
    if (usefraglib)
        for (int i = 0; i < nodesincluded; i++)
        {
            Node* node = (Node*)(nodes + i * nodesize);
            node->fragmap = fraglib.getmapfornode(findcallingbell(node->nodex->nodehead));
        }
#endif
    if (ncompnodes == 0)
        if (makefraglib)
            printf("Finding fragments...\n");
        else
            printf("Composing...\n");
    printf("Best     ");
    if (makefraglib)
        printf("Frags     ");
    else
        printf("Comps     ");
    printf("Mnodes    Mnode/s  Comps/s  Time        %%complete\n");
    resetcompbuffer();
    lastcheckpoint = clock();
    stats.starttime = ((long long)lastcheckpoint) - stats.elapsed;
    maxpalilength = maxlength;
    if (palindromic)
        maxpalilength /= 2;

#ifdef ONECOMP
    compose_palindromic_MMX();
#else
    if (rotationalsort)
        if (maxnodefalse == 0)
            if (exclude.minnparts > 1)
                if (usefraglib)
                    compose_regen_MMXfrag_cps_multipart();
                else if (countingcalls)
                    compose_regen_cps_multipart_callcount();
                else
                    compose_regen_cps_multipart();
            else if (usefraglib)
                compose_regen_MMXfrag_cps();
            else if (countingcalls)
                compose_regen_cps_callcount();
            else
                compose_regen_cps();
        else if (exclude.minnparts > 1)
            if (bitwisetruthflags)
                if (usefraglib)
                    compose_regen_MMXfrag_falsebits_multipart();
                else if (countingcalls)
                    compose_regen_falsebits_multipart_callcount();
                else
                    compose_regen_falsebits_multipart();
            else if (usefraglib)
                compose_regen_MMXfrag_multipart();
            else if (countingcalls)
                compose_regen_multipart_callcount();
            else
                compose_regen_multipart();
        else if (bitwisetruthflags)
            if (usefraglib)
                compose_regen_MMXfrag_falsebits();
            else if (countingcalls)
                compose_regen_falsebits_callcount();
            else
                compose_regen_falsebits();
        else if (usefraglib)
            compose_regen_MMXfrag();
        else if (countingcalls)
            compose_regen_callcount();
        else
            compose_regen();
    else if (maxnodefalse == 0)
        if (palindromic)
            compose_palindromic_cps();
        else if (usefraglib)
            compose_MMXfrag_cps();
        else if (countingcalls)
            compose_cps_callcount();
        else
            compose_cps();
    else if (bitwisetruthflags)
        if (palindromic)
            compose_palindromic_falsebits();
        else if (usefraglib)
            compose_MMXfrag_falsebits();
        else if (countingcalls)
            compose_falsebits_callcount();
        else
            compose_falsebits();
    else if (palindromic)
        compose_palindromic();
    else if (usefraglib)
        compose_MMXfrag();
    else if (countingcalls)
        compose_callcount();
    else
        compose_();
#endif
    stats.elapsed = ((long long)clock()) - stats.starttime;
    printf("\n");
#ifdef CYCLETIMER
#if CYCLETIMER == TIMEFALSE
    printf("Cycles taken by inner loop: %d (nfalse=%d)\n", ncycles, cyclenfalse);
#else
    printf("Cycles taken by inner loop: %d\n", ncycles);
#endif
#endif
    finaloutput();
    // Sanity check on call counts - must all have returned to starting values
    if (countingcalls)
    {
        for (int call = 0; call < ncalltypes; call++)
        {
            if (ncallsleft[call] != maxcalls[call])
                printf("WARNING: Call count for call %d incorrect!\n", calltypes[call]);
            for (int i = 0; i < nbells; i++)
                if (ncallsleftperpos[i][call] != maxcallsperpos[i][call])
                    printf("WARNING: Call count for call %d at position %d incorrect!\n", calltypes[call], rounds[i]);
        }
    }
}
