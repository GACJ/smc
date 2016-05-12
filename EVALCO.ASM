;*********************************************************
;* SMC - Single Method Composer                          *
;* Copyright Graham A C John 1991-98 All rights reserved *
;*********************************************************
TITLE EVALCO - Evaluate Composition 
;
;    This routine evaluates compositions against exclusion criteria.
;    If the composition meets the criteria, the new composition flag is set
;    and it is added to the composition list. If rotations are required and
;    the composition comes round at a course end, the composition is rotated
;    starting from every course end, before evaluating
;    the rotated composition as above.
.MODEL SMALL
.386
 INCLUDE smc.inc                  ;Data Definitions
 INCLUDE dos.inc                  ;DOS Macros
EXTERN message:NEAR, status:NEAR
.CODE
evalco PROC USES es fs gs
  pushad
  mov GenNew,FALSE		  ;Assume no new composition found
  mov rotcnt,0			  ;initialise rotation variables
  mov rotarray,0
  mov evarot,0
  .IF excrot			  ;indicate first time thru if rotation
    mov rotftt,FALSE		  ; not excluded
  .ELSE
    mov rotftt,TRUE
  .ENDIF
  mov es,genlst 		  ;set list segment to current composition
evalrpt:
  movzx eax,genlea                ;Check for list overflow
  add eax,4                       ;adjust by 1 since genlea is a pointer
		                  ;plus 3 to round-up following division
  shr eax,2                       ;Divide by 4 since 4 calls held per byte
  add eax,OFFSET lstcal + 15      ;Add stats & round up to nearest paragraph
  shr eax,4                       ;convert list length to paragraphs
  mov bx,es
  add bx,ax
  mov cx,cmpgrp
  sub cx,100                      ;reduce list space by sort overflow area (100x16bytes)
  .IF bx > cx                     ;lstgrp overflowing
    .IF excsort
      mov si,offset mess4         ;Display message
      call message
      mov si,offset staclear      ;Clear Status
      call status
      mov genofl,TRUE             ;Suspend composition
      popad
      ret
    .ELSE                         ;Lose last composition
      call dropcmp                ;lose composition with lowest score (last in list)
      jmp evalrpt                 ;Try again
    .ENDIF
  .ENDIF
  mov gs,cmpseg                   ;start composition from rounds in table
  mov lstsiz,ax                   ;keep list entry size in list
  mov ax,genlen                   ;keep composition length
  mov lstlen,ax
  xor eax,eax                     ;evaluate composition
  mov di,offset lstscore          ;Zero list counts
  mov cx,offset lstcal - offset lstscore;length of list fields to clear
  rep stosb
  mov ebx,rotcnt		  ;start composition from current rotation point
  mov bx,rotarray[ebx*2]
  .IF rotcnt != 0		  ;apart from first time thru
    dec rotcnt			  ; count down through rotations evaluated
  .ENDIF
  xor edi,edi			  ;edi is pointer to composition in list (lstcal)
  mov lstcal,0                    ;clear first call byte
  mov edx,6                       ;edx is shift count for call compression
  .REPEAT
    mov ax,gs                     ;set lho for table entry
    .IF ax == 0 		  ;invalid composition perhaps because
      mov rotftt,FALSE
      jmp evalfin		  ;rotation contains an excluded-try next
    .ENDIF
    mov fs,ax
    movzx eax,gencal[ebx]	  ;get next call
    mov gs,cmpcal[eax*2]	  ;store next lho
    .IF !(cmpmus8 & 8000h)        ;don't count call for lead containing rounds
	 inc lstcalls[eax*2]         ;add up number of calls in composition
	 mov cl,dl
	 shl ax,cl                   ;Call compression, 4 calls per byte
	 or lstcal[edi],al           ;save in list
	 .IF dl == 0
	   inc edi                   ;increment call pointer for list
	   mov lstcal[edi],0         ;clear next call byte
	   mov dl,6
	 .ELSE
	   sub dl,2
	 .ENDIF
    .ENDIF
    mov al,cmpmus87               ;add up music counts for composition
    add lstmus87,ax
    mov al,cmpmus82
    add lstmus82,ax
    mov al,cmpmus83
    add lstmus83,ax
    mov al,cmpcrub
    add lstcrub,al
    mov al,cmpcruf
    add lstcruf,al
    mov al,cmp5678b
    add lst5678b,ax
    mov al,cmp5678f
    add lst5678f,ax
    mov ecx,15
    .REPEAT
	 mov al,cmpmusb4[ecx-1]
	 add lstmusb4[ecx-1],al
	 mov al,cmpmusf4[ecx-1]
	 add lstmusf4[ecx-1],al
    .UNTILCXZ
    mov ax,cmpmus8
    or lstmus8,ax
    .IF rotftt \ 		  ;if first time thru and rotations required
     && (cmptenor == 7 \	  ; keep ebx for each home lead in rotarray
     ||  excsync ) \ 		  ; but keep all leads if no sync required
     && ! (cmpmus8 & 8000h)	  ; excluding final course end
       inc rotcnt		  ; count number of rotations
       mov eax,rotcnt
       mov rotarray[eax*2],bx	  ; store composition pointer (ebx)
    .ENDIF
    inc ebx
    .IF bx == genlea              ;If at end of calls return to beginning (for rotation)
      xor ebx,ebx
    .ENDIF
  .UNTIL cmpmus8 & 8000h          ;Repeat until composition comes round
  .IF rotftt && cmplen != 0
      mov rotcnt,0		  ;rotation only possible if round at course end
  .ENDIF
  mov rotftt,FALSE
  xor eax,eax			  ;EVALUATE COMPOSITION FOR EXCLUSIONS******
  mov al,lstcrub                  ;compare against composition exclusions
  cmp exccrub,al                  ;crus back
  ja evalfin
  mov al,lstcruf
  cmp exccruf,al                  ;crus front
  ja evalfin
  mov ax,lst5678b
  cmp exc5678b,ax                 ;5678 combinations back
  ja evalfin
  mov ax,lst5678f                 ;5678 combinations front
  cmp exc5678f,ax
  ja evalfin
  mov ecx,15
  .REPEAT
    mov al,lstmusb4[ecx-1]        ;4 bell combinations back
    cmp excmusb4[ecx-1],al
    ja evalfin
    mov al,lstmusf4[ecx-1]        ;4 bell combinations front
    cmp excmusf4[ecx-1],al
    ja evalfin
  .UNTILCXZ
  mov ax,lstmus8                  ;8 bell combinations
  and ax,excmus8
  cmp ax,excmus8
  jne evalfin
  call cntparts                   ;count number of parts in composition
  mov ax,1                        ;check part exclusions
  mov cl,lstpar
  dec cl
  shl ax,cl
  and ax,excpar
  jnz evalfin
