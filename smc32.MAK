ORIGIN		= Symantec C++
ORIGIN_VER	= Version 7.22
VERSION		= RELEASE

!IFDEF SUB_DEBUG
DEBUG		= $(SUB_DEBUG)
NDEBUG		= !$(SUB_DEBUG)
!ELSE
DEBUG		= 0
NDEBUG		= 1
!ENDIF

PROJ		= smc32
APPTYPE		= WIN32 CONSOLE
PROJTYPE	= EXE

CC		= SC
CPP		= SPP
JAVAC		= sj
MAKE		= SMAKE
RC		= RCC
HC		= HC31
ASM		= SC
DISASM		= OBJ2ASM
LNK		= LINK
DLLS		= 

HEADERS		= C:\..\SC\include\stdio.h C:\..\SC\include\string.h C:\..\SC\include\ctype.h  \
		smc.h ring.h C:\..\SC\include\stdlib.h C:\..\SC\include\time.h  \
		bulklist.h C:\..\SC\include\limits.h filer.h basedef.h  \
		C:\..\SC\include\conio.h C:\..\SC\include\excpt.h C:\..\SC\include\math.h bulkhash.h  \
		..\..\SC\include\stdlib.h ..\..\SC\include\time.h ..\..\SC\include\stdio.h ..\..\SC\include\string.h  \
		..\..\SC\include\limits.h ..\..\SC\include\math.h ..\..\SC\include\ctype.h ..\..\SC\include\excpt.h  \
		fragment.h 

DEFFILE		= smc32.DEF

!IF $(DEBUG)
OUTPUTDIR	= c:\source\smc32
!IF EXIST (c:\source\smc32)
CREATEOUTPUTDIR	=
!ELSE
CREATEOUTPUTDIR	= if not exist $(OUTPUTDIR)\*.* md $(OUTPUTDIR)
!ENDIF
TARGETDIR	= c:\source\smc32
!IF EXIST (c:\source\smc32)
CREATETARGETDIR	=
!ELSE
CREATETARGETDIR	= if not exist $(TARGETDIR)\*.* md $(TARGETDIR)
!ENDIF

SYMROOT		= 
SYMS		= 
LIBS		= KERNEL32.LIB GDI32.LIB USER32.LIB 

CFLAGS		=  -mn -C -WA -S -3 -a8 -c -gf 
LFLAGS		=  /CO /NOI /DO /DE /NOPACKF /XN /NT /ENTRY:mainCRTStartup /BAS:4194304 /A:512
DEFINES		= -D_CONSOLE=1
!ELSE
OUTPUTDIR	= C:\source\smc32
!IF EXIST (C:\source\smc32)
CREATEOUTPUTDIR	=
!ELSE
CREATEOUTPUTDIR	= if not exist $(OUTPUTDIR)\*.* md $(OUTPUTDIR)
!ENDIF
TARGETDIR	= C:\source\smc32
!IF EXIST (C:\source\smc32)
CREATETARGETDIR	=
!ELSE
CREATETARGETDIR	= if not exist $(TARGETDIR)\*.* md $(TARGETDIR)
!ENDIF

SYMROOT		= 
SYMS		= 
LIBS		= KERNEL32.LIB GDI32.LIB USER32.LIB 

CFLAGS		=  -mn -o+time -WA -3 -a8 -c 
LFLAGS		=  /NOI /DO /DE /NOPACKF /XN /NT /ENTRY:mainCRTStartup /BAS:4194304 /A:512
DEFINES		= -D_CONSOLE=1
!ENDIF

HFLAGS		= $(CFLAGS) 
MFLAGS		= MASTERPROJ=$(PROJ) 
LIBFLAGS	=  /C 
RESFLAGS	=  -32 
DEBUGGERFLAGS	=  
AFLAGS		= $(CFLAGS) 
HELPFLAGS	= 

MODEL		= N

PAR		= PROJS BATS OBJS

RCDEFINES	= 

INCLUDES	= 

INCLUDEDOBJS	= 

OBJS		=  $(OUTPUTDIR)\smc.OBJ  $(OUTPUTDIR)\transpose.OBJ  $(OUTPUTDIR)\method.OBJ  \
		 $(OUTPUTDIR)\ring.OBJ  $(OUTPUTDIR)\buildtable.OBJ  $(OUTPUTDIR)\compose.OBJ  $(OUTPUTDIR)\filer.OBJ  \
		 $(OUTPUTDIR)\smcfile.OBJ  $(OUTPUTDIR)\music.OBJ  $(OUTPUTDIR)\testbed.OBJ  $(OUTPUTDIR)\library.OBJ 

RCFILES		= 

RESFILES	= 

HELPFILES	= 

BATS		= 

.SUFFIXES: .C .CP .CPP .CXX .CC .H .HPP .HXX .COM .EXE .DLL .LIB .RTF .DLG .ASM .RES .RC .OBJ 

.C.OBJ:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$*.obj $*.c

.CPP.OBJ:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$*.obj $*.cpp

