// SMC32
// Ring method-generation library
// Copyright Mark B Davies 1995-1998

#include <ctype.h>
#include <new>
#include <stdio.h>
#include <string.h>

#include "ring.h"

Method* mlibrary;

char rounds[] = "1234567890ETABCDFGHJ";
const char* numbers[] = { "Singles", "Minimus", "Doubles", "Minor", "Triples", "Major", "Caters", "Royal", "Cinques", "Maximus", "Sextuples", "Fourteen", "Septuples", "Sixteen", "Octuples", "Eighteen", "Nonuples", "Twenty" };
char callstrings[] = "-SXU";
char fourthsbob[] = "14";
char fourthssingle[] = "1234";
char sixthsbob[] = "16";      // These get written to depending on
char sixthssingle[] = "1678"; // number of bells!

char* nextpn(char* pn);
int pnlen(char* pn);
char* buildclass(char* p, const char* classname, int abbreviated);

int Method::newmethod(char* newname, char* newpn)
{
    int i, ret;

    delete name;
    name = new (std::nothrow) char[strlen(newname) + 1];
    if (name == nullptr)
        return (FALSE);
    strcpy(name, newname);
    delete pn;
    pn = new (std::nothrow) char[strlen(newpn) + 1];
    if (pn != nullptr)
    {
        strcpy(pn, newpn);
        if (!parsepn())
            return (FALSE);
        analyse();
        // Set standard calls
        ret = TRUE;
        sixthsbob[1] = rounds[nbells - 3];
        sixthssingle[1] = rounds[nbells - 3];
        sixthssingle[2] = rounds[nbells - 2];
        sixthssingle[3] = rounds[nbells - 1];
        if (fourthsplacebobs())
            ret &= newcall(BOB, fourthsbob);
        else
            ret &= newcall(BOB, sixthsbob);
        for (i = 2; i < NDIFFCALLS; i++)
            if (fourthsplacebobs())
                ret &= newcall((Call)i, fourthssingle);
            else
                ret &= newcall((Call)i, sixthssingle);
        return (ret);
    }
    return (FALSE);
}

// Leadend calls only!
int Method::newcall(Call call, char* newpn)
{
    delete callpn[call];
    callpn[call] = new (std::nothrow) char[strlen(newpn) + 1];
    if (callpn[call] == nullptr)
    {
        printf("ERROR: failed to allocate call pn\n");
        return (FALSE);
    }
    strcpy(callpn[call], newpn);
    return (TRUE);
}

// Returns TRUE if testpn defines the same method as us
// NB doesn't check for expanded symmetric pns!
int Method::issamepn(char* testpn)
{
    char c, *p;

    p = pn;
    while (*p != 0)
    {
        // Skip through white space in each pn
        while (*p == ' ')
            p++;
        while (*testpn == ' ')
            testpn++;
        // See if pn character effectively the same
        c = *testpn++;
        if (c == 'x' || c == 'X' || c == '-')
            c = CHARCROSS;
        else if (c == 'l' || c == 'L')
            c = CHARLH;
        else
            c = toupper(c);
        if (c != *p++)
            return (FALSE);
    }
    return (TRUE);
}

int Method::parsepn()
{
    int i, j;
    char c, *p, *q;

    p = pn;
    c = *p;
    // Convert characters to upper case
    while (c != 0)
    {
        if (c == 'x' || c == 'X' || c == '-')
            c = CHARCROSS;
        else if (c == 'l' || c == 'L')
            c = CHARLH;
        else
            c = toupper(c);
        *p++ = c;
        c = *p;
    }
    // Calculate lead length and number of bells, and set pn pointers
    symmetrical = FALSE;
    p = pn;
    leadlen = nbells = 0;
    while ((c = *p++) != 0)
    {
        if (leadlen > MAXLEADLEN)
            return (FALSE);
        if (c == CHARCROSS)
            pnptrs[leadlen++] = p - 1;
        else if (c == CHARLH)
        {
            symmetrical = TRUE;
            leadlen = leadlen * 2 - 1;
        }
        else if (c != '.' && c != ' ')
        {
            q = strchr(rounds, c);
            if (q == 0)
                break;
            pnptrs[leadlen++] = p - 1;
            do
            {
                j = (int)(q - rounds);
                if (j >= nbells) // Find largest bell
                    nbells = j + 1;
                c = *p++;
                if (c == 0)
                    break;
                q = strchr(rounds, c);
            } while (q);
            p--;
        }
    }
    if (c != 0)
        return (FALSE);
    if (symmetrical)
        for (i = 0; i < leadlen / 2 - 1; i++)
            pnptrs[leadlen - i - 2] = pnptrs[i];

    if (nbells < MINNBELLS)
        nbells = MINNBELLS;
    else if (nbells > MAXNBELLS)
        nbells = MAXNBELLS;

    return (TRUE);
}

