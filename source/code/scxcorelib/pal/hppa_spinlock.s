;------------------------------------------------------------------------------
;    Copyright (c) Microsoft Corporation.  All rights reserved.
;
; Provides assembler funcions needed for HPPA spinlocks.    
; Date        2008-01-24 11:11:11

    .code
;   .level 2.0W ;  use this option for 64-bit assembly
    .export load_and_clear,entry,priv_lev=3,rtnval=gr
    .proc

load_and_clear
    .callinfo no_calls
    .enter

;  create a 16 byte aligned pointer to the load+clear word area
    addi 15,%arg0,%arg2 ; add 15 to the pointer to round up

;  Choose one of these statements and comment out the other:
    depi 0,31,4,%arg2   ;  mask the lower 4 bits (32-bit)
    ;depdi 0,63,4,%arg2 ;  mask the lower 4 bits (64-bit)
    bv (%r2) ;  return 0 if already locked, else !0
    ldcws,co (%arg2),%ret0  ; (in branch delay slot)
                            ;  load and clear (coherent) the spinlock word

;  This code is never executed. This endless loop reduces the
;  prefetching after mispredicted bv on pre PA 8700 processors.
;  Leave this dead code here to improve performance.
load_and_clear_loop
    b load_and_clear_loop
    nop
    .leave
    .procend
        
