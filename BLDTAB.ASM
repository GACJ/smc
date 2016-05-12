TITLE BLDTAB - Build Proving Table for Method
;
;    This routine builds the proving and composing table for a method
;    and optimises the table for speed. Leads are excluded if Lead
;    exclusions have been selected or if call exclusions prevent access
;    to them. A recursive table generation subroutine is used to chase
;    down only those paths which the composition can include, given the
;    specified exclusions.
;
;    Note: due to the recursive nature of this routine, it requires a 64K
;	   stack. It uses the list group for this purpose
;
	.MODEL SMALL
	.386
        INCLUDE dos.inc           ;DOS Calls
	INCLUDE smc.inc           ;Data Definitions
	.CODE
	EXTERN clclho:NEAR, status:NEAR, transp:NEAR, dissta:NEAR, message:NEAR
bldtab	PROC USES eax ebx ecx edi
  cli				  ;change stack segment to list segment
  mov ax,ss			  ;save stack segment and registers
  mov stkseg,ax
  mov stksp,sp
  mov stkbp,bp
  mov ax,lstseg 		  ;new stack is list segment
  mov ss,ax
  mov sp,65534
  mov bp,sp
  sti				  ;new stack is in operation
  movzx eax,metnum		  ;modification security
  mov ecx,93			  ;check that method area has not been changed
  mul cl
  mov cx,ax
  xor eax,eax
  xor ebx,ebx
  mov si,offset metn01
  .REPEAT
    lodsb
    add ebx,eax
  .UNTILCXZ
  .IF ebx != metsec
    mov si,offset mess6		  ;issue error message
    call message
    mov dx,FnExclud
    jmp endbld
  .ENDIF
  @GetDate			  ;Keep Build Start Time
  mov genday,al
  @GetTime
  mov genhour,ch
  mov genmin,cl
  mov gensec,dh
  mov gensech,dl
  mov gennew,FALSE		  ;No new compositions
  mov genend,FALSE                ;Composition not ended
  mov gencom,TRUE		  ;Compose routine not suspended
  mov exccom,FALSE		  ;and composition allowed
  mov genofl,FALSE                ;List not overflowing
  mov statcomps,0		  ;Zero stats
  mov statleads,0
  mov statfound,0
  mov statdel,0
  mov stattab,0
  mov statflhhi,0
  mov statflhlo,0
  mov statpcent,0
  .IF !excbest
    mov wrkmin,0                  ;zero minimum length of best so far 
  .ELSE                           ;otherwise set it to the minimum length
    mov ax,excmin
    mov wrkmin,ax
  .ENDIF
  mov exccalcnt,TRUE		  ;check whether call counting is required
  xor eax,eax
  mov ecx,24
  .REPEAT			  ;all minimum exclusions zero?
    .IF excminp[ecx*2-2] > 0
      mov exccalcnt,FALSE
      jmp @F
    .ENDIF
  .UNTILCXZ
  mov ecx,24
  .REPEAT			  ;all maximum exclusions set high or zero?
    .IF excmaxp[ecx*2-2] < 5040 && excmaxp[ecx*2-2] > 0
      mov exccalcnt,FALSE
      jmp @F
    .ENDIF
  .UNTILCXZ
  xor eax,eax			  ;decide whether opt. sync. required
  mov excsync,TRUE		  ;assume sync excluded
  mov al,exccal 		  ;check plains
  and al,7Fh			  ;ignore first bit
  .IF ! (al == 7Fh || al == 0)
    mov excsync,FALSE
  .ENDIF
  mov al,exccal+1		  ;check bobs
  and al,7Fh			  ;ignore first bit
  .IF ! (al == 7Fh || al == 0)
    mov excsync,FALSE
  .ENDIF
  mov al,exccal+2		  ;check singles
  and al,7Fh			  ;ignore first bit
  .IF ! (al == 7Fh || al == 0)
    mov excsync,FALSE
  .ENDIF
