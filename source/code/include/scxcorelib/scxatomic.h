/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Provides atomic increment and decrement operations. 
    
    \date        2008-01-14 11:11:14
    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXATOMIC_H
#define SCXATOMIC_H

#if defined(hpux)
#include <inttypes.h>
#include <machine/sys/inline.h>
#elif defined(linux) && defined(SUSE10)
#include <linux/config.h>
#elif defined(sun)
#if (PF_MAJOR==5) && (PF_MINOR>9)
// atomic.h does not exist on Solaris 8/9 
#include <atomic.h>
#endif
#include <scxcorelib/scxcmn.h> // scxulong

#elif defined(aix)
#include <sys/atomic_op.h>
#elif defined(macos)
#include <libkern/OSAtomic.h>
#elif defined(WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400 //!< Needed for InterlockedIncrement & InterlockedDecrement
#endif
#include <windows.h>
#endif

/* Definitions of the atomic datatype */
#if defined(hpux)
typedef volatile int32_t scx_atomic_t; //!< Atomic counter datatype.
#elif defined(linux)
typedef volatile int scx_atomic_t; //!< Atomic counter datatype.
#elif defined(sun)

#if (PF_MAJOR==5) && (PF_MINOR>9)
typedef volatile scxulong scx_atomic_t; //!< Atomic counter datatype.
#else
typedef long scx_atomic_t;
#endif

#elif defined(WIN32)
typedef volatile long int scx_atomic_t;
#elif defined(aix)
// AIX uses atomic_p but that's just a pointer to int
// No volatile since that messes up the type cast
typedef int scx_atomic_t; //!< Atomic counter datatype.
#elif defined(macos)

typedef int scx_atomic_t; //!< Atomic counter datatype.

#else
#error "Platform not supported"
#endif

/*----------------------------------------------------------------------------*/

/**
    \fn void scx_atomic_increment(scx_atomic_t* v)

    Atomic increment of given counter.

    \param       v Pointer to an atomic value.
    
*/

/*----------------------------------------------------------------------------*/

/**
    \fn bool scx_atomic_decrement_test(scx_atomic_t* v)

    Atomic decrement of given counter.

    \param       v Pointer to an atomic value.
    \returns     true if the counter reaches zero after the decrement, otherwise false.

*/

#if defined(hpux)
#if defined(hppa)
/* inline implementation is not provided due to its complexity, dependency on scxthread header file
   and static global data declaration */
void scx_atomic_increment(scx_atomic_t* v);
bool scx_atomic_decrement_test(scx_atomic_t* v);

#else
/* The implementation below has been taken from machine/sys/builtins.h on a v11.3 machine
   since v11.2 machines does not seem to have these functions.
*/
__inline static void scx_atomic_increment(scx_atomic_t* v)
{
    (void)_Asm_fetchadd(_FASZ_W, _SEM_ACQ, v, 1, _LDHINT_NONE);
}

__inline static bool scx_atomic_decrement_test(scx_atomic_t* v)
{
    uint64_t PreVal;
    PreVal = _Asm_fetchadd(_FASZ_W, _SEM_ACQ, v, -1, _LDHINT_NONE);
    PreVal = _Asm_sxt(_XSZ_4, PreVal);
    return PreVal == 1;
}
#endif

#elif defined(linux)
static __inline__ void scx_atomic_increment(scx_atomic_t* v)
{
    __asm__ __volatile__(
        "lock ; " "incl %0"
        :"=m" (*v)
        :"m" (*v));
}

static __inline__ bool scx_atomic_decrement_test(scx_atomic_t* v)
{
    unsigned char c;

    __asm__ __volatile__(
        "lock ; " "decl %0; sete %1"
        :"=m" (*v), "=qm" (c)
        :"m" (*v) : "memory");
    return c != 0;
}
#elif defined(sun)

// Built in atomic operations are not available at user level on Solaris 8/9, WI7937
// assembler implementation provided instead 
#if (PF_MAJOR==5) && (PF_MINOR<10)
extern "C" {
scx_atomic_t AtomicDecrement( scx_atomic_t* pValue );
scx_atomic_t AtomicIncrement( scx_atomic_t* pValue );
}
#endif 

inline static void scx_atomic_increment(scx_atomic_t* v)
{
#if (PF_MAJOR==5) && (PF_MINOR<10)
        AtomicIncrement(v);
#else
    atomic_inc_64(v);
#endif
}

inline static bool scx_atomic_decrement_test(scx_atomic_t* v)
{
#if (PF_MAJOR==5) && (PF_MINOR<10)
        return AtomicDecrement(v) == 0;
#else
    return atomic_dec_64_nv(v) == 0;
#endif
}
#elif defined(WIN32)
inline static void scx_atomic_increment(scx_atomic_t* v)
{
    InterlockedIncrement(v);
}

inline static bool scx_atomic_decrement_test(scx_atomic_t* v)
{
    return InterlockedDecrement(v) == 0;
}
#elif defined(aix)
static inline void scx_atomic_increment(scx_atomic_t* v)
{
    fetch_and_add(v, 1);
}

static inline bool scx_atomic_decrement_test(scx_atomic_t* v)
{
    return fetch_and_add(v, -1) - 1 == 0;
}
#elif defined(macos)
static inline void scx_atomic_increment(scx_atomic_t* v)
{
    OSAtomicIncrement32(v);
}

static inline bool scx_atomic_decrement_test(scx_atomic_t* v)
{
    return OSAtomicDecrement32(v) == 0;
}

#endif


#endif /* SCXATOMIC_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
