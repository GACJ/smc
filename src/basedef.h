#ifndef INCBASEDEF
#define INCBASEDEF

// (C) 1998 Mark B Davies

enum Bool {FALSE,TRUE};

#define safedelete(p) {delete(p); p=0;}

inline int filesize(const char * path)
{
    FILE * fp = fopen(path, "rb");
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return (int)size;
}

#endif // INCBASEDEF