@@:
  mov al,PLAIN			  ;determine first and last valid call types
  .IF exccal >= 7Fh		  ;Plains excluded?
    inc al
    .IF exccal+1 >= 7Fh		  ;Bobs excluded?
      inc al
    .ENDIF			  ;Singles assumed
  .ENDIF
  mov ah,SINGLE
  .IF exccal+2 >= 7Fh		  ;Singles excluded?
    dec ah
    .IF exccal+1 >= 7Fh		  ;Bobs excluded?
      dec ah
    .ENDIF			  ;Plains assumed
  .ENDIF
  .IF !excrev			  ;exchange first and last call if reverse composing
    xchg al,ah
  .ENDIF
  mov gencalfst,al		  ;store first and last possible call
  mov gencallst,ah
  call dissta			  ;Display stats
  mov si,offset stabld            ;Set Status
  call status
  mov si,offset mess0             ;Clear messages
  call message
  push es                         ;Clear Proving Table
  xor eax,eax       
  mov dx,cmpseg
  mov es,dx
  xor edi,edi
  mov ecx,26624/4
  rep stosd                       ;Clear 1st comp table segment
  add dx,26624/16                 ;Clear other 7 full 64K segments
  mov ecx,7
  .REPEAT
    push ecx
    mov es,dx
    xor edi,edi
    mov ecx,65536/4
    rep stosd
    add dx,65536/16
    pop ecx
  .UNTILCXZ
  pop es
  mov stattab,-1                  ;Zero included lead count (-1 for 2nd rounds)
  mov fs,cmpseg                   ;Set LHO ie fs for first Rounds
  mov DWORD PTR cmpleh,04030201h  ;Set Rounds in table
  mov DWORD PTR cmpleh+4,08070605h
  mov esi,offset wrklea           ;Initialise position of leadend
  call tabgen                     ;Generate composition table (recursive routine)
  call flhgen                     ;Transpose flhs for each table entry
  mov fs,cmpseg                   ;Reset generation variables
  mov genlho,fs                   ;Set first lead in lho list
  mov es,lstseg                   ;Set beginning of composition list
  mov genlst,es
  mov genlea,0                    ;Set lho pointer into lho list
  mov ax,cmplen                   ;Set length from first lead
  mov genlen,ax
  or cmpmus8,1                    ;Mark first rounds lead included
  mov cmptenor,7                  ;Set tenor position of first lead
  mov ecx,24			  ;Clear generation call counts
  xor ax,ax
  mov di,offset gencalct
  mov es,datseg
  rep stosw
  mov statleads,0                 ;Zero leads composed count
  mov genptr,0                    ;Zero optimisation pointer
  mov edi,offset gencal 	  ;clear call list to first call
  mov al,gencalfst
  mov ecx,5040
  rep stosb
  mov es,lstseg
  @GetDate                        ;Keep Composition Start Time
  mov genday,al
  @GetTime
  mov genhour,ch
  mov genmin,cl
  mov gensec,dh
  mov gensech,dl
  call dissta
  .IF exccom
    mov dx,FnExclud		  ;If flh overflow goto exclusions next
  .ELSE
    mov dx,FnDisLst		  ;otherwise Display Composition List
  .ENDIF
endbld:
  cli				  ;restore original stack segment
  mov ax,stkseg			  ;restore stack segment and registers
  mov ss,ax
  mov sp,stksp
  mov bp,stkbp
  sti				  ;original stack is back in operation
  ret				  ;Return to caller
