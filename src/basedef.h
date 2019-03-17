#ifndef INCBASEDEF
#define INCBASEDEF

#include <stdio.h>
#include <string.h>

// (C) 1998 Mark B Davies

enum Bool
{
    FALSE,
    TRUE
};

#define safedelete(p) \
    {                 \
        delete (p);   \
        p = 0;        \
    }

inline int filesize(const char* path)
{
    int size = 0;
    FILE* fp = fopen(path, "rb");
    if (fp != nullptr)
    {
        fseek(fp, 0, SEEK_END);
        size = (int)ftell(fp);
        fclose(fp);
    }
    return size;
}

inline char* stpcpy(char* dest, const char* src)
{
    strcpy(dest, src);
    return strchr(dest, 0);
}

#if defined(_MSC_VER)
#define strcmpi _strcmpi
#define strncmpi _strnicmp
#else
#define strcmpi strcasecmp
#define strncmpi strncasecmp
#endif

#endif // INCBASEDEF