;*********************************************************
;* SMC - Single Method Composer                          *
;* Copyright Graham A C John 1991-98 All rights reserved *
;*********************************************************
TITLE DISTAB - Display Proving Table for Method
;
;    This routine displays the proving table
;
.MODEL SMALL
.386
INCLUDE smc.inc                   ;Data Definitions
.CODE
EXTERN getkey:near, status:near, display:near, elapse:near
distab  PROC
  mov es,scrseg                   ;Write to program screen buffer
  mov dislho,cmpgrp		  ;Display start of the table
  mov fs,dislho
  .IF cmpleh != 04030201h	  ;exit if table has never been built
    mov dx,FnExclud
    jmp endproc
  .ENDIF
newpage:
  mov discol,colstd               ;Set display colour
  mov diszero,"Z"                 ;Zero suppression on
  mov si,offset statab            ;Display Status
  call status
  mov di,160*4                    ;1st 4 lines of Screen are reserved
  mov ah,discol                   ;Clear Screen to standard colour
  mov al," "
  mov cx,80*21
  rep stosw
  mov cx,disframe                 ;Display titles
  mov esi,offset distit0
  .WHILE cx != 0
   add esi,80
   dec cx
  .ENDW
  mov edi,160*4
  mov ecx,80
  mov ah,coltit
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  movzx ebx,dislho                ;LHO
  mov ecx,20                      ;Display 20 lines of table
newline:
  mov ax,cmpgrp
  add ax,5040*6
  .IF bx > ax                     ;past end of table?
    jmp labdisplay
  .ENDIF
  mov fs,bx                       ;Set segment for table element
  .IF cmpleh == -1 \              ;Ignore excluded leads
   || cmpleh == 0
    add bx,6
    jmp newline
  .ENDIF
  push ecx
  mov discol,colstd               ;use standard display colour
  mov ah,discol                   ;Clear line to selected colour
  mov al," "
  mov cx,80
  push di
  rep stosw
  pop di
  mov eax,ebx                     ;Display LHO equivalent
  mov ecx,4
  call lhoasc
  add edi,10
  mov ah,discol
  mov ax,fs                       ;Display leadhead (all frames)
  call lhodisp
  add edi,18
  .IF disframe == 0
    movzx eax,cmplen              ;cmplen
    mov ecx,3
    call binasc
    add edi,8
    movzx eax,cmppar              ;cmppar
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,cmpcal              ;cmpcal x 3
    call lhodisp
    add edi,18
    movzx eax,cmpcal+2
    call lhodisp
    add edi,18
    movzx eax,cmpcal+4
    call lhodisp
    add edi,18
    mov dx,cmpmus8                ;cmpmus8
    mov ah,discol
    mov ecx,16
    .REPEAT
      shl dx,1
      .IF CARRY? && cl != 2	  ;suppress the display of nature (bit 14)
        mov al,"Y"
      .ELSE
        mov al," "
      .ENDIF
      stosw
    .UNTILCXZ
    add edi,2
    mov al,cmpclo                 ;cmpclo
    .IF al == 0
      mov al,"P"
    .ENDIF
    .IF al == 1
      mov al,"B"
    .ENDIF
    .IF al == 2
      mov al,"S"
    .ENDIF
    mov ah,discol
    stosw
    add edi,2
    movzx eax,cmpmus87            ;cmpmus87
    mov ecx,3
    call binasc
    add edi,8
    movzx eax,cmpmus82            ;cmpmus82
    mov ecx,3
    call binasc
    add edi,8
    movzx eax,cmpmus83            ;cmpmus83
    mov ecx,3
    call binasc
    add edi,10                    ;end of line
  .ELSE
  .IF disframe == 1
    movzx eax,cmpcruf             ;cmpcruf/cmpcrub
    mov ecx,3
    call binasc
    add edi,6
    mov al," "
    mov ah,discol
    stosw
    movzx eax,cmpcrub
    mov ecx,3
    call binasc
    add edi,8
    movzx eax,cmp5678f            ;cmp5678f/cmp5678b
    mov ecx,3
    call binasc
    add edi,6
    mov al," "
    mov ah,discol
    stosw
    movzx eax,cmp5678b
    mov ecx,3
    call binasc
    add edi,8
    xor esi,esi                   ;cmpmusf4/cmpmusb4
    mov edx,8
    .REPEAT
      movzx eax,cmpmusf4[esi]
      mov ecx,2
      call binasc
      add edi,4
      mov al," "
      mov ah,discol
      stosw
      movzx eax,cmpmusb4[esi]
      mov ecx,2
      call binasc
      add edi,6
      inc esi
      dec edx
    .UNTIL edx == 0
    add edi,4                     ;End of line
  .ELSE
    .IF disframe == 2
      mov esi,8                 ;cmpmusf4/cmpmusb4
      mov edx,7
      .REPEAT
        movzx eax,cmpmusf4[esi]
        mov ecx,2
        call binasc
        add edi,4
        mov al," "
        mov ah,discol
        stosw
        movzx eax,cmpmusb4[esi]
        mov ecx,2
        call binasc
        add edi,6
        inc esi
        dec edx
      .UNTIL edx == 0
      add edi,48                  ;End of Line
    .ELSE ;disframe is 3 to 7
      movzx eax,cmpfno            ;cmpfno
      mov ecx,3
      call binasc
      add edi,8
      xor edx,edx                 ;adjust to correct flh according to frame
      mov ax,disframe
      .WHILE ax > 3
        add edx,7
        dec ax
      .ENDW
      mov cx,7
      .REPEAT
        xor eax,eax               ;ensure zeroes displayed if non-existant
        .IF dl < cmpfno
          movzx eax,cmpflh[edx*2] ;cmpflh
        .ENDIF
        call lhodisp
        add edi,18
        inc edx
      .UNTILCXZ
      sub edi,2                   ;End of Line
    .ENDIF
    .ENDIF
  .ENDIF
  add ebx,6                       ;next table entry (LHO)
  pop ecx
  dec ecx
  .IF !ZERO?
    jmp newline
  .ENDIF
