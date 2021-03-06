#ifndef INCRING
#define INCRING

// Ring method-generation library
// Copyright Mark B Davies 1995-1998
// Ring.h

#include "basedef.h"

const int MINNBELLS = 3;
const int MAXNBELLS = 20;
const int MAXABBREV = 3;
const int MAXLEADLEN = 1260;
const int MAXCALLPOS = 4;
#define NDIFFCALLS 4
const int PBLEADCODE = 0x80;

const char CHARCROSS = 'x';
const char CHARLH = 'l';

enum Call
{
    PLAIN,
    BOB,
    SINGLE,
    EXTREME,
    USER
};
enum MethodType
{
    TYPENHUNT = 0xC0,
    TYPEPRINCIPLE = 0,
    TYPESINGLEHUNT = 0x40,
    TYPEMANYHUNT = 0x80,
    TYPESLOWCOURSE = 0xC0,
    TYPEDOUBLE = 0x20,
    TYPELITTLE = 0x10,
    TYPEPLAINM = 0x08,
    TYPEBOB = 0x04,
    TYPE = 0x03,
    TYPETREBLEBOBM = 0x04,
    TYPETB = 0x01,
    TYPEDELIGHT = 0x02,
    TYPESURPRISE = 0x03,
    TYPEUNKNOWN = 0,
    TYPETREBLEPLACE = 0x01,
    TYPEALLIANCE = 0x02,
    TYPEHYBRID = 0x03,
    TYPESYMMETRICAL = 0x100,
    TYPEUNCLASSIFIED = 0x200,
    TYPEILLEGAL = 0x400
};

class Ring;

class Method
{
    friend class Ring;

public:
    char* name;
    int nbells{};
    int leadlen;
    char leadhead[MAXNBELLS];

protected:
    char* pn;
    char* pnptrs[MAXLEADLEN];
    char* callpn[NDIFFCALLS]; // Only leadend calls at the moment
    int type;
    char leadheadcode;
    char symmetrical;

public:
    Method()
    {
        pn = nullptr;
        name = nullptr;
        for (int i = 0; i < NDIFFCALLS; i++)
            callpn[i] = nullptr;
    }
    ~Method()
    {
        delete[] name;
        delete[] pn;
        for (int i = 0; i < NDIFFCALLS; i++)
            delete[] callpn[i];
    }
    int newmethod(char* newname, char* newpn);
    int newcall(Call call, char* newpn);
    void setcompletename(char* buf, int abbreviated);
    void showleadcode(char* buf);
    MethodType gettype() { return (MethodType)type; }
    int fourthsplacebobs();
    int courselength();
    char* getpn() { return pn; }
    int issamepn(char* testpn);
    char* getcallpn(int call) { return callpn[call]; }

protected:
    int parsepn();
    void analyse();
    void findleadheadcode();
    // char *buildclass(char *p,char *class,int abbreviated);
    void classify(char* treblepath);
    void copypn(char* buf, char* pn);
    void reversepn(char* buf, char* pn);
    void classifyplain();
    void classifytreblebob(int ndodges);
};

class Ring
{
    friend class Method;

public:
    Method* m;
    int changen;
    int nbells;
    char row[MAXNBELLS + 1];
    char calllist[4];

protected:
    char plain;
    char incall;
    char callqueued;
    char* callpn;
    char* nextcall;

public:
    Ring() {}
    Ring(Method* method) { init(method); }
    void init(Method* method)
    {
        m = method;
        plain = TRUE;
        nbells = m->nbells;
    }
    void starttouch();
    void startlead();
    void change();
    void transpose(const char* source, const char* transposer, char* dest);
    void inversetrans(char* source, const char* transposer, const char* dest);
    void unknowntrans(const char* source, char* transposer, char* dest);
    inline int samerow(char* row1, char* row2)
    {
        for (int i = 0; i < nbells; i++)
            if (row1[i] != row2[i])
                return (FALSE);
        return (TRUE);
    }
    inline void copyrow(char* source, char* dest)
    {
        for (int i = 0; i < nbells; i++)
            dest[i] = source[i];
    }
};

#define UNTILPNSEP while (c != 0 && c != CHARCROSS && c != ' ' && c != '.' && c != CHARLH)
#define UNTILPN while (c == ' ' || c == '.')

extern char rounds[];

#endif // INCRING
