;*********************************************************
;* SMC - Single Method Composer                          *
;* Copyright Graham A C John 1991-98 All rights reserved *
;*********************************************************
TITLE DISCMP - Display Composition from list
;
;    This routine displays a single composition from the list
;
.MODEL SMALL
.386
INCLUDE smc.inc                   ;Data Definitions
.CODE
EXTERN getkey:near, status:near, display:near, binasc:near
ASSUME gs:lstgrp,es:nothing
discmp PROC USES fs es eax ebx ecx esi edi
  mov disfirst,0                  ;Initialise display line to start of comp
  mov es,scrseg                   ;Write to program screen buffer
newpage:
  mov discol,colstd               ;Standard display colour
  mov di,160*6                    ;1st 6 lines of Screen are reserved
  mov ah,discol                   ;Clear Screen to standard colour
  mov al," "
  mov cx,80*19
  rep stosw
  mov edi,160*6                   ;Display title lines
  mov esi,offset distit8          ;and distit9 (consecutive)
  mov ecx,160
  mov ah,discol
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  movzx ebx,disnum                ;Find composition in list
  mov ax,lstseg
  mov gs,ax
  xor ecx,ecx
  .WHILE ecx < ebx
    add ax,lstsiz
    mov gs,ax                     ;Set list segment
    inc ecx
  .ENDW
  mov fs,cmpseg                   ;Set composition segment
  xor ebx,ebx                     ;Display 16 lines of composition (unless shorter)
  .IF disfirst > 0                ;Display dots over heading dashes if lines missing
    push di
    mov si,offset distit10
    mov di,160*7
    mov ecx,80
    mov ah,discol
    .REPEAT
      lodsb
      stosw
    .UNTILCXZ
    pop di
  .ENDIF
  mov dislin,0
  xor esi,esi                     ;Initialise list pointer
  mov disshift,6                  ;and list compression pointer
  mov diszero," "                 ;Zero suppression type (shouldn't be needed)
  mov dislen,0                    ;Zero composition length counter
  call nextcall                   ;fetch first call
newline:
  .REPEAT
    mov discalcnt,1               ;reset call count
    mov eax,edx
@@: mov ecx,cmpleh                ;keep previous call and leadhead details
    mov disleh,ecx
    mov ecx,cmpleh+4
    mov disleh+4,ecx
    .IF ! (eax & 80000000h)       ;get next call if composition not finished
      call nextcall               ;next call - returns edx - dl=call, dh=col
      .IF dl == 0 \               ;keep the plain course end, otherwise ignore
       && ! (eax & 40000000h)     ;and not end of part
        xor dx,dx                 ;avoid overwriting previous call & column
        or eax,edx                ;keep new flags if any
        jmp @B
      .ENDIF
      .IF dx == ax \              ;if call same type and position
       && ! (eax & 40000000h)     ;and not end of part
        inc discalcnt             ;count identical calls in the same position
        mov eax,edx
        jmp @B
      .ENDIF
      .IF dh > ah \               ;if more columns to display but leadhead flag set
       && (eax & 20000000h) \
       && !(eax & 40000000h)      ;but not end of part
	and eax,0DFFFFFFFh	  ;suppress the leadhead flag
      .ENDIF
      .IF dh <= ah		  ; if call in earlier col or diff call in same col
	or eax,10000000h	  ; force leadhead display and new line
      .ENDIF
    .ENDIF
    call dispcol                  ;Display column
    .IF eax & 0F0000000h	  ;composition line to display?
      call displeh                ;Display leadhead
      .IF bx < disfirst           ;Clear Suppressed lines
        mov ah,discol
        mov al," "
        mov cx,80
        push di
        rep stosw
        pop di
      .ELSE
        add edi,160             ;Next row
        inc dislin              ;Count display lines
        .BREAK .IF eax & 80000000h;Exit loop if Composition has come round
        .IF eax & 40000000h \  ;display part end line if room on screen
          && dislin < 16
          push esi
          mov esi,offset distit9
          mov ecx,80
          mov ah,discol
          .REPEAT
            lodsb
            stosw
          .UNTILCXZ
          pop esi
          inc dislin            ;count display lines
        .ENDIF
      .ENDIF
      inc ebx                   ;Count composition lines
    .ENDIF
  .UNTIL dislin == 16           ;repeat to end of screen or rounds
  push eax
  .IF eax & 80000000h           ;display end line if rounds
    mov esi,offset distit9
    mov ecx,80
    mov ah,discol
    .REPEAT
      lodsb
      stosw
    .UNTILCXZ
  .ELSE
    mov esi,offset distit10       ;Display dots if more pages
    mov ecx,80
    mov ah,discol
    .REPEAT
      lodsb
      stosw
    .UNTILCXZ
  .ENDIF
  pop eax
  mov si,offset stacmp            ;Display status and screen
  call status
labgetkey:
  call getkey
  .IF  dx == FnUp                 ;Cursor up
    .IF disfirst != 0
      dec disfirst
    .ENDIF
  .ELSE
  .IF  dx == FnDown               ;Cursor down
    .IF !(eax & 80000000h)
      inc disfirst
    .ENDIF
  .ELSE
  .IF dx == FnHome                ;Home (start of comp)
    mov disfirst,0
  .ELSE
  .IF dx == FnEnter \             ;Enter, Space or Display to return to list
   || dx == FnEsc \
   || dx == FnSpace \
   || dx == FnPageDown \          ;Page up or Page Down via Dislst
   || dx == FnPageUp \
   || dx == FnPageLeft \          ;Page Left or Left via Dislst
   || dx == FnLeft \
   || dx == FnPageRt \            ;Page Right or Right via Dislst
   || dx == FnRight \
   || dx == FnDelete \  	  ;Delete via Dislst
   || FnNew                       ;exit for new function
    jmp endproc                   ;Continue until another function requested
  .ELSE
    jmp labgetkey                 ;Try another key
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  jmp newpage
endproc:ret
discmp ENDP
nextcall PROC USES eax ecx
  .REPEAT
    xor edx,edx
    movzx eax,lstcal[esi]         ;Load next call
    mov cl,disshift               ;Use call shift to decompress
    shr ax,cl
    and eax,3                     ;mask off bits except last 2
    mov dl,al                     ;keep call type
    .IF disshift == 0             ;Position for next call
      mov disshift,6
      inc esi
    .ELSE
      sub disshift,2
    .ENDIF
    mov ax,cmpcal[eax*2]          ;Fetch Leadhead produced by call
    mov fs,ax
    mov ax,cmplen                 ;Count composition length so far
    add dislen,ax
    mov eax,0                     ;Find Tenor position
    .WHILE BYTE PTR cmpleh[eax] != TENOR
      inc eax
    .ENDW
    mov dh,metcol[eax]            ;Convert tenor position to display column
    .IF ax == 7                   ;Is leadhead a coursehead
      or edx,20000000h            ;flag for coursehead
    .ENDIF
    push ebx
    push edx
    movzx eax,lstlen              ;Is leadhead a parthead
    xor edx,edx                   ; ie if MOD (dislen/(lstlen/lstpar)) > 0
    movzx ebx,lstpar
    div bx
    mov ebx,eax
    movzx eax,dislen
    xor edx,edx
    div bx
    mov eax,edx
    pop edx
    pop ebx
    .IF ax == 0                   ;If divides exactly
      or edx,40000000h            ;flag for parthead
    .ENDIF
    .IF cmpmus8 & 8000H           ;Is leadhead the last in composition
      or edx,80000000H            ;flag for end of composition
    .ENDIF
  .UNTIL edx & 0FFFF00FFh         ;Return if coursehead, parthead, endcomp or call not plain
  ret                             ;returns dl=call,dh=column
nextcall ENDP
dispcol PROC USES di eax edx ecx  ;displays the call in al in column ah
.IF al == 1 || al == 2            ;display bobs or singles, not plains
  xor edx,edx
  mov dl,ah                       ;adjust display position to column
  add di,dx
  mov dl,al
  movzx eax,discalcnt             ;Number of calls
  mov ecx,2
  call binasc
  .IF dl == 2                     ;Display S for singles
    mov dx,ax                     ;number of calls
    mov al,"S"
    mov ah,discol
    .IF dx == 2
      stosw
    .ELSE
      .IF dx == 1
        add di,2
      .ELSE
        add di,4
      .ENDIF
    .ENDIF
    stosw
  .ELSE
    .IF dl == 1 \                 ;Display - for 1 bob
        && ax == 1
      mov al,"-"
      mov ah,discol
      add di,2
      stosw
    .ENDIF
  .ENDIF
.ENDIF
  ret
dispcol ENDP
displeh PROC USES esi edi eax ecx edx
  add edi,106                     ;Adjust to coursehead column
  mov edx,eax                     ;Move flags from eax to edx
  mov ah,discol
  .IF edx & 20000000h
    mov al," "                    ;Course head
  .ELSE
    mov al,"("                    ;Bracket if not course head
  .ENDIF
  stosw
  mov esi,1
  mov ecx,7
  .REPEAT
    mov al,BYTE PTR disleh[esi]   ;Display course/lead head
    add al,"0"                    ;Adjust to ASCII
    stosw
    inc esi
  .UNTILCXZ
  .IF edx & 20000000h
    mov al," "                    ;Course head
  .ELSE
    mov al,")"                    ;Bracket if Lead head
  .ENDIF
  stosw
  ret
displeh ENDP
END
