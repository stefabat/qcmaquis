//
//  kernels_cpu.cpp
//  VLI_ASM
//
//  Created by Tim Ewart on 22.03.12.
//  Copyright (c) 2012 __Université de Genève__. All rights reserved.
//
#include "kernels_cpu_asm.h"
#include "vli/utils/macro.h"
#include <boost/preprocessor/arithmetic/mul.hpp>
#include <boost/preprocessor/stringize.hpp>

namespace vli{
    namespace detail{
                   void muladd128_64_64(unsigned long int* x/* %%rdi */, unsigned long int const* y/* %%rsi */, unsigned long int const* z/* %%rdx -> rcx */){
                      asm( 
                          "movq (%%rsi)          ,%%rax             \n" /* a0 into rax */                   
                          "movq %%rdx            ,%%rcx             \n" /* save a0-rcx faster than stack */ 
                          "imulq (%%rcx)                            \n" /* lo rax, hi rdx   a0*b0 */        
                          "addq (%%rdi)          ,%%rax             \n"
                          "adcq 8(%%rdi)         ,%%rdx             \n"
                          "movq %%rax            ,(%%rdi)           \n" /* lo part save */
                          "movq %%rdx            ,8(%%rdi)          \n" /* hi part sve */
                          : : :"rax","rdx","rcx","memory"
                          );
                   }