labdisplay:
  call display                   ;Display screen
labgetkey:
  call getkey
  mov ax,cmpgrp                  ;Calculate start of last page of display
  add ax,5021*6
  .IF  dx == FnPageUp \          ;Page up
    || dx == FnUp
    mov cx,20
    .REPEAT
      .IF dislho > cmpgrp
        sub dislho,6
        mov bx,dislho
        mov fs,bx
        .IF cmpleh == -1 \
         || cmpleh == 0
          inc cx
        .ENDIF
      .ENDIF
    .UNTILCXZ
  .ELSE
  .IF dx == FnPageDown \         ;Page down
   || dx == FnDown
    mov cx,20
    .REPEAT
      mov ax,cmpgrp
      add ax,5040*6
      .IF dislho < ax
        add dislho,6
        mov bx,dislho
        mov fs,bx
        .IF cmpleh == -1 \
         || cmpleh == 0
          inc cx
        .ENDIF
      .ENDIF
    .UNTILCXZ
  .ELSE
  .IF dx == FnCtrlHome \         ;Ctrl Home (start of table)
   || dx == FnHome
    mov dislho,cmpgrp
  .ELSE
  .IF dx == FnCtrlEnd \          ;Ctrl End (end of table)
   || dx == FnEnd
    mov dislho,cmpgrp
    add dislho,5040*6
    mov cx,19
    .REPEAT
      .IF dislho > cmpgrp
        sub dislho,6
        mov bx,dislho
        mov fs,bx
        .IF cmpleh == -1
          inc cx
        .ENDIF
      .ENDIF
    .UNTILCXZ
  .ELSE
  .IF (dx == FnPageLeft \        ;Page Left
   || dx == FnLeft ) \
   && disframe != 0
    dec disframe
  .ELSE
  .IF (dx == FnPageRt \          ;Page Right
    || dx == FnRight) \
    && disframe != 7
    inc disframe
  .ELSE
  .IF FnNew
    jmp endproc                  ;Continue until another function requested
  .ELSE
    jmp labgetkey                ;Try another key
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  jmp newpage
endproc:ret
distab ENDP
binasc PROC USES es esi di eax bx edx cx
; eax     = binary number to convert
; di      = destination (start of field on screen)
; cx      = number of ascii digits
; discol  = display colour
; diszero = Suppress all (" "), all but last ("Z"), None ("0")
  mov es,scrseg                   ;Program screen buffer segment
  mov esi,10                      ;set base 10 divisor
  add di,cx                       ;mov to last char of field
  add di,cx
  sub di,2
  mov bl,diszero
  .IF bl == "Z"
    mov bl,"0"
  .ENDIF
  .REPEAT
    .IF eax == 0
      mov dl,bl
    .ELSE
      xor edx,edx
      div esi
      add dx,30h
    .ENDIF
    .IF diszero == "Z"
      mov bl," "
    .ENDIF
    mov dh,discol
    mov es:[edi],dx
    sub di,2
  .UNTILCXZ
  ret
