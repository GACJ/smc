TITLE COMGEN - Composition Generator
;
;    This routine uses the composing table to generate compositions
;    for the defined method. It checks for keyboard input periodically and
;    drops out if a key has been pressed. On re-entry it resumes where
;    it left off, thus acting as a background task. Whilst running a small
;    window for composing statistics is updated to monitor progress.
;    Once a composition has been found which meets the
;    specified minimum and maximum length, its properties are evaluated
;    against the composition exclusions. If OK the routine drops out to 
;    allow the composition list display to be updated (if currently active).
;
;    Register use is as follows:
;
;    ds :
;    es :  False leadhead LHOs
;    fs :  Current LHO in composition 
;    gs :  Next LHO in composition
;   esi :  Count of leads generated during composition since last update of statleads 
;   edi :  Pointer to current composition (genlea)
;   eax :  transient
;   ebx :  Current call (cmpclo)
;   ecx :  transient/loop control
;   edx :  Current length of composition
;
.MODEL SMALL
.486
.NOLIST
 INCLUDE dos.inc                  ;DOS Calls
 INCLUDE smc.inc		  ;Data Definitions
.LIST
.CODE
 EXTERN message:NEAR, dissta:NEAR, calpct:NEAR, status:NEAR, evalco:NEAR
 ASSUME ES:NOTHING
comgen PROC USES ES FS GS
  pushad
  mov si,offset stacmpg           ;display Composing
  call status
  mov gennew,FALSE                ;switch off new composition indicator
  xor esi,esi			  ;zero wait count for keyboard checking
  xor ebx,ebx			  ;clear current call register
  movzx edi,genlea                ;edi is lead offset in composition list
  mov fs,genlho[edi*2]            ;fs is current LHO (for composition resume)
  mov dx,genlen 		  ;dx is composition length (working)
  jmp comploop			  ;start composing
nextcall:
  .IF bl == gencallst \		  ;if last call
   || dx > excparmax		  ; or maximum part length exceeded
backtrack:			  ; then backtrack
    mov bl,gencalfst		  ;reset call of backtracked lead
    mov cmpclo,bl
    mov gencal[edi],bl		  ;clear composition while backtracking
    and cmpmus8,0FFFEh		  ;Reset include flag
    sub dx,cmplen		  ;reduce composition length
    .IF edi == 0		  ;Compositions exhausted?
      mov genend,TRUE		  ;indicate no more compositions
      mov si,offset mess3	  ;display message
      call message
      mov si,offset staclear	  ;Clear status
      call status
      mov genptr,0		  ;Clear Regeneration pointer (optimisation)
      mov optsync,FALSE		  ;Set optimization synchronization off
      jmp stopcomp
    .ENDIF
    dec edi			  ;reduce composition pointer
    .IF !exccalcnt		  ;Reduce count of calls by tenor position
      movzx ecx,cmptenor	  ;keep tenor position of lead being removed
      mov fs,genlho[edi*2]	  ;Make previous lead current
      mov bl,cmpclo		  ;and use call from it
      shl ebx,4
      dec gencalct[ecx*2+ebx]
      dec gencalct[ebx]		  ;and in total for type
      shr ebx,4			  ;reinstate ebx
      jmp nextcall		  ;shift call on backtrack lead
    .ENDIF
    mov fs,genlho[edi*2]	  ;Make previous lead current
    mov bl,cmpclo		  ;fetch call
    jmp nextcall		  ;shift call on backtrack lead
  .ENDIF
  .IF excrev
    inc cmpclo			  ;next call when composing forwards
  .ELSE
    dec cmpclo			  ;next call when composing backwards
  .ENDIF
  mov genptr,0			  ;Clear Regeneration pointer (optimisation)
  mov optsync,FALSE		  ;Set optimization synchronization off
