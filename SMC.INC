;*********************************************************
;* SMC - Single Method Composer                          *
;* Copyright Graham A C John 1991-98 All rights reserved *
;*********************************************************
;************************************
;******** DATA INCLUDE FILE - SMC.INC
;************************************
datgrp    GROUP datseg1
		ASSUME ds:datgrp
datseg1   SEGMENT COMMON
		EXTERNDEF   EXTDAT:BYTE
;*********Equates
TRUE      EQU -1
FALSE	  EQU 0
PLAIN	  EQU 0
BOB	  EQU 1
SINGLE	  EQU 2
TIMEOUT   EQU 10000		 ;Number of leads generated before dropping out
TENOR     EQU 8
STATIC    EQU 0
FLASH     EQU 80h
coltit    EQU 1Eh                ;Blue/Yellow     = Main Titles
colstd    EQU 17h                ;Blue/White      = Main screen normal
colhigh   EQU 71h                ;White/Blue      = Main screen highlight
colerr    EQU 0Ch                ;Black/Int Red   = Error message
colwrn    EQU 0Eh                ;Black/Yellow    = Warning message
colinf    EQU 0Fh                ;Black/Int White = Information message
colleg    EQU 02h                ;Black/Green     = Legends for stats
colsta    EQU 04h                ;Black/Red       = Status
colinp    EQU 0Bh                ;Black/Cyan      = Input fields
;*********Screen Buffer
scrbuf    WORD 80*25 DUP(0)       ;Screen Buffer
;*********Constants
factorial WORD 5040,0,1,2,6,24,120,720          ;Factorials for bell positions 2-8
calltit   BYTE " ","I","B","4","V","M","W","H"  ;Column headings for place bells 1-8
rounds    DWORD 04030201h,08070605h
pblhs     DWORD 00000000h,00000000h
          DWORD 02050301h,06080407h             ;Plain Bob Lead Heads
          DWORD 03070501h,04060208h
          DWORD 05080701h,02040306h
          DWORD 07060801h,03020504h
          DWORD 08040601h,05030702h
          DWORD 06020401h,07050803h
lhg2      BYTE "?abcdef"                        ;Plain Bob Leadhead groups
lhg8      BYTE "?ghjklm"                        ;8ths place methods
;*********Segment Group values
datseg    WORD 0
cmpseg    WORD 0
lstseg    WORD 0
dirseg    WORD 0B800h
scrseg	  WORD 0
stkseg	  WORD 0
stksp	  WORD 0
stkbp	  WORD 0
;*********Composition Stats
statfound DWORD 0		  ;Number of compositions found
statcomps WORD 0		  ;Number of compositions kept
statdel   WORD 0                  ;Number of compositions deleted
statleads DWORD 0		  ;Number of leads checked in TIMEOUTS
statleadm DWORD 0		  ;Number of leads checked in millions
statsdate WORD 0		  ;Compose start date
statstime WORD 0                  ;Compose start time
stattab   WORD 0                  ;Count of the number of included table entries
statflhhi BYTE 0                  ;The highest no of FLHs in a table entry
statflhlo BYTE 0                  ;The lowest no of FLHs in a table entry
statpcent BYTE 0                  ;Percentage complete
	  BYTE 0                  ;Spare
