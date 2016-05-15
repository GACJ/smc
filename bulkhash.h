#ifndef INCBULKHASH
#define INCBULKHASH

#include "bulklist.h"

// Bulk Hash class
// Copyright Mark B Davies 1998

typedef (*HashItemMatchFn)(void *item,int hashvalue,void *fndata);

// Hash table using BulkList classes for expansion
// Caller must supply hash values

class BulkHash
{
protected:
 int tablesize;		// !! Must be a power of two
 BulkList **hashtable;
 BulkListError err;
// Statistic
public:
 int nitems;
 int entriesused;
 int biggestentry;

public:
 BulkHash() {hashtable=NULL; err=BULKLISTOK;}
 ~BulkHash() {freeall();}
 void freeall() {if (hashtable) for (int i=0; i<tablesize; i++)
 			   safedelete(hashtable[i]);
 			  hashtable=NULL;}
 inline init(int tsize,int bulksize,int itemsize,int bulklimit=-1);
 BulkListError geterror() {BulkListError ret=err; err=BULKLISTOK; return(ret);}
 inline void *add(void *item,int hashvalue);
 int gettablesize() {return tablesize;}
 int getlistsize(int hashvalue) {int hv = hashvalue&(tablesize-1);
 			   if (hashtable[hv]==NULL) return(0);
 			   return hashtable[hv]->listsize();}
 inline void *getitem(int hashvalue,int n);
 inline void *finditem(int hashvalue,HashItemMatchFn f,void *fndata);
};

BulkHash::init(int tsize,int bulksize,int itemsize,int bulklimit)
{
 int i;

 if (tsize<=0)
 {
  err = BULKLISTBADINIT;
  return(FALSE);
 }
// Check tsize is a power of two
 i = tsize;
 while ((i&1)==0)
  i>>= 1;
 if (i!=1)
 {
  err = BULKLISTBADINIT;
  return(FALSE);
 }
 freeall();
 hashtable = new BulkList *[tsize];
 if (hashtable==NULL)
 {
  err = BULKLISTNOMEM;
  return(FALSE);
 }
 tablesize = tsize;
 for (i=0; i<tablesize; i++)
 {
  hashtable[i] = new BulkList(bulksize,itemsize,bulklimit);
  if (hashtable[i]==NULL)
  {
   err = BULKLISTNOMEM;
   return(FALSE);
  }
 }
 nitems = entriesused = biggestentry = 0;
 return(TRUE);
}

void *BulkHash::add(void *item,int hashvalue)
{
 int hv = hashvalue&(tablesize-1);
 int entrysize;
 void *iteminlist;

 iteminlist = hashtable[hv]->add(item);
 if (iteminlist==NULL)
 {
  err = hashtable[hv]->geterror();
  return(NULL);
 }
 nitems++;
 entrysize = hashtable[hv]->listsize();
 if (entrysize==1)
  entriesused++;
 if (entrysize>biggestentry)
  biggestentry = entrysize;
 return(iteminlist);
}

// Gets item number n from list for this hashvalue
void *BulkHash::getitem(int hashvalue,int n)
{
 int hv = hashvalue&(tablesize-1);
 void *item;

 item = hashtable[hv]->getitem(n);
 if (item==NULL)
  err = hashtable[hv]->geterror();
 return(item);
}

// Searches list for matching item
void *BulkHash::finditem(int hashvalue,HashItemMatchFn f,void *fndata)
{
 int hv = hashvalue&(tablesize-1);
 void *item;
 int i,n;

 if (hashtable[hv]==NULL)
  return(NULL);
 n = hashtable[hv]->listsize();
 for (i=0; i<n; i++)
 {
  item = hashtable[hv]->getitem(i);
  if (f(item,hashvalue,fndata))
   return(item);
 }
 return(NULL);
}

#endif // INCBULKHASH
