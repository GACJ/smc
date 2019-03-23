#ifndef INCFILER
#define INCFILER

// SMC32 File-handling routines Copyright Mark B Davies 1998

#include <stdio.h>
#include <string.h>

#include "basedef.h"

const int MAXFNAME = 300;
const int MAXLINEBUF = 10240;

class File
{
protected:
    char name[MAXFNAME];
    char mode[4];
    FILE* handle;
    long mark;

public:
    File(const char* filename)
    {
        strcpy(name, filename);
        handle = nullptr;
        strcpy(mode, "r");
    }
    ~File() { close(); }
    int exists()
    {
        if (handle)
            return (TRUE);
        else
            return filesize(name) > 0;
    }
    char* getname() { return name; }
    char* getextension()
    {
        auto p = strrchr(name, '.');
        if (p == nullptr)
            return strchr(name, '\0');
        else
            return p + 1;
    }
    void changeextension(const char* ext);
    void changeexttype(const char* ext); // Changes 1st 2 chars, leaves number
    int sameexttype(const char* ext);    // Checks 1st 2 chars only
    int incextension(const char* ext);
    void newfile(const char* filename)
    {
        close();
        strcpy(name, filename);
    }
    void setmode(const char* rwmode)
    {
        strncpy(mode, rwmode, 3);
        mode[sizeof(mode) - 1] = 0;
    }
    int open()
    {
        if (handle == nullptr)
            handle = fopen(name, mode);
        return handle != nullptr;
    }
    void close()
    {
        if (handle)
        {
            fclose(handle);
            handle = nullptr;
        }
    }
    void markpos() { mark = ftell(handle); }
    int resetpos();
    bool isabsolute() const
    {
#ifdef _WIN32
        if (name[0] == '\\' || name[0] == '/')
        {
            return true;
        }
        if (name[1] == ':')
        {
            if (name[2] == '\\' || name[2] == '/')
            {
                return true;
            }
        }
        return false;
#else
        return name[0] == '/';
#endif
    }
};

class LineFile : public File
{
public:
    char buffer[MAXLINEBUF];

public:
    LineFile(const char* filename)
        : File(filename)
    {
    }
    int readline();
    int writeline(const char* line);
    int writeline() { return writeline(buffer); }
    int multiwrite(const char* buf); // Assumes newlines already present
};

void get_directory(char* dst, size_t dstLen, const char* filename);
char* concat_path(char* dst, size_t dstLen, const char* p);

#endif // INCFILER