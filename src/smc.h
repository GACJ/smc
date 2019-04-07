// SMC - Single Method Composer
// Copyright Graham A C John 1991-98 All rights reserved
// SMC32 32-bit conversion
// Copyright Mark B Davies 1998-2000

#pragma once

#include "bulkhash.h"
#include "filer.h"
#include "fragment.h"
#include "ring.h"
#include <new>
#include <stdio.h>
#include <time.h>

#if defined(_MSC_VER)
#define COMPILER "msvc"
#elif defined(__GNUC__)
#define COMPILER "gcc"
#else
#error "Unknown compiler"
#endif

#if defined(_WIN64) || defined(__LP64__)
#define APPNAME "SMC64"
#else
#define APPNAME "SMC32"
#endif
#define VERSION APPNAME " v1.1-" COMPILER
#define COPYRIGHT "(c) 1998-2019 Mark B Davies & Graham A C John"
#define INEXT "smc"
#define MUSEXT "mus"
#define OUTEXT "sf0"
#define COMPEXT "cf0"
#define LIBEXT "lf0"

#define PREVNODES
#define STOREPALI
#define STOREPATT
#define FRAGPTRINNODE

// REVERSECALLS => search lastcalltype first. Gives minor improvement on P200MMX
//#define REVERSECALLS
// SWAPCALLORDER => lastcalltype=PLAIN & vice versa
// !!! No longer works
//#define SWAPCALLORDER

// How many million nodes between each showstats() call
#define SHOWSTATSFREQ 2

//#define ONECOMP

const int MAXSTACKDEPTH = 5000; // In recursefindnodes()
const int ALIGNMENT = 8;        // !! Must be a power of two
const int TRUTHTABLEWORDSIZE = 32;
const int BULKROWALLOC = 4096;       // Number of rows in each BulkList
const int MAXCOMPSPACE = MAXLINEBUF; // Maximum length in chars of output comp
const int COMPBUFFERSIZE = 4000 + MAXCOMPSPACE;
const char CHECKPOINT_SYMBOL = '!';

const int MAXBLOCKCHARS = 256; // Maximum length in chars of a "must-have" block

const int MAXCALLPOSNAME = 8;
const int MAXNPARTS = 3 * 4 * 5 * 7; // Maximum number of parts for 20 bells!!

const int COMPSPERBULKLIST = 1000; // Comps per BulkList
const int COMPSPERBULKHASH = 16;   // Comps per BulkHash

constexpr auto TOKEN_MUSIC = "MUSIC";
constexpr auto TOKEN_START = "SMC32";
constexpr auto TOKEN_END = "SMC32";
constexpr auto TOKEN_FRAGMENTLIB = "FRAGMENTS";

class Node;
class NodeExtra;

class ExtMethod : public Method
{
public:
    int buildtables(Composer& ring);

protected:
};

enum MusicType
{
    MUSICANYROW,
    MUSICHANDSTROKE,
    MUSICBACKSTROKE,
    MUSICLEADHEAD,
    MUSICCOURSINGORDER,
    MUSICWRAP
};

struct MusicRow
{
    char row[MAXNBELLS]; // Wildcard indicated with -1
    char wrapbackstroke[MAXNBELLS];
    char handstrokewrapmatch; // Set when a handstroke matches first part of wrap
    char sign;                // 0 = any, -1 = negative, +1 = positive
    char type;                // enum MusicType
};

// MusicDef defines one type of musical row. A row matches a particular MusicDef
// if it matches any of the wildcard rows in matches[0..nrows-1]
// Any matching row is given a score of 'weighting'. The total score of all matching
// rows must be greater than 'minscore'.
// Negative weightings and minscores can be used to exclude more than a certain
// amount of undesirable rows
class MusicDef
{
public:
    char* name;
    int nrows;
    MusicRow* matches;
    int weighting;
    int minscore;

public:
    MusicDef()
    {
        name = nullptr;
        matches = nullptr;
        nrows = weighting = minscore = 0;
    }
    ~MusicDef()
    {
        delete[] name;
        delete[] matches;
    }
    int ensurespace(int n);
};

