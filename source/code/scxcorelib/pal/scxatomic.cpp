/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

 */
/**
    \file

    \brief     Platform independent atomic funcitonality implementation

    \date      2007-08-24 12:00:00

 */
/*----------------------------------------------------------------------------*/


#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxatomic.h>

#if defined(hpux)
#if defined(hppa)

#include <scxcorelib/scxthread.h>

//! Used to allign address on 32 bit.
#define alignedaddr(x) (volatile char *)(((int)(x)+15)&~0xf)

/**
  Using the same spinlock for all atomic operations. This simplifies the use 
  of scx_atomic_t since we do not need accessors (needed if it was a struct).
  The drawback of this is that all atomic operations will block on the same 
  spinlock but since this is such a short time we can live with this since it
  simplifies things for us in other places.
*/
static volatile char s_scx_atomic_spin_lock[16] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

extern "C" int load_and_clear(volatile char*);
extern "C" void _flush_globals();

/**
   Aquires a spin lock.
  
   \param lock - spinlock to aquire; must be at least 16 bytes of size.
  
   \date 2007-01-25 12:30:00

   Will sleep shortly after 100 tries to avoid consuming resources.
*/
__inline static void scx_spin_lock_aquire(volatile char* lock)
{
    while (1)
    {
        for (int spins=0; spins <= 100; ++spins)
        {
            if (*alignedaddr(lock))
            {
                if (load_and_clear(lock))
                {
                    return;
                }
            }
        }
        // Back off for a small period of time in order to not eat resources.
        SCXCoreLib::SCXThread::Sleep(1);
    }
}

/**
   Release a spin lock.
  
   \param lock - spinlock to release; must be at least 16 bytes of size.
  
   \date 2007-01-25 12:30:00
*/
__inline static void scx_spin_lock_release(volatile char* lock)
{
    _flush_globals();
    *alignedaddr(lock) = 1;
}

void scx_atomic_increment(scx_atomic_t* v)
{
    scx_spin_lock_aquire(s_scx_atomic_spin_lock);
    ++(*v);
    scx_spin_lock_release(s_scx_atomic_spin_lock);
}

bool scx_atomic_decrement_test(scx_atomic_t* v)
{
    bool r = false;
    scx_spin_lock_aquire(s_scx_atomic_spin_lock);
    r = (--(*v) == 0);
    scx_spin_lock_release(s_scx_atomic_spin_lock);
    return r;
}

#endif
#endif

