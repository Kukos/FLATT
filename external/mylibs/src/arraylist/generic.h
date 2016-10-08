#ifndef GENERIC_H
#define GENERIC_H

#include<string.h>

#if __SIZEOF_POINTER__ == 8
	#define __ARCH_64__
#else
	#define __ARCH_32__
#endif

#ifdef __ARCH_64__

    #define BYTE char
    #define HALF_WORD short
    #define WORD int
    #define DWORD long
    #define QWORD long double

    #define BYTE_SIZE 1
    #define HALF_WORD_SIZE 2
    #define WORD_SIZE 4
    #define DWORD_SIZE 8
    #define QWORD_SIZE 16

    #define MAXWORD 16

#elif defined(__ARCH_32__)

    #define BYTE char
    #define WORD short
    #define DWORD long
    #define QWORD long long
    #define SIXWORD long double

    #define BYTE_SIZE 1
    #define WORD_SIZE 2
    #define DWORD_SIZE 4
    #define QWORD_SIZE 8
    #define SIXWORD_SIZE 12

    #define MAXWORD 12

#endif

BYTE __buffer__[MAXWORD];

#define __SWAP__(A,B,S) do{ \
                            if( &A != &B) \
                            { \
                                memcpy(__buffer__,&A,S); \
                                memcpy(&A,&B,S); \
                                memcpy(&B,__buffer__,S); \
                            } \
                        }while(0);

#define __ASSIGN__(A,B,S) do{\
                              if( &A != &B) \
                                memcpy(&A,&B,S); \
                            }while(0);

#endif
