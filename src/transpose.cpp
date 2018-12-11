// SMC - Single Method Composer
// Copyright Graham A C John 1991-98 All rights reserved
// SMC32 32-bit conversion
// Copyright Mark B Davies 1998

#include "smc.h"

//  TITLE     TRANSP - Row Transposition
//
//        (ah=0)          (ah=1)            (ah=2)
//       Normal          Unknown      Inverse
//   Transposition        Transposer    Transposition
//
// A      142635          142635         [142635]    A
// B  x   164523      x  [164523]      x  164523     B
//        ------          ------          ------
// C  =  [156342]     =   156342       =  156342     C

void Ring::transpose(const char* source, const char* transposer, char* dest)
{
    for (int i = 0; i < nbells; i++)
    {
        *dest++ = source[*transposer++];
    }
}

void Ring::unknowntrans(const char* source, char* transposer, char* dest)
{
    for (int i = 0; i < nbells; i++)
    {
        auto b = *dest++;
        auto j = -1;
        while (source[++j] != b)
        {
        }
        *transposer++ = j;
    }
}

void Ring::inversetrans(char* source, const char* transposer, const char* dest)
{
    for (int i = 0; i < nbells; i++)
    {
        source[*transposer++] = *dest++;
    }
}

int factorial[MAXNBELLS];

void Composer::calcfactorials()
{
    int i;

    factorial[0] = factorial[1] = 0;
    factorial[2] = 1;
    for (i = 3; i < nbells; i++)
        factorial[i] = factorial[i - 1] * (i - 1);
}

//    This routine calculates a Lead Head Offset (LHO) into a proving array
//    for the supplied row. The LHO is a word indicating the segment address
//    of the array element (ie real address / 16)
//
//    es:edi        Source Row Address
//    eax       LHO Result
//    8              Number of bells
//
//    Rows are expected to be in internal format (binary 1 to 8)
//    Validated data only!
//
//    The algorithm is designed to arrange leads in order of tenor position,
//    followed by the seventh's position etc
//    ie 12345678 = 0, 18765432 = 5039, adjusted to a segment offset
//
//    eg:  Row       -   1    2    3    4    5    6    7    8
//         Positions -   1    3    5    2    7    4    8    6
//         Inverted  -   1    5    3    6    1    4    0    2
//         Reduced   -        0    0    2    0    2    0    2
//         * Factor  -        0    1    2    6   24  120  720
//         Gives     -        0    0    4    0   48    0 1440 = 1492

// !! row[0] must be treble (0)
int Composer::calcLHfactnum(char* row)
{
    char trans[MAXNBELLS];
    int LHnum = 0;
    int multiplier;
    int i, j;

    unknowntrans(row, trans, binrounds); // Transpose row into bell positions
    for (i = 1; i < nbells; i++)
        trans[i] = nbells - trans[i] - 1; // Invert positions
    for (i = nbells - 1; i > 1; i--)
    {
        multiplier = trans[i];
        for (j = nbells - 1; j > i; j--)
            if (trans[i] > trans[j]) // in higher position in row
                if (--multiplier == 0)
                    break; // don't continue with multiplier if zero
        LHnum += factorial[i] * multiplier;
    }
    return (LHnum);
}