comploop:
    .IF si >= TIMEOUT		  ;check keyboard periodically
      inc statleads		  ;count leads checked for in TIMEOUTs
      xor esi,esi		  ;clear temporary lead counter
      @GetStatus		  ;Check for key press
      .IF al || gendyn		  ;if key pressed or dynamic display then drop out
	jmp stopcomp		  ;stop composing
      .ELSE			  ;update progress status
	mov genlen,dx		  ;keep current composition length
	mov genlea,di		  ;keep list lho position
	call calpct		  ;display %complete
	call dissta		  ;display status of composition
      .ENDIF
    .ENDIF
    mov bl,cmpclo		  ;ebx is current CLO
    .IF !excopt			  ;Use optimisation
      .IF optsync \ 		  ;If synchronized regenerate composition
      || (bl == gencalfst \ 	  ;or the next call has not been set
      && (cmptenor == 7 \ 	  ;and either its after a home lead
	  || excsync))		  ;or synchronisation is not required
	mov optsync,TRUE
	mov ecx,genptr
	mov bl,gencal[ecx]	  ;set current call to regenerated call
	mov cmpclo,bl
	inc genptr
      .ENDIF
    .ENDIF
    cmp cmpcal[ebx*2],0		  ;If call excluded
    je nextcall
    inc si			  ;count leads generated and checked
    mov gs,cmpcal[ebx*2]	  ;gs is LHO of call
    test gs:cmpmus8,1		  ;If Lead already in the composition
    jne nextcall
    movzx ecx,gs:cmpfno		  ;Check false lead heads
    jcxz @F			  ;but only if there are some flhs
    .REPEAT
      mov es,gs:cmpflh[ecx*2-2]   ;Check false lead head
      test es:cmpmus8,1		  ;If false leadhead is in composition then
      jnz nextcall		  ;try next call
    .UNTILCXZ
@@: add dx,gs:cmplen		  ;add leadlength to composition length
    .IF dx > excmax		  ;if composition is too long
      sub dx,gs:cmplen		  ;reverse out previous addition
      jmp nextcall		  ;try next call
    .ENDIF
    .IF !exccalcnt		  ;If max/min call exclusions set then count calls
      movzx ecx,gs:cmptenor	  ;Count calls by tenor position
      shl ebx,4
      inc gencalct[ecx*2+ebx]
      mov ax,gencalct[ecx*2+ebx]
      .IF ax > excmaxp[ecx*2+ebx] ;if max number of calls in position exceeded
	sub dx,gs:cmplen	  ;reverse out previous additions
	dec gencalct[ecx*2+ebx]
	shr ebx,4		  ;reinstate ebx
	jmp nextcall		  ;try next call
      .ENDIF
      inc gencalct[ebx]		  ;Count calls by type
      mov ax,gencalct[ebx]
      .IF ax > excmaxp[ebx]	  ;if max number of calls of type exceeded
	sub dx,gs:cmplen	  ;reverse out previous additions
	dec gencalct[ecx*2+ebx]
	dec gencalct[ebx]
	shr ebx,4		  ;reinstate ebx (cmpclo)
	jmp nextcall		  ;try next call
      .ENDIF
      shr ebx,4			  ;reinstate ebx (cmpclo)
    .ENDIF
    mov gencal[edi],bl		  ;Include call in composition list
    inc edi			  ;Increment current compostion pointer
    mov genlho[edi*2],gs	  ;Include lead in composition list
    mov fs,genlho[edi*2]	  ;Make next lead current
    or cmpmus8,1		  ;Set include bit
    test cmpmus8,8000h		  ;if composition has not come round
    je comploop			  ; continue composition
				  ;COMPOSITION HAS COME ROUND
    cmp	dx,wrkmin		  ;Is it long enough (wrkmin/excmin)?
    jb backtrack		  ; if not force backtrack
    .IF !exccalcnt
      mov ecx,24		  ;check whether minimum call criteria met
      .REPEAT
	mov ax,gencalct[ecx*2-2]
	cmp ax,excminp[ecx*2-2]
	jb backtrack
      .UNTILCXZ
    .ENDIF
    mov genlen,dx		  ;keep current composition length
    mov genlea,di		  ;keep list position
    call evalco			  ;evaluate composition
    cmp genNew,0		  ;exit if new composition found
    jne stopcomp
    cmp genofl,0		  ;exit if list overflow (only if not sorted)
    je backtrack
stopcomp:
  mov genlen,dx			  ;keep current composition length
  mov genlea,di			  ;keep list lho position
  call calpct			  ;display %complete
  call dissta                     ;display status of composition
  popad
  mov dx,gendyn 		  ;set dx to TRUE if display or end required
  or dx,genNew			  ;otherwise set to FALSE for keypress
  or dx,genend
  or dx,genofl
  ret
comgen ENDP
END