                   void muladd256_128_128(unsigned long int* x/* %%rdi */, unsigned long int const* y/* %%rsi */, unsigned long int const* z/* %%rdx -> rbx */){
                      asm( 
                          "subq $0x20            ,%%rsp             \n" /* create stack frame */            
                          "movq %%rdx            ,%%rbx             \n" /* rdx uses by mul             */   
                          "xorq %%r10            ,%%r10             \n" /* r10 = 0 due to carry effect */   
                          "xorq %%r11            ,%%r11             \n" /* r11 = 0 due to carry effect */   
                          "xorq %%r14            ,%%r14             \n" /* r14 = 0 it is the sign 0+ 1-*/   \
                          "xorq %%r15            ,%%r15             \n" /* r15 = 0 it is the sign 0+ 1-*/   \
                           /* te a if negative and store into stack, in reverse order due to universal access */ 
                          "movq 8(%%rsi)         ,%%rax             \n" /* load a3 into r8, for the sign */ 
                          "cmpq $0               ,%%rax             \n" /* test a is negative(sign on a3)*/ 
                          "jns _Negativea_256_128_                  \n" /* number is negative, we negate */ 
                          "movq (%%rsi)          ,%%r8              \n" /* load a0 */                       
                          "notq %%r8                                \n" /* C2M, ~a0 */                      
                          "notq %%rax                               \n" /* C2M, ~a1 */                      
                          "addq $0x1             ,%%r8              \n" /* C2M, ~a0+1 */                    
                          "adcq $0x0             ,%%rax             \n" /* C2M, ~a1+CB */                   
                          "movq %%r8             ,-0x10(%%rsp)      \n" /* a0 into the stack -16 rsp */     
                          "movq %%rax            ,-0x08(%%rsp)      \n" /* a1 into the stack -8 rsp */      
                          "leaq  -0x10(%%rsp)    ,%%rsi             \n" /* rsi points to stack a0 > 0 */    
                          "movq $1               ,%%r14             \n" /* r14 = 0 it is the sign 0+ 1-*/   
                          "_Negativea_256_128_ :                    \n" /* end if structure */              
                          /* te a if negative and store into stack, in reverse order due to universal access */ 
                          "movq 8(%%rbx)         ,%%rax             \n" /* load a3 into r8, for the sign */ 
                          "cmpq $0               ,%%rax             \n" /* test a is negative(sign on a3)*/ 
                          "jns _Negativeb_256_128_                  \n" /* number is negative, we negate */ 
                          "movq (%%rbx)          ,%%r8              \n" /* load b0 */                       
                          "notq %%r8                                \n" /* C2M, ~b0 */                      
                          "notq %%rax                               \n" /* C2M, ~b1 */                      
                          "addq $0x1             ,%%r8              \n" /* C2M, ~b0+1 */                    
                          "adcq $0x0             ,%%rax             \n" /* C2M, ~b1+CB */                   
                          "movq %%r8             ,-0x20(%%rsp)      \n" /* b0 into the stack -16 rsp */     
                          "movq %%rax            ,-0x18(%%rsp)      \n" /* b1 into the stack -8 rsp */      
                          "leaq  -0x20(%%rsp)    ,%%rbx             \n" /* rsi points to stack b0 > 0 */    
                          "movq $1               ,%%r15             \n" /* r15 = 0 it is the sign 0+ 1-*/   
                          "_Negativeb_256_128_ :                    \n" /* end if structure */              
                          /*----------------------- a0 * b0, a0 * b1 start ------------------------*/ 
                          "movq (%%rsi)          ,%%rax             \n" /* a0 into rax */                   
                          "movq %%rax            ,%%rcx             \n" /* save a0-rcx faster than stack */ 
                          "mulq (%%rbx)                             \n" /* lo rax, hi rdx   a0*b0 */        
                          "movq %%rax            ,%%r8              \n" /* only one term, write into c0 */  
                          "movq %%rdx            ,%%r9              \n" /* a0b0hi into r8 */                
                          "movq %%rcx            ,%%rax             \n" /* reload rax(a0) from the stack */ 
                          "mulq 8(%%rbx)                            \n" /* a0 * b1 */                       
                          "addq %%rax            ,%%r9              \n" /* add a0b0hi + a0b1lo */           
                          "adcq %%rdx            ,%%r10             \n" /* save the a0b1hi into r9 */       
                          /*----------------------- a0 * b0, a0 * b1 end --------------------------*/ 
                          /*----------------------- a1 * b0, a1 * b1 start ------------------------*/ 
                          "movq 8(%%rsi)         ,%%rax             \n" /* a1 into rax */                   
                          "movq %%rax            ,%%rcx             \n" /* save a0-rcx faster than stack */ 
                          "mulq (%%rbx)                             \n" /* a1 * b0 */                       
                          "addq %%rax            ,%%r9              \n" /* l46 + a1b0lo */                  
                          "adcq %%rdx            ,%%r10             \n" /* l47 + a1b0hi + c */              
                          "adcq $0               ,%%r11             \n" /* possible carry, for 192 one adcq */ 
                          "movq %%rcx            ,%%rax             \n" /* reload rax(a1) from the stack */ 
                          "mulq 8(%%rbx)                            \n" /* a1*b1 */                         
                          "addq %%rax            ,%%r10             \n" /* a1b2lo to r9 */                  
                          "adcq %%rdx            ,%%r11             \n" /* a1b2hi + c  */                   
                          /*----------------------- a1 * b0, a1 * b1 end --------------------------*/ 
                          "xorq %%r14            ,%%r15             \n"                                     
                          "cmpq $0               ,%%r15             \n" /* r15 = 1 we negate */             
                          "je _IsNegativeResult_256_128_            \n" /* not equal ZF = 0, negate*/       
                          "notq %%r8                                \n" /* start2ComplementMethod negate */ 
                          "notq %%r9                                \n" /* 2CM negate */                    
                          "notq %%r10                               \n" /* 2CM negate */                    
                          "notq %%r11                               \n" /* 2CM negate */                    
                          "addq $0x1             ,%%r8              \n" /* 2CM add 1 */                     
                          "adcq $0x0             ,%%r9              \n" /* 2CM propagate CB */              
                          "adcq $0x0             ,%%r10             \n" /* 2CM propagate CB */              
                          "adcq $0x0             ,%%r11             \n" /* 2CM propagate CB */              
                          "_IsNegativeResult_256_128_ :             \n" /* end if negative result */        
                          "addq (%%rdi)          , %%r8             \n" /* add a0+b0 */      
                          "adcq 8(%%rdi)         , %%r9             \n" /* add a1+b1+c */    
                          "adcq 16(%%rdi)        , %%r10            \n" /* add a2+b2+c */    
                          "adcq 24(%%rdi)        , %%r11            \n" /* add a3+b3+c */    
                          "movq %%r8             ,(%%rdi)           \n" /* r8 -> c1 */                      
                          "movq %%r9             ,8(%%rdi)          \n" /* r9 -> c1 */                      
                          "movq %%r10            ,16(%%rdi)         \n" /* r10 -> c2 */                     
                          "movq %%r11            ,24(%%rdi)         \n" /* r11 -> c3 */                     
                          "addq $0x20            ,%%rsp             \n" /* destroy stack frame */           
                          : : : "rax","rbx","rcx","rdx","r8","r9","r10","r11","r14","r15","memory"   
                         );
                   }

