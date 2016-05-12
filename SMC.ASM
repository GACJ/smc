	TITLE        SMC - Single Method Composer - Major Methods
;
;    The following segment model is used
;
;    CODE              cs
;    DGROUP              
;    STACK	       ss
;    DATGRP datseg1    ds
;    LSTGRP lstseg1    es
;    CMPGRP cmpseg1    fs (gs)   Composition Array
;           cmpseg2
;           cmpseg3        5056 elements of 96 bytes
;           cmpseg4        = 485376 bytes or 474K
;           cmpseg5        = 1*26K(26624) segment + 7 64K segments
;           cmpseg6
;           cmpseg7
;           cmpseg8
;
	.MODEL      SMALL,FARSTACK
	.486
	.STACK
	INCLUDE   dos.inc         ;DOS Calls
	INCLUDE   bios.inc        ;BIOS Calls
	INCLUDE   smc.inc         ;Data Definitions
	.CODE
	EXTERN bldtab:NEAR,distab:NEAR,method:NEAR,comgen:NEAR,dislst:near
	EXTERN exclud:near,disdyn:near,includ:near
	.STARTUP
  mov ax,datgrp                   ;Initialise segment registers
  mov ds,ax
  mov datseg,ax
  mov scrseg,ax
  mov ax,lstgrp
  mov lstseg,ax
  mov ax,cmpgrp
  mov fs,ax
  mov gs,ax
  mov cmpseg,ax
  @Setmode 03h                     ;Set Screen Mode
  @SetCsrPos 0,25		   ;Set Cursor position off-screen
  mov es,scrseg                    ;Display Status Titles
  mov di,0
  mov si,offset distit4
  mov cx,240
  mov ah,colleg
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
	mov  dx,FnMethod    ;Force first function into method
	.WHILE 1            ;Loop forever
	.IF  dx == FnDisHlp
	  call disphlp	    ;Display help summary
	.ENDIF
	.IF  dx == FnMethod
	call method         ;Enter method definition
	.ENDIF       
	.IF  dx == FnExclud
	call exclud	    ;Enter exclusions
	.ENDIF
	.IF  dx == FnInclud
	call includ	    ;Enter inclusions
	.ENDIF
	.IF  dx == FnBldtab
	   call bldtab	    ;Build composition table
	.ENDIF
	.IF  dx == FnDislst
	call dislst         ;Display Composition List
	.ENDIF
	.IF  dx == FnDismus
	call dismus         ;Display Music Analysis 
	.ENDIF
	.IF  dx == FnDistab
	call distab         ;Display Composition Table
	.ENDIF
	.IF  dx == FnDisdyn
	call disdyn	    ;Dynamic Display of Current Composition
	.ENDIF
	.IF  dx == FnExit
          @Setmode 03h                     ;Clear Screen on exit
	  .EXIT
	.ENDIF
	.ENDW
getkey     PROC USES AX
;     This routine is for keyboard get character. It runs the background
;     function COMGEN whenever waiting for a key depression. COMGEN will
;     drop out if a key is pressed, then this routine will check for
;     extended keycodes and return a word in dx for the key pressed.
  mov FnNew,FALSE                 ;Clear New function indicator
@@:
  mov dx,0
  @GetStatus                      ;Check whether a key has been pressed
  or al,al
  .IF ZERO? \                     ;Compose while no character waiting
    && gencom && !genend && !genofl
				  ;and composition not suspended, ended or list overflow
    call comgen 		  ;Composition routine will drop out if key pressed
				  ;or if a new composition has been found
  .ENDIF			  ;or compositions exhausted or dynamic display in effect
  .IF dx == 0			  ;keypress detected by getkey or comgen
    @GetChar 0                    ;Get Keystroke no echo
    mov dh,al
    .IF al == 0
	 @GetChar 0               ;Get Extended Keystroke
	 mov dl,al
	    .IF  dx == FnComgen && !exccom
	   xor gencom,TRUE	  ;Toggle composition
	   mov si,offset stasusp  ;issue suspended status (overwritten if not)
	   call status
	   jmp @B
	 .ENDIF
	 .IF  (dx == FnMethod) \
	   || (dx == FnDisHlp) \
	   || (dx == FnBldtab) \
	   || (dx == FnDisLst) \
	   || (dx == FnDisTab) \
	   || (dx == FnDisMus) \
	   || (dx == FnDisDyn) \
	   || (dx == FnExclud) \
	   || (dx == FnInclud) \
	   || (dx == FnExit)
	mov FnNew,TRUE           ;Indicate New function
	 .ENDIF
    .ENDIF
  .ENDIF
  ret                              ;Key is returned in dx
getkey ENDP
dismus PROC
  mov si,offset stamus
  call status
  .REPEAT
    call getkey                   ;Wait on keystroke
  .UNTIL FnNew
  ret
dismus ENDP
status PROC USES ax cx es si di;Indicate current status (si) on screen
  mov es,scrseg
  mov di,480
  mov cx,20
  mov ah,colsta
  or ah,[si]                      ;Set Flash bit for status concerned
  inc si
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  call display
  ret
status ENDP
message PROC USES ax cx es si di;Indicate current status (si) on screen
  mov es,scrseg
  mov di,520
  mov cx,60
  mov ah,[si]                     ;Message colour
  inc si
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  call display
  ret
message ENDP
display  PROC USES ds es cx di si
  xor si,si
  xor di,di
  mov es,dirseg
  mov ds,scrseg
  mov cx,1000
  rep movsd
  ret
display  ENDP
dispban PROC USES ES
  pushad
  mov es,dirseg
  mov ah,colstd			  ;display banner
  mov edi,9*160+36
  mov esi,offset banner
  mov ebx,9
  .REPEAT
    mov ecx,44
    .REPEAT
      lodsb
      stosw
    .UNTILCXZ
    add di,160-88
    dec bx
  .UNTIL bx == 0
  popad
  ret
dispban ENDP
disphlp PROC USES ES
  pushad
  mov es,scrseg
  mov di,160*4			  ;1st 4 lines of Screen are reserved
  mov ah,coltit			  ;Set colour for title
  mov si,offset dishlp		  ;Display title
  mov cx,80
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  mov cx,80*20			  ;Display remainder of help screen
  mov ah,colstd
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  mov si,offset stahlp		  ;Display Status and screen
  call status
  popad
  .REPEAT
    call getkey                   ;Wait on keystroke
  .UNTIL FnNew
  ret
disphlp ENDP
END
