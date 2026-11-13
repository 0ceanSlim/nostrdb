#ifndef NDB_THREAD_H
#define NDB_THREAD_H

#ifdef _WIN32
  #include <windows.h>

  #define     ErrCode()       GetLastError()
// Define POSIX-like thread types
typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

#define ErrCode() GetLastError()

// Mutex functions
#define pthread_mutex_init(mutex, attr) \
    (InitializeCriticalSection(mutex), 0)

#define pthread_mutex_destroy(mutex) \
    (DeleteCriticalSection(mutex), 0)

#define pthread_mutex_lock(mutex) \
    (EnterCriticalSection(mutex), 0)

#define pthread_mutex_unlock(mutex) \
    (LeaveCriticalSection(mutex), 0)

// Condition variable functions
#define pthread_cond_init(cond, attr) \
    (InitializeConditionVariable(cond), 0)

#define pthread_cond_destroy(cond)

#define pthread_cond_signal(cond) \
    (WakeConditionVariable(cond), 0)

#define pthread_cond_broadcast(cond) \
    (WakeAllConditionVariable(cond), 0)

#define pthread_cond_wait(cond, mutex) \
    (SleepConditionVariableCS(cond, mutex, INFINITE) ? 0 : ErrCode())

#define pthread_cond_broadcast(cond) \
    (WakeAllConditionVariable(cond), 0)

// Thread functions
#define THREAD_CREATE(thr, start, arg) \
    (((thr = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start, arg, 0, NULL)) != NULL) ? 0 : ErrCode())

#define THREAD_FINISH(thr) \
    (WaitForSingleObject(thr, INFINITE), CloseHandle(thr), 0)

#define THREAD_TERMINATE(thr) \
    (TerminateThread(thr, 0) ? ErrCode() : 0)

#else // _WIN32
  #include <pthread.h>

  //#define     ErrCode()       errno

  // grain fork: ask for an explicit stack. musl gives new threads 128 KiB,
  // and the writer thread's batch arrays alone take ~290 KiB of its frame,
  // so on Alpine it overflowed into whatever sat below it. 8 MiB is what
  // glibc hands out by default, so every libc now behaves the same.
  #define NDB_THREAD_STACK_SIZE (8 * 1024 * 1024)

  static inline int ndb_thread_create(pthread_t *thr,
				      void *(*start)(void *), void *arg)
  {
	pthread_attr_t attr;
	int rc;

	if ((rc = pthread_attr_init(&attr)))
		return rc;
	if ((rc = pthread_attr_setstacksize(&attr, NDB_THREAD_STACK_SIZE)) == 0)
		rc = pthread_create(thr, &attr, start, arg);
	pthread_attr_destroy(&attr);
	return rc;
  }

  #define THREAD_CREATE(thr,start,arg)	ndb_thread_create(&(thr),start,arg)
  #define THREAD_FINISH(thr)	pthread_join(thr,NULL)
  #define THREAD_TERMINATE(thr)	pthread_exit(&thr)
  
  #define LOCK_MUTEX(mutex)	pthread_mutex_lock(mutex)
  #define UNLOCK_MUTEX(mutex)	pthread_mutex_unlock(mutex)

#endif

#endif // NDB_THREAD_H
