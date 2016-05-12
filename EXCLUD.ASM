TITLE EXCLUD - Enter Composition Exclusion Criteria
;
;    This routine displays the current exclusions for amendment. There are
;    three types:
;        Composition exclusions (applied in evaluate phase - ie after each comp found)
;        Call exclusions (applied in build, compose and evaluate phases)
;        Lead exclusions (applied in build phase)
.MODEL SMALL
.386
INCLUDE smc.inc                   ;Data Definitions
INCLUDE dos.inc
INCLUDE bios.inc
.CODE
EXTERN getkey:near, status:near, display:near, binasc:near, dispban:near
EXTERN message:near
ASSUME es:nothing
exclud PROC
  mov curfld,0			  ;Set first field for edit
  mov curpos,0
  mov si,offset staexc            ;Display status
  call status
  mov es,scrseg                   ;Write to program screen buffer
newpage:
  mov discol,colleg               ;display colour for legends
  mov di,160*4                    ;1st 4 lines of Screen are reserved
  mov si,offset disexc            ;Display exclusion legends
  mov ah,discol                   ;Clear Screen to standard colour
  mov cx,80*21
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  mov diszero,"Z"                 ;Set standard zero suppression
  mov discol,colinp               ;Set input colour
  movzx eax,excmin                ;min length
  mov di,(160*5)+54
  mov ecx,5
  call binasc
  movzx eax,excmax                ;max length
  mov di,(160*5)+82
  mov ecx,5
  call binasc
  .IF excbest                     ;best so far
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  mov di,(160*5)+122
  mov ah,discol
  stosw
  .IF excopt                      ;optimisation
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  mov di,(160*5)+158
  mov ah,discol
  stosw
  .IF excrev			  ;reverse composition
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  mov di,(160*7)+122
  mov ah,discol
  stosw
  .IF excrot			  ;rotation
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  mov di,(160*7)+158
  mov ah,discol
  stosw
  .IF exctschs			 ;tenors split courseheads
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  mov di,(160*7)+70
  mov ah,discol
  stosw
  .IF exchnd			  ;rounds at hand,back,internal,leadhead
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  mov di,(160*9)+72
  mov ah,discol
  stosw
  .IF excbak
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  add di,2
  stosw
  .IF excint
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  add di,2
  stosw
  .IF excleh
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  add di,2
  stosw
  .IF exc87s                      ;87s at back
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  mov di,(160*9)+114
  stosw
  .IF exc82s                      ;82s
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  mov di,(160*9)+130
  stosw
  .IF exc83s                      ;83s
    mov al,"X"
  .ELSE
    mov al,"_"
  .ENDIF
  mov di,(160*9)+146
  stosw
  mov dx,excpar                   ;parts
  mov di,(160*11)+62
  mov cx,1
  .REPEAT
    .IF cx != 8 && cx != 9 && cx !=11
      .IF dx & 1
        mov al,"X"
      .ELSE
        mov al,"_"
      .ENDIF
      stosw
      add di,10
    .ENDIF
    shr dx,1
    inc cx
  .UNTIL cx == 13
  xor eax,eax                     ;Calls below and above
  xor si,si
  mov cx,4
  mov edi,(160*14)+56
  .REPEAT
    mov ax,excminp[si]
    call binasc
    add edi,12
    add si,2
  .UNTIL si == 16
  xor si,si
  mov edi,(160*15)+56
  .REPEAT
    mov ax,excmaxp[si]
    call binasc
    add edi,12
    add si,2
  .UNTIL si == 16
  xor si,si
  mov edi,(160*16)+56
  .REPEAT
    mov ax,excminb[si]
    call binasc
    add edi,12
    add si,2
  .UNTIL si == 16
  xor si,si
  mov edi,(160*17)+56
  .REPEAT
    mov ax,excmaxb[si]
    call binasc
    add edi,12
    add si,2
  .UNTIL si == 16
  xor si,si
  mov edi,(160*18)+56
  .REPEAT
    mov ax,excmins[si]
    call binasc
    add edi,12
    add si,2
  .UNTIL si == 16
  xor si,si
  mov edi,(160*19)+56
  .REPEAT
    mov ax,excmaxs[si]
    call binasc
    add edi,12
    add si,2
  .UNTIL si == 16
  movzx eax,exccrub               ;crus
  mov edi,(160*22)+50
  mov ecx,3
  call binasc
  add edi,6
  mov al,"/"
  mov ah,discol
  stosw
  movzx eax,exccruf
  call binasc
  add edi,8
  xor si,si                      ;4 bell combinations
  .REPEAT
    .IF si == 8
      mov edi,(160*24)+66
    .ENDIF
    movzx eax,excmusb4[si]
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,excmusf4[si]
    mov ecx,2
    call binasc
    add edi,6
    inc si
  .UNTIL si == 15
  movzx eax,exc5678b               ;5678 combinations
  mov edi,(160*24)+50
  mov ecx,3
  call binasc
  add edi,6
  mov al,"/"
  mov ah,discol
  stosw
  movzx eax,exc5678f
  call binasc
  mov dx,excmus8                  ;8 bell combinations
  mov di,(160*24)+4
  mov ah,discol
  shl dx,1                        ;ignore rounds bit
  mov cx,8
  .REPEAT
    shl dx,1
    .IF CARRY?
      mov al,"1"
    .ELSE
      mov al,"0"
    .ENDIF
      stosw
      add di,2
  .UNTILCXZ