; *********************************COMPOSITION FOUND
  call score			  ;score composition
  .IF evarot == 0
    inc evaseq
    mov eax,evaseq
    mov lstseq,eax		  ;keep sequence in list
    mov lstrot,0
    inc evarot
  .ELSE
    mov eax,evaseq		  ;keep sequence in list
    mov lstseq,eax
    mov eax,evarot		  ;keep rotation number in list
    mov lstrot,ax
    inc evarot
  .ENDIF
  call elapse			  ;Keep Elapsed composition time in list
  mov lstsec,dh
  mov lstmin,cl
  shr ecx,8
  mov lsthour,cx
  .IF !excbest			  ;Increase length of best so far
    mov ax,lstlen
    .IF ax < excmin
	mov wrkmin,ax
	inc wrkmin		  ;avoid finding short comps of the same length
	mov rotcnt,0		  ;and don't look for rotations
    .ELSE
	mov ax,excmin
	mov wrkmin,ax
    .ENDIF
  .ENDIF
  mov cx,es                       ;Increment list segment
  movzx eax,cx                    ;keep new comp segment for sortlist in eax
  add cx,lstsiz
  mov es,cx
  mov genlst,es
  .IF !excsort
    call sortlist                 ;sort last item into list by score
  .ENDIF
  .IF !gendup			  ;If not a duplicate composition
    or genNew,TRUE		  ;Signal new composition found
    inc statfound		  ;count compositions satisfying criteria
    inc statcomps		  ;count compositions listed
  .ELSE
    mov es,ax			  ;remove duplicate comp from list segment
    mov genlst,es
    dec evarot			  ;reverse out sequence and rotation number
    .IF ZERO?
      dec evaseq
    .ENDIF
  .ENDIF
