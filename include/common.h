#ifndef COMMON_H
#define COMMON_H

/*
    File contains common definisions for compiler project

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com
*/


/* standarts libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* SSE */
#include <stdint.h> /* rly safe for bits operation */
#include <gmp.h>
#include <ctype.h>
#include <math.h>

/* linux main libs */
#include <unistd.h>
#include <fcntl.h>

/* my debug library */
#include <log.h>

#ifndef BOOL
    #define BOOL char
    #define TRUE 1
    #define FALSE 0
#endif


/* simple macros */
#define MIN(a ,b) ( (a) <= (b) ? (a) : (b) )
#define MAX(a, b) ( (a) > (b) ? (a) : (b) )

#define LOGK(n ,k) ( log(n) / log(k) )

/* compile to as fast code as posible, so we use built in functions */
#define LOG2(n) ( (n) ? ((unsigned) ( (sizeof(unsigned long long) << 3) - __builtin_clzll((n)) )) : -1)
#define HAMM_WEIGHT(n, k) (  __builtin_popcountll( (n) ^ (k) )  )

#define SWAP(a, b) \
    do{ \
        typeof(a) __temp = a; \
        a = b; \
        b = __temp; \
    }while(0)

#define FREE(PTR) \
    do { \
    	free(PTR); \
    	PTR = NULL; \
    }while(0)

/* BIT operations */
#define GET_BIT(n ,k)       ( ((n) & (1ull << (k) )) >> k)
#define SET_BIT(n ,k)       ( (n) |= (1ull << (k)))
#define CLEAR_BIT(n ,k)     ( (n) &= ~(1ull << (k)))

/* FOR BIG INT */
#define BGET_BIT(n, k)      mpz_tstbit(n , k)
#define BSET_BIT(n, k)      mpz_setbit(n , k)
#define BCLEAR_BIT(n, k)    mpz_clrbit(n , k)


#define ARRAY_SIZE(array) ( sizeof( (array) ) / sizeof( (array[0]) ) )

/* force inlining, if I said inline I want to INLINE !!!! */
#define __inline__ __attribute__((always_inline)) inline

/* make program safer, give me warn iff i put NULL to function requires non-null ptr */
#define __nonull__(...) __attribute__((nonnull(__VA_ARGS__ )))

/*
    convert mpz_t to unsigned long long
*/
__inline__ unsigned long long mpz2ull(mpz_t z)
{
    unsigned long long result = 0;
    mpz_export(&result, 0, -1, sizeof(result), 0, 0, z);
    return result;
}

/*
    convert unsigned long long to mpz_t
*/
__inline__ void ull2mpz(mpz_t z, unsigned long long ull)
{
    mpz_import(z, 1, -1, sizeof(ull), 0, 0, &ull);
}

__inline__ int BHAMM_WEIGHT(mpz_t val, uint64_t k)
{
    mpz_t z;
    int res;

    mpz_init(z);
    ull2mpz(z, k);

    mpz_xor(z, z, val);

    if(mpz_cmp_ui(z, 0) == 0)
        res = 0;
    else
        res = mpz_popcount(z);

    mpz_clear(z);

    return res;
}

/* ESCAPE COLORS */
#define RESET           "\033[0m"
#define BLACK           "\033[30m"
#define RED             "\033[31m"
#define GREEN           "\033[32m"
#define YELLOW          "\033[33m"
#define BLUE            "\033[34m"
#define MAGENTA         "\033[35m"
#define CYAN            "\033[36m"
#define WHITE           "\033[37m"
#define BOLDBLACK       "\033[1m\033[30m"
#define BOLDRED         "\033[1m\033[31m"
#define BOLDGREEN       "\033[1m\033[32m"
#define BOLDYELLOW      "\033[1m\033[33m"
#define BOLDBLUE        "\033[1m\033[34m"
#define BOLDMAGENTA     "\033[1m\033[35m"
#define BOLDCYAN        "\033[1m\033[36m"
#define BOLDWHITE       "\033[1m\033[37m"

#endif