binasc ENDP
lhoasc PROC USES eax esi edi edx ecx;Convert LHO to number between 0 and 5040
  .IF ax == 0
    call binasc
  .ELSE
    .IF ax == 0FFFFh
      mov ah,colhigh              ;Highlight excluded leadheads
      mov al,"X"
      mov cx,4
      rep stosw
    .ELSE
      sub eax,cmpgrp
      mov si,6
      xor edx,edx
      div si
      call binasc
    .ENDIF
  .ENDIF
  ret
lhoasc ENDP
dissta PROC
  pushad
  mov diszero,"Z"                 ;Set for Normal zero suppression
  movzx eax,stattab               ;Display Available Leads
  mov discol,colinf               ;   Information colour
  mov di,202                      ;   Screen position
  mov cx,4                        ;   Number of digits
  call binasc                     ;   Convert to ascii
  movzx eax,flhcnt                ;Display flh (poss)
  mov discol,colinf               ;   Information colour
  mov di,366                      ;   Screen position
  mov cx,2                        ;   Number of digits
  call binasc                     ;   Convert to ascii
  movzx eax,statflhhi             ;Display table flh (high)
  mov discol,colinf               ;   Information colour
  mov di,360                      ;   Screen position
  mov cx,2                        ;   Number of digits
  call binasc                     ;   Convert to ascii
  mov eax,statfound		  ;Display Comps Found
  mov discol,colinf               ;   Information colour
  mov di,136			  ;   Screen position
  mov cx,12			  ;   Number of digits
  call binasc                     ;   Convert to ascii
  movzx eax,statcomps		  ;Display Comps Kept
  mov discol,colinf               ;   Information colour
  mov di,296			  ;   Screen position
  mov cx,12			  ;	Number of digits
  call binasc                     ;   Convert to ascii
  movzx eax,statpcent             ;Display Complete%
  mov discol,colinf               ;   Information colour
  mov di,472			   ;   Screen position
  mov cx,3                        ;   Number of digits
  call binasc                     ;   Convert to ascii
  mov eax,statleads               ;Display Leads Checked
  xor edx,edx
  mov ecx,1000000/TIMEOUT	  ;   convert to millions
  jecxz @F			  ;   avoid divide by zero
  div ecx
@@:
  mov statleadm,eax		  ;   keep millions of leads
  mov discol,colinf               ;   Information colour
  mov di,254                      ;   Screen position
  mov cx,6                        ;   Number of digits
  call binasc			  ;   Convert to ascii