;*********Composition Work Area
cmplho    WORD 0                  ;Current LHO
;*********Key Codes
FnDisHlp   WORD 003Bh		   ;F1
FnMethod   WORD 003Ch              ;F2
FnExclud   WORD 003Dh		   ;F3
FnInclud   WORD 006Ah		   ;Alt F3
FnBldtab   WORD 003Eh              ;F4
FnDisLst   WORD 003Fh              ;F5
FnDisDyn   WORD 0040h		   ;F6
FnDisMus   WORD 0041h		   ;F7
FnDisTab   WORD 0042h              ;F8
FnComGen   WORD 0043h              ;F9
FnExit	   WORD 0044h		   ;F10
FnNew	   WORD 0		   ;New Function indicator (TRUE/FALSE)
FnDown     WORD 0050h              ;Cursor Down
FnUp       WORD 0048h              ;Cursor Up
FnLeft     WORD 004Bh              ;Cursor Left
FnRight    WORD 004Dh              ;Cursor Right
FnHome     WORD 0047h              ;Cursor Home
FnEnd      WORD 004Fh              ;Cursor End
FnPageDown WORD 0051h              ;Page Down
FnPageUp   WORD 0049h              ;Page Up
FnPageLeft WORD 0073h              ;Page Left / Ctrl Left
FnPageRt   WORD 0074h              ;Page Right / Ctrl Right
FnCtrlHome WORD 0077h              ;Ctrl Home
FnCtrlEnd  WORD 0075h              ;Ctrl End
FnEnter    WORD 0D00h              ;Enter
FnSpace    WORD 2000h              ;Space bar
FnEsc      WORD 1B00h              ;Escape
FnTab      WORD 0900h              ;Tab
FnBackTab  WORD 000Fh              ;Shift Tab
FnDelete   WORD 0053h              ;Delete
;*********WorkArea
wrklea    DWORD 2*64 dup (0)      ;Lead Generation Workspace
wrkleh    DWORD 2 dup (0)         ;Call Leadhead
wrktrans  DWORD 2 dup (0)         ;Tranposition result workspace
wrkmin    WORD 0                  ;For show longest option
wrkexc    WORD 0                  ;TRUE if the current lead is excluded
wrkbak    WORD 0                  ;TRUE if the current row is a backstroke
wrklrnd   WORD 0                  ;TRUE if the current lead contains Rounds
wrkrrnd   WORD 0                  ;TRUE if the current row is rounds
wrkfno	  BYTE 0,0		  ;Count of false leadheads in overflow conditions
natrow	  DWORD 0		  ;workspace for calculation of leadhead nature
;*********Method Definition
metbel WORD 8                     ;Number of bells (fixed)
metcur WORD -1                    ;Current method number
metnum WORD (metend-metn01)/93    ;Number of methods
metnam BYTE "                         "
metlen WORD 0                     ;Current method definition
metnot BYTE 63 DUP(0)
metcal BYTE 0,0,0
metn01 BYTE "Belfast Surprise         "
       WORD 32
       BYTE 60h,0,28h,0A0h,0,0C0h,0,48h,0C0h,0,0A0h,48h,90h,0,0C0h,48h
       BYTE 0C0h,0,90h,48h,0A0h,0,0C0h,48h,0,0C0h,0,0A0h,28h,0,60h
       BYTE 32 dup(0)
       BYTE 88h,0A0h,0F0h
       BYTE "Belfast Surprise (6th Pl)"
       WORD 32
       BYTE 60h,0,28h,0A0h,0,0C0h,0,48h,0C0h,0,0A0h,48h,90h,0,0C0h,48h
       BYTE 0C0h,0,90h,48h,0A0h,0,0C0h,48h,0,0C0h,0,0A0h,28h,0,60h
       BYTE 32 dup(0)
       BYTE 88h,090h,09Ch
       BYTE "Bristol Surprise         "
       WORD 32
       BYTE 0,28h,0,0A0h,28h,0,28h,50h,0A0h,0,0A0h,28h,0,0A0h,0,88h
       BYTE 0,0A0h,0,28h,0A0h,0,0A0h,50h,28h,0,28h,0A0h,0,28h,0
       BYTE 32 dup(0)
       BYTE 88h,0A0h,0F0h
       BYTE "Cambridge Surprise       "
       WORD 32
       BYTE 0,48h,0,0A0h,0,0D4h,0,50h,0,0A0h,0,28h,0,90h,0,18h
       BYTE 0,90h,0,28h,0,0A0h,0,50h,0,0D4h,0,0A0h,0,48h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Caterham Surprise        "
       WORD 32
       BYTE 50h,0,30h,0A0h,28h,0,28h,50h,0,60h,0C0h,28h,90h,0,90h,28h
       BYTE 90h,0,90h,28h,0C0h,60h,0,50h,28h,0,28h,0A0h,30h,0,50h
       BYTE 32 dup(0)
       BYTE 88h,0A0h,0F0h
       BYTE "Catherine Surprise       "
       WORD 32
       BYTE 0,48h,0,0A0h,0,0D8h,0,90h,0,0C0h,0,28h,0A0h,0,0A0h,28h
       BYTE 0A0h,0,0A0h,28h,0,0C0h,0,90h,0,0D8h,0,0A0h,0,48h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Cray Surprise            "
       WORD 32
       BYTE 0,30h,0,0A0h,0,30h,0,50h,0,60h,0,28h,0,60h,0,18h
       BYTE 0,60h,0,28h,0,60h,0,50h,0,30h,0,0A0h,0,30h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Glasgow Surprise         "
       WORD 32
       BYTE 50h,0,30h,0A0h,28h,0,28h,50h,0,0A0h,0,48h,90h,0,90h,48h
       BYTE 90h,0,90h,48h,0,0A0h,0,50h,28h,0,28h,0A0h,30h,0,50h
       BYTE 32 dup(0)
       BYTE 88h,090h,09Ch
       BYTE "Dartmoor Surprise        "
       WORD 32
       BYTE 30h,0,28h,0A0h,0,28h,0,50h,0,0C0h,0,28h,0,90h,0,88h
       BYTE 0,90h,0,28h,0,0C0h,0,50h,0,28h,0,0A0h,28h,0,30h
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Jersey Surprise          "
       WORD 32
       BYTE 0,28h,0,90h,0,0C0h,0,90h,0,0A0h,0,28h,0,50h,0,28h
       BYTE 0,50h,0,28h,0,0A0h,0,90h,0,0C0h,0,90h,0,28h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Kent Treble Bob          "
       WORD 32
       BYTE 60h,0,60h,88h,0,0C0h,0,88h,0,0C0h,0,88h,0,0C0h,0,88h
       BYTE 0,0C0h,0,88h,0,0C0h,0,88h,0,0C0h,0,88h,60h,0,60h
       BYTE 32 dup(0)
       BYTE 88h,0A0h,0F0h
       BYTE "Lincolnshire Surprise    "
       WORD 32
       BYTE 0,48h,0,0A0h,0,28h,0,90h,0,0A0h,0,28h,0,50h,0,18h
       BYTE 0,50h,0,28h,0,0A0h,0,90h,0,28h,0,0A0h,0,48h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Lindum Surprise          "
       WORD 32
       BYTE 0,50h,0,0A0h,0,28h,0,90h,0,0A0h,0,48h,0,0A0h,0,28h
       BYTE 0,0A0h,0,48h,0,0A0h,0,90h,0,28h,0,0A0h,0,50h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "London Surprise          "
       WORD 32
       BYTE 48h,0,48h,0A0h,0,0C0h,0,48h,0A0h,0,0A0h,28h,90h,0,90h,28h
       BYTE 90h,0,90h,28h,0A0h,0,0A0h,48h,0,0C0h,0,0A0h,48h,0,48h
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "MC Little Surprise       "
       WORD 24
       BYTE 0,48h,0,0A0h,0,30h,0,48h,0A0h,0,0C0h,30h
       BYTE 0C0h,0,0A0h,48h,0,30h,0,0A0h,0,48h,0
       BYTE 40 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Norwich Surprise         "
       WORD 32
       BYTE 0,60h,0,0A0h,0,0C0h,0,48h,0,60h,0,48h,0,60h,0,88h
       BYTE 0,60h,0,48h,0,60h,0,48h,0,0C0h,0,0A0h,0,60h,0
       BYTE 32 dup(0)
       BYTE 88h,0A0h,0F0h
       BYTE "Oxford Treble Bob        "
       WORD 32
       BYTE 0,60h,0,88h,0,0C0h,0,88h,0,0C0h,0,88h,0,0C0h,0,88h
       BYTE 0,0C0h,0,88h,0,0C0h,0,88h,0,0C0h,0,88h,0,60h,0
       BYTE 32 dup(0)
       BYTE 88h,0A0h,0F0h
       BYTE "Stedman Triples          "
       WORD 12
       BYTE 0A0h,0C0h,0A0h,0C0h,0A0h,88h,0C0h,0A0h,0C0h,0A0h,0C0h
       BYTE 52 dup(0)
       BYTE 88h,090h,09Ch
       BYTE "Plain Bob                "
       WORD 16
       BYTE 0,88h,0,88h,0,88h,0,88h,0,88h,0,88h,0,88h,0
       BYTE 48 dup (0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Pudsey Surprise          "
       WORD 32
       BYTE 0,28h,0,90h,0,0C0h,0,48h,0,0A0h,0,28h,0,90h,0,18h
       BYTE 0,90h,0,28h,0,0A0h,0,48h,0,0C0h,0,90h,0,28h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Rutland Surprise         "
       WORD 32
       BYTE 0,48h,0,0A0h,0,28h,0,90h,0,0A0h,0,48h,0,60h,0,88h
       BYTE 0,60h,0,48h,0,0A0h,0,90h,0,28h,0,0A0h,0,48h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "St Clements Court Bob    "
       WORD 16
       BYTE 0,88h,0,048h,0,48h,0,48h
       BYTE 0,48h,0,48h,0,88h,0
       BYTE 48 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Superlative Surprise     "
       WORD 32
       BYTE 0,50h,0,0A0h,0,28h,0,50h,0,0A0h,0,28h,0,50h,0,18h
       BYTE 0,50h,0,28h,0,0A0h,0,50h,0,28h,0,0A0h,0,50h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
       BYTE "Warkworth Surprise       "
       WORD 32
       BYTE 0,60h,0,0A0h,0,0C0h,0,50h,0A0h,0,0C0h,28h,90h,0,0A0h,28h
       BYTE 0A0h,0,90h,28h,0C0h,0,0A0h,50h,0,0C0h,0,0A0h,0,60h,0
       BYTE 32 dup(0)
       BYTE 88h,0A0h,0F0h
       BYTE "Yorkshire Surprise       "
       WORD 32
       BYTE 0,48h,0,0A0h,0,028h,0,90h,0,0C0h,0,48h,0,0A0h,0,18h
       BYTE 0,0A0h,0,48h,0,0C0h,0,90h,0,28h,0,0A0h,0,48h,0
       BYTE 32 dup(0)
       BYTE 0C0h,0A0h,0F0h
metend BYTE 0			  ;Marks End of methods
metleh DWORD 2 DUP (0)		  ;First Plain Leadhead of the Method
metlhg WORD "  "                  ;Lead Head Group of method
metcol BYTE 8 DUP (0)		  ;Column positions of calls for discmp
metsec DWORD 108570		  ;Security check on method area
;*********FLH List
flhcnt    BYTE 0                  ;Count of the number of False Lead Heads
flhlist   DWORD 128*2 DUP(0)      ;List of False Course Heads
flhrow    BYTE 64 DUP (0)         ;List of row numbers generating flhs
;*********Composition Generation variables
gencom	  WORD FALSE		  ;Composing suspended if False
genend    WORD FALSE              ;Composition exhausted indicator
gennew    WORD FALSE              ;New composition found indicator
genofl	  WORD FALSE		  ;Composition list overflow indicator
gendyn	  WORD FALSE		  ;Dynamic Composition display indicator
gendup	  WORD FALSE		  ;Duplicate composition indicator
genlst	  WORD 0		  ;Current Composition List Segment
genlen    WORD 0                  ;Current Composition Length
genlea    WORD 0                  ;Lead Pointer into LHO list (genlho)
genptr    DWORD 0                 ;Pointer into call list (gencal)
genspare  BYTE 0
genday    BYTE 0                  ;Start day of week 0-7
genhour   BYTE 0		  ;Start hour 0-23
genmin    BYTE 0                  ;Start minutes 0-59
gensec    BYTE 0                  ;Start second 0-59
gensech   BYTE 0                  ;Start hundreths of a second 0-99
genlho    WORD 5041 DUP (0)       ;LHO list (current composition)
gencal    BYTE 5042 DUP (0)       ;Call list (current composition) for optimisation
gencalct  WORD 24 DUP (0)         ;Counts of calls by tenor position (8p,8b,8s)
gencalfst BYTE 0		  ;First call choice in composition (nb excrev affects)
gencallst BYTE 0		  ;Last call choice in composition  (nb excrev affects)
;*********Display Variables
dislho    WORD 0                  ;LHO of Display table element
disnum    WORD 0                  ;Number of 1st composition on display page
dissel    WORD 0                  ;Number of currently selected composition
dislen    WORD 0                  ;Composition length for part checking
disframe  WORD 0                  ;Display frame (sideways page)
diszero   BYTE " "                ;Zero suppress all " ", except last "Z", None "0"
discol    BYTE 07h                ;Current display colour
discoh    DWORD 2 DUP(0)          ;Last course head
disleh    DWORD 2 DUP(0)          ;Last lead head at a call
disfirst  WORD 0                  ;Discmp first display line
dislin    WORD 0                  ;Display line counter
discalcnt WORD 0                  ;For counting consecutive calls in discmp
disclm    BYTE 0                  ;Discmp current display column
disshift  BYTE 0                  ;For decompression of calls in list
curfld    BYTE 0                  ;data entry field number
curpos	  BYTE 0		  ;cursor position in field
curlin	  BYTE 0		  ;cursor line
curcol	  BYTE 0		  ;cursor column
newnum	  WORD 0		  ;number of inserted new composition
;*********Display Titles
distit0   BYTE "LHNo Leadhead Len Pt Plain    Bob      Single   RQWTBHR6723    I C 87s 82s 83s  "
distit1   BYTE "LHNo Leadhead   crus  (5678)s 5678s 6578s 4578s 4678s 6478s 5478s 3578s 3478s   "
distit2   BYTE "LHNo Leadhead 8765s 7568s 7658s 2345s 5432s 3456s 6543s                         "
distit3   BYTE "LHNo Leadhead flh flh 1    flh 2    flh 3    flh 4    flh 5    flh 6    flh 7   "
distit3a  BYTE "LHNo Leadhead flh flh 8    flh 9    flh 10   flh 11   flh 12   flh 13   flh 14  "
distit3b  BYTE "LHNo Leadhead flh flh 15   flh 16   flh 17   flh 18   flh 19   flh 20   flh 21  "
distit3c  BYTE "LHNo Leadhead flh flh 22   flh 23   flh 24   flh 25   flh 26   flh 27   flh 28  "
distit3d  BYTE "LHNo Leadhead flh flh 29   flh 30   flh 31   flh 32   flh 33   flh 34   flh 35  "
distit4   BYTE "                            Elapsed Time  :     :  :   Comps Found :            "
	  BYTE "Available Leads   :         Leads Checked :          m Comps Listed:            "
	  BYTE "FLHs High/Poss    :   ,     Leads/sec     :          k Complete    :           %"
distit5   BYTE " No   Elapsed  Lnth Prt   Ps  Bs  Ss 87s 82s 83s --crus- 5678s 6578s 8765s 7568s"
distit6   BYTE " No   Elapsed  Lnth Prt  4578s 4678s 6478s 5478s 3578s 3478s 5678cmb 7658s 2345s"
distit7   BYTE " No   Elapsed  Lnth Prt  5432s 3456s 6543s QWTBHR6723     Score     Seq Rotation"
distit8   BYTE "                   V    I    M    W    B    4    H    2345678                   "
distit9   BYTE "                  ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ   ฤฤฤฤฤฤฤ                   "
distit10  BYTE 19 DUP(" "),31 DUP(0FAh),"    ",7 DUP (0FAh),19 DUP (" ")
disexc    BYTE "                                                                                "
	  BYTE "EXCLUDE  Lengths -  Below: *****  Above: *****  Longest yet: *   Optimisation: *"
	  BYTE "                                                                                "
	  BYTE "         Tenors split courseheads: *            Reverse    : *   Rotation    : *"
	  BYTE "                                                                                "
	  BYTE "         Rounds @ Hand,Back,Int,LH: *,*,*,*  87s @ Back: *  82s: *  83s: *      "
	  BYTE "                                                                                "
	  BYTE "         Number of Parts  - 1: *  2: *  3: *  4: *  5: *  6: *  7: * 10: * 12: *"
	  BYTE "                                                                                "
	  BYTE "                             Any  2nds  3rds  4ths  5ths  6ths  7ths  8ths      "
	  BYTE "         Plains  -  Below:  ****  ****  ****  ****  ****  ****  ****  ****      "
	  BYTE "                    Above:  ****  ****  ****  ****  ****  ****  ****  ****      "
	  BYTE "         Bobs    -  Below:  ****  ****  ****  ****  ****  ****  ****  ****      "
	  BYTE "                    Above:  ****  ****  ****  ****  ****  ****  ****  ****      "
	  BYTE "         Singles -  Below:  ****  ****  ****  ****  ****  ****  ****  ****      "
	  BYTE "                    Above:  ****  ****  ****  ****  ****  ****  ****  ****      "
	  BYTE "                                                                                "
	  BYTE "MINIMUM  Musical Rows:     crus  5678s 6578s 4678s 6478s 4578s 5478s 3578s 3478s"
	  BYTE "                         ***/*** **/** **/** **/** **/** **/** **/** **/** **/**"
	  BYTE "  Q W T B H R 6 7 2 3    5678cmb 8765s 7568s 7658s 2345s 5432s 3456s 6543s      "
	  BYTE "  * * * * * * * * * *    ***/*** **/** **/** **/** **/** **/** **/** **/**      "
disinc	  BYTE "                                                                                "
	  BYTE "INCLUDE  Courseheads matching masks below (suffix with +/- to restrict nature)  "
	  BYTE "                                                                                "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "         12345678+  12345678+                                                   "
	  BYTE "                                                                                "
	  BYTE "                                                                                "
dishlp	  BYTE "Function Description                        Keys available                      "
	  BYTE "                                                                                "
	  BYTE "    F1   This help summary screen           none                                "
	  BYTE "    F2   Cycle through available methods    none                                "
	  BYTE "    F3   Define exclusion criteria          X,x,0-9,space,left,right,up,down    "
	  BYTE "                                            tab,backtab,home,end,enter          "
	  BYTE "Alt F3   Define inclusion criteria          +,-,?,0-9,space,left,right,up,down  "
	  BYTE "                                            tab,backtab,home,end,enter          "
	  BYTE "    F4   Build composing table              none                                "
	  BYTE "    F5   Display composition list           left,right,up,down,page up,page down"
	  BYTE "                                            home,end,enter,space,delete         "
	  BYTE " Enter   Display composition from list      left,right,up,down,page up,page down"
	  BYTE "                                            enter,space,delete                  "
	  BYTE "    F6   Display composition in progress    none                                "
	  BYTE "    F7   Music analysis (not yet available) none                                "
	  BYTE "    F8   Display composing table            left,right,up,down,page up,page down"
	  BYTE "                                            home,end                            "
	  BYTE "    F9   Suspend/Resume composition         none                                "
	  BYTE "    F10  Quit SMC                           none                                "
	  BYTE "                                                                                "
	  BYTE "                                                                                "
banner	  BYTE "ษออออออออออออออออออออออออออออออออออออออออออป"
	  BYTE "บ                                          บ"
	  BYTE "บ          SINGLE METHOD COMPOSER          บ"
	  BYTE "บ                                          บ"
	  BYTE "บ             SMC  Version 1.2             บ"
	  BYTE "บ                                          บ"
	  BYTE "บ  Copyright (C)  Graham A C John 1993-98  บ"
	  BYTE "บ                                          บ"
	  BYTE "ศออออออออออออออออออออออออออออออออออออออออออผ"
;*********Input Fields
;                    Ed,Li,Co,Le,Nx,Pr,Do,Up
flddef		BYTE 01,05,27,05,01,00,112,00	  ;00 Length Below
		BYTE 01,05,41,05,02,00,112,01	  ;01 Length Above
		BYTE 02,05,61,01,109,01,110,02	  ;02 Longest yet
		BYTE 02,09,36,01,04,111,10,112	  ;03 Rounds @ H
		BYTE 02,09,38,01,05,03,11,112	  ;04 Rounds @ B
		BYTE 02,09,40,01,06,04,11,112	  ;05 Rounds @ I
		BYTE 02,09,42,01,07,05,12,112	  ;06 Rounds @ L
		BYTE 02,09,57,01,08,06,14,110	  ;07 87s
		BYTE 02,09,65,01,09,07,16,110	  ;08 82s
		BYTE 02,09,73,01,10,08,17,111	  ;09 83s
		BYTE 02,11,31,01,11,09,19,03      ;10 1 parts
		BYTE 02,11,37,01,12,10,20,03      ;11 2 parts
		BYTE 02,11,43,01,13,11,21,03      ;12 3 parts
		BYTE 02,11,49,01,14,12,22,03      ;13 4 parts
		BYTE 02,11,55,01,15,13,23,07	  ;14 5 parts
		BYTE 02,11,61,01,16,14,24,07      ;15 6 parts
		BYTE 02,11,67,01,17,15,25,08      ;16 7 parts
		BYTE 02,11,73,01,18,16,26,09      ;17 10parts
		BYTE 02,11,79,01,19,17,26,09      ;18 12parts
		BYTE 01,14,28,04,20,18,27,10      ;19 plains below
		BYTE 01,14,34,04,21,19,28,11      ;20
		BYTE 01,14,40,04,22,20,29,12      ;21
		BYTE 01,14,46,04,23,21,30,13      ;22
		BYTE 01,14,52,04,24,22,31,14      ;23
		BYTE 01,14,58,04,25,23,32,15      ;24
		BYTE 01,14,64,04,26,24,33,16      ;25
		BYTE 01,14,70,04,27,25,34,17      ;26
		BYTE 01,15,28,04,28,26,35,19      ;27 plains above
		BYTE 01,15,34,04,29,27,36,20      ;28
		BYTE 01,15,40,04,30,28,37,21      ;29
		BYTE 01,15,46,04,31,29,38,22      ;30
		BYTE 01,15,52,04,32,30,39,23      ;31
		BYTE 01,15,58,04,33,31,40,24      ;32
		BYTE 01,15,64,04,34,32,41,25      ;33
		BYTE 01,15,70,04,35,33,42,26      ;34
		BYTE 01,16,28,04,36,34,43,27      ;35 bobs below
		BYTE 01,16,34,04,37,35,44,28      ;36
		BYTE 01,16,40,04,38,36,45,29      ;37
		BYTE 01,16,46,04,39,37,46,30      ;38
		BYTE 01,16,52,04,40,38,47,31      ;39
		BYTE 01,16,58,04,41,39,48,32      ;40
		BYTE 01,16,64,04,42,40,49,33      ;41
		BYTE 01,16,70,04,43,41,50,34      ;42
		BYTE 01,17,28,04,44,42,51,35      ;43 bobs above
		BYTE 01,17,34,04,45,43,52,36      ;44
		BYTE 01,17,40,04,46,44,53,37      ;45
		BYTE 01,17,46,04,47,45,54,38      ;46
		BYTE 01,17,52,04,48,46,55,39      ;47
		BYTE 01,17,58,04,49,47,56,40      ;48
		BYTE 01,17,64,04,50,48,57,41      ;49
		BYTE 01,17,70,04,51,49,58,42      ;50
		BYTE 01,18,28,04,52,50,59,43      ;51 singles below
		BYTE 01,18,34,04,53,51,60,44      ;52
		BYTE 01,18,40,04,54,52,61,45      ;53
		BYTE 01,18,46,04,55,53,62,46      ;54
		BYTE 01,18,52,04,56,54,63,47      ;55
		BYTE 01,18,58,04,57,55,64,48      ;56
		BYTE 01,18,64,04,58,56,65,49      ;57
		BYTE 01,18,70,04,59,57,66,50      ;58
		BYTE 01,19,28,04,60,58,67,51      ;59 singles above
		BYTE 01,19,34,04,61,59,69,52      ;60
		BYTE 01,19,40,04,62,60,71,53      ;61
		BYTE 01,19,46,04,63,61,73,54      ;62
		BYTE 01,19,52,04,64,62,75,55      ;63
		BYTE 01,19,58,04,65,63,77,56      ;64
		BYTE 01,19,64,04,66,64,79,57      ;65
		BYTE 01,19,70,04,67,65,81,58      ;66
		BYTE 01,22,25,03,68,66,93,59      ;67 crusb
		BYTE 01,22,29,03,69,67,94,59      ;68
		BYTE 01,22,33,02,70,68,95,60      ;69 5678s
		BYTE 01,22,36,02,71,69,96,60      ;70
		BYTE 01,22,39,02,72,70,97,61      ;71 6578s
		BYTE 01,22,42,02,73,71,98,61      ;72
		BYTE 01,22,45,02,74,72,99,62      ;73 4678s
		BYTE 01,22,48,02,75,73,100,62     ;74
		BYTE 01,22,51,02,76,74,101,63     ;75 6478s
		BYTE 01,22,54,02,77,75,102,63     ;76
		BYTE 01,22,57,02,78,76,103,64     ;77 4578s
		BYTE 01,22,60,02,79,77,104,64     ;78
		BYTE 01,22,63,02,80,78,105,65     ;79 5478s
		BYTE 01,22,66,02,81,79,106,65     ;80
		BYTE 01,22,69,02,82,80,107,66     ;81 3578s
		BYTE 01,22,72,02,83,81,108,66     ;82
		BYTE 01,22,75,02,84,82,108,66     ;83 3478s
		BYTE 01,22,78,02,85,83,108,66     ;84
		BYTE 01,24,02,01,86,84,85,67      ;85 8 bell combinations
		BYTE 01,24,04,01,87,85,86,67      ;86      
		BYTE 01,24,06,01,88,86,87,67      ;87  
		BYTE 01,24,08,01,89,87,88,67      ;88      
		BYTE 01,24,10,01,90,88,89,67      ;89  
		BYTE 01,24,12,01,91,89,90,67      ;90      
		BYTE 01,24,14,01,92,90,91,67      ;91  
		BYTE 01,24,16,01,113,91,92,67	  ;92
		BYTE 01,24,25,03,94,114,93,67	  ;93 5678 combs
		BYTE 01,24,29,03,95,93,94,68      ;94      
		BYTE 01,24,33,02,96,94,95,69      ;95 8765s
		BYTE 01,24,36,02,97,95,96,70      ;96      
		BYTE 01,24,39,02,98,96,97,71      ;97 7568s 
		BYTE 01,24,42,02,99,97,98,72      ;98      
		BYTE 01,24,45,02,100,98,99,73     ;99 7658s 
		BYTE 01,24,48,02,101,99,100,74    ;100     
		BYTE 01,24,51,02,102,100,101,75   ;101 2345s
		BYTE 01,24,54,02,103,101,102,76   ;102     
		BYTE 01,24,57,02,104,102,103,77   ;103 5432s
		BYTE 01,24,60,02,105,103,104,78   ;104     
		BYTE 01,24,63,02,106,104,105,79   ;105 3456s
		BYTE 01,24,66,02,107,105,106,80   ;106     
		BYTE 01,24,69,02,108,106,107,81   ;107 6543s
		BYTE 01,24,72,02,108,107,108,82   ;108
		BYTE 02,05,79,01,112,002,111,109  ;109 Optimisation
		BYTE 02,07,61,01,111,112,007,002  ;110 Reverse
		BYTE 02,07,79,01,003,110,009,109  ;111 Rotation
		BYTE 02,07,35,01,110,109,003,000  ;112 Tenors split courseheads
		BYTE 01,24,18,01,114,92,113,67	  ;113 8 bell co=24653
		BYTE 01,24,20,01,93,113,114,67	  ;114 8 bell co=35642
;*********Status and Message Texts
stahlp	  BYTE STATIC,"Help Summary        "
stamet	  BYTE STATIC,"Define Method       "
staexc	  BYTE STATIC,"Define Exclusions   "
stainc	  BYTE STATIC,"Define Inclusions   "
stabld    BYTE STATIC,"Compiling Table     "
stalst    BYTE STATIC,"Composition List    "
stamus    BYTE STATIC,"Music Analysis      "
stacmp	  BYTE STATIC,"Composition Display "
stadyn	  BYTE STATIC,"Current Composition "
statab    BYTE STATIC,"Table Display       "
staquit   BYTE STATIC,"Quit                "
staflh    BYTE STATIC,"Adding FLHs to Table"
stacmpg   BYTE FLASH ,"Composing           "
stasusp   BYTE STATIC,"Composing Suspended "
staclear  BYTE STATIC,"                    "
mess0     BYTE colinf,"                                                            "
mess1	  BYTE colwrn,"FLH table array overflow. Increase lead/call exclusions     "
mess2	  BYTE colwrn,"FLH list overflow. Modify method or increase list size      "
mess3     BYTE colwrn,"Composition search exhausted                                "
mess4     BYTE colwrn,"Composition List Full. Delete some compositions to resume   "
mess5     BYTE colwrn,"Composition table must be recompiled (F4) to effect changes "
mess6	  BYTE colwrn,"Critical error, contact author. Press F10 to exit           "
;*********Lead Exclusions
exchnd    WORD FALSE              ;Exclude Rounds at Handstroke
excbak    WORD FALSE              ;Exclude Rounds at Backstroke
excint    WORD FALSE              ;Exclude Internal Rounds
excleh    WORD FALSE              ;Exclude Leadhead Rounds
exc87s    WORD TRUE               ;Exclude 87s at Backstroke
exc82s    WORD FALSE              ;Exclude 82s
exc83s    WORD FALSE              ;Exclude 83s
exctschs  WORD TRUE		  ;Exclude tenors split courseheads
incmask	  DWORD 0F0h,31 DUP(0FFFFFFFFh);Include coursehead masks - tenor pos masked if nature irrelevent
inccomp   DWORD 010h,31 DUP(048372615h);Include coursehead fixed bells - tenor set to F if -ve lhs only
;*********Call Exclusions
exccal    BYTE 00h,00h,00h        ;Exclude Plains, Bobs, Singles (bit strings)
exccalcnt WORD FALSE		  ;Set to TRUE if any max/min call values are set
excmaxp   WORD 8 DUP (9999)	  ;Maximum no of Plains in pos 1-8 (1=total for any pos)
excmaxb   WORD 8 DUP (9999)       ;Maximum no of Bobs in pos 1-8
excmaxs   WORD 8 DUP (9999)	  ;Maximum no of Singles in pos 1-8
excminp   WORD 8 DUP (0)	  ;Minimum no of Plains in pos 1-8
excminb   WORD 8 DUP (0)          ;Minimum no of Bobs in pos 1-8
excmins   WORD 8 DUP (0)          ;Minimum no of Singles in pos 1-8
;*********Composition Exclusions
excbest   WORD TRUE		  ;Exclude best/longest so far option
excmin    WORD 5000               ;Minimum Composition Length
excmax    WORD 5376               ;Maximum Composition Length
excpar    WORD 00h                ;Exclude parts - bits (only 12,10,7,6,5,4,3,2,1 valid)
excparmax WORD 40320		  ;Maximum part length (calculated)
excmus8   WORD 0		  ;Exclude compositions without specific rows
exc5678b  WORD 0                  ;Minimum (5678) combinations - back
exc5678f  WORD 0                  ;Minimum (5678) combinations - front
exccrub   BYTE 0                  ;Minimum crus - back
exccruf   BYTE 0                  ;Minimum crus - front
excmusb4  BYTE 15 DUP (0)         ;Minimum 4 bell combinations - back
excmusf4  BYTE 15 DUP (0)         ;Minimum 4 bell combinations - front
;*********Optimisation/Rotation etc
exccom	  WORD FALSE		  ;Exclude composition until rebuild
excban	  WORD FALSE		  ;Program Banner Display
excsort   WORD FALSE              ;Sort composition list if false
excopt	  WORD FALSE		  ;Optimise if FALSE
excrev	  WORD TRUE		  ;Reverse composing if FALSE (ie S,B,P)
optsync   WORD FALSE              ;Course synchronization
excsync   WORD FALSE		  ;Exclude synchronization
excrot	  WORD FALSE		  ;Generate composition rotations if FALSE
evaseq	  DWORD 0		  ;Sequence number of valid compositions found
evarot	  DWORD 0		  ;Count of valid rotations
rotftt	  WORD FALSE		  ;Rotation first time through indicator
rotcnt	  DWORD 0		  ;Count (& Ptr) of number of rotations
rotarray  WORD 2520 DUP (0)	  ;array of composition pointers for rotations
;*********Composition Weighting Factors for scoring (0 to 17 binary weight)
wgtlenhi  BYTE  0                 ;Weight for longer compositions
wgtlenlo  BYTE  1                 ;Weight for shorter compositions
wgtmus87  BYTE  17                ;Weight of 87s at backstroke
wgtmus82  BYTE	5		  ;Weight of 82s
wgtmus83  BYTE	5		  ;Weight of 83s
wgt5678b  BYTE  5                 ;Weight of (5678) combinations - back
wgt5678f  BYTE  5                 ;Weight of (5678) combinations - front
wgtpar    BYTE  17                ;Weight of parts in composition
wgtcrub   BYTE	18		  ;Weight of crus - back
wgtcruf   BYTE  16                ;Weight of crus - front
wgtmusb4  BYTE	18,18,14,14,13,13,14,14,15,15,15,12,12,12,12
wgtmusf4  BYTE	18,18,12,12,11,11,12,12,13,13,13,12,12,12,12
;*********4 Bell Music Masks (Note DWORDS are reversed)
musmsk4   DWORD 08070605h,08070506h,08070504h,08070604h,
			 08070406h,08070405h,08070503h,08070403h,
			 05060708h,08060507h,08050607h,05040302h,
			 02030405h,06050403h,03040506h
muscnt4   WORD LENGTHOF musmsk4
;*********8 Bell Music Masks - compressed into a dword by shift and overlay
;               Rounds     Queens     Whitt      Tittums
;               12345678   13572468   12753468   15263748
;               Backrounds Hagdyke    Rollercstr
;               87654321   12563478   14327658   65432178
;			   co=24653   co=35642
;		76543218   17823456   18723456	 00000000
;
;               00000000   00000000   00000000   00000000
musmsk8   DWORD 00000000h, 78563412h, 58762413h, 68245713h,
		51627384h, 68572413h, 28354617h, 38475162h,
		48516273h, 26857413h, 26758413h, 00000000h,
		00000000h, 00000000h, 00000000h, 00000000h
muscnt8   WORD  LENGTHOF musmsk8
datseg1   ENDS
lstgrp    GROUP lstseg1
		EXTERNDEF   EXTLST:BYTE
		ASSUME es:lstgrp
lstseg1   SEGMENT COMMON  
lstsiz    WORD  ?                 ;Size of composition definition in paragraphs
lstlen    WORD  ?                 ;Length of composition
lstscore  DWORD ?		  ;Composition score
lstcalls  WORD	3 DUP(?)	  ;Number of calls (P,B,S) in composition
lstmus8   WORD  ?                 ;8 bell rows in composition
lstmus87  WORD  ?                 ;Number of 87s at backstroke
lstmus82  WORD  ?                 ;Number of 82s
lstmus83  WORD  ?                 ;Number of 83s
lst5678b  WORD  ?                 ;Number of (5678) combinations - back
lst5678f  WORD  ?                 ;Number of (5678) combinations - front
lstpar    BYTE  ?                 ;Number of parts in composition
lstcrub   BYTE  ?                 ;Number of crus - back
lstcruf   BYTE  ?                 ;Number of crus - front
lstmusb4  BYTE  15 DUP (?)        ;Number of 4 bell combinations - back
lstmusf4  BYTE  15 DUP (?)        ;Number of 4 bell combinations - front
lsthour   WORD	?		  ;Timestamp hours 0-32767
lstmin    BYTE  ?                 ;Timestamp minutes 0-59
lstsec    BYTE  ?                 ;Timestamp second 0-59
lstseq	  DWORD ?		  ;Sequence number of composition
lstrot	  WORD	?		  ;Sequence number of rotation
lstcal	  BYTE	?		  ;List of calls (with workspace allowance)
lstrest   BYTE	65536 - 70 DUP(?)
lstseg1   ENDS
cmpgrp    GROUP cmpseg1,cmpseg2,cmpseg3,cmpseg4,cmpseg5,cmpseg6,cmpseg7,cmpseg8
		ASSUME fs:cmpgrp
cmpseg1   SEGMENT COMMON
		EXTERNDEF EXTCMP:BYTE
;*********Composition Table
						    ;Composition Table length in bytes (485376)
cmpleh	  DWORD 2 dup (?)	  ;Leadhead Row (1st DWORD = FFFFFFFFh if excluded)
cmpcal    WORD 3 DUP(?)           ;LHOs for plain, bobbed and singled lead
cmpflh    WORD 18 DUP(?)          ;LHOs of false leads
cmpfno    BYTE ?                  ;Number of false lead heads
cmpclo    BYTE ?                  ;Call offset (CLO) 0=Plain,1=Bob,2=Single
cmplen    WORD ?                  ;Number of rows in lead (to rounds if present)
cmpmus8   WORD ?                  ;Special rows in lead (8 bell masks)
						    ;bit  0 - 12345678 Rounds
						    ;     1 - 13572468 Q
						    ;     2 - 12753468 W
						    ;     3 - 15263748 T
						    ;     4 - 87654321 B
						    ;     5 - 12563478 H
						    ;     6 - 14327658 R
						    ;     7 - 65432178
						    ;	  8 - 76543218
						    ;	  2 - 17823456 co=24653
						    ;	  3 - 18723456 co=35642
						    ;	 14 - Nature of leadhead (set if -ve)
						    ;    15 - Included in composition flag
cmppar    BYTE ?                  ;Number of parts if leadhead is parthead
cmpmus87  BYTE ?                  ;87s at backstroke
cmpmus82  BYTE ?                  ;82s in lead
cmpmus83  BYTE ?                  ;83s in lead
cmpcrub   BYTE ?                  ;crus for lead off back
cmpcruf   BYTE ?                  ;crus for lead off front
cmp5678b  BYTE ?                  ;5678 combinations back
cmp5678f  BYTE ?                  ;5678 combinations front
cmpmusb4  BYTE 15 DUP(?)          ;back 4 Musical counts for lead
cmpmusf4  BYTE 15 DUP(?)          ;front 4 Musical counts for lead
cmptenor  BYTE ?                  ;position of tenor - 1. eg 7 = Home
cmpspare  BYTE ?
cmprest   BYTE ((5055*96)-(65536*7)) DUP (?)
;***************************************************************
cmpseg1   ENDS
cmpseg2   SEGMENT COMMON
		BYTE     65536 DUP (?)
cmpseg2   ENDS
cmpseg3   SEGMENT   COMMON
		BYTE   65536 DUP (?)
cmpseg3   ENDS
cmpseg4   SEGMENT   COMMON
		BYTE   65536 DUP (?)
cmpseg4   ENDS
cmpseg5   SEGMENT COMMON
		BYTE   65536 DUP (?)
cmpseg5   ENDS
cmpseg6   SEGMENT COMMON
		BYTE   65536 DUP (?)
cmpseg6   ENDS
cmpseg7   SEGMENT   COMMON
		BYTE   65536 DUP (?)
cmpseg7   ENDS
cmpseg8   SEGMENT   COMMON
		BYTE   65536 DUP (?)
cmpseg8   ENDS
