#ifndef INCBASEDEF
#define INCBASEDEF

// (C) 1998 Mark B Davies

enum Bool {FALSE,TRUE};

#define safedelete(p) {delete(p); p=0;}

#endif // INCBASEDEF