class MusicCount
{
public:
    int nmusicdefs;
    int* score;

public:
    MusicCount()
    {
        score = nullptr;
        nmusicdefs = 0;
    }
    ~MusicCount() { delete[] score; }
    int alloc(int n)
    {
        delete[] score;
        score = new (std::nothrow) int[n];
        if (score == nullptr)
            return (FALSE);
        nmusicdefs = n;
        return (TRUE);
    }
    int ensurespace(int n);
    int set(MusicCount& m);
};

// Holds data for "must-have" blocks: calling and start/end lhs, plus music counts
class Block
{
public:
    int blocknumber; // Seperate numbering for musthave and shouldhave blocks; starts at 1 for each
                     // Fields read from input
    char entrylh[MAXNBELLS];
    char exitlh[MAXNBELLS];
    char calling[MAXBLOCKCHARS];
    char essential; // Set for musthave blocks, clear for shouldhave
    char spare1;
    char spare2;
    char spare3;
    // Fields calculated by table build
    NodeExtra* entrynode;
    NodeExtra* exitnode;

public:
    Block() { essential = FALSE; }
    const char* blocktype();
};

// One call in the composition - type of call and node pointer
struct Composition
{
    int call;
    Node* node;
#ifdef STOREPATT
    unsigned long long pattern;
#endif
#ifdef STOREPALI
    Node* palinode;
#endif
};

class CompStore
{
public:
    char* calling;
    int allocsize;
    unsigned int hashvalue; // Unmasked hash value - quick test for node-indentity
    int nparts;
    int length;
    int nodesperpart;

public:
    CompStore()
    {
        calling = nullptr;
        allocsize = 0;
    }
    ~CompStore() { delete (calling); }
    int ensurespace(int nnodes);
    int copyincomp(int nodesperpart, Composition* composition);
};

class CompMusicStore : public CompStore
{
public:
    int rot;
    int score;
    MusicCount music;
    int nTVs;
    int callcount[NDIFFCALLS];
    int ncalls;
    int nmusthaveblocks; // Includes shouldhaves
    char* author;

public:
    CompMusicStore()
        : CompStore()
    {
        nTVs = 0;
        author = nullptr;
    }
    ~CompMusicStore() { delete author; }
    int isTV(CompMusicStore& storedcomp);
};

class CompSorter
{
public:
    CompSorter()
        : bulkalloc(COMPSPERBULKLIST, sizeof(CompMusicStore))
    {
        n = listsize = 0;
        list = nullptr;
    }
    int init(int ncomps)
    {
        bulkalloc.freeall();
        listsize = n = 0;
        delete list;
        list = nullptr;
        list = new (std::nothrow) CompMusicStore*[ncomps];
        if (list == nullptr)
            return (FALSE);
        listsize = ncomps;
        return (TRUE);
    }
    int addcomp(CompMusicStore& newcomp);
    int adduniquecomp(CompMusicStore& newcomp, Composer* ring); // Debug only
    int ncompslisted() { return n; }
    CompMusicStore* get(int compn)
    {
        if (compn < 0 || compn >= n)
            return (nullptr);
        return list[compn];
    }

protected:
    int n;
    int listsize;
    BulkList bulkalloc;
    CompMusicStore** list;
};

class CompHasher : public BulkHash
{
public:
    int init(int tablesize);
    int addcomp(Composer* ring);
};

// Used when Composer.bitwisetruthflags set: encodes a point and
// mask to the bitwise truth table.
struct FalseBits
{
    unsigned int* tableptr;
    unsigned int tablemask;

    int get_and()
    {
        return *tableptr & tablemask;
    }

    void do_or()
    {
        *tableptr |= tablemask;
    }