evalfin:
  .IF rotcnt != 0		  ;if more rotations to try
    jmp evalrpt			  ;evaluate next rotation
  .ENDIF
  popad
  ret                             ;return composition to caller
evalco ENDP
cntparts PROC USES fs gs          ;Count number of parts in composition
  pushad
  mov lstpar,1                    ;Default to 1 part
  xor eax,eax
  mov cx,12
  .REPEAT                         ;Check for 12,10,7,6,5,4,3,2 parts
    .CONTINUE .IF cx == 11 \      ;Ignore number of parts which cannot exist
			|| cx == 9  \
			|| cx == 8
    mov ax,genlea                 ;Divide no. of leads in comp by no. of parts
    xor edx,edx
    div cx
    .IF dx == 0                   ;If exactly divisible
	 mov fs,genlho[eax*2]        ;Check whether part end is suitable
	 .CONTINUE .IF cmppar != cl  ;And try next partsize if not
	 xor ebx,ebx                 ;Compare parts
	 .REPEAT
	   mov gs,genlho[ebx*2]
	   mov fs,genlho[eax*2]
	   mov dl,gs:cmpclo
	   inc eax
	   inc ebx
	   .IF cmpmus8 & 8000H       ;Multipart found if end of composition
		mov lstpar,cl           ;Store number of parts in list
		jmp @F                  ;and exit
	   .ENDIF
	 .UNTIL dl != cmpclo         ;Until calls different
   .ENDIF
  .UNTILCXZ
@@:popad
   ret
