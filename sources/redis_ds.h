#ifndef REDIS_DS_H
#define REDIS_DS_H

#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#ifndef VERSION
#define VERSION "0"
#endif

typedef struct redis_server
{
    char *host;
    int port;
    char *auth;
    int timeout;
} redis_server;

typedef struct redis_dataspace
{
    int base;
    char *prefix;
    struct redisContext *context;
} redis_dataspace;

int redisDS_serverOpen(char *host,
                       int port,
                       char *auth,
                       int timeout);
void redisDS_serverClose();

redis_dataspace *redisDS_object(int base, char *prefix);
redis_dataspace *redisDS_free(redis_dataspace *dataspace);

cJSON *redisDS_read(redis_dataspace *dataspace, char *key, ...);

long long redisDS_set(redis_dataspace *dataspace, char *key, char *value, long long ttl, ...);
long long redisDS_append(redis_dataspace *dataspace, char *key, char *value, long long ttl, ...);
long long redisDS_increment(redis_dataspace *dataspace, char *key, int value, long long ttl, ...);
char *redisDS_version();

#endif // REDIS_DS_H
