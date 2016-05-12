;*********************************************************
;* SMC - Single Method Composer                          *
;* Copyright Graham A C John 1991-98 All rights reserved *
;*********************************************************
	TITLE     CLCLHO - Calculate LeadHead Offset for Row
;
;    This routine calculates a Lead Head Offset (LHO) into a proving array
;    for the supplied row. The LHO is a word indicating the segment address
;    of the array element (ie real address / 16)
;
;    es:edi	    Source Row Address
;    eax	    LHO Result
;    8              Number of bells
;
;    Rows are expected to be in internal format (binary 1 to 8)
;    Validated data only!
;
;    The algorithm is designed to arrange leads in order of tenor position,
;    followed by the seventh's position etc
;    ie 12345678 = 0, 18765432 = 5039, adjusted to a segment offset
;
;    eg:  Row       -   1    2    3    4    5    6    7    8
;         Positions -   1    3    5    2    7    4    8    6
;         Inverted  -   1    5    3    6    1    4    0    2
;         Reduced   -        0    0    2    0    2    0    2
;         * Factor  -        0    1    2    6   24  120  720   
;         Gives     -        0    0    4    0   48    0 1440 = 1492
;
;         lho=cmpseg+6(1492+1)
;                                                     
.MODEL SMALL
.386
INCLUDE   smc.inc	     ;Data Definitions
EXTERN    transp:near
.CODE
clclho	PROC USES ebx ecx edx esi edi
  mov esi,edi                     ;Transpose row into bell positions
  mov edi,offset rounds
  mov edx,offset wrktrans
  mov ah,1
  call transp
  mov edi,edx
  mov ecx,7
  .REPEAT                         ;invert positions
    mov al,8
    sub al, es:[edi+ecx]
    mov es:[edi+ecx],al
  .UNTILCXZ
  xor ebx,ebx                     ;zero lho accumulator
  mov ecx,7
  .REPEAT		          ;Process each number
    push ecx
    mov	dl,es:[edi+ecx]
    mov	dh,dl		          ;dl is multiplier, dh is current number
    mov eax,ecx
    mov ecx,7
    .REPEAT		          ;decrement multiplier for smaller numbers
      .BREAK .IF cx == ax
      .IF dh > es:[edi+ecx]       ;in higher position in row
	dec dl
        jz @F                     ;no point continuing with multiplier if zero
      .ENDIF
    .UNTILCXZ
    mov	ax,factorial[ecx*2]       ;Multiply factorial by the multiplier in dl
    xor	dh,dh
    mul	dx		          ;Intermediate result in ax
    add	bx,ax		          ;Accumulate result in bx
@@: pop ecx
  .UNTILCXZ
  movzx eax,bx                    ;Calculate final result in eax
  inc ax		          ;Avoid an LHO of zero (reserved)
  mov dx,6		          ;Adjust for segment length of table element ie 96/16)
  mul dx		          ;LHO is in (e)ax
  add ax,cmpseg                   ;Convert to absolute address
  ret		        	  ;Return LHO in (e)ax to caller
clclho ENDP
END