void Method::analyse()
{
    char* treblepath;
    Ring ring(this);

    treblepath = new (std::nothrow) char[leadlen + 1];
    leadheadcode = 0;
    ring.starttouch();
    while (ring.changen < leadlen)
    {
        if (treblepath != nullptr)
            treblepath[ring.changen] = (char)(strchr(ring.row, rounds[0]) - ring.row);
        ring.change();
    }
    if (treblepath != nullptr)
        treblepath[ring.changen] = (char)(strchr(ring.row, rounds[0]) - ring.row);
    strcpy(leadhead, ring.row);
    findleadheadcode();
    classify(treblepath);
    delete[] treblepath;
    // setcompletename(methodn,ring.completename,FALSE);
}

void Method::setcompletename(char* buf, int abbreviated)
{
    static char duble[] = "Double";
    static char slowcourse[] = "Slow Course";
    static char little[] = "Little";
    static char bob[] = "Bob";
    static char place[] = "Place";
    static const char* treblebob[] = { "Treble Bob", "Delight", "Surprise" };
    static const char* other[] = { "Treble Place", "Alliance", "Hybrid" };
    char* p;

    p = buf;
    if ((type & TYPEPLAINM) && (type & TYPEDOUBLE))
    {
        p = buildclass(p, duble, abbreviated);
        *p++ = ' ';
    }
    p = stpcpy(p, name);
    *p++ = ' ';
    if (type & TYPENHUNT)
    {
        if (type & TYPELITTLE)
            p = buildclass(p, little, abbreviated);
        if ((type & TYPENHUNT) == TYPESLOWCOURSE)
            p = buildclass(p, slowcourse, abbreviated);
        if (type & TYPEPLAINM)
        {
            if (type & TYPEBOB)
                p = buildclass(p, bob, abbreviated);
            else
                p = buildclass(p, place, abbreviated);
        }
        else if (type & TYPETREBLEBOBM)
        {
            if (type & TYPE)
                p = buildclass(p, treblebob[(type & TYPE) - 1], abbreviated);
        }
        else
        {
            if (type & TYPE)
                p = buildclass(p, other[(type & TYPE) - 1], abbreviated);
        }
    }
    if (*(p - 1) != ' ')
        *p++ = ' ';
    strcpy(p, numbers[nbells - MINNBELLS]);
}

char* buildclass(char* p, const char* classname, int abbreviated)
{
    if (abbreviated)
    {
        *p++ = *classname;
        classname = strchr(classname, ' ');
        if (classname)
            *p++ = classname[1];
        *p = 0;
        return (p);
    }
    else
        return stpcpy(p, classname);
}

void Method::showleadcode(char* buf)
{
    if (leadheadcode & PBLEADCODE)
    {
        *buf++ = (leadheadcode & 0x0F) + 'a';
        if ((leadheadcode & 0x0F) + 'a' == 'm')
            *buf++ = 'x';
        else if (leadheadcode & 0x70)
            *buf++ = (leadheadcode & 0x70) / 16 + '0';
    }
    else
        *buf++ = leadheadcode;
    *buf = 0;
}

// Returns TRUE if 4ths place bobs should be used
int Method::fourthsplacebobs()
{
    if (leadheadcode & PBLEADCODE)
    {
        if ((leadheadcode & 0x0F) + 'a' <= 'f' || (leadheadcode & 0x0F) + 'a' == 'm')
            return (TRUE);
        else
            return (FALSE);
    }
    return (TRUE);
}

void Method::findleadheadcode()
{
    char c, *p;
    int b, i;

    leadheadcode = 0;
    // Only give codes for even stages of treble-hunt methods
    if ((nbells & 1) || leadhead[0] != rounds[0])
        return;
    // Must have a Plain Bob leadhead
    i = 2;
    b = (int)(strchr(rounds, leadhead[1]) - rounds + 1);
    if (b == 2)
    {
        leadheadcode = '-'; // 2nd is a hunt bell
        return;
    }
    while (i != 3)
    {
        if (i & 1)
            i -= 2;
        else
        {
            i += 2;
            if (i == nbells + 2)
                i = nbells - 1;
        }
        if (b & 1)
        {
            b -= 2;
            if (b == 1)
                b = 2;
        }
        else
        {
            b += 2;
            if (b == nbells + 2)
                b = nbells - 1;
        }
        if (leadhead[i - 1] != rounds[b - 1])
        {
            leadheadcode = 'z';
            return;
        }
    }
    // Lead head change must be 12 or 1n
    p = pnptrs[leadlen - 1];
    c = p[1];
    if (c != '2' && c != rounds[nbells - 1])
    {
        leadheadcode = '-';
        return;
    }
    // By now we can assign code
    if (c == '2')
        c = 'a';
    else
        c = 'g';
    b = (int)(strchr(rounds, leadhead[1]) - rounds + 1);
    i = (b <= 8 ? b : 8);
    if (b & 1)
        c = c + (i - 3) / 2;
    else
        c = c - 'a' + 'f' - (i - 3) / 2;
    if (c >= 'i')
        c++;
    if (b > 8)
        leadheadcode = PBLEADCODE + ((b - 7) / 2) * 16 + c - 'a';
    else
        leadheadcode = PBLEADCODE + c - 'a';
}

