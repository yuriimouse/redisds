#ifndef REDIS_DS_H
#define REDIS_DS_H

#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VERSION
#define VERSION "0"
#endif


/**
 * Returns redisDS version
 *
 * @return unsigned char
 */
unsigned char redisDS_version();

#endif // REDIS_DS_H
