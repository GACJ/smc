// SMC - Single Method Composer
// Copyright Graham A C John 1991-98 All rights reserved
// SMC32 32-bit conversion
// Copyright Mark B Davies 1998-2003

#include "ring.h"

const int MAXFRAGTABLESIZE = 1 << 9 * 2; // !! Must be a power of FOUR
const int MAXFRAGLENGTH = 32;            // Number of nodes
typedef unsigned int Pattern;
const int PATTLEN = sizeof(Pattern) * 4; // One node packed in 2 bits
const int NPATTS = (MAXFRAGLENGTH + PATTLEN - 1) / PATTLEN;

class Composer;

struct CompressedFrag
{
    Pattern duplicate[NPATTS];
    Pattern mask[NPATTS];
};

class Fragment
{
public:
    char length{};               // In nodes, or bits*2
    char startplacebell{};       // Only needed for input/output conversion
                                 // Fragments are held last node first, and packed one node in 2 bits
    Pattern duplicate[NPATTS]{}; // 16 nodes / int
    Pattern primary[NPATTS]{};

public:
    void clear()
    {
        length = 0;
        for (int i = 0; i < NPATTS; i++)
            duplicate[i] = primary[i] = 0;
    }
    void shiftupduplicate()
    {
        for (int i = NPATTS - 1; i > 0; i--)
            duplicate[i] = (duplicate[i] << 2) | (duplicate[i - 1] >> (sizeof(Pattern) * 8 - 2));
        duplicate[0] <<= 2;
    }
    void shiftupprimary()
    {
        for (int i = NPATTS - 1; i > 0; i--)
            primary[i] = (primary[i] << 2) | (primary[i - 1] >> (sizeof(Pattern) * 8 - 2));
        primary[0] <<= 2;
    }
    inline int isdup(Pattern* testpatt);
    int sameduplicate(Fragment* f)
    {
        if (length != f->length)
            return (FALSE);
        for (int i = 0; i < (length + PATTLEN - 1) / PATTLEN; i++)
            if (duplicate[i] != f->duplicate[i])
                return (FALSE);
        return (TRUE);
    }
    int sameprimary(Fragment* f)
    {
        if (length != f->length)
            return (FALSE);
        for (int i = 0; i < (length + PATTLEN - 1) / PATTLEN; i++)
            if (primary[i] != f->primary[i])
                return (FALSE);
        return (TRUE);
    }
    void replaceduplicate(Pattern* newdup)
    {
        for (int i = 0; i < (length + PATTLEN - 1) / PATTLEN; i++)
            duplicate[i] = newdup[i];
    }
    void replaceprimary(Pattern* newprim)
    {
        for (int i = 0; i < (length + PATTLEN - 1) / PATTLEN; i++)
            primary[i] = newprim[i];
    }
};

// No longer used from within composeloop
int Fragment::isdup(Pattern* testpatt)
{
    unsigned int patt;
    int i, l;

    l = length * 2;
    for (i = 0; i < NPATTS; i++)
        if (l >= 32)
        {
            if (testpatt[i] != duplicate[i])
                return (FALSE);
            l -= 32;
        }
        else
        {
            patt = testpatt[i] & ((1 << l) - 1);
            if (patt != duplicate[i])
                return (FALSE);
            return (TRUE);
        }
    return (TRUE);
}

struct FragMap
{
    union
    {
        BulkList* bulklist;
        int* fraglist; // Points to nfrags, CompressedFrag, CompressedFrag...
    };
};

class FragmentLibrary
{
protected:
    int mapsize; // !! Must be power of two
    int mask;    // = mapsize-1
    FragMap* fragmap[MAXNBELLS];
    LineFile f;

protected:
    Fragment* findprimary(Fragment* frag, int endplacebell);
    Fragment* findduplicate(Fragment* frag, int endplacebell);
    int searchorder(Pattern* patt1, Pattern* patt2, int length);

public:
    FragmentLibrary(int size)
        : f("tmplib")
    {
        mapsize = size;
        mask = mapsize - 1;
        for (int i = 0; i < MAXNBELLS; i++)
            fragmap[i] = nullptr;
    }
    ~FragmentLibrary()
    {
        for (int i = 0; i < MAXNBELLS; i++)
            delete fragmap[i];
    }
    int add(Fragment& newfrag, int endplacebell);
    void normalise();
    int newfile(char* filename);
    char* getname() { return f.getname(); }
    FragMap* getmapfornode(int endplacebell) { return fragmap[endplacebell]; }
    int read(Composer* ring, char* filename);
    int writelibheader(Composer* ring);
    int writefragment(Composer* ring, Fragment* frag, int endplacebell);
    int compress();
};
