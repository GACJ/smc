// clang-format off

#ifdef __ASM_IMPL__

#include "smc.h"

// Both the following optimisations appear to make cycle performance in the inner
// loop on the Cyrix chip WORSE. However, overall performance is still improved!!
// MMXCOUNTER saves 2 cycles/loop on a Pentium
// !! Now moved to individual composing loops
//#define MMXCOUNTER
// EXPANDFALSELOOP save (?) cycles on a Pentium
#define EXPANDFALSELOOP

// Various cycle timings possible in the composing code
// Can only have one going at a time!
#define TIMECOMPOSE 1
#define TIMEBACKTRACK 2
#define TIMEFALSE 3

// Overhead for timer is 14 cycles (Pentium MMX) 13 cycles (Cyrix M2)
//#define CYCLETIMER TIMEFALSE

#define STARTTIMEBT
#define STOPTIMEBT
#define STARTTIMEC
#define STOPTIMEC
#define STARTTIMEF
#define STOPTIMEF

#ifdef CYCLETIMER
// clang-format off
#define CYCLESTART  asm rdtsc; \
        __asm   mov [cyclelo],eax
#define CYCLESTOP   asm     rdtsc; \
        __asm   sub eax,[cyclelo]; \
        __asm   cmp eax,[ncycles]; \
        __asm   ja  cont; \
        __asm   mov [ncycles],eax; \
        __asm   mov [cyclenfalse],edx \
        __asm cont:
#define CYCLESTOPF  asm     rdtsc; \
        __asm   sub eax,[cyclelo]; \
        __asm   cmp ecx,6 \
        __asm   jb  cont \
        __asm   cmp eax,[ncycles]; \
        __asm   ja  cont; \
        __asm   mov [ncycles],eax; \
        __asm   mov [cyclenfalse],ecx \
        __asm cont:
// clang-format on
#if CYCLETIMER == TIMECOMPOSE
#define STARTTIMEC CYCLESTART
#define STOPTIMEC CYCLESTOP
#elif CYCLETIMER == TIMEBACKTRACK
#define STARTTIMEBT CYCLESTART
#define STOPTIMEBT CYCLESTOP
#elif CYCLETIMER == TIMEFALSE
// clang-format off
#define STARTTIMEF __asm mov ecx,0; CYCLESTART; asm mov edx,0
// clang-format on
#define STOPTIMEF CYCLESTOPF
#endif
int cyclelo, ncycles;
int cyclenfalse;
#endif

//#define UNVISITABLE
#ifdef UNVISITABLE
#undef EXPANDFALSELOOP
#endif

#ifdef ONECOMP

#undef REGENERATION
#define PALINDROMIC
#undef CLEANPROOF
#undef FALSEBITS
#define MMXCOUNTER
#undef MULTIPART
#undef MMXFRAGOPT
#undef CALLCOUNT

void Composer::compose_palindromic_MMX()
#include "composeloop.inc"

#else // ONECOMP

#define REGENERATION
#undef PALINDROMIC
#define CLEANPROOF
#undef FALSEBITS
#define MMXCOUNTER
#define MULTIPART
#define MMXFRAGOPT
#undef CALLCOUNT

void Composer::compose_regen_MMXfrag_cps_multipart()
#include "composeloop.inc"

#undef MMXFRAGOPT
/*
void Composer::compose_regen_MMX_cps_multipart()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_MMX_cps_multipart_callcount()
#include "composeloop.inc"
#undef CALLCOUNT
*/

#undef MULTIPART
#define MMXFRAGOPT
void Composer::compose_regen_MMXfrag_cps()
#include "composeloop.inc"

#undef MMXFRAGOPT
/*
void Composer::compose_regen_MMX_cps()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_MMX_cps_callcount()
#include "composeloop.inc"
#undef CALLCOUNT
*/

#undef MMXCOUNTER
#define MULTIPART
void Composer::compose_regen_cps_multipart()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_cps_multipart_callcount()
#include "composeloop.inc"
#undef CALLCOUNT