.IF !genend
  call elapse			  ;Display elapsed time since comp started
  mov ebx,ecx
  mov eax,ebx			  ; Display Hours Elapsed
  shr eax,8
  mov discol,colinf		  ;   Information colour
  mov di,88			  ;   Screen position
  mov cx,4			  ;   Number of digits
  call binasc                     ;   Convert to ascii
  mov diszero,"0"                 ; Set no zero suppression
  movzx eax,bl                    ; Display Minutes Elapsed
  mov discol,colinf               ;   Information colour
  mov di,98                       ;   Screen position
  mov cx,2                        ;   Number of digits
  call binasc                     ;   Convert to ascii
  movzx eax,dh                    ; Display Seconds elapsed
  mov discol,colinf               ;   Information colour
  mov di,104                      ;   Screen position
  mov cx,2                        ;   Number of digits
  call binasc                     ;   Convert to ascii
  mov eax,ebx			  ; Display composing speed
  shr eax,8
  mov ecx,60			  ;   convert hours to minutes
  mul cl
  and ebx,0FFh
  add eax,ebx			  ;   add minutes
  mov bl,dh			  ;   keep seconds
  mul ecx			  ;   convert minutes to seconds
  add ebx,eax			  ;   add seconds and keep in ebx
  mov eax,statleads		  ;   convert TIMEOUTLeads to kLeads
  mov ecx,TIMEOUT/1000		  ;   compensate for timeout frequency
  mul ecx
  mov ecx,ebx
  xor edx,edx
  jecxz @F			  ;   avoid divide by zero
  div ecx
@@:
  mov diszero,"Z"		  ;   set zero suppression on
  mov discol,colinf		  ;   Information colour
  mov di,414			  ;   Screen position
  mov cx,6			  ;   Number of digits
  call binasc			  ;   Convert to ascii
.ENDIF
  call display
  popad
  ret
dissta ENDP
calpct PROC USES ax ebx ecx dx fs ;Calculate percentage complete
.IF genend                        ;Composition finished therefor %age = 100%
  mov dh,100
.ELSE
  mov ax,100                      ;eax is remaining fraction of composition tree
  mov dh,0                        ;dh is percentage complete, start with 0%
  xor ebx,ebx                     ;ebx is composition pointer
  .WHILE bx != genlea \           ;For all leads in current composition (or part)
    && ax != 0                    ;but stop once fraction is below 1%
    mov fs,genlho[ebx*2]          ;How many calls possible from this lead?
    xor ecx,ecx
    .IF cmpcal != 0
      inc ecx
    .ENDIF
    .IF cmpcal+2 != 0
      inc ecx
    .ENDIF
    .IF cmpcal+4 != 0
      inc ecx
    .ENDIF
    .IF ecx == 3                  ;3 branches of composition tree exist
      div cl                      ;Divide remaining fraction by 3
      xor ah,ah                   ;and clear remainder
      mov dl,cmpclo               ;How many branches exhausted
      .IF dl == 2                 ;2/3rds of branches exhausted
        mov dl,al                 ;multiply fraction by 2
        shl dl,1
        add dh,dl                 ;and add to percentage complete
      .ELSE
        .IF dl == 1               ;1/3rd of branches exhausted
          add dh,al               ;add fraction to percentage complete
        .ENDIF
      .ENDIF
    .ELSE
      .IF ecx == 2                ;2 branches of composition tree exist
        shr al,1                  ;Divide remaining fraction by 2
        mov dl,cmpclo             ;How many branches exhausted
        .IF dl == 2 \             ;1/2 of branches exhausted
        || (dl == 1 && cmpcal+4 == 0)
          add dh,al               ;add fraction to percentage complete
        .ENDIF
      .ENDIF
    .ENDIF
    inc ebx                       ;Next lead
  .ENDW
.ENDIF
.IF excrev
  mov statpcent,dh
.ELSE
  mov statpcent,100
  sub statpcent,dh
.ENDIF
  ret
calpct ENDP
lhodisp PROC USES fs eax ecx esi edi
.IF eax != 0
  mov fs,ax
  mov ecx,8
  xor esi,esi
  mov ah,discol
  .REPEAT
    mov al,BYTE PTR cmpleh[esi]    ;Display leadhead
    add al,"0"
    stosw
    inc esi
  .UNTILCXZ
  .IF cmpmus8 & 2		   ;Display nature of leadhead
    mov al,"-"
  .ELSE
    mov al,"+"
  .ENDIF
  stosw
.ENDIF
  ret
lhodisp ENDP
  END