bldtab	  ENDP
tabgen    PROC USES ax bx
;**************************** Generate Lead of Method in workspace
;                                  fs contains LHO
  push DWORD PTR ds:[esi]         ;Save leadend across recursive call
  push DWORD PTR ds:[esi+4]
  mov eax,DWORD PTR cmpleh        ;Copy Leadhead to workspace
  mov DWORD PTR wrklea,eax
  mov eax,DWORD PTR cmpleh+4
  mov DWORD PTR wrklea+4,eax
  mov esi,offset wrklea           ;Expand Lead
  mov edi,offset wrklea+8
  xor edx,edx
  movzx ecx,metlen
  mov wrkexc,FALSE                ;Set lead not excluded
  mov wrkbak,TRUE                 ;Set Backstroke
  mov wrklrnd,FALSE               ;Set lead does not contain rounds
  mov cmplen,cx                   ;Set leadlength in table
  call cntlea                     ;Compile Music Counts for Leadhead
  call cntprt                     ;Count parts
  dec ecx
  .REPEAT
    xor wrkbak,TRUE               ;Alternate Handstroke and Backstroke
    mov al,metnot[edx]            ;Load Place Notation
    call execpn                   ;Execute Place Notation
    add esi,8
    add edi,8
    call cntlea                   ;Compile Music Counts for Row
    inc edx
  .UNTILCXZ wrkexc                ;Early exit if row excluded
  .IF wrkexc
    mov cmpleh,-1                 ;Mark Lead excluded
  .ELSE
    .IF !wrklrnd                  ;No Call generation if lead contains rounds
      .WHILE cmpclo <= 2          ;While more calls exist
        movzx edx,cmpclo          ;Execute Call
        mov al,metcal[edx]        ;Load Call notation
        mov edi,offset wrkleh     ;Call leadhead destination
        call execpn               ;Execute PN
        xor ecx,ecx               ;Find Tenor Position
        mov al,80h
        .WHILE BYTE PTR wrkleh[ecx] != TENOR
          shr al,1
          inc ecx
        .ENDW
        test al,exccal[edx]       ;Check Call exclusions
        .IF ZERO?                 ;if not excluded
          mov es,datseg           ;calculate LHO for call
          mov edi,offset wrkleh
          mov bx,fs               ;save current LHO in bx
          call clclho             ;returns LHO in ax
          mov fs,ax               ;fs now points to LHO of call
          .IF cmpleh == 0         ;Has table entry for call been generated?
            mov edx,wrkleh        ;If no, set call lead head in table
            mov cmpleh,edx
            mov edx,wrkleh+4
            mov cmpleh+4,edx
            mov cmptenor,cl       ;keep tenor position - 1
            call tabgen           ;Recursive Call of tablegen
            mov fs,ax             ;Restore call lho
          .ENDIF
          .IF cmpleh != -1        ;If Lead not excluded for called lead
            mov fs,bx             ;Restore current LHO
            movzx edx,cmpclo
            mov cmpcal[edx*2],ax  ;Load LHO of called lead into table
          .ELSE
            mov fs,bx             ;Ensure current lead LHO is restored
          .ENDIF
        .ENDIF
        inc cmpclo                ;Set Next Call offset
      .ENDW
      mov al,gencalfst		  ;Default first call (depends on excrev)
      mov cmpclo,al
      .IF (cmpcal == 0) \
        && (cmpcal+2 == 0) \
        && (cmpcal+4 == 0)
        mov cmpleh,-1             ;If no calls possible & no internal rounds - exclude lead
      .ELSE
        inc stattab               ;Count included leads
        call dissta               ;Display updated stats
      .ENDIF
    .ENDIF
  .ENDIF
  pop DWORD PTR ds:[esi+4]        ;Restore leadend in workspace
  pop DWORD PTR ds:[esi]
  ret
tabgen ENDP
execpn PROC USES eax ecx esi edi
  mov ecx,esi
  add ecx,8
  .REPEAT
    shl al,1                      ;Test PN
    .IF CARRY?
	 movsb                       ;Place Made
    .ELSE
	 inc edi                     ;Swap Pair
	 movsb
	 sub edi,2
	 movsb
	 inc edi
    .ENDIF
  .UNTIL esi>=ecx
  ret
execpn ENDP
cntlea PROC USES eax ecx edx edi  ;Check music counts and exclusions for row [esi]
   mov edx,ecx                    ;Keep remaining rows in lead
   mov wrkrrnd,FALSE              ;init Row is not rounds
  .IF (DWORD PTR [esi] == 04030201h) \ ;Check and signal rounds
   && (DWORD PTR [esi+4] == 08070605h)
    mov ax,fs
    .IF ax != cmpgrp              ;Exclude 1st lead of table
      mov wrkrrnd,TRUE         ;Row is rounds
      mov wrklrnd,TRUE         ;Indicate that lead contains rounds
      sub cmplen,dx            ;Adjust leadlength if lead contains rounds
      or cmpmus8,8000h         ;Set rounds flag
    .ENDIF
  .ENDIF