    void do_xor()
    {
        *tableptr ^= tablemask;
    }
};

// Data for a node which is not directly used by the composing loop
class NodeExtra
{
public:
    Node* node;
    char nodehead[MAXNBELLS]; // Leadhead row for start of node
    int num;                  // Consecutive numbering
    int truthflagbit;         // Optimised numbering for bitwise truthtables
    int truthflagword;
    int nleads;
    union
    {
        int* music;
        int combinedscore; // Used if 'optimisemusic' is set
    };
    char callingbellpos[NDIFFCALLS]; // Position at end of node, after call
    char negative;
    char excluded; // Set if contains excluded change (must be allowed in regen search until composition rot check)
    char musicallocated;
    char renumbered;      // When using bitwise truth, set if node number optimised
    char inmusthaveblock; // Set if this node contained in a "must-have" block (but not entry node)
    char blockentry;      // Set if this node is the entry point to a "must-have" block
    char essential;       // Set if this node must be contained in the comp (ie exclude all false nodes)
                          // The following are duplicates of fields in Node
                          // They are needed here because the NodeExtra array is allocated before
                          // the Node array, and we need somewhere to put them in the interval!
    char comesround;
    int nrows; // to rounds if present
    int nfalsenodes;
    int* falsenodes;
    int nfalsebits;
    FalseBits* falsebits; // Only used if Composer.bitwisetruthflags set
    int nextnode[NDIFFCALLS] = { -1 };
#ifdef PREVNODES
    int prevnode[NDIFFCALLS] = { -1 };
#endif

public:
    NodeExtra()
    {
        falsenodes = nullptr;
        music = nullptr;
        musicallocated = FALSE;
        renumbered = FALSE;
        essential = FALSE;
        excluded = FALSE;
    }
    ~NodeExtra()
    {
        delete[] falsenodes;
        if (musicallocated)
            delete[] music;
    }
    // Kills all nextnode pointers except that for calltype "call" (internal call num)
    void forcenextnodeto(int call)
    {
        for (int i = 0; i < NDIFFCALLS; i++)
            if (i != call)
                nextnode[i] = -1;
    }
};

// A basic structure which is stored in a BulkHash during the initial recursive
// search for included nodes
// Using this table we can then easily run through all included nodes without
// having to perform the recursive search again
struct HashedNode
{
    int factnum;
    char included; // Set if node visited AND included
    char excluded; // Set if contains excluded change
    char nodehead[MAXNBELLS];
    NodeExtra* nodex; // Used for quick lookup in false node calculation
};

// Data defining one node (i.e. a lead or a number of leads between calling
// positions) which is used by the composing loop
class Node
{
public:
    char included; // Set if included in composition
    char nparts;
    char comesround;
    char callcountposindex; // NDIFFCALLS * (position of calling bell at start of node)
    NodeExtra* nodex;
    int unvisitable;
    int regenoffset[NDIFFCALLS];
#ifdef FRAGPTRINNODE
    FragMap* fragmap;
#endif
#ifdef PREVNODES
    Node* prevnode[NDIFFCALLS];
#endif
    Node* nextnode[NDIFFCALLS];
    int nrows; // to rounds if present
    int nfalsenodes;
    // !!! MUST be the last field in structure - extended with extra alloc
    union
    {
        Node* falsenodes[1];
        FalseBits falsebits[1]; // This one is used if "bitwisetruthflags" is set.
    };
};

class Exclusions
{
public:
    char nointernalrounds;
    char noleadheadrounds;
    char allpartsallowed;
    char allowedcalls[NDIFFCALLS][MAXNBELLS]; // wrt callingbell
    char defaultcalls[NDIFFCALLS];            // cleared if call positions or limits given
    int minnparts;
    char allowedparts[MAXNPARTS + 1];
    int nrows;
    MusicRow* rows; // excluded row matches

public:
    Exclusions()
    {
        nrows = 0;
        rows = nullptr;
    }
    ~Exclusions() { delete[] rows; }
};

