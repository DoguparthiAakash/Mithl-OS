#ifndef _INTTYPES_H
#define _INTTYPES_H

#include <stdint.h>

// Minimal PRI macros
#define PRIu32 "u"
#define PRIx32 "x"
#define PRId32 "d"

#define MC_PRIx64 "llx" // TCC internal? TCC headers often assume standard PRI
#define PRIx64 "llx"
#define PRIu64 "llu"
#define PRId64 "lld"

#endif