                  void muladd384_192_192(unsigned long int* x/* %%rdi */, unsigned long int const* y/* %%rsi */, unsigned long int const* z/* %%rdx -> rbx */){
                           asm( 
                        /*-01*/ "subq $0x30            ,%%rsp             \n" /* create stack frame */            
                        /*00*/  "movq %%rdx            ,%%rbx             \n" /* rdx uses by mul             */   
                        /*01*/  "xorq %%r11            ,%%r11             \n" /* r10 = 0 due to carry effect */   
                        /*02*/  "xorq %%r12            ,%%r12             \n" /* r11 = 0 due to carry effect */   
                        /*03*/  "xorq %%r13            ,%%r13             \n" /* r12 = 0 due to carry effect */   
                        /*03*/  "xorq %%r14            ,%%r14             \n" /* r13 = 0 it is the sign 0+ 1-*/   
                        /*03*/  "xorq %%r15            ,%%r15             \n" /* r13 = 0 it is the sign 0+ 1-*/   
                        /* negate a if negative and store into stack, in reverse order due to universal access */ 
                        /*04*/  "movq 16(%%rsi)        ,%%rax             \n" /* load a3 into r8, for the sign */ 
                        /*05*/  "cmpq $0               ,%%rax             \n" /* test a is negative(sign on a3)*/ 
                        /*06*/  "jns _Negativea_384_192_                  \n" /* number is negative, we negate */ 
                        /*07*/  "movq (%%rsi)          ,%%r8              \n" /* load a0 */                       
                        /*08*/  "movq 8(%%rsi)         ,%%r9              \n" /* load a1 */                       
                        /*09*/  "notq %%r8                                \n" /* C2M, ~a0 */                      
                        /*10*/  "notq %%r9                                \n" /* C2M, ~a1 */                      
                        /*11*/  "notq %%rax                               \n" /* C2M, ~a2 */                      
                        /*12*/  "addq $0x1             ,%%r8              \n" /* C2M, ~a0+1 */                    
                        /*13*/  "adcq $0x0             ,%%r9              \n" /* C2M, ~a1+CB */                   
                        /*14*/  "adcq $0x0             ,%%rax             \n" /* C2M, ~a2+CB */                   
                        /*15*/  "movq %%r8             ,-0x18(%%rsp)      \n" /* a0 into the stack -24 rsp */     
                        /*16*/  "movq %%r9             ,-0x10(%%rsp)      \n" /* a1 into the stack -16 rsp */     
                        /*17*/  "movq %%rax            ,-0x08(%%rsp)      \n" /* a2 into the stack -8 rsp */      
                        /*18*/  "leaq  -0x18(%%rsp)    ,%%rsi             \n" /* rsi points to stack a0 > 0 */    
                        /*19*/  "movq $1               ,%%r14             \n" /* r13 = 0 it is the sign 0+ 1-*/   
                        /*20*/  "_Negativea_384_192_ :                    \n" /* end if structure */              
                        /* negate a if negative and store into stack, in reverse order due to universal access */ 
                        /*21*/  "movq 16(%%rbx)        ,%%rax             \n" /* load a3 into r8, for the sign */ 
                        /*22*/  "cmpq $0               ,%%rax             \n" /* test a is negative(sign on a3)*/ 
                        /*23*/  "jns _Negativeb_384_192_                  \n" /* number is negative, we negate */ 
                        /*24*/  "movq (%%rbx)          ,%%r8              \n" /* load b0 */                       
                        /*25*/  "movq 8(%%rbx)         ,%%r9              \n" /* load b1 */                       
                        /*26*/  "notq %%r8                                \n" /* C2M, ~b0 */                      
                        /*27*/  "notq %%r9                                \n" /* C2M, ~b1 */                      
                        /*28*/  "notq %%rax                               \n" /* C2M, ~b2 */                      
                        /*29*/  "addq $0x1             ,%%r8              \n" /* C2M, ~b0+1 */                    
                        /*30*/  "adcq $0x0             ,%%r9              \n" /* C2M, ~b1+CB */                   
                        /*31*/  "adcq $0x0             ,%%rax             \n" /* C2M, ~b2+CB */                   
                        /*32*/  "movq %%r8             ,-0x30(%%rsp)      \n" /* b0 into the stack -48 rsp */     
                        /*33*/  "movq %%r9             ,-0x28(%%rsp)      \n" /* b1 into the stack -40 rsp */     
                        /*34*/  "movq %%rax            ,-0x20(%%rsp)      \n" /* b2 into the stack -32 rsp */     
                        /*35*/  "leaq  -0x30(%%rsp)    ,%%rbx             \n" /* rsi points to stack b0 > 0 */    
                        /*36*/  "movq $1               ,%%r15             \n" /* r13 = 0 it is the sign 0+ 1-*/   
                        /*37*/  "_Negativeb_384_192_ :                    \n" /* end if structure */              
                        /*38*/  "xorq %%r10            ,%%r10             \n" /* r9 = 0  due to carry effect */   
                        /* --------------------------- a0 * b0, a0 * b1, a0 * b2 start ------------------------*/ 
                        /*39*/  "movq (%%rsi)          ,%%rax             \n" /* a0 into rax */                   
                        /*40*/  "movq %%rax            ,%%rcx             \n" /* save a0-rcx faster than stack */ 
                        /*41*/  "mulq (%%rbx)                             \n" /* lo rax, hi rdx   a0*b0 */        
                        /*42*/  "movq %%rax            ,%%r8              \n" /* only one term, write into c0 */  
                        /*43*/  "movq %%rdx            ,%%r9              \n" /* a0b0hi into r8 */                
                        /*44*/  "movq %%rcx            ,%%rax             \n" /* reload rax(a0) from the stack */ 
                        /*45*/  "mulq 8(%%rbx)                            \n" /* a0 * b1 */                       
                        /*46*/  "addq %%rax            ,%%r9              \n" /* add a0b0hi + a0b1lo */           
                        /*47*/  "adcq %%rdx            ,%%r10             \n" /* save the a0b1hi into r9 */       
                        /*48*/  "movq %%rcx            ,%%rax             \n" /* reload rax(a0) from the stack */ 
                        /*49*/  "mulq 16(%%rbx)                           \n" /* a0 * b2 */                       
                        /*50*/  "addq %%rax            ,%%r10             \n" /* add l11 + a0b2lo + c */          
                        /*51*/  "adcq %%rdx            ,%%r11             \n" /* add l01 + a0b2hi + c, end */     
                        /* --------------------------- a0 * b0, a0 * b1, a0 * b2 end --------------------------*/ 
                        /* --------------------------- a1 * b0, a1 * b1, a1 * b2 start ------------------------*/ 
                        /*52*/  "movq 8(%%rsi)         ,%%rax             \n" /* a1 into rax */                   
                        /*53*/  "movq %%rax            ,%%rcx             \n" /* save a0-rcx faster than stack */ 
                        /*54*/  "mulq (%%rbx)                             \n" /* a1 * b0 */                       
                        /*55*/  "addq %%rax            ,%%r9              \n" /* l46 + a1b0lo */                  
                        /*56*/  "adcq %%rdx            ,%%r10             \n" /* l47 + a1b0hi + c */              
                        /*57*/  "adcq $0               ,%%r11             \n" /* possible carry, for 192 one adcq , 256 two adcq, 320 tree adcq .... */                \
                        /*58*/  "movq %%rcx            ,%%rax             \n" /* reload rax(a1) from the stack */ 
                        /*59*/  "mulq 16(%%rbx)                           \n" /* a1*b2 */                         
                        /*60*/  "addq %%rax            ,%%r11             \n" /* l57 + a1b2lo + c */              
                        /*61*/  "adcq %%rdx            ,%%r12             \n" /* a1b2hi + c */                    
                        /*62*/  "movq %%rcx            ,%%rax             \n" /* reload rax(a1) from the stack */ 
                        /*63*/  "mulq 8(%%rbx)                            \n" /* a1*b1 */                         
                        /*64*/  "addq %%rax            ,%%r10             \n" /* a1b2lo to r9 */                  
                        /*65*/  "adcq %%rdx            ,%%r11             \n" /* a1b2hi + c  */                   
                        /*66*/  "adcq $0               ,%%r12             \n" /* r12 + c  */                      
                        /* --------------------------- a1 * b0, a1 * b1, a1 * b2 end --------------------------*/ 
                        /* --------------------------- a2 * b0, a2 * b1, a2 * b2 start ------------------------*/ 
                        /*67*/  "movq 16(%%rsi)        ,%%rax             \n" /* a2 to rax */                     
                        /*68*/  "movq %%rax            ,%%rcx             \n" /* copy into the stack */           
                        /*69*/  "mulq (%%rbx)                             \n" /* a2*b0 */                         
                        /*70*/  "addq %%rax            ,%%r10             \n" /* l64 + a2b0lo */                  
                        /*71*/  "adcq %%rdx            ,%%r11             \n" /* l65 + a2b0hi + c */              
                        /*72*/  "adcq $0               ,%%r12             \n" /* possible carry */                
                        /*73*/  "movq %%rcx            ,%%rax             \n" /* reload rax(a2) */                
                        /*74*/  "mulq 16(%%rbx)                           \n" /* a2*b2 */                         
                        /*75*/  "addq %%rax            ,%%r12             \n" /* a2b2lo + l31 + c*/               
                        /*76*/  "adcq %%rdx            ,%%r13             \n" /* a2b2hi + l03 + c*/               
                        /*77*/  "movq %%rcx            ,%%rax             \n" /* reload a2 */                     
                        /*78*/  "mulq 8(%%rbx)                            \n" /* a2*b1 */                         
                        /*79*/  "addq %%rax            ,%%r11             \n" /* a2b1lo + l36 */                  
                        /*80*/  "adcq %%rdx            ,%%r12             \n" /* a2b1hi + l39 */                  
                        /*81*/  "adcq $0               ,%%r13             \n" /* r12 + c */                       
                        /* --------------------------- a2 * b0, a2 * b1, a2 * b2 end --------------------------*/ 
                        /* ---------------------------           sign                --------------------------*/ 
                        /*81*/  "xorq %%r14            ,%%r15             \n"                                     
                        /*82*/  "cmpq $0               ,%%r15             \n" /* r15 = 1 we negate */             
                        /*83*/  "je _IsNegativeResult_384_192_            \n" /* not equal ZF = 0, negate*/       
                        /*84*/  "notq %%r8                                \n" /* start2ComplementMethod negate */ 
                        /*85*/  "notq %%r9                                \n" /* 2CM negate */                    
                        /*86*/  "notq %%r10                               \n" /* 2CM negate */                    
                        /*87*/  "notq %%r11                               \n" /* 2CM negate */                    
                        /*88*/  "notq %%r12                               \n" /* 2CM negate */                    
                        /*89*/  "notq %%r13                               \n" /* 2CM negate */                    
                                "addq $0x1             ,%%r8             \n"
                                "adcq $0x0             ,%%r9             \n"
                                "adcq $0x0             ,%%r10            \n"
                                "adcq $0x0             ,%%r11            \n"
                                "adcq $0x0             ,%%r12            \n"
                                "adcq $0x0             ,%%r13            \n"
                        /*96*/  "_IsNegativeResult_384_192_ :             \n" /* end if negative result */        
                                "addq (%%rdi)          ,%%r8             \n"
                                "adcq 8(%%rdi)         ,%%r9              \n" /* add a1+b1+c */                   
                                "adcq 16(%%rdi)        ,%%r10             \n" /* add a2+b2+c */                   
                                "adcq 24(%%rdi)        ,%%r11             \n" /* add a3+b3+c */                   
                                "adcq 32(%%rdi)        ,%%r12             \n" /* add a4+b4+c */                   
                                "adcq 40(%%rdi)        ,%%r13             \n" /* add a5+b5+c */                   
                        /*97*/  "movq %%r8             ,(%%rdi)           \n" /* r8 -> c1 */                      
                        /*98*/  "movq %%r9             ,8(%%rdi)          \n" /* r9 -> c1 */                      
                        /*99*/  "movq %%r10            ,16(%%rdi)         \n" /* r10 -> c2 */                     
                        /*100*/ "movq %%r11            ,24(%%rdi)         \n" /* r11 -> c3 */                     
                        /*101*/ "movq %%r12            ,32(%%rdi)         \n" /* r12 -> c4 */                     
                        /*102*/ "movq %%r13            ,40(%%rdi)         \n" /* r13 -> c5 */                     
                        /*103*/ "addq $0x30            ,%%rsp             \n" /* destroy stack frame */           
                           : : : "rax","rdx","rcx","rbx","r8","r9","r10","r11","r12","r13","r14","r15","memory"   
                           ); 
                        }

