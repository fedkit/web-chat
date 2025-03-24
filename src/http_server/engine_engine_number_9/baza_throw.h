#ifndef ENGINE_ENGINE_NUMBER_9_BAZA_THROW_H
#define ENGINE_ENGINE_NUMBER_9_BAZA_THROW_H

#include "baza.h"

#define een9_THROW(err) throw een9::ServerError(err, __FILE__, __func__, __LINE__)
#define een9_THROW_on_errno(err) een9_THROW(een9::prettyprint_errno(err))
#define een9_THROW_on_errno_pl() een9_THROW(een9::prettyprint_errno(""))
#define een9_ASSERT(cond, err) do { if (!(cond)) { een9_THROW(err); } } while (0);
#define een9_ASSERT_pl(cond) een9_ASSERT(cond, "Failed assertion `" #cond "`")
#define een9_ASSERT_on_iret(iret, err) een9_ASSERT((iret) >= 0, een9::prettyprint_errno(err));
#define een9_ASSERT_on_iret_pl(iret) een9_ASSERT(iret >= 0, een9::prettyprint_errno(""));


#endif
