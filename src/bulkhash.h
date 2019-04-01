#ifndef INCBULKHASH
#define INCBULKHASH

#include "basedef.h"
#include "bulklist.h"
#include <new>

// Bulk Hash class
// Copyright Mark B Davies 1998

typedef int (*HashItemMatchFn)(void* item, int hashvalue, void* fndata);

// Hash table using BulkList classes for expansion
// Caller must supply hash values

class BulkHash
{
protected:
    int tablesize; // !! Must be a power of two
    BulkList** hashtable;
    BulkListError err;
    // Statistic
public:
    int nitems;
    int entriesused;
    int biggestentry;

public:
    BulkHash()
    {
        hashtable = nullptr;
        err = BULKLISTOK;
    }
    ~BulkHash() { freeall(); }
    void freeall()
    {
        if (hashtable != nullptr)
        {
            for (int i = 0; i < tablesize; i++)
            {
                delete hashtable[i];
                hashtable[i] = nullptr;
            }
        }
        delete[] hashtable;
        hashtable = nullptr;
    }
    inline int init(int tsize, int bulksize, int itemsize, int bulklimit = -1);
    BulkListError geterror()
    {
        BulkListError ret = err;
        err = BULKLISTOK;
        return (ret);
    }
    inline void* add(void* item, int hashvalue);
    int gettablesize() { return tablesize; }
    int getlistsize(int hashvalue)
    {
        int hv = hashvalue & (tablesize - 1);
        if (hashtable[hv] == nullptr)
            return (0);
        return hashtable[hv]->listsize();
    }
    inline void* getitem(int hashvalue, int n);
    inline void* finditem(int hashvalue, HashItemMatchFn f, void* fndata);
};

int BulkHash::init(int tsize, int bulksize, int itemsize, int bulklimit)
{
    int i;

    if (tsize <= 0)
    {
        err = BULKLISTBADINIT;
        return (FALSE);
    }
    // Check tsize is a power of two
    i = tsize;
    while ((i & 1) == 0)
        i >>= 1;
    if (i != 1)
    {
        err = BULKLISTBADINIT;
        return (FALSE);
    }
    freeall();
    hashtable = new (std::nothrow) BulkList*[tsize];
    if (hashtable == nullptr)
    {
        err = BULKLISTNOMEM;
        return (FALSE);
    }
    tablesize = tsize;
    for (i = 0; i < tablesize; i++)
    {
        hashtable[i] = new BulkList(bulksize, itemsize, bulklimit);
        if (hashtable[i] == nullptr)
        {
            err = BULKLISTNOMEM;
            return (FALSE);
        }
    }
    nitems = entriesused = biggestentry = 0;
    return (TRUE);
}

void* BulkHash::add(void* item, int hashvalue)
{
    int hv = hashvalue & (tablesize - 1);
    int entrysize;
    void* iteminlist;

    iteminlist = hashtable[hv]->add(item);
    if (iteminlist == nullptr)
    {
        err = hashtable[hv]->geterror();
        return (nullptr);
    }
    nitems++;
    entrysize = hashtable[hv]->listsize();
    if (entrysize == 1)
        entriesused++;
    if (entrysize > biggestentry)
        biggestentry = entrysize;
    return (iteminlist);
}

// Gets item number n from list for this hashvalue
void* BulkHash::getitem(int hashvalue, int n)
{
    int hv = hashvalue & (tablesize - 1);
    void* item;

    item = hashtable[hv]->getitem(n);
    if (item == nullptr)
        err = hashtable[hv]->geterror();
    return (item);
}

// Searches list for matching item
void* BulkHash::finditem(int hashvalue, HashItemMatchFn f, void* fndata)
{
    int hv = hashvalue & (tablesize - 1);
    void* item;
    int i, n;

    if (hashtable[hv] == nullptr)
        return (nullptr);
    n = hashtable[hv]->listsize();
    for (i = 0; i < n; i++)
    {
        item = hashtable[hv]->getitem(i);
        if (f(item, hashvalue, fndata))
            return (item);
    }
    return (nullptr);
}

#endif // INCBULKHASH