dispscreen:
  mov si,offset staexc            ;Display status and screen
  call status
  .IF !excban			  ;Display banner first time through
    call dispban
    mov excban,TRUE
  .ENDIF
  movzx ebx,curfld		  ;Set cursor position
  mov dh,flddef[ebx*8+1]          ;line
  mov dl,flddef[ebx*8+2]          ;column
  add dl,curpos                   ;plus offset
  @SetCsrPos
labgetkey:
  call getkey
  .IF  dx == FnEnter \            ;Enter
    || dx == FnTab                ;or Tab
entereturn:
    call fedit                    ;complete field edits
    movzx ebx,curfld              ;set next field
    mov bl,flddef[ebx*8+4]
    mov curfld,bl
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF  dx == FnBackTab            ;BackTab (Previous)
backtab:
    call fedit                    ;complete field edits
    movzx ebx,curfld              ;set previous field
    mov bl,flddef[ebx*8+5]
    mov curfld,bl
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF  dx == FnRight              ;Cursor right
right:
    movzx ebx,curfld
    mov al,curpos
    inc al
    .IF al < flddef[ebx*8+3]
      inc curpos
    .ELSE
      jmp entereturn
    .ENDIF
    jmp dispscreen
  .ENDIF
  .IF  dx == FnLeft               ;Cursor left
    .IF curpos > 0
      dec curpos
    .ELSE
      jmp backtab
    .ENDIF
    jmp dispscreen
  .ENDIF
  .IF  dx == FnDown               ;Cursor down
    call fedit                    ;complete field edits
    movzx ebx,curfld              ;set down field
    mov bl,flddef[ebx*8+6]
    mov curfld,bl
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF  dx == FnUp                 ;Cursor up
    call fedit                    ;complete field edits
    movzx ebx,curfld              ;set up field
    mov bl,flddef[ebx*8+7]
    mov curfld,bl
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF dx == FnHome                ;Home (fierst field)
    call fedit
    mov curfld,0
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF dx == FnEnd                 ;End (last field)
    call fedit
    mov curfld,108
    mov curpos,0
    jmp newpage
  .ENDIF
  .IF FnNew                       ;exit for new function
    call fedit
    jmp endproc                   ;Continue until another function requested
  .ENDIF
				  ;Allow restricted character set
  .IF dh == " " || dh == "X" || dh == "x" || (dh >= "0" && dh <="9")
    movzx ebx,curfld		  ;keep and display entered character
    movzx eax,flddef[ebx*8+1]	  ;line
    mov cx,160
    push dx
    mul cx
    pop dx
    movzx ecx,flddef[ebx*8+2]     ;column
    shl ecx,1
    add eax,ecx
    movzx ecx,curpos              ;plus position in field
    shl ecx,1
    add eax,ecx
    mov es:[eax],dh
    jmp right
  .ENDIF
  jmp labgetkey                   ;Try another key