void Method::classify(char* treblepath)
{
    char pn1[MAXNBELLS + 1], pn2[MAXNBELLS + 1];
    char hunts[MAXNBELLS];
    char trebleextent[MAXNBELLS];
    int ntrebleplaces = 0, halfleadplace = TRUE;
    int i, c = 0, alliance = 0, little = 0;

    type = TYPEUNKNOWN;
    // First count number of hunts
    for (i = 0; i < nbells; i++)
    {
        trebleextent[i] = 0;
        if (leadhead[i] == rounds[i])
        {
            c++;
            hunts[i] = TRUE;
        }
        else
            hunts[i] = FALSE;
    }
    if (c >= nbells / 2)
        type |= TYPEILLEGAL; // Too many hunts for CC requirements
    if (nbells - c != courselength() / leadlen)
        type |= TYPEILLEGAL; // CC requires #working bells = #leads
    if (c > 2)
        c = 2;
    type |= c * TYPESINGLEHUNT;

    // Should check place notation for illegal places longer than four changes

    // Check place notation to see if method is symmetrical or even double
    if ((leadlen & 1) == 0)
    {
        for (i = 0; i < leadlen / 2 - 1; i++)
        {
            copypn(pn1, pnptrs[i]);
            copypn(pn2, pnptrs[leadlen - i - 2]);
            if (strcmp(pn1, pn2) != 0)
                break;
        }
        if (i == leadlen / 2 - 1)
        {
            type |= TYPESYMMETRICAL;
            for (i = 0; i <= leadlen / 4; i++)
            {
                reversepn(pn1, pnptrs[leadlen / 2 - 1 + i]);
                copypn(pn2, pnptrs[leadlen - 1 - i]);
                if (strcmp(pn1, pn2) != 0)
                    break;
            }
            if (i > leadlen / 4)
                type |= TYPEDOUBLE;
        }
    }

    // Determine treble path
    // !!! Nothing extra is done about many-hunt methods - Slow Course methods
    //     need identifying
    if (!hunts[0])
        return;
    if (treblepath == 0)
    {
        type |= TYPEUNCLASSIFIED; // Not enough memory to classify
        return;
    }

    // If treble path is asymmetrical, must be a Hybrid method
    if (leadlen & 1)
    {
        type |= TYPEHYBRID;
        return;
    }
    for (i = 0; i < leadlen / 2; i++)
        if (treblepath[i] != treblepath[leadlen - 1 - i])
        {
            type |= TYPEHYBRID;
            return;
        }

    // Find how many blows the treble spends in each place - ie path extent
    trebleextent[0] = 1;
    for (i = 1; i < leadlen; i++)
    {
        trebleextent[treblepath[i]]++;
        if (i == leadlen / 2)
        {
            if (treblepath[i - 1] != treblepath[i])
                halfleadplace = FALSE;
        }
        else if (treblepath[i - 1] == treblepath[i])
            ntrebleplaces++;
    }

    // Check treble path extent to see whether method is Little or Alliance
    c = trebleextent[0];
    for (i = 1; i < nbells; i++)
        if (trebleextent[i] == 0)
            little++;
        else if (trebleextent[i] != c)
            alliance++;
    if (little)
        type |= TYPELITTLE;

    // If path is not alliance and there are no internal places,
    //  must be Plain or Treble Bob
    if (alliance == 0 && ntrebleplaces == 0 && halfleadplace)
    {
        if (leadlen == (nbells - little) * 2)
            classifyplain();
        else
            classifytreblebob((nbells - little) / 2);
        return;
    }

    // Otherwise, the method is Alliance or Treble Place
    if (alliance)
        type |= TYPEALLIANCE;
    else
        type |= TYPETREBLEPLACE;
}

void Method::classifyplain()
{
    // Classification not complete - Place methods not detected, Bob assumed
    type |= TYPEPLAINM;
    type |= TYPEBOB;
}

