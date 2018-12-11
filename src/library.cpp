// SMC32 library.cpp Copyright Mark B Davies 1998-2000

#include <limits.h>
#include "smc.h"

extern char callchars[];

// Move current Composer composition into storedcomp
int Composer::storecomposition(CompStore &storedcomp)
{
 if (!storedcomp.copyincomp(nodesperpart,comp))
  return(FALSE);
 storedcomp.nparts = nparts;
 return(TRUE);
}

// Move current Composer composition into storedcomp
int Composer::storecomposition(CompMusicStore &storedcomp)
{
 storedcomp.rot = comprot;
 storedcomp.score = score;
 if (!storedcomp.music.set(music))
  return(FALSE);
 storedcomp.nTVs++;
 return storecomp2(storedcomp);
}

// Used by music analyser - copies Composer composition into storedcomp, but
// assumes rot, score, music and nTVs have already been set up
int Composer::storecomp2(CompMusicStore &storedcomp)
{
 int i;

 if (!storedcomp.copyincomp(nodesperpart,comp))
  return(FALSE);
 storedcomp.ncalls = ncallsincomp;
 for (i=0; i<=ncalltypes; i++)
  storedcomp.callcount[i] = callcount[i];
 storedcomp.nparts = nparts;
 storedcomp.length = complength;
 storedcomp.author = author;
 author = NULL;		// Unlinks author from Composer!!
 return(TRUE);
}

// !! Overwrites current Composer state
// Also sets up include flags, testing for truth as it goes
int Composer::loadcomposition(CompMusicStore &storedcomp)
{
 Node *node;
 int rot;
 int i,j;

 author = storedcomp.author;
 storedcomp.author = NULL;		// Author unlinked from storedcomp!
 score = storedcomp.score;
 if (!music.set(storedcomp.music))
  return(FALSE);
 nodesperpart = storedcomp.nodesperpart;
 nparts = storedcomp.nparts;
 ncompnodes = nparts*nodesperpart;
 complength = storedcomp.length;
 rot = storedcomp.rot;
// Reset included flags
 for (i=0; i<nodesincluded; i++)
 {
  node = (Node *)(nodes+i*nodesize);
  node->included = FALSE;
  node->unvisitable = 0;
 }
 node = comp[0].node;
 for (i=0; i<ncompnodes; i++)
 {
  node->included = TRUE;
  node->unvisitable++;
  comp[i].call = storedcomp.calling[rot];
  node = node->nextnode[comp[i].call];
  if (node==NULL)
  {
   printf("ERROR: bad stored composition\n");
   return(FALSE);
  }
  comp[i+1].node = node;
  if (node->included)
  {
   printf("ERROR: false composition\n");
   return(FALSE);
  }
  for (j=0; j<node->nfalsenodes; j++)
   if (node->falsenodes[j]->included)
   {
    printf("ERROR: false composition %d %d\n",j,node->nfalsenodes);
    printrow(node->nodex->nodehead);
    printrow(node->falsenodes[j]->nodex->nodehead);
    return(FALSE);
   }
  if (++rot>=storedcomp.nodesperpart)
   rot = 0;
 }
 if (node)
 {
  node->included = TRUE;
  node->unvisitable++;
 }
 comprot = 0;
 return(TRUE);
}

void Composer::countcalls()
{
 int i;

 for (i=0; i<=ncalltypes; i++)
  callcount[i] = 0;
 for (i=0; i<nodesperpart; i++)
  callcount[comp[i].call]+= nparts;
 ncallsincomp = 0;
 for (i=1; i<=ncalltypes; i++)
  ncallsincomp+= callcount[i];
}