class Stats
{
public:
    unsigned long long ncompsfound;
    unsigned long long nrotsfound;
    unsigned int ncompsoutput;
    unsigned int nfragsfound;
    unsigned long long nodesgenerated;
    unsigned int nodecount; // temporary count, added in to nodesgenerated
    unsigned int evalcount; // # comps evaluated in this display interval
    int bestscore;
    int longestlength;
    unsigned int prunecount; // # successful fragment prunes
    long long starttime;
    long long elapsed;
    clock_t lastdisplaytime; // time when last showstats() done
                             // The following are copies of the above which are saved when a composition is
                             // buffered. This is so that they can be output later with the composition.
    unsigned long long save_ncompsfound;
    unsigned long long save_nrotsfound;
    long long save_nodesgenerated;
    long long save_elapsed;

public:
    void clear()
    {
        ncompsfound = nrotsfound = nodesgenerated = 0;
        ncompsoutput = nfragsfound = nodecount = evalcount = 0;
        prunecount = 0;
        bestscore = longestlength = 0;
        elapsed = 0;
        lastdisplaytime = 0;
    }
    void save()
    {
        save_ncompsfound = ncompsfound;
        save_nrotsfound = nrotsfound;
        save_nodesgenerated = nodesgenerated;
        save_elapsed = elapsed;
    }
};

// This is used to store the false LHs of the first lead (from rounds)
// FLHs for each included lead are calculated by transposition from these
struct FalseLH
{
    int pos;
    char row[MAXNBELLS];
};

class Composer : public Ring
{
    friend class CompHasher;
    friend class CompSorter;

public:
    char methodname[200];

    // Various flags controlling operation of composing loop
    char coursestructured;  // Clear to allow any part end for multiparts
    char singleleadnodes;   // Don't use nodes if set
    char useMMX;            // Use MMX instructions if set
    char rotationalsort;    // Use comp regeneration if set
    char disableregen;      // Debugging flag, forces rotationalsort off
    char palindromic;       // Set for palindromic search
    char midnodeapex;       // Set if mid-node apex found
    char showbestyet;       // If set, show highest-scoring comp so far
    char showlongestyet;    // If set, show longest length so far
    char showallrots;       // If not set, show best rotation only
    char optimisemusic;     // Set if no music minimums
    char countingcalls;     // Set if call counting
    char bitwisetruthflags; // Set if using bitwise truth flags (see Composer.truthtable)
    char startinrounds;     // FALSE if using a non-rounds starting point
    char endinrounds;       // FALSE if using a non-rounds ending point
    char samestartandend;   // Set if start row = end row

    char cancomeround;         // Check during table building
    char extendinglead;        // Set if call at Home produces Home
    char redomusic;            // Set if analyser to use music in .sf file
    char printcourseendsfirst; // Set to print course ends in left column

    // False LHs for 1st lead from rounds
    int nfalseLHs;
    FalseLH* falseLHs;
    char* workinglead;

    char binrounds[MAXNBELLS];
    char startrow[MAXNBELLS];  // The row to start composing from (rounds if startinrounds set)
    char finishrow[MAXNBELLS]; // Finishing row - see also Composer::isfinishrow()

    // Node data
    int nodespercourse;
    int nodesincluded;
    int leadspernode[MAXNBELLS];
    int leadstonodeend[MAXNBELLS];
    int nodesize; // Size (in bytes) of Node structure
    char* nodealloc;
    int maxnodefalse; // Controls Node alloc size
    int maxnodealloc; // Used when bitwisefalseflags set to control Node alloc size
    int minnodefalse;
    int courseenddist[MAXNBELLS]; // For each place bell - used by regen code
    Node* startnode;              // If start row = finish row, will be "extra" node 0
    NodeExtra* finishrounds;      // -> finishing rounds
    int truthtablesize;           // Only used if bitwisetruthflags set
                                  // For temporary use in table building - keeps a list of every visited nodehead
    BulkHash nodehasher;
    HashedNode nodehashitem;
    int maxstackdepth; // Max allowed recursion depth
    int stackhiwater;  // Max recursion depth reached