void Method::classifytreblebob(int ndodges)
// ndodges is the number of pairs of places dodged in, not how long
//  each dodge lasts!
{
    char c, *p, *p2;
    int i, j, sectionlen, internals = 0;

    type |= TYPETREBLEBOBM;

    // Count internal places across sections
    sectionlen = leadlen / (ndodges * 2);
    p = pn;
    for (i = 0; i < ndodges * 2 - 1; i++)
    {
        if (*p == CHARLH)
        {
            internals *= 2;
            break;
        }
        for (j = 0; j < sectionlen - 1; j++)
            p = nextpn(p);
        if (i != ndodges - 1) // Don't count half-lead
        {
            c = *(p2 = p);
            UNTILPNSEP
            {
                if (c != rounds[0] && c != rounds[nbells - 1])
                {
                    internals++;
                    break;
                }
                c = *++p2;
            }
        }
        p = nextpn(p);
    }
    if (internals == 0)
        type |= TYPETB;
    else if (internals == (ndodges - 1) * 2)
        type |= TYPESURPRISE;
    else
        type |= TYPEDELIGHT;
}

void Method::copypn(char* buf, char* pn)
{
    char c;

    c = *pn++;
    if (c == CHARCROSS)
        *buf++ = c;
    else
        UNTILPNSEP
        {
            *buf++ = c;
            c = *pn++;
        }
    *buf = 0;
    return;
}

void Method::reversepn(char* buf, char* pn)
{
    char c, *p = pn;
    c = *p;
    if (c == CHARCROSS)
        *buf++ = c;
    else
    {
        UNTILPNSEP
        c = *++p;
        while (p != pn)
            *buf++ = rounds[nbells - 1 - (strchr(rounds, *--p) - rounds)];
    }
    *buf = 0;
    return;
}

int pnlen(char* pn)
{
    char c, *p = pn;
    c = *p;
    if (c == CHARCROSS)
        return (1);
    UNTILPNSEP
    c = *++p;
    return (int)(p - pn);
}

void Ring::starttouch()
{
    strncpy(row, rounds, m->nbells);
    row[m->nbells] = 0;
    callqueued = incall = FALSE;
    startlead();
#if 1
    nextcall = calllist;
    calllist[0] = PLAIN;
#else
    setcompletename(ring.methodn, ring.completename, TRUE);
    return (preroundscall());
#endif
}

void Ring::startlead()
{
    // Sets up ring parameters for new method at start of lead
    changen = 0;
}

void Ring::change()
{
    char c, x, *pn = nullptr;
    Call call;
    int i;

    // If we are ringing a call, the pn comes from ring.callpn, but ring.pn
    // must still be updated!
    if (incall)
    {
        if (*callpn == 0)
            incall = FALSE;
        else
            pn = callpn;
    }
    if (!incall)
        pn = m->pnptrs[changen];
    if ((c = *pn) == CHARCROSS)
    {
        c = (char)255; // Ensure no match with an 'X' bell!
        pn++;
    }
    i = 0;
    do
    {
        if (c == rounds[i])
            c = *++pn;
        else
        {
            x = row[i];
            row[i] = row[i + 1];
            row[++i] = x;
        }
        i++;
    } while (i < m->nbells - 1);
    if (i == m->nbells - 1 && c == rounds[m->nbells - 1])
        pn++;
    c = *pn;
    UNTILPN
    c = *++pn;
    // Is a call ready to take effect? Note that a new call overrides any
    // earlier call which may still be being rung.
    if (callqueued)
    {
        callqueued = FALSE;
        incall = TRUE;
    }
    // Have we reached the calling position for a default PBLEADCODE method?
    // if ((m->leadheadcode&PBLEADCODE) && changen==m->leadlen-3)
    if (changen == m->leadlen - 3)
    {
        call = (Call)*nextcall++;
        if (call)
        {
            callqueued = TRUE;
            callpn = m->callpn[call];
        }
    }
    /*
    // Have we reached the next calling position?
     else if (ring.changen == ring.method->callpos[ring.nextcallpos])
     {
      call = *ring.nextcall++;
      if (call)
      {
       ring.callqueued = TRUE;
       ring.callpn =
        mlibrary->method[ring.nextmethodn].callpn[ring.nextcallpos][call-1];
      }
      ring.nextcallpos++;
     }
    */
    changen++;
    // return(call);
}

char* nextpn(char* pn)
{
    char c = *pn;

    if (c == CHARCROSS)
        pn++;
    else
    {
        UNTILPNSEP
        c = *++pn;
        UNTILPN
        c = *++pn;
    }
    return (pn);
}
