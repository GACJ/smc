// SMC - Single Method Composer
// Copyright Graham A C John 1991-98 All rights reserved
// SMC32 32-bit conversion
// Copyright Mark B Davies 1998-2000

#include "smc.h"
#include <new>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

int ExtMethod::buildtables(Composer& ring)
{
    clock_t time;

    time = clock();
    if (!ring.setup())
        return (FALSE);
    // Count the number of leads between each calling position. Each such segment of
    // the course is treated as one node for compositional purposes
    ring.findnodes();
    // This sets up the courseenddist[] array, so that for each place bell the number of
    // nodes to the course end is known. This is used by the regen optimisation
    ring.findcourseenddists();
    // Finds false LHs for 1st lead from rounds
    if (!ring.findfalseLHs())
        return (FALSE);
    // Uses a hash table (BulkHash) to store nodeheads which can be reached with the
    // current call exclusions. A count of the number of included nodes is kept, which
    // is then used to allocate the nodes array in gennodetable() below
    // The BulkHash can also be searched for included nodeheads, so the recursive
    // search doesn't need to be run again
    if (!ring.findtablesize())
        return (FALSE);
    // Now generate remaining tables (NodeExtra and Node) including false nodes & music
    if (!ring.gennodetable())
        return (FALSE);
    // Calculate an estimate of the average search depth, in nodes

    time = clock() - time;
    printf(" Table building took %d.%03d seconds\n", (int)(time / CLOCKS_PER_SEC), (int)(time % CLOCKS_PER_SEC));
    return (TRUE);
}

// Counts how many nodes are available to the composition
// Stores nodeheads found in a BulkHash for later use
int Composer::findtablesize()
{
    printf(" Counting number of non-excluded nodes...");
    // Initialise node hash table
    if (!nodehasher.init(1 << 16, sizeof(HashedNode*), sizeof(HashedNode)))
    {
        printf("\nERROR: failed to alloc node hashtable\n");
        return (FALSE);
    }
    // Start from rounds
    copyrow(startrow, row);

    nodesincluded = 0;
    cancomeround = FALSE; // Check we can come back to rounds
                          // Set up maximum stack depth for recursive node search
                          // This is limited by the length of the composition - there is no point searching
                          // deeper than the maximum length, especially in big split-tenors fields.
                          // In fact, keeping the depth restricted makes possible short tenors-split searches
                          // on higher numbers
#if 0
// !! Doesn't work - deadend nodes at end of recursion are left in
    maxstackdepth = maxlength / m->leadlen;
    if (maxstackdepth > MAXSTACKDEPTH)
#endif
    maxstackdepth = MAXSTACKDEPTH;
    stackhiwater = 0;
    recursefindnodes(0);
    if (nodehasher.geterror() != BULKLISTOK)
    {
        printf("\nINTERNAL ERROR: node hashtable error - probably alloc failure\n");
        return (FALSE);
    }
    printf(" %d found\n", nodesincluded);
    if (stackhiwater >= maxstackdepth)
        printf("WARNING: stack limit reached\n");
    if (!palindromic && !cancomeround)
    {
        printf("ERROR: rounds cannot be reached!\n");
        if (exclude.noleadheadrounds)
            printf("You need singles or less strict calling positions to reach internal rounds\n");
        return (FALSE);
    }
    return (TRUE);
}

int matchhashednode(HashedNode* hnode, int, Composer* ring)
{
    return ring->samerow(ring->row, hnode->nodehead);
}