endproc:
  push dx
  @SetCsrPos 0,25		  ;Set Cursor off-screen again
  pop dx
  ret
exclud ENDP
fedit PROC
  movzx ebx,curfld                ;check validation type
  .IF flddef[ebx*8] == 1
    call fedit1
  .ELSE
    call fedit2
  .ENDIF
  ret
fedit ENDP
fedit1 PROC                       ;edit and convert field to binary
  pushad
  movzx eax,flddef[ebx*8+1]       ;line
  mov cx,160
  mul cx
  movzx edx,flddef[ebx*8+2]       ;column
  shl edx,1
  add eax,edx
  mov esi,eax
  mov edi,10
  xor eax,eax
  movzx ecx,flddef[ebx*8+3]       ;length of field
  .REPEAT
    push ecx
    movzx ecx, BYTE PTR es:[esi]  ;get characters in order
    sub cl,"0"
    .IF cl < 10                   ;valid character ?
      mul edi
      add eax,ecx
    .ENDIF
    add esi,2                     ;next character
    pop ecx
  .UNTILCXZ                       ;until end of field
  .IF eax > 40320                 ;limit maximum value
    mov eax,40320
  .ENDIF
  .IF bx == 0
    mov excmin,ax
    .IF ax < wrkmin \		  ;ensure that best so far <= minimum
     || excbest
      mov wrkmin,ax
    .ENDIF
  .ENDIF
  .IF bx == 1
    mov excmax,ax
    call maxpart
  .ENDIF
  .IF bx > 18 && bx < 27
    mov edx,ebx
    sub edx,19
    mov excminp[edx*2],ax
    .IF ax > excmaxp[edx*2]       ;if min > max
      mov excmaxp[edx*2],ax       ;set max = min
    .ENDIF
  .ENDIF
  .IF bx > 26 && bx < 35
    mov edx,ebx
    sub edx,27
    mov excmaxp[edx*2],ax
    .IF ax < excminp[edx*2]       ;if max < min
      mov excminp[edx*2],ax       ;set min = max
    .ENDIF
    mov ecx,edx
    mov dl,80h                    ;set call bits
    shr dl,cl
    .IF ax == 0
      or exccal,dl                ;exclude plains
      .IF cx == 0                 ;exclude all plains
        mov exccal,0FFh
        mov ecx,7
        .REPEAT
          mov excminp[ecx*2],ax
          mov excmaxp[ecx*2],ax
        .UNTILCXZ
      .ENDIF
    .ELSE
      not dl
      and exccal,dl               ;include plains
    .ENDIF
  .ENDIF
  .IF bx > 34 && bx < 43
    mov edx,ebx
    sub edx,35
    mov excminb[edx*2],ax
    .IF ax > excmaxb[edx*2]       ;if min > max
      mov excmaxb[edx*2],ax       ;set max = min
    .ENDIF
  .ENDIF
  .IF bx > 42 && bx < 51
    mov edx,ebx
    sub edx,43
    mov excmaxb[edx*2],ax
    .IF ax < excminb[edx*2]       ;if max < min
      mov excminb[edx*2],ax       ;set min = max
    .ENDIF
    mov ecx,edx
    mov dl,80h                    ;set call bits
    shr dl,cl
    .IF ax == 0
      or exccal+1,dl              ;exclude bobs
      .IF cx == 0                 ;exclude all bobs
        mov exccal+1,0FFh
        mov ecx,7
        .REPEAT
          mov excminb[ecx*2],ax
          mov excmaxb[ecx*2],ax
        .UNTILCXZ
      .ENDIF
    .ELSE
      not dl
      and exccal+1,dl              ;include bobs
    .ENDIF
  .ENDIF
  .IF bx > 50 && bx < 59
    mov edx,ebx
    sub edx,51
    mov excmins[edx*2],ax
    .IF ax > excmaxs[edx*2]       ;if min > max
      mov excmaxs[edx*2],ax       ;set max = min
    .ENDIF
  .ENDIF
  .IF bx > 58 && bx < 67
    mov edx,ebx
    sub edx,59
    mov excmaxs[edx*2],ax
    .IF ax < excmins[edx*2]       ;if max < min
      mov excmins[edx*2],ax       ;set min = max
    .ENDIF
    mov ecx,edx
    mov dl,80h                    ;set call bits
    shr dl,cl
    .IF ax == 0
      or exccal+2,dl              ;exclude singles
      .IF cx == 0                 ;exclude all singles
        mov exccal+2,0FFh
        mov ecx,7
        .REPEAT
          mov excmins[ecx*2],ax
          mov excmaxs[ecx*2],ax
        .UNTILCXZ
      .ENDIF
    .ELSE
      not dl
      and exccal+2,dl               ;include singles
    .ENDIF
  .ENDIF
  .IF ax > 576                      ;no following field can be greater than 576
    mov ax,576
  .ENDIF
  .IF bx == 93                      ;5678 combinations
    mov exc5678b,ax
  .ENDIF
  .IF bx == 94
    mov exc5678f,ax
  .ENDIF
  .IF ax > 144                      ;no following field can be greater than 144
    mov ax,144
  .ENDIF
  .IF bx == 67                      ;4 bell combinations
    mov exccrub,al
  .ENDIF
  .IF bx == 68
    mov exccruf,al
  .ENDIF
  .IF ax > 24                      ;no following field can be greater than 24
    mov ax,24
  .ENDIF
  .IF bx == 69
    mov excmusb4,al
  .ENDIF
  .IF bx == 70
    mov excmusf4,al
  .ENDIF
  .IF bx == 71
    mov excmusb4+1,al
  .ENDIF
  .IF bx == 72
    mov excmusf4+1,al
  .ENDIF
  .IF bx == 73
    mov excmusb4+2,al
  .ENDIF
  .IF bx == 74
    mov excmusf4+2,al
  .ENDIF
  .IF bx == 75
    mov excmusb4+3,al
  .ENDIF
  .IF bx == 76
    mov excmusf4+3,al
  .ENDIF
  .IF bx == 77
    mov excmusb4+4,al
  .ENDIF
  .IF bx == 78
    mov excmusf4+4,al
  .ENDIF
  .IF bx == 79
    mov excmusb4+5,al
  .ENDIF
  .IF bx == 80
    mov excmusf4+5,al
  .ENDIF
  .IF bx == 81
    mov excmusb4+6,al
  .ENDIF
  .IF bx == 82
    mov excmusf4+6,al
  .ENDIF
  .IF bx == 83
    mov excmusb4+7,al
  .ENDIF
  .IF bx == 84
    mov excmusf4+7,al
  .ENDIF
  .IF bx == 95
    mov excmusb4+8,al
  .ENDIF
  .IF bx == 96
    mov excmusf4+8,al
  .ENDIF
  .IF bx == 97
    mov excmusb4+9,al
  .ENDIF
  .IF bx == 98
    mov excmusf4+9,al
  .ENDIF
  .IF bx == 99
    mov excmusb4+10,al
  .ENDIF
  .IF bx == 100
    mov excmusf4+10,al
  .ENDIF
  .IF bx == 101
    mov excmusb4+11,al
  .ENDIF
  .IF bx == 102
    mov excmusf4+11,al
  .ENDIF
  .IF bx == 103
    mov excmusb4+12,al
  .ENDIF
  .IF bx == 104
    mov excmusf4+12,al
  .ENDIF
  .IF bx == 105
    mov excmusb4+13,al
  .ENDIF
  .IF bx == 106
    mov excmusf4+13,al
  .ENDIF
  .IF bx == 107
    mov excmusb4+14,al
  .ENDIF
  .IF bx == 108
    mov excmusf4+14,al
  .ENDIF
  .IF bx > 84 && bx < 93                      ;8 bell combinations
    mov ecx,ebx
    sub cx,84
    mov dx,8000h
    shr dx,cl
    .IF ax == 0
      not dx
      and excmus8,dx
    .ELSE
      or excmus8,dx
    .ENDIF
  .ENDIF
  popad
  ret
