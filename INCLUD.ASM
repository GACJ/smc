;*********************************************************
;* SMC - Single Method Composer                          *
;* Copyright Graham A C John 1991-98 All rights reserved *
;*********************************************************
TITLE INCLUD - Enter Composition Inclusion Criteria
;
;    This routine displays the current inclusions for amendment. These are
;    all coursehead masks applied in the build phase. It does not guarantee
;    that the courseheads can be reached. Exclusions take priority over
;    Inclusions
;
.MODEL SMALL
.486
INCLUDE smc.inc                   ;Data Definitions
INCLUDE dos.inc
INCLUDE bios.inc
FIRSTLIN EQU 7
LASTLIN  EQU 22
FIRSTCOL EQU 10
LASTCOL  EQU 21
.CODE
EXTERN getkey:near, status:near, display:near, dispban:near
ASSUME es:nothing
includ PROC
  mov curlin,FIRSTLIN		  ;set first field for edit
  mov curcol,FIRSTCOL
  mov curpos,0
  mov si,offset stainc		  ;Display status
  call status
  mov es,scrseg                   ;Write to program screen buffer
newpage:
  mov discol,colleg               ;display colour for legends
  mov edi,160*4 		  ;1st 4 lines of Screen are reserved
  mov esi,offset disinc		  ;Display exclusion legends
  mov ah,discol                   ;Clear Screen to standard colour
  mov ecx,80*21
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  mov discol,colinp		  ;Display current inclusions
  xor ebx,ebx			  ;Pointer to incusions
  mov di,(160*FIRSTLIN)+(FIRSTCOL*2)
  mov ecx,32
  .REPEAT			  ;display each inclusion row in turn
    push cx
    mov eax,inccomp[ebx*4]	  ;decompress the row
    and eax,0F0F0F0Fh		  ;mask off last four bells
    add eax,30303030h		  ;convert to ascii
    mov natrow+4,eax		  ;store decompressed ascii row
    mov eax,inccomp[ebx*4]
    shr eax,4			  ;shift bells
    and eax,0F0F0F0Fh		  ;mask off first four bells
    add eax,30303030h		  ;convert to ascii
    mov natrow,eax		  ;store decompressed ascii row
    mov si,offset natrow+1
    mov ah,discol		  ;set input colour
    mov cx,6
    .REPEAT
      lodsb
      .IF al == "0"
	mov al,"?"		  ;set wildcard character
      .ENDIF
      stosw
    .UNTILCXZ
    add di,2
    mov al,BYTE PTR natrow+7	  ;check nature of mask
    .IF al == "0"		  ;0 = no nature specified
      mov al," "
    .ELSE
      .IF al == "8"		  ;8 = positive courseheads only
      mov al,"+"
      .ELSE			  ;F = negative courseheads only
      mov al,"-"
      .ENDIF
    .ENDIF
    stosw			  ;display nature
    pop cx
    .IF cx & 1			  ;if cx is odd then adjust to next line
      add di,122
    .ELSE			  ;otherwise adjust to next column
      add di,6
    .ENDIF
    inc bx			  ;next inccomp
  .UNTILCXZ
dispscreen:
  mov si,offset stainc		 ;Display status and screen
  call status
  .IF !excban			  ;Display banner first time through
    call dispban
    mov excban,TRUE
  .ENDIF
  movzx ebx,curfld		  ;Set cursor position
  mov dh,curlin			  ;line
  mov dl,curcol 		  ;column
  add dl,curpos                   ;plus offset
  @SetCsrPos
labgetkey:
  call getkey
  .IF  dx == FnEnter \            ;Enter
    || dx == FnTab                ;or Tab