mov ax,[esi+6]			  ;Check Exclusions - load back 2
.IF cx == metlen		  ;check leadhead exclusions
  and cmpmus8,0FFFDh		  ; first determine nature of leadhead
  xor eax,eax
  mov ecx,[esi]
  mov natrow,ecx		  ;use natrow as workspace
  mov ecx,[esi+4]
  mov natrow+4,ecx
  mov ecx,8
  .REPEAT
    mov al,BYTE PTR natrow[ecx-1]
    .WHILE al != cl
      xchg al,BYTE PTR natrow[eax-1]
      xor cmpmus8,2		  ;swap nature
    .ENDW
  .UNTILCXZ
  mov ax,[esi+6]
  .IF ((excleh && wrkrrnd) \
  || (exctschs && al == 7 && ah !=8) \
  || (exctschs && ah == 8 && al !=7))
				  ;if rounds excluded as leadhead
				  ; or coursehead is tenors split (78 not home together)
    mov wrkexc,TRUE		  ;  then mark lead excluded
  .ELSE
    .IF ah == 8			  ;if coursehead - check inclusion masks
      xor ecx,ecx
      .REPEAT			  ;check include masks
	mov eax,[esi]		  ;overlay front and back four bells
	shl eax,4
	add eax,[esi+4]
	.IF cmpmus8 & 2
	  or eax,0F000000h	  ;set bell 8 to F if leadhead is -ve
	.ENDIF
	and eax,incmask[ecx*4]	  ;mask out variable bells
	cmp eax,inccomp[ecx*4]	  ;now compare with include row
	je @F			  ;and include if OK
	inc ecx
      .UNTIL ecx == LENGTHOF incmask
      mov wrkexc,TRUE		  ;exclude coursehead if no include rows matched
@@:   mov ax,[esi+6]		  ;reload back two bells
    .ENDIF
  .ENDIF
.ELSE				  ;check non leadhead exclusions
  .IF (excint && wrkrrnd)	  ;if internal rounds
    mov wrkexc,TRUE		  ;  then mark lead excluded
  .ENDIF
.ENDIF
				  ;check all rows for exclusions
.IF (exc87s && wrkbak && ax == 0708h)\;if 87s at backstroke
 || (exc83s && ax == 0308h) \ 	  ;or 83s
 || (exc82s && ax == 0208h) \ 	  ;or 82s
 || (excbak && wrkrrnd && wrkbak)\;or rnds at back
 || (exchnd && wrkrrnd && !wrkbak);or rnds at hand
  mov wrkexc,TRUE		  ;then mark lead excluded
.ENDIF
.IF !wrkexc			  ;if lead not excluded
  .IF !wrklrnd                    ;Accumulate music counts if before rounds
    mov  ax,[esi+6]               ;Load Back 2
    .IF ((ax == 0708h) && wrkbak) ;Count 87s at backstroke
	 inc cmpmus87
    .ELSE
      .IF ax == 0208h             ;Count 82s
        inc cmpmus82
      .ELSE
        .IF ax == 0308h           ;Count 83s
          inc cmpmus83
        .ENDIF
      .ENDIF
    .ENDIF
    mov eax,[esi+4]               ;Load back 4
    mov edi,offset musmsk4
    movzx ecx,muscnt4
    repne scasd                   ;Search back 4 masks
    .IF ZERO?                     ;Update Music count in composition table
	 sub edi,offset musmsk4+4
	 shr edi,2
	 inc cmpmusb4[edi]
    .ENDIF
    .IF (eax > 08070000h) \       ;Check combination roll-ups
	&& (al >= 04h) && (ah >= 04h)
	 inc cmpcrub
    .ENDIF
    add ax,[esi+6]                ;Add 4 bytes together for (5678) combinations
    add al,ah
    .IF al == 5+6+7+8
	 inc cmp5678b
    .ENDIF
    mov eax,[esi]                 ;Load Front 4
    mov edi,offset musmsk4
    movzx ecx,muscnt4
    repne scasd                   ;Search front 4 masks
    .IF ZERO?                     ;Update Music count in composition table
	 sub edi,offset musmsk4+4
	 shr edi,2
	 inc cmpmusf4[edi]
    .ENDIF
    .IF (eax > 08070000h) \       ;Check combination roll-ups
	&&   (al >= 04h) && (ah >= 04h)
	 inc cmpcruf
    .ENDIF
    add ax,[esi+2]                ;Add 4 bytes together for (5678) combinations
    add al,ah
    .IF al == 5+6+7+8
	 inc cmp5678f
    .ENDIF
    mov eax,[esi]                 ;Overlay front 4 & back 4 for 8 bell counts
    shl eax,4
    add eax,[esi+4]
    mov edi,offset musmsk8
    movzx ecx,muscnt8             ;muscnt8 must = 16 for correct shift
    repne scasd                   ;Search 8 bell masks
    .IF ZERO?                     ;Update 8 bell flags in composition table
	 mov ax,1
	 shl ax,cl
	 or cmpmus8,ax
    .ENDIF
  .ENDIF
