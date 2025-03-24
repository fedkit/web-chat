#ifndef ENGINE_ENGINE_NUMBER_9_BAZA_INTER_H
#define ENGINE_ENGINE_NUMBER_9_BAZA_INTER_H

/* Do not export this file */

#include "baza.h"

#define THROW(err) throw ServerError(err, __FILE__, __func__, __LINE__)
#define THROW_on_errno(err) THROW(prettyprint_errno(err))
#define THROW_on_errno_pl() THROW(prettyprint_errno(""))
#define ASSERT(cond, err) do { if (!(cond)) { THROW(err); } } while (0);
#define ASSERT_pl(cond) ASSERT(cond, "Failed assertion `" #cond "`")
#define ASSERT_on_iret(iret, err) ASSERT((iret) >= 0, prettyprint_errno(err));
#define ASSERT_on_iret_pl(iret) ASSERT(iret >= 0, prettyprint_errno(""));

#endif
