TITLE DISDYN - Display Current Composition (Dynamically)
;
;    This routine displays the current composition (incomplete)
;    It will be invoked every time COMGEN drops out to display the
;    latest position
;
.MODEL SMALL
.386
INCLUDE smc.inc                   ;Data Definitions
.CODE
EXTERN getkey:near, status:near, display:near, binasc:near, dispcol:near, displeh:near
ASSUME es:nothing
disdyn PROC USES fs es eax ebx ecx esi edi
  mov gendyn,TRUE		  ;Set flag to comgen to drop out periodically
  mov disfirst,0                  ;Initialise display line to start of comp
  mov es,scrseg                   ;Write to program screen buffer
newpage:
  mov discol,colstd               ;Standard display colour
  mov di,160*4			  ;1st 4 lines of Screen are reserved
  mov ah,discol                   ;Clear Screen to standard colour
  mov al," "
  mov cx,80*21
  rep stosw
  mov edi,160*5			  ;Display title lines
  mov esi,offset distit8          ;and distit9 (consecutive)
  mov ecx,160
  mov ah,discol
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  mov fs,cmpseg 		  ;Set composition segment
  xor ebx,ebx			  ;Display 18 lines of composition (unless shorter)
  .IF disfirst > 0                ;Display dots over heading dashes if lines missing
    push di
    mov si,offset distit10
    mov di,160*6
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
  call dyncall			 ;fetch first call
newline:
  .REPEAT
    mov discalcnt,1               ;reset call count
    mov eax,edx
@@: mov ecx,cmpleh                ;keep previous call and leadhead details
    mov disleh,ecx
    mov ecx,cmpleh+4
    mov disleh+4,ecx
    .IF ! (eax & 80000000h)       ;get next call if composition not finished
      call dyncall		 ;next call - returns edx - dl=call, dh=col
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
	.BREAK .IF eax & 80000000h;Exit loop if end of composition
      .ENDIF
      inc ebx                   ;Count composition lines
    .ENDIF
  .UNTIL dislin == 17		;repeat to end of screen or rounds
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
  mov si,offset stadyn		 ;Display status and screen
  call status
labgetkey:
  call getkey
  .IF FnNew			   ;exit for new function
    jmp endproc                   ;Continue until another function requested
  .ENDIF
  jmp newpage
endproc:
  mov gendyn,FALSE
  ret
disdyn ENDP
dyncall PROC USES eax ecx
  .REPEAT
    xor edx,edx
    movzx eax,gencal[esi]	  ;Load next call
    inc esi
    mov dl,al			  ;keep call
    mov ax,cmpcal[eax*2]	  ;Fetch Leadhead produced by call
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
    .IF si >= genlea		  ;Is leadhead the last in composition
      or edx,80000000H            ;flag for end of composition
    .ENDIF
  .UNTIL edx & 0FFFF00FFh	  ;Return if coursehead, parthead, endcomp or call not plain
  ret                             ;returns dl=call,dh=column
dyncall ENDP
END