// Returns FALSE if dead end
int Composer::recursefindnodes(int stacklevel)
{
    HashedNode* hnode;
    char LH[MAXNBELLS];
    char included, excluded, rounds;
    char call;
    int lead, nodeleads, noderows;

    // See if this node is already in the hashtable
    nodehashitem.factnum = calcLHfactnum(row);
    hnode = (HashedNode*)nodehasher.finditem(nodehashitem.factnum, (HashItemMatchFn)matchhashednode, this);
    // If it is, return TRUE if it was included; FALSE if visited but not included
    if (hnode)
        return hnode->included;
    // Add new node to the hashtable
    // Initially set to 'included' so that if we cycle back to this node, the search
    // will not deadend. If this never happens, and the tree from this node is a deadend,
    // the included flag is reset at the end of this call
    nodehashitem.included = TRUE;
    nodehashitem.excluded = FALSE;
    copyrow(row, nodehashitem.nodehead);
    nodehashitem.nodex = nullptr;
    hnode = (HashedNode*)nodehasher.add(&nodehashitem, nodehashitem.factnum);
    if (hnode == nullptr)
        return (FALSE);
    // Check stack depth - return if too deep
    // Assume node reachable!
    if (stacklevel > maxstackdepth)
    {
        nodesincluded++;
        return (TRUE);
    }
    // Generate node (ie all leads in this course segment) from the current leadhead,
    // for all calls. Check all rows for exclusions.
    copyrow(row, LH);
    nodeleads = leadstonodeend[findcallingbell(row)];
    noderows = 0;
    rounds = FALSE;
    included = FALSE;
    for (lead = 0; lead < nodeleads && !rounds; lead++)
    {
        // Go through all calls, but only process initial leads for call 0
        for (call = 0; call <= ncalltypes; call++)
            // Process the initial nodeleads-1 leads in the node once only.
            if (call == 0 || lead == nodeleads - 1)
            {
                excluded = FALSE;
                starttouch();
                copyrow(LH, row);
                rounds = FALSE;
                calllist[0] = calltypes[call];
                startlead();
                while (changen < m->leadlen)
                {
                    if (isfinishrow(row))
                    {
                        if (changen > 0 && exclude.nointernalrounds)
                        {
                            excluded = TRUE;
                            break;
                        }
                        else if (changen + noderows > 0) // Ignore node starting from rounds
                        {
                            if (changen == 0 && exclude.noleadheadrounds)
                            {
                                excluded = TRUE;
                                break;
                            }
                            cancomeround = TRUE;
                            rounds = TRUE;
                            break;
                        }
                    }
                    if (changen == 0 && isLHexcluded())
                    {
                        excluded = TRUE;
                        break;
                    }
                    if (isrowexcluded())
                    {
                        if (rotationalsort)
                            hnode->excluded = TRUE;
                        else
                        {
                            excluded = TRUE;
                            break;
                        }
                    }
                    change();
                }
                if (excluded)
                {
                    if (lead == nodeleads - 1)
                        continue;
                    else
                        goto deadend;
                }
                // If node had no exclusions, add to table
                // If calling position allowed, recurse from next nodehead
                if (rounds || (lead == nodeleads - 1 && exclude.allowedcalls[call][findcallingbell()]))
                {
                    if (rounds)
                        included = TRUE;
                    else
                        // If 'noleadheadrounds' set, we must exclude this node if it produces nodehead rounds
                        if (isfinishrow(row))
                        if (exclude.noleadheadrounds)
                            continue;
                        else
                        {
                            included = TRUE;
                            cancomeround = TRUE;
                            // For non-rounds starts, have to continue finding nodes beyond rounds
                            recursefindnodes(stacklevel + 1);
                        }
                    else
                        // Don't include this node if all its branches are dead ends
                        included |= recursefindnodes(stacklevel + 1);
                }
            }
        // Move on to next lead in node
        noderows += m->leadlen;
        copyrow(row, LH);
    }
    if (included)
    {
        //printrow(nodehashitem.nodehead);
        nodesincluded++;
        return (TRUE);
    }
    // Must reset included flag to show this is a deadend
deadend:
    hnode->included = FALSE;
    if (stacklevel > stackhiwater)
        stackhiwater = stacklevel;
    return (FALSE);
}

int Composer::isLHexcluded()
{
    int i, failedmatches, callingbellpos;

    // Check coursehead exclusions
    if (coursestructured)
    {
        failedmatches = 0;
        for (i = 0; i < courseend.nrows; i++)
        {
            callingbellpos = findcallingbell(courseend.matches[i].row);
            if (callingbellpos < 0 || row[callingbellpos] == callingbell)
            {
                if (isrowmatch(courseend.matches[i]))
                    break;
                else
                    failedmatches++;
            }
        }
        if (i == courseend.nrows && failedmatches)
            return (TRUE);
    }
    return (FALSE);
}

int Composer::isrowexcluded()
{
    int i;

    for (i = 0; i < exclude.nrows; i++)
        if (isrowmatch(exclude.rows[i]))
            return (TRUE);
    return (FALSE);
}