entereturn:
    call incedit		  ;complete field edits
    .IF curcol == FIRSTCOL	  ;set next field
      mov curcol,LASTCOL
    .ELSE
      mov curcol,FIRSTCOL
      .IF curlin < LASTLIN
	inc curlin
      .ELSE
	mov curlin,FIRSTLIN
      .ENDIF
    .ENDIF
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF  dx == FnBackTab            ;BackTab (Previous)
backtab:
    call incedit		    ;complete field edits
    .IF curcol == LASTCOL	    ;set previous field
      mov curcol,FIRSTCOL
    .ELSE
      mov curcol,LASTCOL
      .IF curlin > FIRSTLIN
	dec curlin
      .ELSE
	mov curlin,LASTLIN
      .ENDIF
    .ENDIF
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF  dx == FnRight              ;Cursor right
right:
    .IF curpos < 6
      inc curpos
      .IF curpos == 6
	inc curpos
      .ENDIF
      jmp dispscreen
    .ELSE
      jmp entereturn
    .ENDIF
  .ENDIF
  .IF  dx == FnLeft               ;Cursor left
    .IF curpos > 0
      dec curpos
      .IF curpos == 6
	dec curpos
      .ENDIF
      jmp dispscreen
    .ELSE
      call incedit		  ;goto end of previous field
      .IF curcol == LASTCOL
	mov curcol,FIRSTCOL
      .ELSE
	mov curcol,LASTCOL
	.IF curlin > FIRSTLIN
	  dec curlin
	.ELSE
	  mov curlin,LASTLIN
	.ENDIF
      .ENDIF
      mov curpos,7
      jmp newpage
    .ENDIF
  .ENDIF
  .IF  dx == FnDown               ;Cursor down
    call incedit		    ;complete field edits
    .IF curlin < LASTLIN
      inc curlin
    .ELSE
      mov curlin,FIRSTLIN
    .ENDIF
    jmp newpage
  .ENDIF
  .IF  dx == FnUp                 ;Cursor up
    call incedit		    ;complete field edits
    .IF curlin > FIRSTLIN
      dec curlin
    .ELSE
      mov curlin,LASTLIN
    .ENDIF
    jmp newpage
  .ENDIF
  .IF dx == FnHome		  ;Home (first field)
    call incedit
    mov curcol,FIRSTCOL
    mov curlin,FIRSTLIN
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF dx == FnEnd                 ;End (last field)
    call incedit
    mov curcol,LASTCOL
    mov curlin,LASTLIN
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF FnNew                       ;exit for new function
    call incedit
    jmp endproc                   ;Continue until another function requested
  .ENDIF
				  ;Restrict character set
  .IF dh == "?" || dh == "+" || dh == "-" || dh == " " \
      || (dh >= "0" && dh <= "9")
    .IF curpos != 7
      .IF dh < "2" || dh >"7"
	mov dh,"?"
      .ENDIF
    .ELSE
      .IF dh == "+" || dh == "="
	mov dh,"+"
      .ELSE
	.IF dh == "-" || dh == "_"
	  mov dh,"-"
	.ELSE
	  mov dh," "
	.ENDIF
      .ENDIF
    .ENDIF
    movzx eax,curlin		  ;keep and display entered character
    mov cx,160
    push dx
    mul cx
    pop dx
    movzx ecx,curcol		  ;column
    shl ecx,1
    add eax,ecx
    movzx ecx,curpos		  ;plus position in field
    shl ecx,1
    add eax,ecx
    mov es:[eax],dh
    jmp right
  .ENDIF
  jmp labgetkey 		  ;try another key
endproc:
  push dx
  @SetCsrPos 0,25		  ;Set Cursor off-screen again
  pop dx
  ret
includ ENDP
incedit PROC
    pushad
    movzx eax,curlin		  ;store field
    mov cx,160
    mul cx
    movzx ecx,curcol		  ;column
    shl ecx,1
    add eax,ecx
    add eax,12
    mov ecx,8
    .REPEAT
      mov dl,es:[eax]
      .IF dl == "?"		  ;change wildcards to zeros
	xor dl,dl
      .ENDIF
      mov BYTE PTR natrow[ecx-1],dl
      sub eax,2
    .UNTILCXZ
    and natrow,0F0F0F0Fh	  ;convert from ascii to binary
    and natrow+4,0F0F0F0Fh
    mov edx,natrow		  ;compress coursehead
    shl edx,4
    add edx,natrow+4
    mov al,es:[eax+18]		  ;check nature
    .IF al == " "
      and edx,0F0FFFFFFh	  ;any nature
    .ENDIF
    .IF al == "-"
      or edx,0F000000h		  ;negative courseheads only
    .ENDIF
    movzx eax,curlin		  ;calculate position in inccomp array
    sub eax,FIRSTLIN
    shl eax,1			  ;multiply by 2 columns
    .IF curcol == LASTCOL
      inc eax			  ;adjust to second column
    .ENDIF
    mov inccomp[eax*4],edx	  ;store compare row
    mov ecx,8			  ;create mask
    .REPEAT			  ;change each bell present in inccomp to F
      .IF edx & 0Fh
	or edx,0Fh
      .ENDIF
      ror edx,4
    .UNTILCXZ
    mov incmask[eax*4],edx	  ;store mask row
    popad
    ret
incedit ENDP
END
