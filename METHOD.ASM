;*********************************************************
;* SMC - Single Method Composer                          *
;* Copyright Graham A C John 1991-98 All rights reserved *
;*********************************************************
	TITLE	  METHOD - Input Method Details
;
;    This routine input the method details , expands the lead and calculates
;    the false lead heads, creating a list. It also derives the leadhead
;    group for the method
;
.MODEL SMALL
.386
INCLUDE   smc.inc             ;Data Definitions
.CODE
EXTERN status:NEAR,transp:NEAR,execpn:NEAR,display:NEAR,message:near
method PROC
  mov si,offset stamet            ;Change Status
  call status
  inc metcur                      ;cycle through methods
  movzx eax,metcur
  .IF ax >= metnum                ;return to first method
    xor ax,ax
    mov metcur,ax
  .ENDIF
  mov exccom,TRUE		  ;prevent composition until table rebuilt
  mov gencom,FALSE
  mov si,offset mess5		  ;issue warning message to recompile table
  call message
  mov bx,93                       ;calculate method offset in eax
  mul bx
  lea si,metn01[eax]              ;copy method to current method
  mov di,offset metnam
  mov cx,93
  mov es,datseg
  rep movsb
;enter method details
  mov wrklea,04030201h            ;Set Rounds
  mov wrklea+4,08070605h
  mov esi,offset wrklea           ;Expand first lead of method
  mov edi,offset wrklea+8
  movzx ecx,metlen
  dec ecx
  xor edx,edx
  .REPEAT
    mov al,metnot[edx]            ;Load Place Notation
    call execpn                   ;Execute Place Notation
    add esi,8
    add edi,8
    inc edx
  .UNTILCXZ
  mov al,metcal                   ;Keep first leadhead of method
  mov edi,offset metleh
  call execpn
  mov eax,metleh                  ;Determine Leadhead Group
  mov edx,metleh+4
  mov ecx,6
  .REPEAT
  .BREAK .IF eax == pblhs[ecx*8] && edx == pblhs[ecx*8+4]
  .UNTILCXZ
  mov al," "
  .IF metcal == 0C0h
    mov ah,lhg2[ecx]
  .ELSE
    .IF metcal == 088h
	 mov ah,lhg8[ecx]
	 .IF metcal+1 == 0A0h
	   mov al,"x"
	 .ENDIF
    .ELSE
	 mov ah,"?"
    .ENDIF
  .ENDIF
  mov metlhg,ax
  mov eax,36                      ;Initialise display columns for method
  mov edx,8
  .REPEAT
    mov ecx,8                     ;find bell position
    .REPEAT
	 dec ecx
    .UNTIL BYTE PTR metleh[ecx] == dl
    mov metcol[ecx],al            ;set column array
    mov ch,calltit[ecx]           ;set call column headings
    mov edx,eax
    shr edx,1
    mov distit8[edx+1],ch
    mov dl,cl                     ;set tenor to its new place bell
    inc dl
    add eax,10                    ;next column
  .UNTIL eax == 106
  mov es,scrseg                   ;Display method name
  mov di,0
  mov si,offset metnam
  mov ah,colinf
  mov cx,25
  .REPEAT
    lodsb
    stosw
  .UNTILCXZ
  add di,2                        ;Display leadhead group
  mov al,BYTE PTR metlhg+1
  stosw
  mov al,BYTE PTR metlhg
  stosw
  call display
  call listflh                    ;List flhs for method
; call listfchg		   ;Convert flhs to fchgs
  mov  dx,FnExclud                ;Force next function to Exclude
  ret
method ENDP
listflh PROC
  mov es,datseg                   ;Used by transp
  mov flhcnt,0                    ;Count of flhs found
  mov esi,offset flhlist          ;esi is flhptr1 to flhlist
  mov edi,offset wrklea           ;edi is rowptr1 to wrklea
  movzx ecx,metlen                ;repeat for each row in lead
  .REPEAT
    push ecx                      ;Find treble in row
    mov ax,metlen                 ;Keep row number for each flh generated
    sub ax,cx
    inc al
    movzx ecx,flhcnt
    mov flhrow[ecx],al
    mov ecx,8                     ;ecx is bellptr in row
    mov al,01h                    ;Search for treble
    .REPEAT
	 dec ecx
    .UNTIL BYTE PTR [edi+ecx] == 01h
    mov edx,offset wrklea         ;edx is rowptr2 to wrklea
    mov eax,ecx                   ;Bellptr is now in eax
    movzx ecx,metlen              ;repeat comparing against each row in lead
    .REPEAT
	 .IF edi != edx \            ;Don't compare row with itself
	   && BYTE PTR [edx+eax] == 01h
	    push eax                 ;save eax around calls
	    mov ah,2                 ;Indicate Inverse transposition
	    call transp              ;?[esi] x [edx] = [edi]
	    call addflh              ;add flh to list if not repeated
	    pop eax
	  .ENDIF
	  add edx,8                  ;Next rowptr2
	.UNTILCXZ
	add edi,8                     ;Next rowptr1
	pop ecx
   .UNTILCXZ
   ret
listflh ENDP
 addflh PROC USES ecx edx
 mov cx,LENGTHOF flhlist          ;Check for flhlist overflow
 shr cx,1
 dec cx
 .IF (flhcnt >= cl)               ;Check for flhlist overflow
   push si
   mov si,offset mess2            ;issue error message
   call message
   pop si
 .ELSE
  movzx ecx,flhcnt                ;Check if flh already in list
  .IF ecx != 0
    mov ebx,offset flhlist        ;ebx is flhptr2
    mov eax,[esi]
    mov edx,[esi+4]               ;eax/edx contain comparison flh
    .REPEAT
	 .IF eax == [ebx] \
	  && edx == [ebx+4]
	   jmp @F                    ;flh already in list
	 .ENDIF
	   add ebx,8                 ;check next flh
    .UNTILCXZ
  .ENDIF
  inc flhcnt                      ;add new flh to list
  add esi,8
  movzx ecx,flhcnt                ;propogate flhrow position
  mov dl,flhrow[ecx-1]
  mov flhrow[ecx],dl
.ENDIF
@@:ret
addflh   ENDP
;listfchg PROC			   ;convert flhs to false course head groups
;  mov ecx,flhcnt
;  .REPEAT			   ;for each false lead head
;    mov eax,flhlist[ecx*8-8]
;    mov wrkfch1,eax
;    mov eax,flhlist[ecx*8-4]
;    mov wrkfch1+4,eax
;    .REPEAT
;    mov si,offset wrkfch1	   ;convert flh to fch
;    mov dx,offset wrkleh	   ;transpose by leadhead of method
;    mov di,offset wrkfch
;    mov eax,wrkfch
;    mov wrkfch1,eax
;    mov eax,wrkfch+4
;    mov wrkfch1,eax
;    .UNTIL BYTE PTR wrkfch+7 == 8 ;until course end reached
;    mov eax,wrkfch		   ;compress fch into a DWORD
;    shl eax,4
;    add eax,wrkfch+4
;    mov di,offset fchlist	   ;search fchlist
;    mov cx,720
;    repne scasd
;
;  .UNTILCXZ
;  ret
;listfchg ENDP
END
