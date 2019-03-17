#ifndef __ASM_IMPL__

#include "composeloop.hpp"
#include "smc.h"

//#define UNVISITABLE
#ifdef UNVISITABLE
#undef EXPANDFALSELOOP
#endif

#ifdef ONECOMP

void Composer::compose_palindromic_MMX()
{
    composeloop<TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE>::run(*this);
}

#else // ONECOMP

void Composer::compose_regen_MMXfrag_cps_multipart()
{
    composeloop<FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE>::run(*this);
}

void Composer::compose_regen_MMXfrag_cps()
{
    composeloop<FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_regen_cps_multipart()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE>::run(*this);
}

void Composer::compose_regen_cps_multipart_callcount()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE>::run(*this);
}

void Composer::compose_regen_cps()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_regen_cps_callcount()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE>::run(*this);
}

void Composer::compose_MMXfrag_cps()
{
    composeloop<FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_palindromic_cps()
{
    composeloop<TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_cps()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_cps_callcount()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE>::run(*this);
}

void Composer::compose_regen_MMXfrag_falsebits_multipart()
{
    composeloop<FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, TRUE>::run(*this);
}

void Composer::compose_regen_MMXfrag_falsebits()
{
    composeloop<FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE>::run(*this);
}

void Composer::compose_regen_falsebits_multipart()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE>::run(*this);
}

void Composer::compose_regen_falsebits_multipart_callcount()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, TRUE>::run(*this);
}

void Composer::compose_regen_falsebits()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE>::run(*this);
}

void Composer::compose_regen_falsebits_callcount()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE>::run(*this);
}

void Composer::compose_MMXfrag_falsebits()
{
    composeloop<FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE>::run(*this);
}

void Composer::compose_palindromic_falsebits()
{
    composeloop<TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE>::run(*this);
}

void Composer::compose_falsebits()
{
    composeloop<FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE>::run(*this);
}

void Composer::compose_falsebits_callcount()
{
    composeloop<FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE>::run(*this);
}

void Composer::compose_regen_MMXfrag_multipart()
{
    composeloop<FALSE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE>::run(*this);
}

void Composer::compose_regen_MMXfrag()
{
    composeloop<FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_regen_multipart()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, FALSE>::run(*this);
}

void Composer::compose_regen_multipart_callcount()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE>::run(*this);
}

void Composer::compose_regen()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_regen_callcount()
{
    composeloop<FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE>::run(*this);
}

void Composer::compose_MMXfrag()
{
    composeloop<FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_MMX_callcount()
{
    composeloop<FALSE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE>::run(*this);
}

void Composer::compose_palindromic()
{
    composeloop<TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_()
{
    composeloop<FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE>::run(*this);
}

void Composer::compose_callcount()
{
    composeloop<FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE>::run(*this);
}

#endif

#endif