    // Fields used by inner composing loop
    char* nodes; // Node structure is variable size!
    Composition* comp;
    unsigned int* truthtable; // Used in place of Node.included if bitwisetruthflags set
    int ncompnodes;           // Length of comp in nodes
    int complength;           // Length of comp in changes
    int lengthcountdown;      // = ebp in composing loop
    int regenptr;
    Composition* lastregen;
    // Normally minlengthnow = minlength. However, when 'showlongestyet' is set,
    // this is set to 0 and gradually works up to minlength
    int minlengthnow;
    int maxpartlength; // Actually maxlength-maxpartlength!
    int maxlength;
    int maxpalilength; // = maxlength/2 for palindromic search
    int minlength;
    int ncallsleft[NDIFFCALLS];                  // Current remaining allowed # of calls in comp
    int ncallsleftperpos[MAXNBELLS][NDIFFCALLS]; // Remaining allowed # of calls per pos
    int maxcalls[NDIFFCALLS];                    // Max number of calls of each type allowed
    int maxcallsperpos[MAXNBELLS][NDIFFCALLS];   // Max calls allowed at each calling pos
    int npartcalls[NDIFFCALLS];                  // Ncallsleft allowed at 1st part end
    int npartcallsperpos[MAXNBELLS][NDIFFCALLS]; // Ncallsperpos allowed at 1st part end

    // Tables
    NodeExtra* nodeextra;
    Stats stats;
    MusicCount music; // Total music in this comp
    int nmusicdefs;
    MusicDef* musicdefs;
    Exclusions exclude;
    int score;
    int minscore;
    BulkList* musthaveblocks; // "Must-have" blocks

    // Composition and call fields
    int callingbell;
    MusicDef courseend;
    int courselen; // In leads
    int nparts;
    int nodesperpart;
    int comprot;
    char* compalloc;                       // Extra alloced for use by regen optimisation
    int ncalltypes;                        // Inclusive count i.e. not counting PLAIN
    Call calltypes[NDIFFCALLS];            // Maps internal call number -> Call type
    int internalcallnums[NDIFFCALLS];      // Maps Call -> internal call number
    char calltrans[NDIFFCALLS][MAXNBELLS]; // Lead tranpositions for each call
    char callposnames[NDIFFCALLS][MAXNBELLS][MAXCALLPOSNAME + 1];
    int callposorder[(NDIFFCALLS - 1) * MAXNBELLS];     // Place bells of call pos, in order
    int callposcallmasks[(NDIFFCALLS - 1) * MAXNBELLS]; // Calls at each calling position
    double percent0;                                    // Tree percentage of plain course (0.0-1.0)
    double percentrange;                                // 1.0/tree range between bob and plain course
    int callcount[NDIFFCALLS];
    int ncallsincomp;

    // File output
    LineFile outfile;
    LineFile musicfile;
    char* musicinclude;
    int ncompsinbuffer;
    char* compbufptr;
    char compbuffer[COMPBUFFERSIZE];
    clock_t lastcheckpoint;
    bool deterministicoutput{};