cntparts ENDP
elapse PROC USES eax;Calculates time elapsed (hhh(ch+ech):mm(cl):ss(dh):hh(dl) since composition started
  @GetDate
  push ax                         ;keep day of the week
  @GetTime
  pop ax
  sub dl,gensech
  .IF SIGN?
    add dl,100
    dec dh
  .ENDIF
  sub dh,gensec
  .IF SIGN?
    add dh,60
    dec cl
  .ENDIF
  sub cl,genmin
  .IF SIGN?
    add cl,60
    dec ch
  .ENDIF
  sub ch,genhour
  .IF SIGN?
    add ch,24
    dec al
  .ENDIF
  sub al,genday
  .IF SIGN?
    add al,7
  .ENDIF
  mov ah,24
  mul ah
  and eax,0FFFFh
  and ecx,0FFFFh
  shl eax,8
  add ecx,eax
  ret
elapse ENDP
score PROC                        ;Score composition using weighted factors
  pushad
  movzx eax,lstlen
  .IF ax >= excmin		  ;weight comps which are NOT best so far
    or lstscore,10000000h	  ;set high bit of score on
  .ELSE
    mov lstscore,eax		  ;set score to comp length if short
    jmp endscore
  .ENDIF
  mov cl,wgtlenhi		  ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lstlen
  neg eax
  add eax,40320
  mov cl,wgtlenlo                 ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lstpar
  mov cl,wgtpar                   ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lstcrub
  mov cl,wgtcrub                  ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lstcruf
  mov cl,wgtcruf                  ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lst5678b
  mov cl,wgt5678b                 ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lst5678f
  mov cl,wgt5678f                 ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  mov ebx,15                      ;four bell counts
.REPEAT
  movzx eax,lstmusb4
  mov cl,wgtmusb4                 ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lstmusf4
  mov cl,wgtmusf4                 ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lstmus87
  neg eax                         ;reverse effect of 87s
  add eax,720
  mov cl,wgtmus87                 ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lstmus82
  neg eax                         ;reverse effect of 82s
  add eax,720
  mov cl,wgtmus82                 ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
  movzx eax,lstmus83
  neg eax                         ;reverse effect of 83s
  add eax,720
  mov cl,wgtmus83                 ;weighting factor (power of 2)
  dec cl                          ;if weight is zero, score is zero
  .IF !SIGN?                      ;if weight is 1, score is field concerned
    shl eax,cl                    ;multiply by weighting factor
    add lstscore,eax              ;add to score
  .ENDIF
    dec ebx
.UNTIL ZERO?
endscore:
  popad
  ret
score ENDP
sortlist PROC USES es		  ;sort last list item into list by score
				  ;check for duplicate compositions
                                  ;eax passed with new comp list segment
  pushad
  mov gendup,0			  ;clear duplicate flag
  mov newnum,0                    ;relative list position of new composition
  mov es,ax                       ;es used for search through list
  mov edx,lstscore                ;new composition score in dx
  mov cx,lstsiz                   ;new composition size (paragraphs)
  mov bx,lstgrp                   ;lstgroup pointer
  .WHILE bx != ax                 ;Check until comparing with itself
    mov es,bx
    .IF edx == lstscore 	  ;if score = list score
      xor si,si			  ;see if comps are identical
      xor di,di 		  ;compare from lstsiz to lstmusf4
      push cx
      mov cx,OFFSET lsthour	  ;lsthour is next field after lstmusf4
      push ds
      mov ds,ax
      repe cmpsb
      pop ds
      pop cx
      .IF ZERO? 		  ;If compositions the same
	or gendup,TRUE		  ;set duplicate flag and
	.BREAK			  ;cancel sort
      .ENDIF
    .ENDIF
    .IF edx > lstscore		  ;if score greater than list score
      mov si,genlst               ;shuffle list from
      sub si,lstgrp
      mov di,si
      shl si,4
      sub si,4
      add di,cx                   ;to
      shl di,4
      sub di,4
      mov cx,genlst               ;calc number of dwords to move
      sub cx,bx
      shl cx,2
      mov es,lstseg
      push ds
        mov ds,lstseg
        std
        rep movsd
        cld
      pop ds
      mov ax,genlst               ;replace composition in list
      xor di,di
      xor si,si
      mov es,bx
      push ds
        mov ds,ax
        mov cx,ds:0               ;lstsiz
        shl cx,2
        rep movsd
      pop ds
      .IF dissel > 0		  ;If top of list is not selected
	mov cx,newnum		  ;adjust display composition number
	.IF cx <= disnum
	  inc disnum
	.ENDIF
	.IF cx <=dissel
	  inc dissel
	.ENDIF
      .ENDIF
      .BREAK
    .ENDIF
    add bx,lstsiz
    inc newnum                    ;calc number of inserted composition
  .ENDW
  popad
  ret
sortlist ENDP
dropcmp PROC USES AX CX           ;drop last composition from list
  mov ax,lstseg
  mov es,ax
  mov cx,statcomps
  dec cx
  .REPEAT
    add ax,lstsiz
    mov es,ax
  .UNTILCXZ
  mov genlst,ax                   ;adjust list pointer
  inc statdel                     ;adjust totals
  dec statcomps
  mov cx,statcomps
  dec cx
  .IF disnum > cx
    mov disnum,cx
  .ENDIF
  .IF dissel > cx
    mov dissel,cx
  .ENDIF
  ret                             ;ES returned as new GENLST value
dropcmp ENDP
END
