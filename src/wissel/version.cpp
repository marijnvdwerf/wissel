#include "openloco.h"

#define NAME "OpenLoco"
#define VERSION "18.02.2"

namespace wissel
{
    const char version[] = NAME ", "
#ifdef WISSEL_VERSION_TAG
        WISSEL_VERSION_TAG
#else
                                "v" VERSION
#endif
#if defined(WISSEL_BRANCH) || defined(WISSEL_COMMIT_SHA1_SHORT) || !defined(NDEBUG)
                                " ("
#if defined(WISSEL_BRANCH) && defined(WISSEL_COMMIT_SHA1_SHORT)
        WISSEL_COMMIT_SHA1_SHORT " on " WISSEL_BRANCH
#elif defined(WISSEL_COMMIT_SHA1_SHORT)
        WISSEL_COMMIT_SHA1_SHORT
#elif defined(WISSEL_BRANCH)
        WISSEL_BRANCH
#endif
#ifndef NDEBUG
                                ", DEBUG"
#endif
                                ")"
#endif
        ;
}
