// SMC32 File-handling routines Copyright Mark B Davies 1998

#include <stdlib.h>
#include <string.h>
#include "filer.h"

void File::changeextension(char *ext)
{
 char *p;

 p = strchr(name,'.');
 if (p==NULL)
 {
  p = name+strlen(name);
  *p = '.';
 }
 strcpy(p+1,ext);
}

// Changes first two characters of extension, leaves number
void File::changeexttype(char *ext)
{
 char *p;

 p = strchr(name,'.');
 if (p==NULL)
 {
  p = name+strlen(name);
  *p = '.';
  strcpy(p+1,ext);
 }
 else
 {
  p[1] = ext[0];
  p[2] = ext[1];
 }
}

// Checks first two characters only
int File::sameexttype(char *ext)
{
 char *p;

 p = strchr(name,'.');
 if (p==NULL)
  return ext==NULL || ext[0]==0;
 return strncmpi(p+1,ext,2)==0;
}

int File::incextension(char *ext)
{
 close();
 changeextension(ext);
 ext = getextension();
 do
 {
  ext[2]++;
  if (ext[2]>'9')
  {
   printf("ERROR: you have too many output files (%s)\n",name);
   return(FALSE);
  }
 } while(exists());
 return(TRUE);
}

int File::resetpos()
{
 if (!open())
 {
  printf("\nERROR: failed to open file %s\n",name);
  return(FALSE);
 }
 if (fseek(handle,mark,SEEK_SET)!=0)
 {
  printf("\nERROR: seek failed on file %s\n",name);
  return(FALSE);
 }
 return(TRUE);
}

// Prints error message and returns FALSE if write failed
int LineFile::writeline(char *line)
{
 if (!open())
 {
  printf("\nERROR: failed to open file %s\n",name);
  return(FALSE);
 }
 if (fprintf(handle,"%s\n",line)<=0)
 {
  printf("\nERROR: write failed on file %s\n",name);
  return(FALSE);
 }
 return(TRUE);
}

// Prints error message and returns FALSE if write failed
int LineFile::multiwrite(char *buf)
{
 if (!open())
 {
  printf("\nERROR: failed to open file %s\n",name);
  return(FALSE);
 }
 if (fputs(buf,handle)<0)
 {
  printf("\nERROR: write failed on file %s\n",name);
  return(FALSE);
 }
 return(TRUE);
}

// Returns NULL and closes the file if nothing read
int LineFile::readline()
{
 char *buf = buffer;
 int c;
 int nothingread=TRUE;

 if (!open())
  return(FALSE);
 c = fgetc(handle);
 while(TRUE)
 {
  while (c!=EOF && (c==10 || c==13))	// Skip blank lines
   c = fgetc(handle);
  if (c=='/')			// Comment line
  {
   while (c!=EOF && c!=10 && c!=13)
    c = fgetc(handle);
  }
  else
   break;
 }
 while (c!=EOF && c!=10 && c!=13)
 {
  *buf++ = c;
  nothingread = FALSE;
  c = fgetc(handle);
 }
 *buf = 0;
 if (c==EOF && nothingread)
  return(FALSE);
 return(TRUE);
}