fedit1 ENDP
fedit2 PROC                       ;edit for "X" or "_"
  pushad
  movzx eax,flddef[ebx*8+1]       ;line
  mov cx,160
  mul cx
  movzx edx,flddef[ebx*8+2]       ;column
  shl edx,1
  add eax,edx
  mov esi,eax
  mov al,es:[esi]
  .IF al == "_" || al == " " || al == "0"
  mov eax,FALSE
  .ELSE
  mov eax,TRUE
  .ENDIF
  .IF bl == 109
    mov excopt,ax
    call maxpart
  .ENDIF
  .IF bl == 110
    .IF excrev != ax
      mov excrev,ax
      mov gencalfst,dl
      mov gencallst,dh
      mov dh,gencalfst
      mov dl,gencallst
      mov exccom,TRUE		  ;prevent composition until table rebuilt
      mov gencom,FALSE
      push si
      mov si,offset mess5	  ;issue warning message to recompile table
      call message
      pop si
    .ENDIF
  .ENDIF
  .IF bl == 111
    mov excrot,ax
  .ENDIF
  .IF bl == 112
    mov exctschs,ax
  .ENDIF
  .IF bl == 2
    .IF excbest != ax		  ;Ensure that minimum length changes
      mov excbest,ax
      .IF excbest
	mov ax,excmin
	mov wrkmin,ax
      .ELSE
	mov wrkmin,0
      .ENDIF
    .ENDIF
  .ENDIF
  .IF bl == 3
    mov exchnd,ax
  .ENDIF
  .IF bl == 4
    mov excbak,ax
  .ENDIF
  .IF bl == 5
    mov excint,ax
  .ENDIF
  .IF bl == 6
    mov excleh,ax
  .ENDIF
  .IF bl == 7
    mov exc87s,ax
  .ENDIF
  .IF bl == 8
    mov exc82s,ax
  .ENDIF
  .IF bl == 9
    mov exc83s,ax
  .ENDIF
  .IF bl > 9 && bl < 19
    mov cx,bx
    sub cx,10
    .IF cx == 8
      mov cx,11
    .ENDIF
    .IF cx == 7
      mov cx,9
    .ENDIF
    mov dx,1
    shl dx,cl
    .IF eax
      or excpar,dx
    .ELSE
      not dx
      and excpar,dx
    .ENDIF
    call maxpart
  .ENDIF
  popad
  ret
fedit2 ENDP
maxpart PROC			  ;calculate maximum part length (for optimisation)
  pushad
  .IF excopt
    mov excparmax,40320 	  ;don't optimise part searching if optimisation excluded
  .ELSE
    mov ax,excpar
    xor cx,cx
rep1:				  ;find the smallest number of parts
    inc cx
    shr ax,1
    jc rep1
    mov ax,excmax
    xor dx,dx
    div cx			  ;divide maximum length by smallest number of parts
    mov excparmax,ax
  .ENDIF
  popad
  ret
maxpart ENDP
END
