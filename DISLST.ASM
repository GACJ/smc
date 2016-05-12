;*********************************************************
;* SMC - Single Method Composer                          *
;* Copyright Graham A C John 1991-98 All rights reserved *
;*********************************************************
TITLE DISLST - Display Composition List
;
;    This routine displays a list of compositions generated
;
.MODEL SMALL
.386
INCLUDE smc.inc                   ;Data Definitions
.CODE
EXTERN getkey:near, status:near, display:near, binasc:near, discmp:near
EXTERN message:near, dissta:near, evalco:near
ASSUME gs:lstgrp,es:nothing
dislst PROC
  mov es,scrseg                   ;Write to program screen buffer
  mov disframe,0                  ;Initialise display frame
  mov disnum,0                    ;Number of 1st composition on page
  mov dissel,0                    ;Number of selected composition
newpage:
  call disppage                   ;Display page
  mov si,offset stalst            ;Display Status and screen
  call status
labgetkey:
  call getkey
  .IF  dx == FnPageUp            ;Page up
    sub disnum,20
    mov bx,disnum
    mov dissel,bx
    .IF disnum < SWORD PTR 0
      mov disnum,0
      mov dissel,0
    .ENDIF
  .ELSE
  .IF dx == FnPageDown           ;Page down
    mov bx,disnum
    add bx,20
    .IF bx < statcomps
      mov disnum,bx
      mov dissel,bx
    .ENDIF
  .ELSE
  .IF  dx == FnUp                ;Cursor up
    mov bx,dissel
    dec bx
    .IF bx >= SWORD PTR 0
      mov dissel,bx
      .IF bx < disnum
        mov disnum,bx
      .ENDIF
    .ENDIF
  .ELSE
  .IF  dx == FnDown              ;Cursor down
    mov bx,dissel
    inc bx
    .IF bx < statcomps
      mov dissel,bx
      sub bx,20
      .IF bx == disnum
        inc disnum
      .ENDIF
    .ENDIF
  .ELSE
  .IF dx == FnCtrlHome \         ;Ctrl Home (start of table)
   || dx == FnHome
    mov disnum,0
    mov dissel,0
  .ELSE
  .IF dx == FnCtrlEnd \          ;Ctrl End (end of table)
   || dx == FnEnd
    mov bx,statcomps
    dec bx
    mov dissel,bx
    sub bx,19
    .IF bx < SWORD PTR 0
      mov disnum,0
    .ELSE
      mov disnum,bx
    .ENDIF
  .ELSE
  .IF (dx == FnPageLeft \        ;Page Left
   || dx == FnLeft ) \
   && disframe != 0
    dec disframe
  .ELSE
  .IF (dx == FnPageRt \          ;Page Right
    || dx == FnRight) \
    && disframe != 2
    inc disframe
  .ELSE
  .IF dx == FnDelete \           ;Delete
    && statcomps > 0             ;but only if composition exists to delete
    call delcmp
  .ELSE
  .IF (dx == FnEnter \ 		 ;Enter to select composition for display
    || dx == FnSpace ) \
    && statcomps > 0             ;but only if compositions exist to display
    mov bx,dissel                ;Move composition line to top of window
    mov disnum,bx
