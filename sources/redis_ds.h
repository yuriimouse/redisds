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

#define FREE_AND_NULL(x) \
    if (x)               \
    {                    \
        free(x);         \
        x = NULL;        \
    }

typedef struct redis_server
{
    char *host;
    int port;
    char *auth;
    int timeout;
} redis_server;

int redisDS_serverOpen(char *host,
                       int port,
                       char *auth,
                       int timeout);
int redisDS_register(char *name, int base, char *prefix, ...);
void redisDS_serverClose();

cJSON *redisDS_read(char *name, char *key, ...);

long long redisDS_set(char *name, char *key, char *value, long long ttl, ...);
long long redisDS_append(char *name, char *key, char *value, long long ttl, ...);
long long redisDS_increment(char *name, char *key, int value, long long ttl, ...);

// for testing
long long redisDS_store(char *name, cJSON *object, long long ttl);

char *redisDS_version();

#endif // REDIS_DS_H