// Returns TRUE if storedcomp visits exactly the same nodes as the composition
// which is currently set up in Composer
int Composer::isnodeidentical(CompStore *storedcomp,unsigned int hashvalue)
{
 Node *node;
 int storednodes = storedcomp->nodesperpart*storedcomp->nparts;
 int i,rot;

// Must have the same hash value
// (This is not masked to table size, so is a useful secondary check)
 if (hashvalue!=storedcomp->hashvalue)
  return(FALSE);
// Must be the same length
 if (ncompnodes!=storednodes)
  return(FALSE);
// Go through stored composition, checking visited nodes
 node = comp[0].node;
 rot = 0;
 for (i=0; i<storednodes; i++)
 {
  node = node->nextnode[storedcomp->calling[rot]];
  if (!node->included)
   return(FALSE);
  if (++rot>=storedcomp->nodesperpart)
   rot = 0;
 }
 return(TRUE);
}

// This should only be used if Composer::isnodeidentical(storedcomp) is TRUE
// If checks the two touches to see if there is ONE fragment with different callings
// It's no good if two separate fragments exist, because they might duplicate
// nodes in each other - at the moment, we have no way of checking node usage of
// an individual fragment, just the overall composition
// If a suitable fragment is found, it is added to the fragment library
int Composer::extractfragment(CompStore *storedcomp)
{
 Node *node = comp[0].node;
 Fragment frag;
 int start,finish;
 int i,rot,endplacebell,primary;

// First find the beginning of the fragment - ie where callings FIRST differ
 i = 0;
 rot = 0;
 frag.startplacebell = callingbell;
 while (i<ncompnodes && comp[i].call==storedcomp->calling[rot])
 {
  frag.startplacebell = node->nodex->callingbellpos[comp[i].call];
  node = node->nextnode[comp[i].call];
  if (node==NULL)
  {
   printf("ERROR: failed to parse compositions for duplicate fragment\n");
   return(FALSE);
  }
  if (++rot>=storedcomp->nodesperpart)
   rot = 0;
  i++;
 }
 if (i>=ncompnodes)			// Compositions identical!
  return(TRUE);
 start = i;
// Continue through both touches, finding LAST place where callings differ
 while (i<ncompnodes)
 {
  if (comp[i].call!=storedcomp->calling[rot])
  {
   finish = i;
   endplacebell = node->nodex->callingbellpos[comp[i].call];
  }
  node = node->nextnode[comp[i].call];
  if (node==NULL)
  {
   printf("ERROR: failed to parse compositions for duplicate fragment\n");
   return(FALSE);
  }
  if (++rot>=storedcomp->nodesperpart)
   rot = 0;
  i++;
 }
 if (!coursestructured)
  endplacebell = 0;
 frag.clear();
 frag.length = finish-start+1;
 if (frag.length>MAXFRAGLENGTH)		// Fragment too long?
  return(TRUE);
 rot = start;
 while (rot>=storedcomp->nodesperpart)
  rot-= storedcomp->nodesperpart;
// Work out which fragment comes first in the search order
 primary = (comp[start].call < storedcomp->calling[rot]);
// Extract the fragment
 for (i=start; i<=finish; i++)
 {
  frag.shiftupduplicate();
  frag.shiftupprimary();
  if (primary)
  {
   frag.primary[0]|= comp[i].call&3;
   frag.duplicate[0]|= storedcomp->calling[rot]&3;
  }
  else
  {
   frag.primary[0]|= storedcomp->calling[rot]&3;
   frag.duplicate[0]|= comp[i].call&3;
  }
  if (++rot>=storedcomp->nodesperpart)
   rot = 0;
 }
 i = fraglib.add(frag,endplacebell);
 if (i<0)
  return(FALSE);
 if (i>0)			// New fragment - write to disk
 {
  if (!fraglib.writefragment(this,&frag,endplacebell))
   return(FALSE);
  stats.nfragsfound++;
 }
 return(TRUE);
}

// !! Doesn't mask to table size - this is now done in BulkHash
unsigned int Composer::calcnodehash()
{
 Node *node = comp[0].node;
 int hashvalue=0;
 int i;

 for (i=0; i<ncompnodes-1; i++)
 {
  node = node->nextnode[comp[i].call];
  if (node==NULL)
   return hashvalue;
  hashvalue+= node->nodex->num;
 }
 return hashvalue;
}