.ENDIF
  ret
cntlea ENDP
cntprt PROC USES es edx edi esi cx
  mov es,datseg
  mov edx,offset wrklea           ;Transposer (ie the leadhead)
  mov DWORD PTR [edi],04030201h   ;Start Row
  mov DWORD PTR [edi+4],08070605h
  mov cx,0
  .REPEAT
    add esi,8
    add edi,8
    inc cx                        ;cx = number of parts
    mov ax,0                      ;Indicate normal transposition
    call transp                   ;Tranpose Rounds by leadhead
  .UNTIL (DWORD PTR [edi] == 04030201h) \ ;Until Rounds
	 && (DWORD PTR [edi+4] == 08070605h)
  mov cmppar,cl                   ;Record number of parts in table
  ret
cntprt ENDP
flhgen PROC                       ;Generate FLH's for each lead in table
  mov si,offset staflh            ;Set status
  call status
  mov stattab,-1                  ;Zero included lead count (-1 for 2nd rounds)
  mov statflhhi,0                 ;Set Hi/Lo FLH stats
  mov statflhlo,100
  mov bx,cmpseg
  mov cx,5041                     ;Repeat for each table entry
flhnext:
    push cx
    mov fs,bx                     ;Set Segment for lead
    mov eax,cmpleh                ;Copy leadhead to workspace
  .IF eax != 0 \                  ;Only process non excluded leads
   && cmpleh != -1
    mov wrklea,eax
    mov eax,cmpleh+4
    mov wrklea+4,eax
    mov esi,offset wrklea
    mov edi,offset wrkleh
    mov edx,offset flhlist
    mov wrkfno,0		  ;zero count of false leadheads for lead
    movzx cx,flhcnt               ;Repeat for each flh in list
    .REPEAT
      movzx eax,flhcnt            ;Only count flhs before rounds, if in lead
      sub al,cl                   ;
      mov al,flhrow[eax]          ;
      .BREAK .IF cmplen < ax      ;
	 push cx
	 mov ah,0                    ;Indicate normal transposition
	 call transp                 ;tranpose into wrkleh
	 call clclho                 ;derive lho of transposed lead
	 mov gs,ax                   ;gs is segment pointer for new lead
	 .IF gs:cmpleh > SDWORD PTR 0;If lead not excluded, record lho of flh
	   movzx ecx,cmpfno
	   .IF cx < LENGTHOF cmpflh  ;Check for flh table limit exceeded
             mov cmpflh[ecx*2],ax    ;and if ok record lho of flh in array
             sub ax,6
             .IF ax == cmpseg
		mov cmpflh[ecx*2],ax ;make flhs of rounds ref lead 0 not lead 1
             .ENDIF
		inc cmpfno		;count flh's in array
	   .ELSE
		mov eax,esi
		mov si,offset mess1     ;issue error message
		call message
		mov gencom,FALSE
		mov exccom,TRUE
		mov esi,eax
	   .ENDIF
	   inc wrkfno			;count total flh's
	 .ENDIF
	 add edx,8                   ;Next FLH
	 pop cx
    .UNTILCXZ
    mov cl,wrkfno
    .IF cl > statflhhi
	 mov statflhhi,cl            ;Maintain flh hi count
    .ELSE
	 .IF cl < statflhlo
	   mov statflhlo,cl          ;Maintain flh lo count
	 .ENDIF
    .ENDIF
    inc stattab                   ;Count leads for which FLHs calculated
    call dissta                   ;Display stats
  .ENDIF
    add bx,96/16                  ;Next lead
    pop cx
    dec cx
    .IF cx != 0
	 jmp flhnext
    .ENDIF
  ret
flhgen ENDP
  END