.CXX.OBJ:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$*.obj $*.cxx

.CC.OBJ:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$*.obj $*.cc

.CP.OBJ:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$*.obj $*.cp

.H.SYM:
	$(CC) $(HFLAGS) $(DEFINES) $(INCLUDES) -HF -o$(*B).sym $*.h

.HPP.SYM:
	$(CC) $(HFLAGS) $(DEFINES) $(INCLUDES) -HF -o$(*B).sym $*.hpp

.HXX.SYM:
	$(CC) $(HFLAGS) $(DEFINES) $(INCLUDES) -HF -o$(*B).sym $*.hxx

.C.EXP:
	$(CPP) $(CFLAGS) $(DEFINES) $(INCLUDES)   $*.c   -o$*.lst

.CPP.EXP:
	$(CPP) $(CFLAGS) $(DEFINES) $(INCLUDES) $*.cpp -o$*.lst

.CXX.EXP:
	$(CPP) $(CFLAGS) $(DEFINES) $(INCLUDES) $*.cxx -o$*.lst

.CP.EXP:
	$(CPP) $(CFLAGS) $(DEFINES) $(INCLUDES)  $*.cp  -o$*.lst

.CC.EXP:
	$(CPP) $(CFLAGS) $(DEFINES) $(INCLUDES)  $*.cc  -o$*.lst

.ASM.EXP:
	$(CPP) $(CFLAGS) $(DEFINES) $(INCLUDES) $*.asm -o$*.lst

.OBJ.COD:
	$(DISASM) $*.OBJ -c

!IF $(DEBUG)
.OBJ.EXE:
		$(LNK) $(LFLAGS) @<<$(PROJ).LNK
c:\source\smc32\smc.OBJ+
c:\source\smc32\transpose.OBJ+
c:\source\smc32\method.OBJ+
c:\source\smc32\ring.OBJ+
c:\source\smc32\buildtable.OBJ+
c:\source\smc32\compose.OBJ+
c:\source\smc32\filer.OBJ+
c:\source\smc32\smcfile.OBJ+
c:\source\smc32\music.OBJ+
c:\source\smc32\testbed.OBJ+
c:\source\smc32\library.OBJ
c:\source\smc32\$$SCW$$.EXE
NUL
KERNEL32.LIB GDI32.LIB USER32.LIB 
smc32.DEF;
<<
!ELSE
.OBJ.EXE:
		$(LNK) $(LFLAGS) @$(PROJ).LNK<<
C:\source\smc32\smc.OBJ+
C:\source\smc32\transpose.OBJ+
C:\source\smc32\method.OBJ+
C:\source\smc32\ring.OBJ+
C:\source\smc32\buildtable.OBJ+
C:\source\smc32\compose.OBJ+
C:\source\smc32\filer.OBJ+
C:\source\smc32\smcfile.OBJ+
C:\source\smc32\music.OBJ+
C:\source\smc32\testbed.OBJ+
C:\source\smc32\library.OBJ
C:\source\smc32\$$SCW$$.EXE
NUL
KERNEL32.LIB GDI32.LIB USER32.LIB 
smc32.DEF;
<<
!ENDIF

.RTF.HLP:
	$(HC) $(HELPFLAGS) $*.HPJ

.ASM.OBJ:
	$(ASM) $(AFLAGS) $(DEFINES) $(INCLUDES) -o$*.obj $*.asm

.RC.RES: 
	$(RC) $(RCDEFINES) $(RESFLAGS) $(INCLUDES) $*.rc -o$*.res

.DLG.RES:
	echo ^#include "windows.h" >$$$*.rc
	echo ^IF EXIST "$*.h" >>$$$*.rc
	echo ^#include "$*.h" >>$$$*.rc
	echo ^#include "$*.dlg" >>$$$*.rc
	$(RC) $(RCDEFINES) $(RESFLAGS) $(INCLUDES) $$$*.rc
	-del $*.res
	-ren $$$*.res $*.res



all:	noteout createdir $(PRECOMPILE) $(SYMS) $(OBJS) $(INCLUDEDOBJS) $(POSTCOMPILE) $(TARGETDIR)\$(PROJ).$(PROJTYPE) $(POSTLINK) _done


all2:	createdir $(PRECOMPILE) $(SYMS) $(OBJS) $(INCLUDEDOBJS) $(POSTCOMPILE) $(TARGETDIR)\$(PROJ).$(PROJTYPE) $(POSTLINK) _done

noteout:
	REM Output to $(OUTPUTDIR)

createdir:
	$(CREATEOUTPUTDIR)
	$(CREATETARGETDIR)
	
$(TARGETDIR)\$(PROJ).$(PROJTYPE): $(OBJS) $(INCLUDEDOBJS) $(RCFILES) $(RESFILES) $(HELPFILES) $(DEFFILE)
			-del $(TARGETDIR)\$(PROJ).$(PROJTYPE)
!IF $(DEBUG)
		$(LNK) $(LFLAGS) @<<$(PROJ).LNK
