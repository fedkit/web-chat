#include "thread_synchronization.h"
#include "baza_inter.h"
#include <assert.h>
namespace een9 {
    /*==================================================== RwlockObj ============ */

    void RwlockObj::unlock(int el, const char *who) {
        assert(lock == el);
        lock = 0;
        if (me)
            printf("Rwl %s unlocked by %s\n", me, who);
        int ret = pthread_rwlock_unlock(&mut);
        assert(ret == 0);
    }

    RwlockObj::RwlockObj(const char *me): me(me) {
        int ret = pthread_rwlock_init(&mut, NULL);
        ASSERT_on_iret(ret, "pthread rwlock init");
    }

    void RwlockObj::read_lock(const char *who) {
        int ret = pthread_rwlock_rdlock(&mut);
        assert(ret == 0);
        if (me)
            printf("Rwl %s read-locked by %s\n", me, who);
        lock = 1;
    }

    void RwlockObj::write_lock(const char *who) {
        int ret = pthread_rwlock_wrlock(&mut);
        assert(ret == 0);
        if (me)
            printf("Rwl %s write-locked by %s\n", me, who);
        lock = 2;
    }

    void RwlockObj::read_unclock(const char *who) {
        unlock(1, who);
    }

    void RwlockObj::write_unclock(const char *who) {
        unlock(2, who);
    }

    RwlockObj::~RwlockObj() {
        pthread_rwlock_destroy(&mut);
    }

    /*==================================================== MutexObj ============ */

    MutextObj::MutextObj(const char *me): me(me) {
        int ret = pthread_mutex_init(&mut, NULL);
        ASSERT_on_iret(ret == 0, "pthread mutex init");
    }

    void MutextObj::lock(const char *who) {
        int ret = pthread_mutex_lock(&mut);
        assert(ret == 0);
        if (me)
            printf("Mut %s locked by %s\n", me, who);
        locked = 1;
    }

    void MutextObj::unlock(const char *who) {
        assert(locked == 1);
        locked = 0;
        if (me)
            printf("Mut %s unlocked by %s\n", me, who);
        int ret = pthread_mutex_unlock(&mut);
        assert(ret == 0);
    }

    MutextObj::~MutextObj() {
        pthread_mutex_destroy(&mut);
    }

    /*==================================================== CondVarBedObj ============ */

    CondVarBedObj::CondVarBedObj(const char *me): MutextObj(me) {
        int ret = pthread_cond_init(&alarm, NULL);
        ASSERT_on_iret(ret, "pthread cond variable init");
    }

    void CondVarBedObj::sleep(const char *who) {
        assert(locked == 1);
        locked = 0;
        if (me)
            printf("Mut %s unlocked. Sleeping (%s)\n", me, who);
        int ret = pthread_cond_wait(&alarm, &mut);
        assert(ret == 0);
        if (me)
            printf("Mut %s locked. Woke up (%s)\n", me, who);
        locked = 1;
    }

    void CondVarBedObj::din_don() {
        int ret = pthread_cond_signal(&alarm);
        assert(ret == 0);
    }

    void CondVarBedObj::wake_them_all() {
        int ret = pthread_cond_broadcast(&alarm);
        assert(ret == 0);
    }

    CondVarBedObj::~CondVarBedObj() {
        pthread_mutex_destroy(&mut);
        pthread_cond_destroy(&alarm);
    }

    /*==================================================== MutexLockGuard ============ */

    MutexLockGuard::MutexLockGuard(MutextObj &mut, const char *who): mut(mut), who(who) {
        mut.lock(who);
    }

    void MutexLockGuard::unlock() {
        assert(!premature_unlock);
        premature_unlock = true;
        mut.unlock(who);
    }

    MutexLockGuard::~MutexLockGuard() {
        if (!premature_unlock)
            mut.unlock(who);
    }

    /*==================================================== RwLockReadLockGuard ============ */

    RwlockReadGuard::RwlockReadGuard(RwlockObj &mut, const char *who): mut(mut), who(who) {
        mut.read_lock(who);
    }

    void RwlockReadGuard::unlock() {
        assert(!premature_unlock);
        premature_unlock = true;
        mut.read_unclock(who);
    }

    RwlockReadGuard::~RwlockReadGuard() {
        if (!premature_unlock)
            mut.read_unclock(who);
    }

    /*==================================================== RwLockWriteLockGuard ============ */

    RwlockWriteGuard::RwlockWriteGuard(RwlockObj &mut, const char *who): mut(mut), who(who) {
        mut.write_lock(who);
    }

    void RwlockWriteGuard::unlock() {
        assert(!premature_unlock);
        premature_unlock = true;
        mut.write_unclock(who);
    }

    RwlockWriteGuard::~RwlockWriteGuard() {
        if (!premature_unlock)
            mut.write_unclock(who);
    }
}