enterdiscmp:
    call disppage
    call discmp                  ;display composition
    .IF dx == FnPageUp           ;Page up for DISCMP (ie line up and return)
      .IF disnum > 0
        dec disnum
        dec dissel
      .ENDIF
      jmp enterdiscmp          ;re-enter discmp
    .ENDIF
    .IF dx == FnPageDown       ;Page down for DISCMP (ie line down and return)
      mov bx,disnum
      inc bx
      .IF bx < statcomps
        inc disnum
        inc dissel
      .ENDIF
      jmp enterdiscmp          ;re-enter discmp
    .ENDIF
    .IF (dx == FnPageLeft \    ;Page Left for DISCMP
      || dx == FnLeft )
      .IF disframe != 0
	dec disframe
      .ENDIF
      jmp enterdiscmp          ;re-enter discmp
    .ENDIF
    .IF (dx == FnPageRt \      ;Page Right for DISCMP
      || dx == FnRight)
      .IF disframe != 2
	inc disframe
      .ENDIF
      jmp enterdiscmp          ;re-enter discmp
    .ENDIF
    .IF dx == FnDelete \       ;Delete for DISCMP
	&& statcomps > 0       ;but only if composition exists to delete
      call delcmp
      .IF statcomps > 0        ;if any compositions left
	jmp enterdiscmp	       ;re-enter discmp
      .ENDIF
    .ENDIF
    .IF FnNew
      jmp endproc
    .ENDIF
  .ELSE
  .IF FnNew
    jmp endproc                  ;Continue until another function requested
  .ELSE
  .IF genNew                     ;Redisplay if new composition
  .ELSE
    jmp labgetkey                ;Try another key
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  .ENDIF
  jmp newpage
endproc:ret
dislst ENDP
disppage PROC
  pushad
  movzx ebx,disnum
  mov diszero,"Z"                 ;Normal zero suppression
  mov di,160*4                    ;1st 4 lines of Screen are reserved
  mov ah,colstd                   ;Clear Screen to standard colour
  mov al," "
  mov cx,80*21
  rep stosw
  .IF disframe == 2                ;Display titles
    mov esi,offset distit7
  .ELSE
    .IF disframe == 1
      mov esi,offset distit6
    .ELSE ;disframe 0
      mov esi,offset distit5
    .ENDIF
  .ENDIF
  mov edi,160*4
  mov ecx,80
  mov ah,coltit
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  mov ax,lstseg
  mov gs,ax
  xor ecx,ecx
  .WHILE ecx < ebx
    add ax,lstsiz
    mov gs,ax                     ;Set List start segment
    inc ecx
  .ENDW
  mov ecx,20                      ;Display 20 lines of list (unless shorter)
