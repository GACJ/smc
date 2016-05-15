#ifndef INCFILER
#define INCFILER

// SMC32 File-handling routines Copyright Mark B Davies 1998

#include <stdio.h>
#include <string.h>

#include "basedef.h"

const MAXFNAME = 300;
const MAXLINEBUF = 10240;

class File
{
protected:
 char name[MAXFNAME];
 char mode[4];
 FILE *handle;
 long mark;

public:
 File(char *filename) {strcpy(name,filename); handle=NULL; strcpy(mode,"r");}
 ~File() {close();}
 exists() {if (handle) return(TRUE); else return filesize(name)>0;}
 char *getname() {return name;}
 char *getextension() {char *p=strchr(name,'.');
 		   if (p==NULL) return ""; else return p+1;}
 void changeextension(char *ext);
 void changeexttype(char *ext);	// Changes 1st 2 chars, leaves number
 sameexttype(char *ext);		// Checks 1st 2 chars only
 incextension(char *ext);
 void newfile(char *filename) {close(); strcpy(name,filename);}
 void setmode(char *rwmode) {strncpy(mode,rwmode,3); mode[4] = 0;}
 open() {if (handle==NULL) handle=fopen(name,mode); return handle!=NULL;}
 void close() {if (handle) {fclose(handle); handle=NULL;}}
 void markpos() {mark = ftell(handle);}
 resetpos();
};

class LineFile: public File
{
public:
 char buffer[MAXLINEBUF];

public:
 LineFile(char *filename): File(filename) {}
 readline();
 writeline(char *line);
 writeline() {return writeline(buffer);}
 multiwrite(char *buf);		// Assumes newlines already present
};
#endif // INCFILER