// Generate node tables: NodeExtra and Nodes
// This must be done in a specific order to maximise memory efficiency:
// 1. The NodeExtra array is allocated, and nodeheads copied from the BulkHash
// 2. Music for each node is counted (also setting nleads and nrows in NodeExtra)
// 3. If using "must-have" blocks, nextnode pointers are deleted so that, on entering
//    a musthave block, the composing loop has no choice but to proceed through it.
//    "Essential" musthave blocks are marked for later false-node knockout.
// 4. Individual falsenode arrays are allocated in each NodeExtra, large enough
//    to hold the maximum number of possible false nodes
// 5. False nodes are generated and stored in NodeExtra. Maximum included false
//    nodes per lead is calculated (maxnodefalse)
// 6. For non-rotational sort only, "essential" nodes are processed - these automatically
//    exclude all their false nodes. Then, excluded nodes are knocked out of the tables
//    completely. (Note that musical excludes have already been lost in findnodes(), so
//    this is a second exclusion pass). Again, this is all for non-regen search ONLY.
// 7. Now the Node array 'nodes' is allocated, with the smallest possible
//    false node array size (from 'maxnodefalse').
// 8. False nodes and other data are copied from NodeExtra duplicate fields to Nodes
// 9. The false node arrays in NodeExtra structures can be freed.
// 10.Hopefully... all the tables are now ready for the composing loop
int Composer::gennodetable()
{
    HashedNode* hnode;
    int i, j, k;

    if (samestartandend)
        nodesincluded++; // One extra for 'starting' rounds
                         // The number of included nodes ('nodesincluded') is now known.
                         // Allocate the NodeExtra array and copy nodeheads from the BulkList into it
    nodeextra = new (std::nothrow) NodeExtra[nodesincluded];
    if (nodeextra == nullptr)
    {
        printf("ERROR: failed to alloc NodeExtra table!\n");
        return (FALSE);
    }
    i = 0;
    // Add 'starting rounds' node
    // This has normal length and does not have "comesrounds" flag set!
    if (samestartandend)
    {
        nodeextra[0].num = 0;
        copyrow(startrow, nodeextra[0].nodehead);
        i++;
    }
    finishrounds = nullptr;
    for (j = 0; j < nodehasher.gettablesize(); j++)
        for (k = 0; k < nodehasher.getlistsize(j); k++)
        {
            hnode = (HashedNode*)nodehasher.getitem(j, k);
            if (hnode == nullptr)
            {
                printf("INTERNAL ERROR: failed to get node from hashtable\n");
                return (FALSE);
            }
            if (hnode->included)
            {
                if (i >= nodesincluded)
                {
                    printf("INTERNAL ERROR: unexpectedly many nodes in hashtable\n");
                    return (FALSE);
                }
                nodeextra[i].num = i;
                copyrow(hnode->nodehead, nodeextra[i].nodehead);
                nodeextra[i].excluded = hnode->excluded;
                // Set up nodex pointers in hashtable - used for quick lookup by calcfalsenodes()
                hnode->nodex = &nodeextra[i];
                // Keep track of node containing finishing row.
                // This will later be set to 0 rows in length, and have its "comesround" flag set
                if (isfinishrow(hnode->nodehead))
                {
                    finishrounds = &nodeextra[i];
                    // If the starting and ending rows for the search are the same, we'll have added
                    // an extra "starting" node, and the finish node will have 0 length.
                    // However, it is very important that if we attempt to look up the finish node
                    // we should get the full-length starting node NOT the finish node, otherwise e.g.
                    // some false nodes may be missed. To ensure this happens, we reset the nodex
                    // point of the "rounds" hashnode to point to the starting node.
                    if (samestartandend)
                        hnode->nodex = &nodeextra[0];
                }
                i++;
            }
        }
    if (i < nodesincluded)
    {
        printf("INTERNAL ERROR: unexpectedly few nodes (%d) in hashtable\n", i);
        return (FALSE);
    }

    // Go through each node, counting music
    printf(" Linking nodes and counting music...");
    for (i = 0; i < nodesincluded; i++)
    {
        gennodex(i);
        if (nmusicdefs && !optimisemusic)
        {
            nodeextra[i].music = new (std::nothrow) int[nmusicdefs];
            if (nodeextra[i].music == nullptr)
            {
                printf("\nERROR: failed to alloc music table!\n");
                return (FALSE);
            }
            nodeextra[i].musicallocated = TRUE;
        }
        countnodemusic(i);
    }
    printf("\n");

    // If we have any "must-have" blocks, process them now.
    if (musthaveblocks->listsize() > 0)
    {
        if (!preparemusthaveblocks())
            return (FALSE);
    }

    // Generate false nodes for each node, store in NodeExtra table
    printf(" Generating false nodes...");
    for (i = 0; i < nodesincluded; i++)
    {
        // Include space for the node's own number in case we have bitwisetruthflags switched on
        nodeextra[i].falsenodes = new (std::nothrow) int[nodeextra[i].nleads * nfalseLHs + 1];
        if (nodeextra[i].falsenodes == nullptr)
        {
            printf("\nERROR: failed to allocate false node array!\n");
            return (FALSE);
        }
    }
    minnodefalse = 10000;
    maxnodefalse = 0;
    maxnodealloc = 0;
    for (i = 0; i < nodesincluded; i++)
        calcfalsenodes(i);
    // If using bitwise truthtable, false node lists contain the node itself -
    // now subtract this off min and max false node counts.
    if (bitwisetruthflags)
    {
        minnodefalse--;
        maxnodefalse--;
    }
    printf(" min/max falseness per node: %d/%d\n", minnodefalse, maxnodefalse);

    // Can now free nodehasher
    nodehasher.freeall();

    // For non-rotational sort ONLY, process "essential" and "excluded" nodes.
    if (!rotationalsort)
    {
        processessentialnodes();
        processexcludednodes();
    }

    // If using bitwise truthtable, need to determine list of false node numbers
    // for each node, and optimise the numbering system to pack false node lists
    // into the minimum number of bitmasks.
    delete[] truthtable;
    truthtable = nullptr;
    if (bitwisetruthflags)
    {
        // Must allocate truthtable so table pointers can be calculated
        truthtablesize = (nodesincluded + TRUTHTABLEWORDSIZE) / TRUTHTABLEWORDSIZE;
        truthtable = new (std::nothrow) unsigned int[truthtablesize];
        if (truthtable == nullptr)
        {
            printf("\nERROR: failed to allocate bitwise truth table!\n");
            return (FALSE);
        }
        for (i = 0; i < truthtablesize; i++)
            truthtable[i] = 0;

        if (!calcbitwisetruthtables())
            return (FALSE);
        maxnodealloc *= sizeof(FalseBits);
    }
    else
        maxnodealloc = sizeof(Node*) * maxnodefalse;

    // Now we know the maximum number of false nodes per node, we can allocate
    // Node structures of minimum size (important for cache usage in composing loop)
    printf(" Creating nodes table (%d nodes per course)", nodespercourse);
    nodesize = sizeof(Node) + maxnodealloc;
    // Pad to 8-byte multiple
    nodesize = (nodesize + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    printf(" (node size %d) ...", nodesize);
    delete[] nodealloc;
    nodealloc = new (std::nothrow) char[nodesize * nodesincluded + ALIGNMENT];
    if (nodealloc == nullptr)
    {
        printf("\nERROR: failed to alloc Node table!\n");
        return (FALSE);
    }
    // Align on 8-byte boundary
    nodes = (char*)(((uintptr_t)nodealloc + ALIGNMENT - 1) & ~(ALIGNMENT - 1));
    // Fill each Node structure - also find starting node
    startnode = nullptr;
    for (i = 0; i < nodesincluded; i++)
    {
        gennode(i);
    }
    printf("\n");
    // Call printing routines need to know whether call at Home produces another Home
    // !! Should really check separately for other calls
    if (nodeextra[0].callingbellpos[calltypes[BOB]] == callingbell)
        extendinglead = TRUE;
    else
        extendinglead = FALSE;
    return (TRUE);
}

// Fills in NodeExtra structure: nleads, negative, callingbellpos, nextnode
// Some values are later copied into the corresponding Node structure
void Composer::gennodex(int n)
{
    NodeExtra* nodex = &nodeextra[n];
    HashedNode* hnode;
    char nodeend[MAXNBELLS];
    int lead;
    int call;

    copyrow(nodex->nodehead, row);
    nodex->inmusthaveblock = FALSE;
    nodex->blockentry = FALSE;
    // Calculate nature of row
    nodex->negative = isnegative(row);
    nodex->nleads = leadstonodeend[findcallingbell(row)];
    // Find the 'nodeend' row - the leadhead of the final lead in the node
    // This row can be used to find succeeding nodeheads using the calltrans[]
    // lead tranpositions
    copyrow(row, nodeend);
    for (lead = 0; lead < nodex->nleads - 1; lead++)
    {
        transpose(nodeend, calltrans[internalcallnums[PLAIN]], row);
        copyrow(row, nodeend);
    }
    for (call = 0; call <= ncalltypes; call++)
    {
#ifdef PREVNODES
        nodex->prevnode[call] = -1;
        if (exclude.allowedcalls[call][findcallingbell(nodex->nodehead)])
        {
            inversetrans(row, calltrans[call], nodex->nodehead);
            // If the LH is in the middle of a node, back up with plain leads
            // until the corresponding node start row is found
            hnode = findnodefromLH();
            if (hnode && hnode->included)
                nodex->prevnode[call] = hnode->nodex->num;
        }
#endif
        nodex->nextnode[call] = -1;
        // Use 'nodeend' row to calculate nextnode pointers for each call
        transpose(nodeend, calltrans[call], row);
        // Also fill in callingbellpos in NodeExtra structure
        nodex->callingbellpos[call] = findcallingbell(row);

        if (exclude.allowedcalls[call][nodex->callingbellpos[call]])
        {
            // Special code for rows that matches rounds - they must be excluded if
            // exclude.noleadheadrounds set
            if (isfinishrow(row))
            {
                if (!exclude.noleadheadrounds)
                    nodex->nextnode[call] = finishrounds->num;
            }
            else
            {
                hnode = (HashedNode*)nodehasher.finditem(calcLHfactnum(row), (HashItemMatchFn)matchhashednode, this);
                if (hnode && hnode->included)
                    nodex->nextnode[call] = hnode->nodex->num;
            }
        }
    }
}

// Counts up music in this node, store in int array in NodeExtra
// Also finds nrows, comesround
void Composer::countnodemusic(int n)
{
    NodeExtra* nodex = &nodeextra[n];
    char LH[MAXNBELLS];
    int lead, noderows;
    int rounds;
    int i;

    nodex->comesround = FALSE;
    nodex->nrows = nodex->nleads * m->leadlen;
    noderows = 0;
    if (optimisemusic)
        nodex->combinedscore = 0;
    else
        for (i = 0; i < nmusicdefs; i++)
            nodex->music[i] = 0;
    copyrow(nodex->nodehead, LH);
    rounds = FALSE;
    // Go through all leads in this node (course segment)
    for (lead = 0; !rounds && lead < nodex->nleads; lead++)
    {
        starttouch();
        copyrow(LH, row);
        calllist[0] = PLAIN;
        startlead();
        while (changen < m->leadlen)
        {
            if (isfinishrow(row))
                // For starting node 0, don't check rounds at change 0!
                if (changen + noderows > 0 || n > 0 || !samestartandend)
                {
                    nodex->nrows = changen + noderows;
                    nodex->comesround = TRUE;
                    rounds = TRUE;
                    break;
                }
            // Count row music
            // If there are no music minimums, can add all music types together for
            // increased speed in evalcomp()
            if (optimisemusic)
                for (i = 0; i < nmusicdefs; i++)
                {
                    if (ismusicmatch(musicdefs[i]))
                        nodex->combinedscore += musicdefs[i].weighting;
                }
            else
                for (i = 0; i < nmusicdefs; i++)
                    if (ismusicmatch(musicdefs[i]))
                        nodex->music[i] += musicdefs[i].weighting;
            change();
        }
        // Move on to next lead in course segment
        noderows += m->leadlen;
        copyrow(row, LH);
    }
}

int Composer::musthaveblockerror(Block* b, const char* err)
{
    char tempbuf[MAXNBELLS + 1];
    writerow(b->entrylh, tempbuf);
    printf("ERROR in %s block %s ", b->blocktype(), tempbuf);
    writerow(b->exitlh, tempbuf);
    printf("%s %s\n", tempbuf, b->calling);
    printf("%s\n", err);
    return (FALSE);
}

// Should be called after countnodemusic() but before calcfalsenodes().
// Checks "musthave" blocks to make sure the callings and start and end points are valid.
// If so, goes through nextnode pointers in the nodex tables, knocking out any that
// split up blocks. The result should be that the composing loop must run through
// complete blocks whenever it finds them.
int Composer::preparemusthaveblocks()
{
    Block* b;
    HashedNode* hnode;
    NodeExtra* nextnode;
    int i;

    for (i = 0; i < musthaveblocks->listsize(); i++)
    {
        b = (Block*)musthaveblocks->getitem(i);
        if (b == nullptr)
        {
            printf("ERROR: null returned from musthaveblocks::getitem\n");
            exit(-1);
        }
        // First find nodes corresponding to entry and exit leadheads
        copyrow(b->entrylh, row);
        hnode = findnodefromLH();
        if (hnode == nullptr)
            return musthaveblockerror(b, "Failed to find node containing entry leadhead");
        if (!hnode->included)
            return musthaveblockerror(b, "Entry node not included in composition tables");
        b->entrynode = hnode->nodex;

        copyrow(b->exitlh, row);
        hnode = findnodefromLH();
        if (hnode == nullptr)
            return musthaveblockerror(b, "Failed to find node containing exit leadhead");
        if (!hnode->included)
            return musthaveblockerror(b, "Exit node not included in composition tables");
        if (hnode->nodex == nodeextra && samestartandend)
            b->exitnode = finishrounds;
        else
            b->exitnode = hnode->nodex;

        // Read block calling and nobble next-node pointers within block so calling
        // must be followed. We also set "inmusthaveblock" flag for all nodes within
        // the block, apart from the entry node.
        if (!readblockcalling(b))
            return (FALSE);
    }

    // As a final step we must check all other nodes and nobble any nextnode pointers
    // that jump into the middle of a musthave block
    for (i = 0; i < nodesincluded; i++)
    {
        NodeExtra* nodex = &nodeextra[i];
        // Only do this for nodes not already in a block!
        if (!nodex->blockentry && !nodex->inmusthaveblock)
            for (int j = 0; j < NDIFFCALLS; j++)
                if (nodex->nextnode[j] >= 0)
                {
                    nextnode = &nodeextra[nodex->nextnode[j]];
                    if (nextnode->inmusthaveblock)
                        nodex->nextnode[j] = -1;
                }
    }
    return (TRUE);
}

// Given a leadhead (in Composer.row), finds the NodeExtra which contains it.
// !!! WARNING: can only be used during table building whilst the NodeHasher exists.
HashedNode* Composer::findnodefromLH()
{
    char tmprow[MAXNBELLS];
    int b, b1;

    // If the LH is in the middle of a node, back up with plain leads
    // until the corresponding node start row is found
    b1 = b = findcallingbell(row);
    while (leadspernode[b1] == 0)
    {
        inversetrans(tmprow, calltrans[internalcallnums[PLAIN]], row);
        copyrow(tmprow, row);
        b1 = findcallingbell(row);
        if (b1 == b) // Stuck in closed loop?
            break;
    }
    if (leadspernode[b1])
        return (HashedNode*)nodehasher.finditem(calcLHfactnum(row), (HashItemMatchFn)matchhashednode, this);
    return nullptr;
}

// Calculates false nodes for each node, store in NodeExtra structure
// These are later copied out into Node structures
void Composer::calcfalsenodes(int n)
{
    NodeExtra* nodex = &nodeextra[n];
    HashedNode* hnode;
    char tmprow[MAXNBELLS];
    char LH[MAXNBELLS];
    int lead, noderows;
    int b, b1, i, j;

    nodex->nfalsenodes = 0;
    // If using bitwise truth tables, we add ourself to the false node list
    if (bitwisetruthflags)
    {
        nodex->falsenodes[0] = nodex->num;
        nodex->nfalsenodes++;
    }

    copyrow(nodex->nodehead, LH);
    noderows = 0;
    // Go through all leads in this node (course segment)
    for (lead = 0; lead < nodex->nleads; lead++)
    {
        // Process all false leads
        for (i = 0; i < nfalseLHs; i++)
            if (falseLHs[i].pos + noderows < nodex->nrows) // Ignore if past rounds
            {
                transpose(LH, falseLHs[i].row, row);
                // If the false LH is in the middle of a node, back up with plain leads
                // until the corresponding node start row is found
                b1 = b = findcallingbell(row);
                while (leadspernode[b1] == 0)
                {
                    inversetrans(tmprow, calltrans[internalcallnums[PLAIN]], row);
                    copyrow(tmprow, row);
                    b1 = findcallingbell(row);
                    if (b1 == b) // Stuck in closed loop?
                        break;
                }
                if (leadspernode[b1])
                {
                    hnode = (HashedNode*)nodehasher.finditem(calcLHfactnum(row), (HashItemMatchFn)matchhashednode, this);
                    if (hnode && hnode->included)
                    {
                        b = hnode->nodex->num;
                        // Make sure we haven't already included this false node
                        for (j = 0; j < nodex->nfalsenodes; j++)
                            if (b == nodex->falsenodes[j])
                                break;
                        // Add FLH to list
                        if (j == nodex->nfalsenodes)
                        {
                            nodex->nfalsenodes++;
                            nodex->falsenodes[j] = b;
                        }
                    }
                }
            }
        // Move on to next lead in course segment
        noderows += m->leadlen;
        transpose(LH, calltrans[internalcallnums[PLAIN]], row);
        copyrow(row, LH);
    }
    // See if maxnodefalse or minnodefalse exceeded
    if (nodex->nfalsenodes > maxnodefalse)
        maxnodefalse = nodex->nfalsenodes;
    else if (nodex->nfalsenodes < minnodefalse)
        minnodefalse = nodex->nfalsenodes;

    // Print false nodes
    /*
     if (nodex->nfalsenodes>0)
     {
      for (j=0; j<nbells; j++)
       printf("%c",rounds[nodex->nodehead[j]]);
      printf(":");
      for (i=0; i<nodex->nfalsenodes; i++)
      {
       printf(" ");
       for (j=0; j<nbells; j++)
        printf("%c",rounds[nodeextra[nodex->falsenodes[i]].nodehead[j]]);
      }
      printf("\n");
     }
    */
}

// For non-rotational sort ONLY, process essential nodes.
// These have the nodex->essential flag set, for example because they are
// contained in "must-have" blocks. What we do is mark all their false nodes
// as excluded; these excluded false nodes are then knocked out in the next
// step (processexcludednodes).
// Note if two essential nodes are mutually false, they are both left in.
void Composer::processessentialnodes()
{
    int i, j;

    for (i = 0; i < nodesincluded; i++)
    {
        NodeExtra* nodex = &nodeextra[i];
        if (!nodex->essential) // Don't knock out nodes which are essential!
            for (j = 0; j < nodex->nfalsenodes; j++)
                if (nodeextra[nodex->falsenodes[j]].essential)
                {
                    if (nodex->essential)
                    {
                        printf("WARNING: essential nodes mutually false!\n");
                        printrow(nodex->nodehead);
                        printrow(nodeextra[nodex->falsenodes[j]].nodehead);
                        break;
                    }
                    nodex->excluded = TRUE; // Knock out this node if any false node is essential
                    break;
                }
    }
}

// For non-rotational sort ONLY, process excluded nodes.
// These have the nodex->excluded flag set, but note that "music-excluded"
// nodes WON'T be present, since they are knocked out much earlier, in findnodes().
// Excluded nodes present at this stage (just after calcfalsenodes() but before
// generation of composer-loop Node structures) will have been marked for reasons
// such as falseness against "essential" nodes (see processessentialnodes() above).
// What we do is delete any next-node or false node pointers that reference
// an excluded node. Since this could in theory leave a non-excluded node stranded,
// that is with no nextnode pointers available, a seperate pass kills stranded
// nodes too. We repeat until no more excluded or stranded nodes are left.
void Composer::processexcludednodes()
{
    int i, j, k, firstfalse = 0;

    // If using bitwise truth table, false node lists start with self node - ignore
    if (bitwisetruthflags)
        firstfalse = 1;
    do
    {
        // First check for "stranded" nodes, i.e. those with no valid next-node pointers
        // (and which don't have rounds in!)
        for (i = 0; i < nodesincluded; i++)
        {
            NodeExtra* nodex = &nodeextra[i];
            if (!nodex->excluded && !nodex->comesround)
            {
                k = 0;
                for (j = 0; j < NDIFFCALLS; j++)
                    if (nodex->nextnode[j] >= 0)
                        k++;
                if (k == 0)
                    nodex->excluded = TRUE;
            }
        }
        // Next knock out all nextnode and falsenode pointers to excluded nodes.
        // Keep count of how many we do, so we know whether it is worth looping
        // back to check for stranded nodes again.
        k = 0;
        for (i = 0; i < nodesincluded; i++)
        {
            NodeExtra* nodex = &nodeextra[i];
            if (!nodex->excluded)
            {
                for (j = 0; j < NDIFFCALLS; j++)
                    if (nodex->nextnode[j] >= 0 && nodeextra[nodex->nextnode[j]].excluded)
                    {
                        nodex->nextnode[j] = -1;
                        k++;
                    }
                for (j = firstfalse; j < nodex->nfalsenodes; j++)
                    if (nodeextra[nodex->falsenodes[j]].excluded)
                    {
                        nodex->falsenodes[j] = nodex->falsenodes[nodex->nfalsenodes - 1];
                        nodex->nfalsenodes--;
                        k++;
                    }
            }
        }
    } while (k > 0);
}

// If bitwisetruthflags set, we use an optimised bitwise truth table in place
// of Node.included and the false node pointer lists.
// Here we carry out the following table-building steps on the NodeExtra array
// (note we must already have done calcfalsenodes!):
// 1. For each node, initial set NodeExtra.truthflagnum to be equal to NodeExtra.num.
// 2. The numbering scheme (NodeExtra.truthflagnum) is then optimised to try
//    and cluster the node numbers in each false node list. If node numbers are
//    closely clustered, they can be packed into fewer bitmasks.
// 3. Next, the false node lists are packed into truth table (pointer, mask) pairs
//    and stored in NodeExtra.falsebits. The optimisation phase (2) ensures that
//    the size of this array is minimised. The maximum falsebits array size for
//    any node is noted for used in allocating the Node tables to minimum size.
// Later in gennode, the falsebits array is copied from each NodeExtra to corresponding
// Node.
int Composer::calcbitwisetruthtables()
{
    NodeExtra* nodex;
    int pass;
    int i, j, k, x, f;
    int iword, jword, niwords, njwords, score, scoredelta;
    char swapsdone, ipresent, jpresent;

    for (i = 0; i < nodesincluded; i++)
    {
        nodex = &nodeextra[i];
        nodex->truthflagbit = i % TRUTHTABLEWORDSIZE; // Numbering scheme starts off non-optimised
        nodex->truthflagword = i / TRUTHTABLEWORDSIZE;
        nodex->falsebits = new (std::nothrow) FalseBits[nodex->nfalsenodes + 1]; // 1 extra to hold single bit for self
        if (nodex->falsebits == nullptr)
            return FALSE;
    }

    // Optimisation phase
    printf(" Optimising false node numbering...");
    // First find current node-packing score
    score = 0;
    for (i = 0; i < nodesincluded; i++)
        score += truthflagpackingscore(&nodeextra[i]);
    printf("\n");

    // Pair-swap algorithm
    pass = 0;
    do
    {
        pass++;
        printf("  pass %d: av. falsebit pointers/node: %f\n", pass, (double)score / nodesincluded);
        swapsdone = false;
        for (i = 0; i < nodesincluded; i++)
        {
            iword = nodeextra[i].truthflagword;
            for (j = i + 1; j < nodesincluded; j++)
            {
                jword = nodeextra[j].truthflagword;
                // Only worth swapping i and j nodes if they are in different words
                if (iword != jword)
                {
                    // If we swap i and j what is the effect on the packing score?
                    // Go through all nodes that point to either i or j.
                    // False node relationships are transitive, hence these are precisely the false nodes
                    // of nodes i and j.
                    scoredelta = 0;
                    // Do false nodes of i first
                    for (k = 0; k < nodeextra[i].nfalsenodes; k++)
                    {
                        nodex = &nodeextra[nodeextra[i].falsenodes[k]];
                        // Each of these nodes contains i somewhere in its false node list.
                        // Find out where; also might contain j!
                        jpresent = false;
                        niwords = 0;
                        njwords = 0;
                        for (x = 0; x < nodex->nfalsenodes; x++)
                        {
                            f = nodex->falsenodes[x];
                            if (f == j)
                            {
                                jpresent = true;
                                break;
                            }
                            if (iword == nodeextra[f].truthflagword)
                                niwords++;
                            else if (jword == nodeextra[f].truthflagword)
                                njwords++;
                        }
                        // If node contains both i and j, swap has no effect
                        if (!jpresent)
                        {
                            // If there is only one i-word, then removing i will reduce the score.
                            if (niwords == 1)
                                scoredelta--;
                            // If there are no j-words, then adding j will increase the score.
                            if (njwords == 0)
                                scoredelta++;
                        }
                    }
                    // Now do false nodes of j
                    for (k = 0; k < nodeextra[j].nfalsenodes; k++)
                    {
                        nodex = &nodeextra[nodeextra[j].falsenodes[k]];
                        // Each of these nodes contains j somewhere in its false node list.
                        // Find out where; also might contain i!
                        ipresent = false;
                        niwords = 0;
                        njwords = 0;
                        for (x = 0; x < nodex->nfalsenodes; x++)
                        {
                            f = nodex->falsenodes[x];
                            if (f == i)
                            {
                                ipresent = true;
                                break;
                            }
                            if (iword == nodeextra[f].truthflagword)
                                niwords++;
                            else if (jword == nodeextra[f].truthflagword)
                                njwords++;
                        }
                        // If node contains both i and j, swap has no effect
                        if (!ipresent)
                        {
                            // If there is only one j-word, then removing j will reduce the score.
                            if (njwords == 1)
                                scoredelta--;
                            // If there are no i-words, then adding i will increase the score.
                            if (niwords == 0)
                                scoredelta++;
                        }
                    }
                    // Finally we can see whether it is worth swapping i and j: only if scoredelta<0
                    if (scoredelta < 0)
                    {
                        swapsdone = true;
                        f = nodeextra[i].truthflagword;
                        x = nodeextra[i].truthflagbit;
                        nodeextra[i].truthflagword = nodeextra[j].truthflagword;
                        nodeextra[i].truthflagbit = nodeextra[j].truthflagbit;
                        nodeextra[j].truthflagword = f;
                        nodeextra[j].truthflagbit = x;
                        iword = jword;
                        score += scoredelta;
                    }
                }
            }
        }
    } while (swapsdone);

    // Now pack optimised false nodes
    long int totalfalsebits = 0;
    long int totalfalsenodes = 0;
    for (i = 0; i < nodesincluded; i++)
    {
        nodex = &nodeextra[i];
        calcfalsebitmasks(nodex);
        if (nodex->nfalsebits >= maxnodealloc)
            maxnodealloc = nodex->nfalsebits + 1;
        totalfalsebits += nodex->nfalsebits;
        totalfalsenodes += nodex->nfalsenodes;
    }
    printf(" Average # false nodes / falsebit pointers: %.3f/%.3f\n", ((float)totalfalsenodes) / nodesincluded, ((float)totalfalsebits) / nodesincluded);
    return TRUE;
}

int falsewordstemp[100];

// Find total number of words the false bits will be packed into for this node
int Composer::truthflagpackingscore(NodeExtra* nodex)
{
    int i, j, k, f, n;

    n = 0;
    for (i = 0; i < nodex->nfalsenodes; i++)
    {
        f = nodeextra[nodex->falsenodes[i]].truthflagword;
        for (j = 0; j < n; j++)
            if (falsewordstemp[j] >= f)
                break;
        if (j >= n || falsewordstemp[j] != f)
        {
            for (k = n; k > j; k--)
                falsewordstemp[k] = falsewordstemp[k - 1];
            falsewordstemp[j] = f;
            n++;
        }
    }
    return n;
}

// Convert false node numbers in falsenodes array into a packed list of
// (truthtable ptr, table mask) pairs. Assumes false node array contains
// at least one false node number, the node itself.
void Composer::calcfalsebitmasks(NodeExtra* nodex)
{
    int i, j, k, n;
    unsigned int* f;

    // The 0th falsebit entry contains a single bit mask for the node itself.
    // This is used in code which needs to set or clear what used to be Node.included.
    // Note that this entry is NOT used in the false-node checking loop; the node's
    // own bit is also included in the collapsed masks starting at entry 1.
    nodex->falsebits[0].tableptr = truthtable + nodex->truthflagword;
    nodex->falsebits[0].tablemask = 1 << nodex->truthflagbit;

    n = 0;
    for (i = 0; i < nodex->nfalsenodes; i++)
    {
        f = truthtable + nodeextra[nodex->falsenodes[i]].truthflagword;
        for (j = 0; j < n; j++)
            if (nodex->falsebits[j + 1].tableptr >= f)
                break;
        if (j >= n || nodex->falsebits[j + 1].tableptr != f)
        {
            for (k = n; k > j; k--)
            {
                nodex->falsebits[k + 1].tableptr = nodex->falsebits[k].tableptr;
                nodex->falsebits[k + 1].tablemask = nodex->falsebits[k].tablemask;
            }
            nodex->falsebits[j + 1].tableptr = f;
            nodex->falsebits[j + 1].tablemask = 1 << nodeextra[nodex->falsenodes[i]].truthflagbit;
            n++;
        }
        else
        {
            nodex->falsebits[j + 1].tablemask |= 1 << nodeextra[nodex->falsenodes[i]].truthflagbit;
        }
    }
    nodex->nfalsebits = n;
}

// Fills in Node structure with duplicates from NodeExtra and some newly-calculated
// fields (nparts, nodex pointer)
// Also finds startnode
void Composer::gennode(int n)
{
    Node* node = (Node*)(nodes + n * nodesize);
    NodeExtra* nodex = &nodeextra[n];
    char tmprow[MAXNBELLS], tmprow2[MAXNBELLS];
    int call;
    int i;

    node->nodex = nodex;
    nodex->node = node;
    node->callcountposindex = NDIFFCALLS * findcallingbell(nodex->nodehead);

    // Find startnode
    if (startnode == nullptr && samerow(startrow, nodex->nodehead))
        startnode = node;

    // Copy duplicate fields out of NodeExtra structure
    node->comesround = nodex->comesround;
    node->nrows = nodex->nrows;
    if (bitwisetruthflags)
    {
        node->nfalsenodes = nodex->nfalsebits;
        for (i = 0; i <= nodex->nfalsebits; i++)
            node->falsebits[i] = nodex->falsebits[i];
        delete[] nodex->falsebits;
        nodex->falsebits = nullptr;
    }
    else
    {
        node->nfalsenodes = nodex->nfalsenodes;
        for (i = 0; i < nodex->nfalsenodes; i++)
            node->falsenodes[i] = (Node*)(nodes + nodex->falsenodes[i] * nodesize);
    }
    // Can now delete falsenode arrays in NodeExtra
    delete[] nodex->falsenodes;
    nodex->falsenodes = nullptr;
    for (call = 0; call <= ncalltypes; call++)
    {
        if (nodex->nextnode[call] < 0)
            node->nextnode[call] = nullptr;
        else
            node->nextnode[call] = (Node*)(nodes + nodex->nextnode[call] * nodesize);
#ifdef PREVNODES
        if (nodex->prevnode[call] < 0)
            node->prevnode[call] = nullptr;
        else
            node->prevnode[call] = (Node*)(nodes + nodex->prevnode[call] * nodesize);
#endif
    }
    // Treat nodehead as part end and count parts
    copyrow(binrounds, tmprow); // Start from rounds
    node->nparts = 0;
    // If coursestructured, only set nparts for a course end
    if (!coursestructured || nodex->nodehead[callingbell] == callingbell)
        do
        {
            node->nparts++;
            transpose(tmprow, nodex->nodehead, tmprow2);
            copyrow(tmprow2, tmprow);
        } while (!isrounds(tmprow));
}
