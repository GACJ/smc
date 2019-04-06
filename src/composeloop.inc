// Include file used to create various versions of the composing loop
// Copyright Mark B Davies 1998-2000

{
// !! We cannot access local variables within the inner composing loop, because
//   register ebp is hijacked. These variables are loaded into registers beforehand
 Composition *compptr;
 int i;

#ifdef CYCLETIMER
 ncycles = 10000;
#endif

#ifdef PALINDROMIC
 if (ncompnodes&1)
 {
  midnodeapex = TRUE;
  ncompnodes/= 2;
// Subtract half node length
  complength-= comp[ncompnodes].node->nrows/2;
 }
 else
 {
  midnodeapex = FALSE;
  ncompnodes = (ncompnodes+1)/2;
 }
#endif
 lengthcountdown = maxpalilength-complength;
 compptr = &comp[ncompnodes];
 if (ncompnodes)
 {
  // Need to count up calls already used in the restarted composition
  if (countingcalls)
  {
   for (i=0; i<ncompnodes; i++)
   {
    ncallsleft[comp[i].call]--;
    ncallsleftperpos[comp[i+1].node->callcountposindex/NDIFFCALLS][comp[i].call]--;
   }
  }
  goto reenter;
 }
 __asm
 {
    mov edi,[compptr]
#ifdef MMXCOUNTER
    mov eax,1
    movd    mm1,eax
    pxor    mm0,mm0
#endif
    push    ebp
    push    ebx
    mov ebx,[this]
    mov ecx,[edi]Composition.call
    mov ebp,[ebx]Composer.lengthcountdown
#ifdef REGENERATION
    mov [ebx]Composer.lastregen,edi
#endif
    mov esi,[edi]Composition.node
#ifdef MMXFRAGOPT
// The call pattern mm2 is initially set to 0.
// Entries beyond the start of the composition will all be Plain
// It will not match with any duplicate, because all duplicates start with a call
    pxor    mm2,mm2
#ifdef STOREPATT
    movq    [edi]Composition.pattern,mm2
#endif
#endif
    jmp composeloop
 }
 while(TRUE)
 {
  __asm
  {
//  eax = transient
//  ebx = this
//  ecx = call / falsenode index
//  edx = transient
//  esi = current node
//  edi = compptr
//  ebp = maxpalilength - current complength
// For MMX routines:
//  mm0 = nodecount
//  mm1 = 1
//  mm2 = current call pattern
//  mm6 = transient
//  mm7 = transient
reenter:    mov edi,[compptr]
#ifdef MMXCOUNTER
    mov eax,1
    movd    mm1,eax
#endif
    push    ebp
    push    ebx
    mov ebx,[this]
    mov ebp,[ebx]Composer.lengthcountdown
#ifdef MMXCOUNTER
    movd    mm0,[ebx]Composer.stats.nodecount
#endif
    mov esi,[edi]Composition.node
#ifdef MMXFRAGOPT
#ifdef STOREPATT
    movq    mm2,[edi]Composition.pattern
#endif
#endif
backtrackfromrounds:
// Check if more than SHOWSTATSFREQ million nodes since last showstats()
#ifdef MMXCOUNTER
    movd    edx,mm0
#else
    mov edx,[ebx]Composer.stats.nodecount
#endif
    cmp edx,SHOWSTATSFREQ*1000000
    jb  backtrack
// Drop out to showstats()
    mov eax,[ebx]Composer.maxpalilength
    sub eax,ebp
    mov [ebx]Composer.complength,eax
    mov [ebx]Composer.lengthcountdown,ebp
    mov [ebx]Composer.stats.nodecount,edx
    add dword ptr [ebx]Composer.stats.nodesgenerated,edx
    adc dword ptr [ebx+4]Composer.stats.nodesgenerated,0
    pop ebx
    pop ebp
    mov [compptr],edi
#ifdef MMXCOUNTER
    emms
#endif
  }
  ncompnodes = compptr-comp;
  showstats();
  __asm
  {
    jmp reenter
asmdone:
#ifdef MMXCOUNTER
    movd    edx,mm0
#else
    mov edx,[ebx]Composer.stats.nodecount
#endif
    add dword ptr [ebx]Composer.stats.nodesgenerated,edx
    adc dword ptr [ebx+4]Composer.stats.nodesgenerated,0
    pop ebx
    pop ebp
#ifdef MMXCOUNTER
    emms
#endif
  }
    showstats();
  __asm
  {
    jmp alldone

lbl_loadcallnextcall:
    mov ecx,[edi]Composition.call
#ifdef CALLCOUNT
    mov edx,ecx
    inc [ebx+ecx*4]Composer.ncallsleft
    add dl,[esi]Node.callcountposindex
    inc [ebx+edx*4]Composer.ncallsleftperpos
#endif // CALLCOUNT
lbl_sublennextcall:
    add ebp,[esi]Node.nrows
lbl_nextcall:
    mov esi,[edi]Composition.node
#ifdef MULTIPART
    cmp ebp,[ebx]Composer.maxpartlength
    jb  backtrack
#endif
#ifdef REVERSECALLS
    dec ecx
    jns regen
#else
    inc ecx
    cmp ecx,[ebx]Composer.ncalltypes
    jbe regen
#endif
backtrack:
    STARTTIMEBT
#ifdef FALSEBITS
    mov edx,[esi]Node.falsebits
    mov eax,[esi+4]Node.falsebits
    xor [edx],eax
#else
#ifdef UNVISITABLE
    mov [esi]Node.unvisitable,0 // Reset unvisitable flag
// Decremement unvisitability of false nodes
    xor edx,edx
    or  edx,[esi]Node.nfalsenodes
    je  reducelength
    xor ecx,ecx
falsenodebacktrack:
    cmp ecx,edx
    je  reducelength
    mov eax,[esi+ecx*4]Node.falsenodes
    inc ecx
    dec [eax]Node.unvisitable
    jmp falsenodebacktrack
reducelength:
#else
    mov [esi]Node.included,0    // Reset included flag
#endif
#endif
    sub edi,size Composition    // All compositions found?
#ifdef PALINDROMIC
    mov eax,[edi]Composition.palinode
#endif
    cmp edi,[ebx]Composer.comp
    jb  asmdone
#ifdef PALINDROMIC
#ifdef FALSEBITS
    mov edx,[eax]Node.falsebits
    mov eax,[eax+4]Node.falsebits
    xor [edx],eax
#else
    mov [eax]Node.included,0    // Reset palindromic included flag
#endif
#endif
    mov ecx,[edi]Composition.call
    add ebp,[esi]Node.nrows     // Reduce composition length
#ifdef CALLCOUNT
    mov edx,ecx
    inc [ebx+ecx*4]Composer.ncallsleft
    add dl,[esi]Node.callcountposindex
    inc [ebx+edx*4]Composer.ncallsleftperpos
#endif // CALLCOUNT
    mov esi,[edi]Composition.node   // Load previous lead
#ifdef MULTIPART
    cmp ebp,[ebx]Composer.maxpartlength
    jb  backtrack
#endif // MULTIPART
lbl_nextcall2:
#ifdef REVERSECALLS
    dec ecx
    js  backtrack
#else
    inc ecx
    cmp ecx,[ebx]Composer.ncalltypes
    ja  backtrack
#endif

#ifdef MULTIPART
#ifdef CALLCOUNT
// Before re-entering the composing loop, check maximum number of calls per part not exceeded.
// Note that this test requires the next-node pointer, so we MUST test for allowed call first!
// (Normally done in composing loop)
    cmp [esi+ecx*4]Node.nextnode,0
    je  lbl_nextcall2

    mov eax,[ebx+ecx*4]Composer.ncallsleft
    cmp eax,[ebx+ecx*4]Composer.npartcalls
    jl  lbl_nextcall2
    mov eax,[esi+ecx*4]Node.nextnode
    mov edx,ecx
    add dl,[eax]Node.callcountposindex
    mov eax,[ebx+edx*4]Composer.ncallsleftperpos
    cmp eax,[ebx+edx*4]Composer.npartcallsperpos
    jl  lbl_nextcall2
#endif // CALLCOUNT
#endif // MULTIPART

    STOPTIMEBT
#ifdef CHECKOFTEN
// Check if more than SHOWSTATSFREQ million nodes since last showstats()
#ifdef MMXCOUNTER
    movd    edx,mm0
#else
    mov edx,[ebx]Composer.stats.nodecount
#endif
    cmp edx,SHOWSTATSFREQ*1000000
    jae droptostats
#endif
regen:
#if 0
// Test for allowed call before doing regen save
// Doesn't appear to make much difference
    cmp [esi+ecx*4]Node.nextnode,0
    je  lbl_nextcall2
#endif
#ifdef REGENERATION
// The regen pointer is set to point back to the start of the composition
// i.e. edi - this->comp
// This lead's regenoffset is also added. For the case where course-end
// synchronisation is necessary (not all calling positions allowed) the regenoffset
// for this lead points sufficiently far into the plain lead buffer (beyond the
// START of the composition) to copy in plains up to the course end.
    mov eax,[ebx]Composer.comp
    sub eax,edi
    mov [ebx]Composer.lastregen,edi
    add eax,[esi+ecx*4]Node.regenoffset;
    mov [ebx]Composer.regenptr,eax
#endif
#ifdef MMXFRAGOPT
#ifdef STOREPATT
    movq    mm2,[edi]Composition.pattern
#endif
#endif
composeloop:
    STARTTIMEC
    mov esi,[esi+ecx*4]Node.nextnode
    mov [edi]Composition.call,ecx   // Add call to composition
    cmp esi,0           // This call not possible here?
    je  lbl_nextcall
#ifdef MMXFRAGOPT
    movd    mm7,ecx
#endif
#ifdef MMXCOUNTER
    paddd   mm0,mm1
#else
    inc [ebx]Composer.stats.nodecount
#endif
#ifndef FALSEBITS
#ifdef UNVISITABLE
    cmp [esi]Node.unvisitable,0 // Node unvisitable?
#else
    cmp [esi]Node.included,0    // Already had that node?
#endif
    jne lbl_nextcall
#endif
    sub ebp,[esi]Node.nrows
    js  lbl_sublennextcall      // Composition too long?
#ifdef CALLCOUNT
    mov eax,[ebx+ecx*4]Composer.ncallsleft
    dec eax
    js  lbl_sublennextcall
    mov edx,ecx
    add dl,[esi]Node.callcountposindex
    mov [ebx+ecx*4]Composer.ncallsleft,eax
    mov eax,[ebx+edx*4]Composer.ncallsleftperpos
    dec eax
    jns storecallcount
    inc [ebx+ecx*4]Composer.ncallsleft
    jmp lbl_sublennextcall
storecallcount:
    mov [ebx+edx*4]Composer.ncallsleftperpos,eax
#endif // CALLCOUNT
#ifndef CLEANPROOF
    STARTTIMEF
//#define FALSEBITS
#ifdef FALSEBITS
    mov ecx,[esi]Node.nfalsenodes
//  mov ecx,1
    mov edx,[esi+ecx*8]Node.falsebits
    mov eax,[esi+ecx*8+4]Node.falsebits
    and eax,[edx]
    jne lbl_loadcallnextcall
    dec ecx
//  cmp ecx,[esi]Node.nfalsenodes
    je  leadok
falsenodeloop:              // Check false nodes for next lead
//  inc ecx
    mov edx,[esi+ecx*8]Node.falsebits
    mov eax,[esi+ecx*8+4]Node.falsebits
    and eax,[edx]
    jne lbl_loadcallnextcall
    dec ecx
//  cmp ecx,[esi]Node.nfalsenodes
    jne falsenodeloop
#else // FALSEBITS
    xor edx,edx
    or  edx,[esi]Node.nfalsenodes
    je  leadok
#ifdef EXPANDFALSELOOP
    mov eax,[esi]Node.falsenodes
    mov ecx,1
    cmp [eax]Node.included,0
    jne lbl_loadcallnextcall
#else
    xor ecx,ecx
#endif
falsenodeloop:              // Check false nodes for next lead
    cmp ecx,edx
    je  leadok
    mov eax,[esi+ecx*4]Node.falsenodes
    inc ecx
#ifdef UNVISITABLE
    inc [eax]Node.unvisitable
    jmp falsenodeloop
#else
    cmp [eax]Node.included,0
    je  falsenodeloop
    jmp lbl_loadcallnextcall
#endif
#endif // FALSEBITS
#endif // CLEANPROOF
leadok: STOPTIMEF
#ifdef MMXFRAGOPT
    psllq   mm2,2
    por mm2,mm7
    mov edx,[esi]Node.fragmap
    movd    eax,mm2
    cmp edx,0
    je  notfrag
    and eax,[ebx]Composer.fraglib.mask
    mov edx,[edx+eax*4]
    cmp edx,0
    je  notfrag
// Check call pattern against fragment list
    mov ecx,[edx]
    add edx,4 // size int
pattloop:   dec ecx
    js  notfrag
    movq    mm6,mm2
    movq    mm7,[edx]
    pand    mm6,[edx+8]
    pcmpeqd mm6,mm7
    add edx,size CompressedFrag
    packssdw    mm6,mm6
    movd    eax,mm6
    cmp eax,0
    je  pattloop
    inc [ebx]Composer.stats.prunecount
    jmp lbl_loadcallnextcall
notfrag:
#endif
#ifdef PALINDROMIC
    mov eax,[edi]Composition.palinode
    mov ecx,[edi]Composition.call
    cmp esi,eax
    je  reachednodeheadapex
    mov eax,[eax+ecx*4]Node.prevnode
    cmp eax,0
    je  lbl_sublennextcall
    mov [edi+size Composition]Composition.palinode,eax
    cmp esi,eax
    je  reachedmidnodeapex
#endif
// Sub len was here
#ifdef PALINDROMIC
    mov eax,[edi]Composition.palinode
#ifdef FALSEBITS
    mov edx,[eax]Node.falsebits
    mov eax,[eax+4]Node.falsebits
    or  [edx],eax
#else
    mov [eax]Node.included,1    // Mark palindromic node included
#endif
#endif
    add edi,size Composition    // Increment comp ptr
#ifdef FALSEBITS
    mov edx,[esi]Node.falsebits
    mov eax,[esi+4]Node.falsebits
    or  [edx],eax
#else
#ifdef UNVISITABLE
    mov [esi]Node.unvisitable,1 // Mark node included, unvisitable
#else
    mov [esi]Node.included,1    // Mark node included
#endif
#endif
#ifdef REGENERATION
    mov eax,[ebx]Composer.regenptr
#endif
    mov [edi]Composition.node,esi   // Add lead to composition
#ifdef MMXFRAGOPT
#ifdef STOREPATT
    movq    [edi]Composition.pattern,mm2
#endif
#endif
#ifdef REGENERATION
    mov ecx,[edi+eax]
#else
#ifdef REVERSECALLS
    mov ecx,[ebx]Composer.ncalltypes
#else
    xor ecx,ecx
#endif
#endif
    STOPTIMEC
    cmp [esi]Node.comesround,0  // Has come round?
    je  composeloop
#ifdef PALINDROMIC
// Backtrack on normal rounds - unlinked palindromic 2-part of double length!
    jmp backtrackfromrounds
reachedmidnodeapex:
// Need to add on half the palindromic node's length
    mov eax,[esi]Node.nrows
    mov [ebx]Composer.midnodeapex,1
    sar eax,1
    add eax,[ebx]Composer.maxpalilength
    jmp inccomp
reachednodeheadapex:
    mov [ebx]Composer.midnodeapex,0
    mov eax,[ebx]Composer.maxpalilength
inccomp:    add edi,size Composition    // Increment comp ptr
#ifdef FALSEBITS
    mov edx,[esi]Node.falsebits
    mov eax,[esi+4]Node.falsebits
    or  [edx],eax
#else
    mov [esi]Node.included,1    // Mark node included
#endif // PALINDROMIC
    mov [edi]Composition.node,esi   // Add lead to composition
#else
    mov eax,[ebx]Composer.maxpalilength
#endif
    sub eax,ebp
#ifdef PALINDROMIC
    sub ebp,[esi]Node.nrows
    add eax,eax
#endif
    cmp eax,[ebx]Composer.minlengthnow
    jb  backtrackfromrounds
    mov [ebx]Composer.complength,eax
    mov [ebx]Composer.lengthcountdown,ebp
#ifdef MMXCOUNTER
    movd    [ebx]Composer.stats.nodecount,mm0
#endif
    pop ebx
    pop ebp
    mov [compptr],edi
#ifdef MMXCOUNTER
    emms
#endif
  }
  ncompnodes = compptr-comp;
#ifdef REGENERATION
// Check composition hasn't got 'tail end regeneration'. This occurs when a small
// amount (< part size and >=course length) of the initial call sequence is
// regenerated at the end of the composition. If this final call sequence is
// rotated to the start, it matches an earlier duplicate composition. E.g.
//  H W W WH* H
// can be produced when a backtrack at * causes the initial H course to be
// regenerated, producing rounds. This composition is identical to the already-
// produced
//  H H W W WH
// so can safely be rejected.
// We can detect this condition when the last backtrack position DOESN'T split
// the composition up into an integral number of parts. So, find # parts first.
  lastregen++;
  if (coursestructured)
// Move on to next course end
   while (lastregen<compptr)
   {
    if (lastregen->node->nparts)    // Nparts==0 -> not a course end
      break;
    lastregen++;
   }
  nodesperpart = lastregen-comp;
// Course end reached - can drop through to non course-structured case
// Calculate number of parts
  nparts = ncompnodes/nodesperpart;
// Tail-end regeneration detected if the last backtrack doesn't produce integral parts
  if (nparts*nodesperpart!=ncompnodes)
   continue;
#endif
#ifdef PALINDROMIC
// Expand palindromic composition
  for (i=0; i<ncompnodes; i++)
   comp[ncompnodes-1+i+midnodeapex].call = comp[ncompnodes-1-i].call;
  for (i=0; i<ncompnodes-1+midnodeapex; i++)
  {
   comp[ncompnodes+i+1].node = comp[ncompnodes+i].node->nextnode[comp[ncompnodes+i].call];
   if (comp[ncompnodes+i+1].node==nullptr)
    break;
  }
  ncompnodes = ncompnodes*2-1+midnodeapex;
  if (i+ncompnodes<ncompnodes)
   continue;
#endif
  stats.ncompsfound++;
#if 1
  analysecomp();
#endif
 }
alldone:
 return;
}