int CompHasher::init(int tablesize)
{
 if (!BulkHash::init(tablesize,4,sizeof(CompStore),COMPSPERBULKHASH))
 {
  printf("ERROR: failed to allocate composition hash table\n");
  return(FALSE);
 }
 return(TRUE);
}

// Inserts a new composition into the hashtable
// Checks for node-identical compositions - extracts fragments if found
int CompHasher::addcomp(Composer *ring)
{
 CompStore *storedcomp;
 CompStore newcomp;
 int n,i;

 newcomp.hashvalue = ring->calcnodehash();
// First see if node-identical compositions already exist in the table
 n = getlistsize(newcomp.hashvalue);
 if (n<0)
 {
  printf("INTERNAL ERROR: bad composition hash value!\n");
  return(FALSE);
 }
 for (i=0; i<n; i++)
 {
  storedcomp = (CompStore *)getitem(newcomp.hashvalue,i);
  if (storedcomp==NULL)
  {
   n = getlistsize(newcomp.hashvalue);
   printf("INTERNAL ERROR: failed to retrieve composition from hash table!\n");
   return(FALSE);
  }
// Node-identical composition found - extract fragments and return
  if (ring->isnodeidentical(storedcomp,newcomp.hashvalue))
  {
   if (!ring->extractfragment(storedcomp))
    return(FALSE);
   return(TRUE);
  }
 }
// No node-identical composition found - add new comp to hash table
 if (!ring->storecomposition(newcomp))
  return(FALSE);
 if (!add(&newcomp,newcomp.hashvalue))
  if (geterror()==BULKLISTFULL)
  {
   storedcomp = (CompStore *)getitem(newcomp.hashvalue,clock()%getlistsize(newcomp.hashvalue));
   safedelete(storedcomp->calling);
   *storedcomp = newcomp;
  }
  else
  {
   printf("ERROR: failed to add composition to hash table\n");
   return(FALSE);
  }
 newcomp.calling = NULL;	// Calling has been transferred - dereference in newcomp
 newcomp.allocsize = 0;
 return(TRUE);
}

// Returns -1 if error, 0 if fragment already existed, 1 if new fragment
int FragmentLibrary::add(Fragment &newfrag,int endplacebell)
{
 BulkList *fraglist;
 Fragment *oldfrag;
 unsigned int mapindex;
 int i;

// Check that the map for this place bell has been allocated
 if (fragmap[endplacebell]==NULL)
 {
  fragmap[endplacebell] = new FragMap[mapsize];
  if (fragmap[endplacebell]==NULL)
  {
   printf("ERROR: failed to allocate fragment library\n");
   return(-1);
  }
  for (i=0; i<mapsize; i++)
   fragmap[endplacebell][i].bulklist = NULL;
 }
// Calculate the map index for this fragment
 mapindex = *(unsigned int *)newfrag.duplicate;	// Indexed on duplicate
 mapindex&= mask;
 fraglist = fragmap[endplacebell][mapindex].bulklist;
// Check that the list for this map index has been allocated
 if (fraglist==NULL)
 {
  fraglist = fragmap[endplacebell][mapindex].bulklist = new BulkList(4,sizeof(Fragment));
  if (fraglist==NULL)
  {
   printf("ERROR: failed to allocate fragment storage\n");
   return(-1);
  }
 }
// Check the list doesn't already include this fragment
 else
  for (i=0; i<fraglist->listsize(); i++)
  {
   oldfrag = (Fragment *)fraglist->getitem(i);
   if (newfrag.sameduplicate(oldfrag) && newfrag.sameprimary(oldfrag))
    return(0);
  }
// Add the new fragment to the list
 if (!fraglist->add(&newfrag))
 {
  printf("ERROR: failed to add fragment to library\n");
  return(-1);
 }
 return(1);
}