    // Composition storage and fragment libraries
    char makefraglib; // Set if want to create fragment library
    char usefraglib;  // Set if want to use fragment optimisation
    char storecomps;  // Set if we are storing comps
    int ncompstostore;
    CompSorter compsorter;
    CompMusicStore* specialcomps;
    FragmentLibrary fraglib;
    CompHasher comphasher;
    char* author; // Composer, if not SMC32

public:
    Composer()
        : Ring()
        , outfile("tmp")
        , musicfile("music")
        , fraglib(MAXFRAGTABLESIZE)
    {
        clear();
    }
    Composer(ExtMethod* method)
        : Ring(method)
        , outfile("tmp")
        , musicfile("music")
        , fraglib(MAXFRAGTABLESIZE)
    {
        clear();
    }
    void clear()
    {
        workinglead = nullptr;
        nodealloc = nullptr;
        compalloc = nullptr;
        musicdefs = nullptr;
        nodeextra = nullptr;
        resetcompbuffer();
        falseLHs = nullptr;
        specialcomps = nullptr;
        truthtable = nullptr;
        musthaveblocks = nullptr;
    }
    ~Composer()
    {
        delete[] workinglead;
        delete[] nodealloc;
        delete[] compalloc;
        delete[] musicdefs;
        delete[] nodeextra;
        delete[] falseLHs;
        delete[] truthtable;
        delete musthaveblocks;
    }
    void init(ExtMethod* method) { Ring::init(method); }
    int setdefaults();
    int writefileheader(LineFile& f);
    int writeheaderessential(LineFile& f);
    int readinputfile(LineFile& f);
    int readmusicfile(LineFile& f);
    int readmusicminoverrides(LineFile& f);
    int skipmusicfile(LineFile& f);
    int readcalling(char* compbuf, CompStore& storedcomp, Node* node, int nnodes = 0);
    int setup();
    int newcomp();
    int newsearch();
    int restartsearch();
    void compose();
    void findcourseenddists();
    void findnodes();
    int findfalseLHs();
    int findtablesize();
    int gennodetable();
    int musicsort(int maxncomps);
    char* writepattern(char* buf, Pattern* pattern, int length, int startplacebell);
    Node* findstartingnode(int startplacebell);

protected:
    void resetcompbuffer()
    {
        compbufptr = compbuffer;
        ncompsinbuffer = 0;
        lastcheckpoint = 0;
    }
    void defaultcallingpositions(int call);
    int addcalltype(Call calltype);
    int readcall(int call);

public:
    void showstats() { showstats(false); }
    void showfinalstats() { showstats(true); }

private:
    void showstats(bool isFinal);

protected:
    void printelapsed(char* buf, int nearestsecond = TRUE);
    void finaloutput();

public:
    void analysecomp();

protected:
    void countparts();
    void countcalls();
    int evalcomp();
    int flushcompbuffer(int checkpoint = TRUE);
    int outputcomp();
    int inputcomp(char* compbuf);
    double calcpercentcomplete();
    int findpercentrange();
    char* printcall(char* buf, int call, int pos);
    int recursefindnodes(int stacklevel);
    void gennodex(int n);
    void countnodemusic(int n);
    int preparemusthaveblocks();
    int musthaveblockerror(Block* b, const char* err);
    HashedNode* findnodefromLH();
    int readblockcalling(Block* block);
    void calcfalsenodes(int n);
    void processessentialnodes();
    void processexcludednodes();
    int calcbitwisetruthtables();
    void calcfalsebitmasks(NodeExtra* nodex);
    int truthflagpackingscore(NodeExtra* nodex);
    void gennode(int n);
    int isLHexcluded();
    int isrowexcluded();
    inline int isrounds(char* row)
    {
        for (int i = 0; i < nbells; i++)
            if (row[i] != i)
                return (FALSE);
        return (TRUE);
    }
    inline int isfinishrow(char* row)
    {
        for (int i = 0; i < nbells; i++)
            if (row[i] != finishrow[i])
                return (FALSE);
        return (TRUE);
    }
    inline int isnegative(char* row);
    inline int findcallingbell(char* row)
    {
        for (int i = 0; i < nbells; i++)
            if (row[i] == callingbell)
                return (i);
        return (-1);
    }
    inline int findcallingbell() { return findcallingbell(row); }
    inline void printrow(const char* row)
    {
        for (int i = 0; i < nbells; i++)
            printf("%c", rounds[(unsigned char)row[i]]);
        printf("\n");
    }

