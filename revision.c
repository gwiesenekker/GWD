#include "revision.h"

#ifndef REVISION
#error "REVISION must be supplied by the build"
#endif

const char revision[] = REVISION;

