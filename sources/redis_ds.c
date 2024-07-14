#include "redis_ds.h"
#include <errno.h>

/**
 * Returns redisDS major part of version
 *
 * @return unsigned char
 */
unsigned char redisDS_version()
{
    return VERSION;
}
