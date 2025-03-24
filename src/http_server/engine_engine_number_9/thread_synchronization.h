#ifndef ENGINE_ENGINE_NUMBER_9_THREAD_SYNCHRONIZATION_H
#define ENGINE_ENGINE_NUMBER_9_THREAD_SYNCHRONIZATION_H

#include "baza.h"
#include <pthread.h>

namespace een9 {
    class RwlockObj {
    protected:
        pthread_rwlock_t mut;
        int lock = 0;
        const char* me;

        void unlock(int el, const char* who);

    public:
        RwlockObj(const RwlockObj& ) = delete;
        RwlockObj& operator=(const RwlockObj& ) = delete;

        explicit RwlockObj(const char* me = NULL);

        void read_lock(const char* who = "");

        void write_lock(const char* who = "");

        void read_unclock(const char* who = "");

        void write_unclock(const char* who = "");

        ~RwlockObj();
    };

    class MutextObj {
    protected:
        pthread_mutex_t mut;
        int locked = 0;
        const char* me;

    public:
        MutextObj(const MutextObj&) = delete;
        MutextObj& operator= (const MutextObj&) = delete;

        explicit MutextObj(const char* me = NULL);

        void lock(const char* who = "");

        void unlock(const char* who = "");

        ~MutextObj();
    };

    class CondVarBedObj : public MutextObj{
        pthread_cond_t alarm;
        /* In this case, variable `locked` check won't ensure proper mutex usage, but it can help sometimes */

    public:
        CondVarBedObj(const CondVarBedObj& ) = delete;
        CondVarBedObj& operator=(const CondVarBedObj& ) = delete;

        explicit CondVarBedObj(const char* me = NULL);

        void sleep(const char* who = "");

        void din_don();

        void wake_them_all();

        ~CondVarBedObj();
    };

    class MutexLockGuard {
        MutextObj& mut;
        const char* who;
        bool premature_unlock = false;
    public:
        explicit MutexLockGuard(MutextObj &mut, const char* who = "");

        void unlock();

        ~MutexLockGuard();
    };

    class RwlockReadGuard {
        RwlockObj& mut;
        const char* who;
        bool premature_unlock = false;
    public:
        explicit RwlockReadGuard(RwlockObj &mut, const char *who = "");

        void unlock();

        ~RwlockReadGuard();
    };

    class RwlockWriteGuard {
        RwlockObj& mut;
        const char* who;
        bool premature_unlock = false;
    public:
        explicit RwlockWriteGuard(RwlockObj &mut, const char *who = "");

        void unlock();

        ~RwlockWriteGuard();
    };

}

#endif