c:\source\smc32\smc.OBJ+
c:\source\smc32\transpose.OBJ+
c:\source\smc32\method.OBJ+
c:\source\smc32\ring.OBJ+
c:\source\smc32\buildtable.OBJ+
c:\source\smc32\compose.OBJ+
c:\source\smc32\filer.OBJ+
c:\source\smc32\smcfile.OBJ+
c:\source\smc32\music.OBJ+
c:\source\smc32\testbed.OBJ+
c:\source\smc32\library.OBJ
c:\source\smc32\$$SCW$$.EXE
NUL
KERNEL32.LIB GDI32.LIB USER32.LIB 
smc32.DEF;
<<
!ELSE
		$(LNK) $(LFLAGS) @<<$(PROJ).LNK
C:\source\smc32\smc.OBJ+
C:\source\smc32\transpose.OBJ+
C:\source\smc32\method.OBJ+
C:\source\smc32\ring.OBJ+
C:\source\smc32\buildtable.OBJ+
C:\source\smc32\compose.OBJ+
C:\source\smc32\filer.OBJ+
C:\source\smc32\smcfile.OBJ+
C:\source\smc32\music.OBJ+
C:\source\smc32\testbed.OBJ+
C:\source\smc32\library.OBJ
C:\source\smc32\$$SCW$$.EXE
NUL
KERNEL32.LIB GDI32.LIB USER32.LIB 
smc32.DEF;
<<
!ENDIF

			-ren $(TARGETDIR)\$$SCW$$.$(PROJTYPE) $(PROJ).$(PROJTYPE)
			-echo $(TARGETDIR)\$(PROJ).$(PROJTYPE) built

_done:
		REM  Project is up to date

buildall:	clean	all


clean:
		-del $(TARGETDIR)\$$SCW$$.$(PROJTYPE)
		-del $(PROJ).CLE
		-del $(OUTPUTDIR)\SCPH.SYM
		-del smc32.dpd
		-del $(OBJS)

cleanres:

res:		cleanres $(RCFILES) all


link:
!IF $(DEBUG)
		$(LNK) $(LFLAGS) @<<$(PROJ).LNK
c:\source\smc32\smc.OBJ+
c:\source\smc32\transpose.OBJ+
c:\source\smc32\method.OBJ+
c:\source\smc32\ring.OBJ+
c:\source\smc32\buildtable.OBJ+
c:\source\smc32\compose.OBJ+
c:\source\smc32\filer.OBJ+
c:\source\smc32\smcfile.OBJ+
c:\source\smc32\music.OBJ+
c:\source\smc32\testbed.OBJ+
c:\source\smc32\library.OBJ
c:\source\smc32\$$SCW$$.EXE
NUL
KERNEL32.LIB GDI32.LIB USER32.LIB 
smc32.DEF;
<<
!ELSE
		$(LNK) $(LFLAGS) @<<$(PROJ).LNK
C:\source\smc32\smc.OBJ+
C:\source\smc32\transpose.OBJ+
C:\source\smc32\method.OBJ+
C:\source\smc32\ring.OBJ+
C:\source\smc32\buildtable.OBJ+
C:\source\smc32\compose.OBJ+
C:\source\smc32\filer.OBJ+
C:\source\smc32\smcfile.OBJ+
C:\source\smc32\music.OBJ+
C:\source\smc32\testbed.OBJ+
C:\source\smc32\library.OBJ
C:\source\smc32\$$SCW$$.EXE
NUL
KERNEL32.LIB GDI32.LIB USER32.LIB 
smc32.DEF;
<<
!ENDIF

		-del $(TARGETDIR)\$(PROJ).$(PROJTYPE)
		-ren $(TARGETDIR)\$$SCW$$.$(PROJTYPE) $(PROJ).$(PROJTYPE)




!IF EXIST (smc32.dpd)
!INCLUDE smc32.dpd
!ENDIF



$(OUTPUTDIR)\smc.OBJ:	smc.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\smc.obj smc.cpp



$(OUTPUTDIR)\transpose.OBJ:	transpose.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\transpose.obj transpose.cpp



$(OUTPUTDIR)\method.OBJ:	method.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\method.obj method.cpp



$(OUTPUTDIR)\ring.OBJ:	ring.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\ring.obj ring.cpp



$(OUTPUTDIR)\buildtable.OBJ:	buildtable.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\buildtable.obj buildtable.cpp



$(OUTPUTDIR)\compose.OBJ:	compose.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\compose.obj compose.cpp



$(OUTPUTDIR)\filer.OBJ:	filer.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\filer.obj filer.cpp



$(OUTPUTDIR)\smcfile.OBJ:	smcfile.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\smcfile.obj smcfile.cpp



$(OUTPUTDIR)\music.OBJ:	music.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\music.obj music.cpp



$(OUTPUTDIR)\testbed.OBJ:	testbed.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\testbed.obj testbed.cpp



$(OUTPUTDIR)\library.OBJ:	library.cpp
		$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -o$(OUTPUTDIR)\library.obj library.cpp