#undef MULTIPART
void Composer::compose_regen_cps()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_cps_callcount()
#include "composeloop.inc"
#undef CALLCOUNT

#undef REGENERATION
#define MMXCOUNTER
/*
#define PALINDROMIC
void Composer::compose_palindromic_MMX_cps()
#include "composeloop.inc"

#undef PALINDROMIC
*/
#define MMXFRAGOPT
void Composer::compose_MMXfrag_cps()
#include "composeloop.inc"

#undef MMXFRAGOPT
/*
void Composer::compose_MMX_cps()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_MMX_cps_callcount()
#include "composeloop.inc"
#undef CALLCOUNT
*/

#undef MMXCOUNTER
#define PALINDROMIC
void Composer::compose_palindromic_cps()
#include "composeloop.inc"

#undef PALINDROMIC
void Composer::compose_cps()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_cps_callcount()
#include "composeloop.inc"
#undef CALLCOUNT

#undef CLEANPROOF
#define FALSEBITS

#define REGENERATION
#define MMXCOUNTER
#define MMXFRAGOPT
#define MULTIPART
void Composer::compose_regen_MMXfrag_falsebits_multipart()
#include "composeloop.inc"

#undef MULTIPART
void Composer::compose_regen_MMXfrag_falsebits()
#include "composeloop.inc"
#undef MMXFRAGOPT
#undef MMXCOUNTER

#define MULTIPART
void Composer::compose_regen_falsebits_multipart()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_falsebits_multipart_callcount()
#include "composeloop.inc"
#undef CALLCOUNT

#undef MULTIPART
void Composer::compose_regen_falsebits()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_falsebits_callcount()
#include "composeloop.inc"
#undef CALLCOUNT

#undef REGENERATION

#define MMXCOUNTER
#define MMXFRAGOPT
void Composer::compose_MMXfrag_falsebits()
#include "composeloop.inc"
#undef MMXFRAGOPT
#undef MMXCOUNTER

#define PALINDROMIC
void Composer::compose_palindromic_falsebits()
#include "composeloop.inc"

#undef PALINDROMIC
void Composer::compose_falsebits()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_falsebits_callcount()
#include "composeloop.inc"
#undef CALLCOUNT

#undef FALSEBITS

#define REGENERATION
#define MMXCOUNTER
#define MMXFRAGOPT
#define MULTIPART
void Composer::compose_regen_MMXfrag_multipart()
#include "composeloop.inc"

#undef MMXFRAGOPT
/*
void Composer::compose_regen_MMX_multipart()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_MMX_multipart_callcount()
#include "composeloop.inc"
#undef CALLCOUNT
*/

#undef MULTIPART
#define MMXFRAGOPT
void Composer::compose_regen_MMXfrag()
#include "composeloop.inc"

#undef MMXFRAGOPT
/*
void Composer::compose_regen_MMX()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_MMX_callcount()
#include "composeloop.inc"
#undef CALLCOUNT
*/

#undef MMXCOUNTER
#define MULTIPART
void Composer::compose_regen_multipart()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_multipart_callcount()
#include "composeloop.inc"
#undef CALLCOUNT

#undef MULTIPART
void Composer::compose_regen()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_regen_callcount()
#include "composeloop.inc"
#undef CALLCOUNT

#undef REGENERATION
#define MMXCOUNTER
#define PALINDROMIC
/*
void Composer::compose_palindromic_MMX()
#include "composeloop.inc"
*/

#undef PALINDROMIC
#define MMXFRAGOPT
void Composer::compose_MMXfrag()
#include "composeloop.inc"

#undef MMXFRAGOPT
/*
void Composer::compose_MMX()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_MMX_callcount()
#include "composeloop.inc"
#undef CALLCOUNT
*/

#undef MMXCOUNTER
#define PALINDROMIC
void Composer::compose_palindromic()
#include "composeloop.inc"

#undef PALINDROMIC
void Composer::compose_()
#include "composeloop.inc"

#define CALLCOUNT
void Composer::compose_callcount()
#include "composeloop.inc"
#undef CALLCOUNT

#endif // ONECOMP

#endif // __ASM_IMPL__
