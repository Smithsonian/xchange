/**
 * @file
 *
 * @date Created  on Apr 22, 2026
 * @author Attila Kovacs
 *
 *  Portable mutex macros
 */

#ifndef XMUTEX_H_
#define XMUTEX_H_

#if defined(_USE_PTHREAD) || defined(__unix__) || defined(__unix) || defined(__APPLE__)
#  include <pthread.h>
#  include <stdlib.h>         // for NULL

typedef pthread_mutex_t       xmut_type;                   ///< the mutex type

#  define XMUT_INITIALIZER    PTHREAD_MUTEX_INITIALIZER    ///< mutex initializer macro

#  define xmut_init(x)        pthread_mutex_init(x, NULL)  ///< initialize mutex (ignores return value)
#  define xmut_lock           pthread_mutex_lock           ///< lock mutex
#  define xmut_unlock         pthread_mutex_unlock         ///< unlock mutex
#  define xmut_destroy        pthread_mutex_destroy        ///< destroy mutex
#  define THREAD_SAFE         1                            ///< mutexes may be used

#elif __STDC_VERSION__ >= 201112L
#  include <threads.h>

typedef mtx_t                 xmut_type;                   ///< the mutex type (no initializer macro).

#  define xmut_init(x)        mtx_init(x, mtx_plain)       ///< initialize mutex (ignores return value)
#  define xmut_lock           mtx_lock                     ///< lock mutex
#  define xmut_unlock         mtx_unlock                   ///< unlock mutex
#  define xmut_destroy        mtx_destroy                  ///< destroy mutex
#  define THREAD_SAFE         1                            ///< mutexes may be used

#elif defined(_MSC_VER)
#  include <windows.h>

typedef SRWLOCK               xmut_type;                   ///< the mutex type

#  define XMUT_INITIALIZER    SRWLOCK_INIT                 ///< mutex initializer macro

#  define xmut_init(x)        InitializeSRWLock(x)         ///< initialize mutex
#  define xmut_lock           AcquireSRWLockExclusive      ///< lock mutex
#  define xmut_unlock         ReleaseSRWLockExclusive      ///< unlock mutex
#  define xmut_destroy(x)                                  ///< destroy mutex
#  define THREAD_SAFE         1                            ///< mutexes may be used

#else

typedef int                   xmut_type;                   ///< dummy mutex type

#  define XMUT_INITIALIZER    (-1)                         ///< mutex initializer macro

#  define xmut_init(x)                                     ///< dummy initialize mutex
#  define xmut_lock(x)                                     ///< dummy lock mutex
#  define xmut_unlock(x)                                   ///< dummy unlock mutex
#  define xmut_destroy(x)                                  ///< dummy destroy mutex
#  define THREAD_SAFE         0                            ///< mutexes may not be used


#endif



#endif /* XMUTEX_H_ */