    inline void writerow(const char* row, char* buf)
    {
        int i;
        for (i = 0; i < nbells; i++)
        {
            buf[i] = rounds[(unsigned char)row[i]];
        }
        buf[i] = 0;
    }

    int readlh(char* p, char* buf, const char* errprefix);
    int calcLHfactnum(char* row);
    void calcfactorials();
    void findcalltranspositions();
    int readrowmatch(char* buffer, MusicRow& m);
    void writerowmatch(char* buffer, MusicRow& m);
    int ismusicmatch(MusicDef& m);
    int isrowmatch(MusicRow& m);
    void compose_regen_MMXfrag_cps_multipart();
    void compose_regen_MMX_cps_multipart();
    void compose_regen_MMX_cps_multipart_callcount();
    void compose_regen_MMXfrag_cps();
    void compose_regen_MMX_cps();
    void compose_regen_MMX_cps_callcount();
    void compose_regen_cps_multipart();
    void compose_regen_cps_multipart_callcount();
    void compose_regen_cps();
    void compose_regen_cps_callcount();
    void compose_palindromic_MMX_cps();
    void compose_MMXfrag_cps();
    void compose_MMX_cps();
    void compose_MMX_cps_callcount();
    void compose_palindromic_cps();
    void compose_cps();
    void compose_cps_callcount();

    void compose_regen_MMXfrag_falsebits_multipart();
    void compose_regen_MMX_falsebits_multipart();
    void compose_regen_MMX_falsebits_multipart_callcount();
    void compose_regen_MMXfrag_falsebits();
    void compose_regen_MMX_falsebits();
    void compose_regen_MMX_falsebits_callcount();
    void compose_regen_falsebits_multipart();
    void compose_regen_falsebits_multipart_callcount();
    void compose_regen_falsebits();
    void compose_regen_falsebits_callcount();
    void compose_palindromic_MMX_falsebits();
    void compose_MMXfrag_falsebits();
    void compose_MMX_falsebits();
    void compose_MMX_falsebits_callcount();
    void compose_palindromic_falsebits();
    void compose_falsebits();
    void compose_falsebits_callcount();

    void compose_regen_MMXfrag_multipart();
    void compose_regen_MMX_multipart();
    void compose_regen_MMX_multipart_callcount();
    void compose_regen_MMXfrag();
    void compose_regen_MMX();
    void compose_regen_MMX_callcount();
    void compose_regen_multipart();
    void compose_regen_multipart_callcount();
    void compose_regen();
    void compose_regen_callcount();
    void compose_palindromic_MMX();
    void compose_MMXfrag();
    void compose_MMX();
    void compose_MMX_callcount();
    void compose_palindromic();
    void compose_();
    void compose_callcount();

    int displaycomp(int compn, CompMusicStore* thiscomp, LineFile& f);
    int findcourseend(char* leadhead, int callingbellcourseendpos);
    int storecomposition(CompStore& storedcomp);
    int storecomposition(CompMusicStore& storedcomp);
    int storecomp2(CompMusicStore& storedcomp);
    int loadcomposition(CompMusicStore& storedcomp);
    int extractfragment(CompStore* storedcomp);
    int isnodeidentical(CompStore* storedcomp, unsigned int hashvalue);
    unsigned int calcnodehash();
};

// Can handle wildcard bells (-1)
inline int Composer::isnegative(char* row)
{
    char tmprow[MAXNBELLS];
    int i, j, b, negative = 0;

    copyrow(row, tmprow);
    for (i = 0; i < nbells; i++)
    {
        b = tmprow[i];
        while (b >= 0 && b != i)
        {
            j = tmprow[b];
            tmprow[b] = b;
            b = j;
            negative ^= 1;
        }
    }
    return (negative);
}