                         void muladd512_256_256(unsigned long int* x/* %%rdi */, unsigned long int const* y/* %%rsi */, unsigned long int const* z/* %%rdx -> rbx */){
                            asm( 
                                "subq $0x50            ,%%rsp             \n" /* destroy stack frame */           
                                "movq %%rdx            ,%%rbx             \n" /* rdx uses by mul             */   
                                "xorq %%r10            ,%%r10             \n" /* r10 = 0 due to carry effect */   
                                "xorq %%r11            ,%%r11             \n" /* r10 = 0 due to carry effect */   
                                "xorq %%r12            ,%%r12             \n" /* r11 = 0 due to carry effect */   
                                "xorq %%r13            ,%%r13             \n" /* r12 = 0 due to carry effect */   
                                "xorq %%r14            ,%%r14             \n" /* r13 = 0 it is the sign 0+ 1-*/   
                                "xorq %%r15            ,%%r15             \n" /* r13 = 0 it is the sign 0+ 1-*/   
                                /*te a if negative and store into stack, in reverse order due to universal access */ 
                                "movq 24(%%rsi)        ,%%rax             \n" /* load a3 into rax, for the sign*/ 
                                "cmpq $0               ,%%rax             \n" /* test a is negative(sign on a3)*/ 
                                "jns _Negativea_512_256_                  \n" /* number is negative, we negate */ 
                                "movq (%%rsi)          ,%%r8              \n" /* load a0 */                       
                                "movq 8(%%rsi)         ,%%r9              \n" /* load a1 */                       
                                "movq 16(%%rsi)        ,%%r10             \n" /* load a2 */                       
                                "notq %%r8                                \n" /* C2M, ~a0 */                      
                                "notq %%r9                                \n" /* C2M, ~a1 */                      
                                "notq %%r10                               \n" /* C2M, ~a2 */                      
                                "notq %%rax                               \n" /* C2M, ~a3 */                      
                                "addq $0x1             ,%%r8              \n" /* C2M, ~a0+1 */                    
                                "adcq $0x0             ,%%r9              \n" /* C2M, ~a1+CB */                   
                                "adcq $0x0             ,%%r10             \n" /* C2M, ~a2+CB */                   
                                "adcq $0x0             ,%%rax             \n" /* C2M, ~a3+CB */                   
                                "movq %%r8             ,-0x20(%%rsp)      \n" /* a0 into the stack -32 rsp */     
                                "movq %%r9             ,-0x18(%%rsp)      \n" /* a1 into the stack -24 rsp */     
                                "movq %%r10            ,-0x10(%%rsp)      \n" /* a2 into the stack -16 rsp */      
                                "movq %%rax            ,-0x08(%%rsp)      \n" /* a3 into the stack -8 rsp */      
                                "leaq  -0x20(%%rsp)    ,%%rsi             \n" /* rsi points to stack a0 > 0 */    
                                "movq $1               ,%%r14             \n" /* r14 = 0 it is the sign 0+ 1-*/   
                                "_Negativea_512_256_ :                    \n" /* end if structure */              
                                "movq %%r14            ,-0x48(%%rsp)      \n" /* a0 into the stack -72 rsp */     
                                /*te a if negative and store into stack, in reverse order due to universal access */ 
                                "movq 24(%%rbx)        ,%%rax             \n" /* load b3 into r8, for the sign */ 
                                "cmpq $0               ,%%rax             \n" /* test a is negative(sign on a3)*/ 
                                "jns _Negativeb_512_256_                  \n" /* number is negative, we negate */ 
                                "movq (%%rbx)          ,%%r8              \n" /* load b0 */                       
                                "movq 8(%%rbx)         ,%%r9              \n" /* load b1 */                       
                                "movq 16(%%rbx)        ,%%r10             \n" /* load b2 */                       
                                "notq %%r8                                \n" /* C2M, ~b0 */                      
                                "notq %%r9                                \n" /* C2M, ~b1 */                      
                                "notq %%r10                               \n" /* C2M, ~b2 */                      
                                "notq %%rax                               \n" /* C2M, ~b3 */                      
                                "addq $0x1             ,%%r8              \n" /* C2M, ~b0+1 */                    
                                "adcq $0x0             ,%%r9              \n" /* C2M, ~b1+CB */                   
                                "adcq $0x0             ,%%r10             \n" /* C2M, ~b2+CB */                   
                                "adcq $0x0             ,%%rax             \n" /* C2M, ~b3+CB */                   
                                "movq %%r8             ,-0x40(%%rsp)      \n" /* b0 into the stack -64 rsp */     
                                "movq %%r9             ,-0x38(%%rsp)      \n" /* b1 into the stack -56 rsp */     
                                "movq %%r10            ,-0x30(%%rsp)      \n" /* b1 into the stack -48 rsp */     
                                "movq %%rax            ,-0x28(%%rsp)      \n" /* b2 into the stack -40 rsp */     
                                "leaq  -0x40(%%rsp)    ,%%rbx             \n" /* rsi points to stack b0 > 0 */    
                                "movq $1               ,%%r15             \n" /* r15 = 0 it is the sign 0+ 1-*/   
                                "_Negativeb_512_256_ :                    \n" /* end if structure */              
                                "movq %%r15            ,-0x50(%%rsp)      \n" /* a0 into the stack -80 rsp */     
                                /*----------------------- a0 * b0, a0 * b1, a0 * b2, a0 *b3  start ------------------------*/ 
                                "xorq %%r10            ,%%r10             \n" /* reset to 0 because corrupted */ 
                                "xorq %%r14            ,%%r14             \n" /* reset to 0 because corrupted */   
                                "xorq %%r15            ,%%r15             \n" /* reset to 0 because corrupted */   
                                "movq (%%rsi)          ,%%rax             \n" /* a0 into rax */                   
                                "movq %%rax            ,%%rcx             \n" /* save a0-rcx faster than stack */ 
                                "mulq (%%rbx)                             \n" /* lo rax, hi rdx   a0*b0 */        
                                "movq %%rax            ,%%r8              \n" /* only one term, write into c0 */  
                                "movq %%rdx            ,%%r9              \n" /* a0b0hi into r8 */                
                                "movq %%rcx            ,%%rax             \n" /* reload rax(a0) from the stack */ 
                                "mulq 8(%%rbx)                            \n" /* a0 * b1 */                       
                                "addq %%rax            ,%%r9              \n" /* add a0b0hi + a0b1lo */           
                                "adcq %%rdx            ,%%r10             \n" /* save the a0b1hi into r9 */       
                                "movq %%rcx            ,%%rax             \n" /* reload rax(a0) from the stack */ 
                                "mulq 16(%%rbx)                           \n" /* a0 * b2 */                       
                                "addq %%rax            ,%%r10             \n" /* add l11 + a0b2lo + c */          
                                "adcq %%rdx            ,%%r11             \n" /* add l01 + a0b2hi + c, end */     
                                "movq %%rcx            ,%%rax             \n" /* reload rax(a0) from the stack */ 
                                "mulq 24(%%rbx)                           \n" /* a0 * b3 */                       
                                "addq %%rax            ,%%r11             \n" /* add l11 + a0b2lo + c */          
                                "adcq %%rdx            ,%%r12             \n" /* add l01 + a0b2hi + c, end */     
                                /*----------------------- a0 * b0, a0 * b1, a0 * b2, a0*b3 end -------------------*/ 
                                /*----------------------- a1 * b0, a1 * b1, a1 * b2, a1*b3 start ------------------------*/ 
                                "movq 8(%%rsi)         ,%%rax             \n" 
                                "movq %%rax            ,%%rcx             \n" 
                                "mulq 24(%%rbx)                           \n" 
                                "addq %%rax            ,%%r12             \n" 
                                "adcq %%rdx            ,%%r13             \n" 
                                "movq %%rcx            ,%%rax             \n" 
                                "mulq 16(%%rbx)                           \n" 
                                "addq %%rax            ,%%r11             \n" 
                                "adcq %%rdx            ,%%r12             \n" 
                                "adcq $0               ,%%r13             \n" 
                                "movq %%rcx            ,%%rax             \n" 
                                "mulq 8(%%rbx)                            \n" 
                                "addq %%rax            ,%%r10             \n" 
                                "adcq %%rdx            ,%%r11             \n" 
                                "adcq $0               ,%%r12             \n" 
                                "adcq $0               ,%%r13             \n" 
                                "movq %%rcx            ,%%rax             \n" 
                                "mulq (%%rbx)                             \n" 
                                "addq %%rax            ,%%r9              \n" 
                                "adcq %%rdx            ,%%r10             \n" 
                                "adcq $0               ,%%r11             \n" 
                                "adcq $0               ,%%r12             \n" 
                                "adcq $0               ,%%r13             \n" 
                                /*----------------------- a1 * b0, a1 * b1, a1 * b2, a1*b3 end   ------------------------*/ 
                                /*----------------------- a2 * b0, a2 * b1, a2 * b2, a2*b3 start -----------------------*/ 
                                "movq 16(%%rsi)        ,%%rax             \n" 
                                "movq %%rax            ,%%rcx             \n" 
                                "mulq 24(%%rbx)                           \n" 
                                "addq %%rax            ,%%r13             \n" 
                                "adcq %%rdx            ,%%r14             \n" 
                                "movq %%rcx            ,%%rax             \n" 
                                "mulq 16(%%rbx)                           \n" 
                                "addq %%rax            ,%%r12             \n" 
                                "adcq %%rdx            ,%%r13             \n" 
                                "adcq $0               ,%%r14             \n" 
                                "movq %%rcx            ,%%rax             \n"
                                "mulq 8(%%rbx)                            \n" 
                                "addq %%rax            ,%%r11             \n" 
                                "adcq %%rdx            ,%%r12             \n" 
                                "adcq $0               ,%%r13             \n" 
                                "adcq $0               ,%%r14             \n" 
                                "movq %%rcx            ,%%rax             \n" 
                                "mulq (%%rbx)                             \n" 
                                "addq %%rax            ,%%r10             \n" 
                                "adcq %%rdx            ,%%r11             \n" 
                                "adcq $0               ,%%r12             \n" 
                                "adcq $0               ,%%r13             \n" 
                                "adcq $0               ,%%r14             \n" 
                                /* ----------------------- a2 * b0, a2 * b1, a2 * b2, a2*b3 end -----------------------*/ 
                                /*----------------------- a3 * b0, a3 * b1, a3 * b2, a3*b3 start -----------------------*/ 
                                "movq 24(%%rsi)        ,%%rax             \n" 
                                "movq %%rax            ,%%rcx             \n" 
                                "mulq 24(%%rbx)                           \n" 
                                "addq %%rax            ,%%r14             \n" 
                                "adcq %%rdx            ,%%r15             \n" 
                                "movq %%rcx            ,%%rax             \n" 
                                "mulq 16(%%rbx)                           \n" 
                                "addq %%rax            ,%%r13             \n" 
                                "adcq %%rdx            ,%%r14             \n" 
                                "adcq $0               ,%%r15             \n" 
                                "movq %%rcx            ,%%rax             \n" 
                                "mulq 8(%%rbx)                            \n" 
                                "addq %%rax            ,%%r12             \n" 
                                "adcq %%rdx            ,%%r13             \n" 
                                "adcq $0               ,%%r14             \n" 
                                "adcq $0               ,%%r15             \n" 
                                "movq %%rcx            ,%%rax             \n" 
                                "mulq (%%rbx)                             \n" 
                                "addq %%rax            ,%%r11             \n" 
                                "adcq %%rdx            ,%%r12             \n" 
                                "adcq $0               ,%%r13             \n" 
                                "adcq $0               ,%%r14             \n" 
                                "adcq $0               ,%%r15             \n" 
                                /*----------------------- a3 * b0, a3 * b1, a3 * b2, a3*b3 start ----------------------*/ 
                                /*-----------------------           sign                --------------------------*/ 
                                "movq -0x48(%%rsp)     ,%%rax             \n"
                                "movq -0x50(%%rsp)     ,%%rdx             \n"
                                "xorq %%rax            ,%%rdx             \n"                                     
                                "cmpq $0               ,%%rdx             \n" /* r15 = 1 we negate */             
                                "je _IsNegativeResult_512_256_            \n" /* not equal ZF = 0, negate*/       
                                "notq %%r8                                \n" /* start2ComplementMethod negate */ 
                                "notq %%r9                                \n" /* 2CM negate */                    
                                "notq %%r10                               \n" /* 2CM negate */                    
                                "notq %%r11                               \n" /* 2CM negate */                    
                                "notq %%r12                               \n" /* 2CM negate */                    
                                "notq %%r13                               \n" /* 2CM negate */                    
                                "notq %%r14                               \n" /* 2CM negate */                    
                                "notq %%r15                               \n" /* 2CM negate */                    
                                "addq $0x1             ,%%r8              \n" /* 2CM add 1 */                     
                                "adcq $0x0             ,%%r9              \n" /* 2CM propagate CB */              
                                "adcq $0x0             ,%%r10             \n" /* 2CM propagate CB */              
                                "adcq $0x0             ,%%r11             \n" /* 2CM propagate CB */              
                                "adcq $0x0             ,%%r12             \n" /* 2CM propagate CB */              
                                "adcq $0x0             ,%%r13             \n" /* 2CM propagate CB */              
                                "adcq $0x0             ,%%r14             \n" /* 2CM propagate CB */              
                                "adcq $0x0             ,%%r15             \n" /* 2CM propagate CB */              
                                "_IsNegativeResult_512_256_ :             \n" /* end if negative result */        
                                "addq (%%rdi)          ,%%r8             \n" /* add a0+b0 */                     
                                "adcq 8(%%rdi)         ,%%r9             \n" /* add a1+b1+c */                   
                                "adcq 16(%%rdi)        ,%%r10            \n" /* add a2+b2+c */                   
                                "adcq 24(%%rdi)        ,%%r11            \n" /* add a3+b3+c */                   
                                "adcq 32(%%rdi)        ,%%r12            \n" /* add a4+b4+c */                   
                                "adcq 40(%%rdi)        ,%%r13            \n" /* add a5+b5+c */                   
                                "adcq 48(%%rdi)        ,%%r14            \n" /* add a5+b5+c */                   
                                "adcq 56(%%rdi)        ,%%r15            \n" /* add a5+b5+c */                   
                                "movq %%r8             ,(%%rdi)           \n" /* r8 -> c1 */                     
                                "movq %%r9             ,8(%%rdi)          \n" /* r9 -> c2 */                      
                                "movq %%r10            ,16(%%rdi)         \n" /* r10 -> c3 */                     
                                "movq %%r11            ,24(%%rdi)         \n" /* r11 -> c4 */                     
                                "movq %%r12            ,32(%%rdi)         \n" /* r11 -> c5 */                     
                                "movq %%r13            ,40(%%rdi)         \n" /* r11 -> c6 */                     
                                "movq %%r14            ,48(%%rdi)         \n" /* r11 -> c7 */                     
                                "movq %%r15            ,56(%%rdi)         \n" /* r11 -> c8 */                     
                                "addq $0x50            ,%%rsp             \n" /* destroy stack frame */           
                                 : : : "rax","rbx","rcx","rdx","r8","r9","r10","r11","r12","r13","r14","r15","memory"   
                            ); 
                    } 
    } //namespase detail
} //namespace vli