// FragmentLibrary::add() does not take into account associative chains of
// primary:duplicate pairs. This routine goes through the library, and normalises
// sets such as A:B, B:C so that the 'first' primary is always used - A:C, B:C
// Note that this is really only a user convenience, so we can see a common primary
// root for every duplicate
void FragmentLibrary::normalise()
{
 Fragment *frag,*dupfrag;
 int b,i,j;

 for (b=0; b<MAXNBELLS; b++)
  if (fragmap[b])
   for (i=0; i<mapsize; i++)
    if (fragmap[b][i].bulklist)
     for (j=0; j<fragmap[b][i].bulklist->listsize(); j++)
     {
      frag = (Fragment *)fragmap[b][i].bulklist->getitem(j);
      dupfrag = findduplicate(frag,b);
      if (dupfrag)			// We have a duplicate-duplicate match
       if (searchorder(frag->primary,dupfrag->primary,frag->length))
        dupfrag->replaceprimary(frag->primary);
       else
        frag->replaceprimary(dupfrag->primary);
      dupfrag = findprimary(frag,b);
      if (dupfrag)			// We have a primary-duplicate match
       frag->replaceprimary(dupfrag->primary);
     }
}

// Converts library into compressed format, deletes BulkLists
int FragmentLibrary::compress()
{
 CompressedFrag *compressedlist;
 int *compressedalloc;
 Fragment *frag;
 int noccupied = 0;
 int largest = 0;
 int total = 0;
 int nremoved = 0;
 int ndistinct,listsize;
 int b,i,j,k,l;

 for (b=0; b<MAXNBELLS; b++)
  if (fragmap[b])
   for (i=0; i<mapsize; i++)
    if (fragmap[b][i].bulklist)
    {
     ndistinct = 0;
     listsize = fragmap[b][i].bulklist->listsize();
     for (j=0; j<listsize; j++)
     {
      frag = (Fragment *)fragmap[b][i].bulklist->getitem(j);
      for (k=0; k<listsize; k++)
       if (k!=j)
        if (((Fragment *)fragmap[b][i].bulklist->getitem(k))->isdup(frag->duplicate))
        {
         frag->length = 0;
         nremoved++;
         ndistinct--;
         break;
        }
      ndistinct++;
     }
     compressedalloc = new int[1+ndistinct*NPATTS*2];
     if (compressedalloc==NULL)
     {
      printf("ERROR: failed to alloc compressed frag library\n");
      return(FALSE);
     }
     *compressedalloc = ndistinct;
     compressedlist = (CompressedFrag *)(compressedalloc+1);
     noccupied++;
     total+= listsize;
     if (listsize>largest)
      largest = listsize;
     for (j=0; j<listsize; j++)
     {
      frag = (Fragment *)fragmap[b][i].bulklist->getitem(j);
      if (frag->length)
      {
       l = frag->length*2;
       for (k=0; k<NPATTS; k++)
       {
        compressedlist->duplicate[k] = frag->duplicate[k];
        if (l>=32)
         compressedlist->mask[k] = 0xFFFFFFFF;
        else
         compressedlist->mask[k] = (1<<l)-1;
        l-= 32;
       }
       compressedlist++;
      }
     }
     delete fragmap[b][i].bulklist;
     fragmap[b][i].fraglist = compressedalloc;
    }
 printf("Library compression: %d frags removed, %d/%d entries, max %d, av %.2f\n",
 	nremoved,noccupied,mapsize,largest,float(total)/noccupied);
 return(TRUE);
}

int FragmentLibrary::newfile(char *filename)
{
 if (filename==NULL)
 {
  printf("ERROR: no library filename specified\n");
  return(FALSE);
 }
 f.newfile(filename);
 if (!f.sameexttype(LIBEXT))
 {
  printf("ERROR: bad library filename extension\n");
  return(FALSE);
 }
 return(TRUE);
}