newline:
    .IF bx >= statcomps           ;No more compositions to display
      jmp endline
    .ENDIF
    push ecx
    .IF bx == dissel              
      mov discol,colhigh          ;Highlight Selection line
    .ELSE
      mov discol,colstd           ;Other lines in standard colour
    .ENDIF  
    mov al," "                    ;Clear line to selected colour
    mov ah,discol
    mov cx,80
    push di
    rep stosw
    pop di
    mov diszero,"Z"               ;Standard zero suppression
    mov eax,ebx                   ;Display Composition Number
    inc eax
    mov ecx,3
    call binasc
    add edi,6
    movzx eax,lsthour             ;Display Time elapsed
    mov ecx,4
    call binasc
    add edi,8
    mov al,":"
    mov ah,discol
    stosw
    mov diszero,"0"               ;Cancel zero suppression
    movzx eax,lstmin
    mov ecx,2
    call binasc
    add edi,4
    mov al,":"
    mov ah,discol
    stosw
    movzx eax,lstsec
    mov ecx,2
    call binasc
    add edi,6
    mov diszero,"Z"               ;Reinstate normal zero suppression
    movzx eax,lstlen              ;Display Composition length
    mov ecx,5
    call binasc
    add edi,14
    movzx eax,lstpar              ;Display number of parts
    mov ecx,2
    call binasc
    add edi,6
 .IF disframe == 0
    movzx eax,lstcalls            ;Display Number of plains
    mov ecx,4
    call binasc
    add edi,8
    movzx eax,lstcalls+2          ;Display Number of bobs
    call binasc
    add edi,8
    movzx eax,lstcalls+4          ;Display Number of singles
    call binasc
    add edi,10
    movzx eax,lstmus87            ;87s at backstroke
    mov ecx,3
    call binasc
    add edi,8
    movzx eax,lstmus82            ;82s
    mov ecx,3
    call binasc
    add edi,8
    movzx eax,lstmus83            ;83s
    mov ecx,3
    call binasc
    add edi,8
    movzx eax,lstcrub             ;crus
    mov ecx,3
    call binasc
    add edi,6
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstcruf
    mov ecx,3
    call binasc
    add edi,8
    movzx eax,lstmusb4            ;5678s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+1          ;6578s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+1
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+8           ;8765s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+8
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+9           ;7568s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+9
    mov ecx,2
    call binasc
    add edi,4
  .ENDIF
  .IF disframe == 1
    add edi,2
    movzx eax,lstmusb4+2           ;4578s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+2
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+3           ;4678s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+3
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+4           ;6478s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+4
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+5           ;5478s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+5
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+6           ;3578s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+6
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+7           ;3478s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+7
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lst5678b            ;5678 combs
    mov ecx,3
    call binasc
    add edi,6
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lst5678f
    mov ecx,3
    call binasc
    add edi,8
    movzx eax,lstmusb4+10          ;7658s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+10
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+11	   ;2345s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+11
    mov ecx,2
    call binasc
    add edi,4
  .ENDIF
  .IF disframe == 2
    add edi,2
    movzx eax,lstmusb4+12	  ;5432s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+12
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+13	   ;3456s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+13
    mov ecx,2
    call binasc
    add edi,6
    movzx eax,lstmusb4+14	   ;6543s
    mov ecx,2
    call binasc
    add edi,4
    mov al,"/"
    mov ah,discol
    stosw
    movzx eax,lstmusf4+14
    mov ecx,2
    call binasc
    add edi,6
    mov ah,discol		   ;8 bell counts (1st 10 displayed)
    mov dx,lstmus8
    shl dx,1                       ;ignore rounds
    mov ecx,10
    .REPEAT
      shl dx,1
      .IF CARRY?
        mov al,"Y"
      .ELSE
        mov al," "
      .ENDIF
      stosw
    .UNTILCXZ
    mov eax,lstscore		  ;Composition score
    mov ecx,10
    call binasc
    add edi,18
    mov eax,lstseq		  ;Sequence Number
    mov ecx,9
    call binasc
    add edi,18
    movzx eax,lstrot		  ;Rotation Number
    mov ecx,9
    call binasc
    add edi,18
  .ENDIF
    inc ebx                         ;next list entry
    mov ax,gs
    add ax,lstsiz
    mov gs,ax
    pop ecx
    dec ecx
    .IF !ZERO?
      jmp newline
    .ENDIF
endline:
  popad
  ret
disppage ENDP
delcmp PROC USES ES               ;Delete composition from list
  ASSUME es:lstgrp
  pushad
  mov ax,lstseg                   ;Find selected composition in list
  movzx ebx,dissel
  mov es,ax
  xor ecx,ecx
  .WHILE ecx < ebx
    add ax,lstsiz
    mov es,ax                     ;Set List start segment
    inc ecx
  .ENDW
  mov cx,lstsiz                   ;Adjust current list segment pointer
  sub genlst,cx                   ; by size of deleted composition
  xor si,si                       ;shuffle up compositions
  xor di,di
  mov cx,cmpgrp
  sub cx,ax                       ;cx = shuffle length in paragraphs
  shl ecx,2                       ;convert to doublewords
  add ax,lstsiz
  push ds
  mov ds,ax
  rep movsd
  pop ds
  inc statdel                     ;increment number of comps deleted
  dec statcomps                   ;decrement number of compositions found
  mov bx,dissel                   ;Ensure list not past end
  mov ax,bx
  inc ax
  .IF ax > statcomps
    dec bx
    .IF bx >= SWORD PTR 0
      mov dissel,bx
      .IF bx < disnum
        mov disnum,bx
      .ENDIF
    .ENDIF
  .ENDIF
  .IF genofl                      ;clear list overflow if set
    mov genofl,FALSE              ;switch off overflow indicator
    mov si,offset mess0           ;Clear messages
    call message
    call evalco                   ;re-evaluate overflow composition
  .ENDIF
  call dissta                     ;Display changed totals
  popad
  ret
delcmp ENDP
END
