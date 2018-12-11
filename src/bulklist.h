#ifndef INCBULKLIST
#define INCBULKLIST

// Bulk List class
// Copyright Mark B Davies 1997

enum BulkListError
{
    BULKLISTOK,
    BULKLISTBADINIT,
    BULKLISTNOTINLIST,
    BULKLISTNOMEM,
    BULKLISTFULL,
    BULKLISTINTERNAL
};

class BulkAlloc
{
public:
    BulkAlloc* next;
    char itemstorage[0];
    // Extended dynamically

public:
    void init() { next = nullptr; }
    void free()
    {
        if (next)
        {
            next->free();
            next = nullptr;
        }
    }
};

// A BulkList object contains an allocation block sufficient to hold 'maxitems'
// items of size 'bytesperitem'. If more items are added than this BulkList can
// handle, another BulkList is automatically allocated and chained on
class BulkList
{
protected:
    unsigned int nitems;        // Current number of items stored in BulkAlloc chain
    unsigned int itemsperalloc; // Number of items possible in each BulkAlloc
    unsigned int maxitems;      // Limit on total number of items possible in chain
    unsigned int bytesperitem;  // Item size in bytes
    BulkAlloc* alloc;           // Allocation pointer
    BulkAlloc* last;            // Final BulkAlloc node on chain
    BulkListError err;          // Set if error conditition occurs

public:
    // BulkList constructor needs maxitems and itemsize. These are automatically
    // passed on to any succeeding BulkList nodes in the chain (via 'next')
    BulkList(unsigned int granularity, unsigned int itemsize)
    {
        itemsperalloc = granularity;
        bytesperitem = itemsize;
        maxitems = -1; // No limit
        nitems = 0;
        alloc = last = nullptr;
        err = BULKLISTOK;
    }
    BulkList(unsigned int granularity, unsigned int itemsize, unsigned int itemlimit)
    {
        itemsperalloc = granularity;
        bytesperitem = itemsize;
        maxitems = itemlimit;
        nitems = 0;
        alloc = last = nullptr;
        err = BULKLISTOK;
    }
    // Destructor destroys this node's allocation, and any attached chain
    // !!! Items in the list do not have destructors called !!!
    ~BulkList() { freeall(); }
    void freeall()
    {
        if (alloc)
            alloc->free();
        alloc = last = nullptr;
        nitems = 0;
        err = BULKLISTOK;
    }
    // Returns the current error state of the list, and clears it
    BulkListError geterror()
    {
        BulkListError ret = err;
        err = BULKLISTOK;
        return (ret);
    }
    // Returns the total number of items in entire BulkAlloc chain
    int listsize() { return nitems; }
    // Adds an item (must be of size 'bytesperitem') to the list. Automatically
    // allocates a 'next' BulkAlloc if there is insufficient room in this one.
    inline void* add(void* item);
    // Gets item number 'n' from the list. This may be in a BulkAlloc further down the
    // chain. 'N' should be less than listsize()
    inline void* getitem(unsigned int n);
};

void* BulkList::add(void* item)
{
    char* allocptr;
    int i;

    // Check if we have reached totalitemlimit
    if (nitems >= maxitems)
    {
        err = BULKLISTFULL;
        return (nullptr);
    }
    // If the initial allocation block does not yet exist, allocate it
    if (alloc == 0)
    {
        alloc = (BulkAlloc*)new char[sizeof(BulkAlloc) + bytesperitem * itemsperalloc];
        if (alloc == 0)
        {
            err = BULKLISTNOMEM;
            return (nullptr);
        }
        last = alloc;
        last->init();
        nitems = 0;
        i = 0;
    }
    // Otherwise, see if final block is full
    else
    {
        i = nitems % itemsperalloc;
        if (i == 0)
        {
            last->next = (BulkAlloc*)new char[sizeof(BulkAlloc) + bytesperitem * itemsperalloc];
            if (last->next == 0)
            {
                err = BULKLISTNOMEM;
                return (nullptr);
            }
            last = last->next;
            last->init();
        }
    }
    // Copy item into correct place in final block
    allocptr = &last->itemstorage[i * bytesperitem];
    for (unsigned int j = 0; j < bytesperitem; j++)
        allocptr[j] = ((char*)item)[j];
    nitems++;
    return allocptr;
}

void* BulkList::getitem(unsigned int n)
{
    BulkAlloc* allocptr = alloc;

    // Trying to get an item not in this BulkList?
    if (n >= nitems)
    {
        err = BULKLISTNOTINLIST;
        return (nullptr);
    }
    while (n >= itemsperalloc)
    {
        n -= itemsperalloc;
        allocptr = allocptr->next;
        if (allocptr == 0) // Internal check
        {
            err = BULKLISTINTERNAL;
            return (nullptr);
        }
    }
    return &allocptr->itemstorage[n * bytesperitem];
}

#endif // INCBULKLIST
