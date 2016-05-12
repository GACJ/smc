;*********************************************************
;* SMC - Single Method Composer                          *
;* Copyright Graham A C John 1991-98 All rights reserved *
;*********************************************************
	TITLE     TRANSP - Row Transposition
;
;        (ah=0)          (ah=1)            (ah=2)
;        Normal          Unknown      Inverse
;    Transposition        Transposer    Transposition
;
; A      142635          142635            [142635]    A
; B  x   164523         x  [164523]      x  164523     B
;        ------          ------             ------
; C  =  [156342]        =     156342          =  156342     C
;
;    There are three different forms of this function, as shown above.
;    In each case the required result is shown in brackets and the other
;    two rows in each case are supplied.
;
;    if ah=0  (Normal Transposition) the function transposes Row A by
;          Row B to give Row C.
;    if ah=1  (Unknown Transposer) the function finds the transposer
;          Row B which when applied to Row A results in Row C.
;    if ah=2  (Inverse Transposition) the function finds the source Row A
;          which when transposed by Row B would give Row C.
;    Inverse Transposition
;
;    ds:esi         Source Row Address   (Row A)
;    ds:edx         Transposer Row Address    (Row B)
;    ds:edi         Destination Row Address  (Row C)
;    Bells variable but set here to 8
;
;    Rows are expected to be in internal format (binary 1 to 0Ch)
;    Validated data only!
;
;    All changed registers are restored
;
	.MODEL SMALL
	.386
METBEL  EQU    8                  ;Number of Bells
	.CODE
transp  PROC USES eax ebx ecx
  .IF ah==0                       ;Transposition
    xor ebx,ebx
    mov ecx,metbel                ;Repeat for number of bells
    .REPEAT
      mov bl,[edx+ecx-1]          ;fetch transposer
      mov al,[esi+ebx-1]          ;fetch source bell
      mov [edi+ecx-1],al          ;place in transposed row
    .UNTILCXZ
  .ELSEIF ah==1                   ;Unknown Transposer
    mov ecx,metbel                ;Repeat for number of bells
    .REPEAT
      mov al,[edi+ecx-1]          ;fetch transposed bell
      mov ebx,metbel
      .WHILE al!=[esi+ebx-1]      ;find in source row
        dec ebx
      .ENDW
      mov [edx+ecx-1],bl          ;Result is position of bell in row
    .UNTILCXZ
  .ELSEIF ah==2                   ;Inverse Transposition
    xor ebx,ebx
    mov ecx,metbel                ;Repeat for number of bells
    .REPEAT
      mov bl,[edx+ecx-1]          ;fetch transposer
      mov al,[edi+ecx-1]          ;fetch result bell
      mov [esi+ebx-1],al          ;place in source row
    .UNTILCXZ
  .ENDIF
  ret                             ;Return to caller
transp ENDP
END