int FragmentLibrary::writefragment(Composer *ring,Fragment *frag,int endplacebell)
{
 char *buf;

 f.setmode("a");
 if (!f.open())
 {
  printf("ERROR: Failed to open library file %s\n",f.getname());
  return(FALSE);
 }
// Write out length, starting and ending place bell
 buf = f.buffer;
 buf+= sprintf(buf,"%d %c %c\t",frag->length,rounds[frag->startplacebell],rounds[endplacebell]);
// Write duplicate pattern
 buf = ring->writepattern(buf,frag->duplicate,frag->length,frag->startplacebell);
 if (buf==NULL)
  return(FALSE);
//  continue;
 buf = stpcpy(buf,"\t");
// Write primary pattern
 buf = ring->writepattern(buf,frag->primary,frag->length,frag->startplacebell);
 if (buf==NULL)
  return(FALSE);
//  continue;
 if (!f.writeline())
  return(FALSE);
 f.close();
 return(TRUE);
}

// Finds a node in which calling bell has position 'placebell'
Node *Composer::findstartingnode(int startplacebell)
{
 Node *node;
 int i,call;

 node = comp[0].node;
 i = 0;
 while (findcallingbell(node->nodex->nodehead)!=startplacebell)
 {
  for (call=0; call<=ncalltypes; call++)
   if (node->nextnode[call])
   {
    node = node->nextnode[call];
    break;
   }
  if (++i>=courselen)
  {
   printf("INTERNAL ERROR: could not find initial fragment calling position\n");
   return(NULL);
  }
 }
 return(node);
}

// Write out calls in pattern, high call first
char *Composer::writepattern(char *buf,Pattern *pattern,int length,int startplacebell)
{
 Node *node;
 int i,call;

// Find a starting node with the right calling position place-bell
 node = findstartingnode(startplacebell);
 if (node==NULL)
  return(NULL);
// Write out each call
 if (coursestructured)
  for (i=length-1; i>=0; i--)
  {
   call = (pattern[i/PATTLEN]>>(i%PATTLEN)*2)&3;
   buf = printcall(buf,call,node->nodex->callingbellpos[call]);
   if (buf==NULL)
    return(NULL);
   node = node->nextnode[call];
   if (node==NULL)
   {
    printf("INTERNAL ERROR: failed to convert fragment to output format\n");
    return(NULL);
   }
  }
 else
  for (i=length-1; i>=0; i--)
  {
   call = (pattern[i/PATTLEN]>>(i%PATTLEN)*2)&3;
   *buf++ = callchars[calltypes[call]];
   node = node->nextnode[call];
   if (node==NULL)
   {
    printf("INTERNAL ERROR: failed to convert fragment to output format\n");
    return(NULL);
   }
  }
 *buf = 0;		// Need to NULL-terminate after printcall()
 return(buf);
}

// Returns a Fragment pointer, if one can be found to match with this frag's duplicate
// Otherwise, returns NULL
Fragment *FragmentLibrary::findduplicate(Fragment *frag,int endplacebell)
{
 BulkList *fraglist;
 Fragment *testfrag;
 int i,mapindex;

 if (fragmap[endplacebell]==NULL)
  return(NULL);
 mapindex = *(unsigned int *)frag->duplicate;
 mapindex&= mask;
 fraglist = fragmap[endplacebell][mapindex].bulklist;
 if (fraglist==NULL)
  return(NULL);
 for (i=0; i<fraglist->listsize(); i++)
 {
  testfrag = (Fragment *)fraglist->getitem(i);
  if (frag->sameduplicate(testfrag))
   return(testfrag);
 }
 return(NULL);
}

// Returns a Fragment pointer, if one can be found to match with this fragment's
// PRIMARY pattern. Otherwise, returns NULL
Fragment *FragmentLibrary::findprimary(Fragment *frag,int endplacebell)
{
 Fragment swapfrag;

 swapfrag.length = frag->length;
 swapfrag.replaceduplicate(frag->primary);
 return findduplicate(&swapfrag,endplacebell);
}

// Returns TRUE if patt1 would occur earlier in the search than patt2
int FragmentLibrary::searchorder(Pattern *patt1,Pattern *patt2,int length)
{
 int i;

 for (i=(length+PATTLEN-1)/PATTLEN-1; i>=0; i--)
  if (patt1[i]<patt2[i])
   return(TRUE);
  else if (patt1[i]>patt2[i])
   return(FALSE);
// If we get here, they are actually the same!
 return(TRUE);
}

