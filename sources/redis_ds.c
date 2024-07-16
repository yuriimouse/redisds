#include "redis_ds.h"
#include <errno.h>
#include <hiredis/hiredis.h>
#include <pthread.h>
#include <sys/time.h>

#define FREE_AND_NULL(x) \
    if (x)               \
    {                    \
        free(x);         \
        x = NULL;        \
    }

#define stringEQUALS(src, cmp) ((src) && (cmp) && !strcmp(src, cmp))

#define REDIS_IS_OK(x) (x && (REDIS_REPLY_STATUS || (0 == strcmp(x->str, "OK"))))
#define REDIS_IS_INT(x) (x && (REDIS_REPLY_INTEGER == x->type))
#define REDIS_IS_STRING(x) (x && (REDIS_REPLY_STRING == x->type))
#define REDIS_IS_ARRAY(x) (x && (REDIS_REPLY_ARRAY == x->type))

typedef struct redis_dataspace
{
    char *name;
    int base;
    char *prefix;
    struct redisContext *context;
    struct redis_dataspace *next;
} redis_dataspace;

//
static redis_server _redis_server_ = {NULL, 0, NULL, 0};
static redis_dataspace *_redis_ds_list = NULL;

static pthread_mutex_t redis_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *aprint(char *format, va_list ap)
{
    char *buff = NULL;
    vasprintf(&buff, format, ap);
    return buff;
}

static redis_dataspace *redisDS_object(char *name, int base, char *prefix);
static redis_dataspace *redisDS_free(redis_dataspace *dataspace);
static redis_dataspace *redisDS_object(char *name, int base, char *prefix);
static redis_dataspace *redisDS_get(char *name);

static char *redis_type(redis_dataspace *dataspace, char *key);
static cJSON *redis_string(redis_dataspace *dataspace, char *key);
static cJSON *redis_hash(redis_dataspace *dataspace, char *key);
static void cJSON_AddStringToArray(cJSON *json, const char *string);
static cJSON *redis_list(redis_dataspace *dataspace, char *key);
static cJSON *redis_set(redis_dataspace *dataspace, char *key);
static long long redis_ttl(redis_dataspace *dataspace, char *key);
static long long redis_expire(redis_dataspace *dataspace, char *key, long long expire);

static struct redisContext *redis_connect(char *rhost, int rport, char *rauth, int timeout, int base);
static void redis_disconnect(struct redisContext *redis);
static int redis_auth(struct redisContext *redis, char *rauth);
static int redis_select(struct redisContext *redis, int base);
static redisReply *redis_command(redis_dataspace *dataspace, char *format, ...);
static redisReply *redis_vcommand(redis_dataspace *dataspace, char *format, va_list ap);

/**
 * Sets server options
 *
 * @param host
 * @param port
 * @param auth
 * @param timeout
 * @return int
 */
int redisDS_serverOpen(char *host, int port, char *auth, int timeout)
{
    if (!_redis_server_.host && !_redis_ds_list)
    {
        if (host && port)
        {
            _redis_server_.host = strdup(host);
            _redis_server_.port = port;
            _redis_server_.auth = auth ? strdup(auth) : NULL;
            _redis_server_.timeout = timeout ? timeout : 500;
            return 1;
        }
    }
    errno = EINVAL;
    return 0;
}

/**
 * Cleans up and destroys a dataspace object
 *
 * @param dataspace
 * @return redis_dataspace* next member
 */
static redis_dataspace *redisDS_free(redis_dataspace *dataspace)
{
    redis_dataspace *next = NULL;
    if (dataspace)
    {
        next = dataspace->next;
        FREE_AND_NULL(dataspace->name);
        dataspace->base = 0;
        FREE_AND_NULL(dataspace->prefix);
        dataspace->context = NULL;
        free(dataspace);
    }
    return next;
}

/**
 * Reset server options
 * and free dataset collection
 *
 */
void redisDS_serverClose()
{
    FREE_AND_NULL(_redis_server_.host);
    _redis_server_.port = 0;
    FREE_AND_NULL(_redis_server_.auth);
    _redis_server_.timeout = 0;

    for (redis_dataspace *ptr = _redis_ds_list; ptr; ptr = redisDS_free(ptr))
    {
        redis_disconnect(ptr->context);
    }
}

/**
 * Creates a dataspace object
 *
 * @param name
 * @param base
 * @param prefix
 * @return redis_dataspace*
 */
static redis_dataspace *redisDS_object(char *name, int base, char *prefix)
{
    redis_dataspace *dataspace = malloc(sizeof(redis_dataspace));
    if (dataspace)
    {
        dataspace->name = strdup(name);
        dataspace->prefix = prefix;
        dataspace->base = base;
        dataspace->context = NULL;
        dataspace->next = NULL;
        return dataspace;
    }
    return NULL;
}

/**
 * Registers a dataspace in a collection
 *
 * @param name
 * @param base
 * @param prefix
 * @param ...
 * @return int*
 */
int redisDS_register(char *name, int base, char *prefix, ...)
{
    if (name && name[0])
    {
        va_list ap;
        va_start(ap, prefix);
        redis_dataspace *object = redisDS_object(name, base, prefix ? aprint(prefix, ap) : NULL);
        va_end(ap);

        if (object)
        {
            object->next = _redis_ds_list;
            _redis_ds_list = object;
            return 1;
        }
        errno = ENOMEM;
        return 0;
    }
    errno = EINVAL;
    return 0;
}

/**
 * Gets the dataspace by name
 *
 * @param name
 * @return redis_dataspace*
 */
static redis_dataspace *redisDS_get(char *name)
{
    for (redis_dataspace *ptr = _redis_ds_list; ptr; ptr = ptr->next)
    {
        if (!strcmp(name, ptr->name))
        {
            return ptr;
        }
    }
    return 0;
}

/**
 * Get type of the key
 *
 * @param redis
 * @param key
 *
 * @return char*
 */
static char *redis_type(redis_dataspace *dataspace, char *key)
{
    char *ret = NULL;

    redisReply *reply = redis_command(dataspace, "TYPE %s", key);
    if (reply)
    {
        ret = reply ? strdup(reply->str) : NULL;
    }
    freeReplyObject(reply);

    return ret;
}

static cJSON *redis_string(redis_dataspace *dataspace, char *key)
{
    cJSON *json = NULL;

    redisReply *reply = redis_command(dataspace, "GET %s", key);
    if (REDIS_IS_STRING(reply))
    {
        json = cJSON_CreateString(reply->str);
    }
    else if (REDIS_IS_INT(reply))
    {
        json = cJSON_CreateNumber((double)reply->integer);
    }
    freeReplyObject(reply);

    return json;
}

static cJSON *redis_hash(redis_dataspace *dataspace, char *key)
{
    cJSON *json = NULL;

    redisReply *reply = redis_command(dataspace, "HGETALL %s", key);
    if (REDIS_IS_ARRAY(reply) && reply->elements >= 2)
    {
        json = cJSON_CreateObject();
        for (size_t i = 0; i < (reply->elements / 2); i++)
        {
            char *field = (reply->element)[i * 2]->str;
            char *value = reply->element[i * 2 + 1]->str;
            cJSON_AddStringToObject(json, field, value);
        }
    }
    freeReplyObject(reply);

    return json;
}

static void cJSON_AddStringToArray(cJSON *json, const char *string)
{
    cJSON *jstr = cJSON_CreateString(string);
    if (!cJSON_AddItemToArray(json, jstr))
    {
        cJSON_Delete(jstr);
    }
}

static cJSON *redis_list(redis_dataspace *dataspace, char *key)
{
    cJSON *json = NULL;

    redisReply *reply = redis_command(dataspace, "LRANGE %s 0 -1", key);
    if (REDIS_IS_ARRAY(reply))
    {
        json = cJSON_CreateArray();
        for (size_t i = 0; i < reply->elements; i++)
        {
            cJSON_AddStringToArray(json, reply->element[i]->str);
        }
    }
    freeReplyObject(reply);

    return json;
}

/**
 *
 *
 * @param dataspace
 * @param key
 * @return cJSON*
 */
static cJSON *redis_set(redis_dataspace *dataspace, char *key)
{
    cJSON *json = NULL;

    redisReply *reply = redis_command(dataspace, "SMEMBERS %s", key);
    if (REDIS_IS_ARRAY(reply))
    {
        json = cJSON_CreateArray();
        for (size_t i = 0; i < reply->elements; i++)
        {
            cJSON_AddStringToArray(json, reply->element[i]->str);
        }
    }
    freeReplyObject(reply);

    return json;
}

/**
 * Reads the key value from the dataspace
 *
 * @param name
 * @param key
 * @param ...
 * @return cJSON*
 */
cJSON *redisDS_read(char *name, char *key, ...)
{
    redis_dataspace *dataspace = redisDS_get(name);
    if (dataspace)
    {
        cJSON *json = NULL;

        va_list ap;
        va_start(ap, key);
        char *fullkey = aprint(key, ap);
        va_end(ap);

        char *type = redis_type(dataspace, fullkey);
        if (type)
        {
            if (stringEQUALS(type, "string"))
            {
                json = redis_string(dataspace, fullkey);
            }
            else if (stringEQUALS(type, "hash"))
            {
                json = redis_hash(dataspace, fullkey);
            }
            else if (stringEQUALS(type, "list"))
            {
                json = redis_list(dataspace, fullkey);
            }
            else if (stringEQUALS(type, "set"))
            {
                json = redis_set(dataspace, fullkey);
            }
        }
        FREE_AND_NULL(type);
        FREE_AND_NULL(fullkey);

        return json;
    }
    errno = EINVAL;
    return NULL;
}

static long long redis_ttl(redis_dataspace *dataspace, char *key)
{
    long long ret = -3;

    redisReply *reply = redis_command(dataspace, "TTL %s", key);
    if (REDIS_IS_INT(reply))
    {
        ret = reply->integer;
    }
    freeReplyObject(reply);

    return ret;
}

static long long redis_expire(redis_dataspace *dataspace, char *key, long long expire)
{
    long long oldttl = redis_ttl(dataspace, key);
    int ret = 0;
    if (oldttl <= 0)
    {
        redisReply *reply = redis_command(dataspace, "EXPIRE %s %lld", key, expire);
        ret = REDIS_IS_OK(reply);
        freeReplyObject(reply);
    }

    return ret ? expire : oldttl;
}

/**
 * Sets the value of the scalar key in the dataspace
 * and set key to timeout after a given number of seconds.
 * If key already holds a value, it is overwritten, regardless of its type.
 *
 * @param dataspace
 * @param key
 * @param value
 * @param ttl
 * @param ...
 * @return long long = ttl value
 */
long long redisDS_set(char *name, char *key, char *value, long long ttl, ...)
{
    redis_dataspace *dataspace = redisDS_get(name);
    if (dataspace)
    {
        va_list ap;
        va_start(ap, ttl);
        char *fullkey = aprint(key, ap);
        char *fullval = aprint(value, ap);
        va_end(ap);

        long long newttl = 0;
        redisReply *reply = redis_command(dataspace, "SET %s %s EX %lld", fullkey, fullval, ttl);
        if (REDIS_IS_OK(reply))
        {
            newttl = redis_expire(dataspace, fullkey, ttl);
        }
        freeReplyObject(reply);

        FREE_AND_NULL(fullval);
        FREE_AND_NULL(fullkey);

        return newttl;
    }
    errno = EINVAL;
    return 0;
}

/**
 * Appends a string to the key of type SET in the dataspace
 *
 * @param dataspace
 * @param key
 * @param value
 * @param ttl
 * @param ...
 * @return long long
 */
long long redisDS_append(char *name, char *key, char *value, long long ttl, ...)
{
    redis_dataspace *dataspace = redisDS_get(name);
    if (dataspace)
    {
        long long count = 0;

        va_list ap;
        va_start(ap, ttl);
        char *fullkey = aprint(key, ap);
        char *fullval = aprint(value, ap);
        va_end(ap);

        redisReply *reply = redis_command(dataspace, "SADD %s %s", fullkey, fullval);
        if (REDIS_IS_OK(reply))
        {
            redis_expire(dataspace, fullkey, ttl);
        }
        freeReplyObject(reply);

        reply = redis_command(dataspace, "SCARD %s", fullkey);
        if (REDIS_IS_INT(reply))
        {
            count = reply->integer;
        }

        FREE_AND_NULL(fullval);
        FREE_AND_NULL(fullkey);

        return count;
    }
    errno = EINVAL;
    return 0;
}

/**
 * Increments the key value of a scalar type in the data space
 *
 * @param dataspace
 * @param key
 * @param value
 * @param ttl
 * @param ...
 * @return long long
 */
long long redisDS_increment(char *name, char *key, int value, long long ttl, ...)
{
    redis_dataspace *dataspace = redisDS_get(name);
    if (dataspace)
    {
        long long count = 0;

        va_list ap;
        va_start(ap, ttl);
        char *fullkey = aprint(key, ap);
        va_end(ap);

        redisReply *reply = redis_command(dataspace, "INCRBY %s %d", fullkey, value);
        if (REDIS_IS_OK(reply) && REDIS_IS_INT(reply))
        {
            count = reply->integer;
            redis_expire(dataspace, fullkey, ttl);
        }
        freeReplyObject(reply);

        FREE_AND_NULL(fullkey);

        return count;
    }
    errno = EINVAL;
    return 0;
}

/**
 * !!! FOR TESTING ONLY !!!
 *
 * @param dataspace
 * @param key
 * @param value
 * @param ttl
 * @param ...
 * @return long long
 */
static long long redisDS_write(redis_dataspace *dataspace, char *key, cJSON *value, long long ttl, ...)
{
    long long count = 0;

    va_list ap;
    va_start(ap, ttl);
    char *fullkey = aprint(key, ap);
    va_end(ap);

    redisReply *reply = NULL;
    cJSON *element = NULL;
    if (value)
    {
        switch (value->type)
        {
        case cJSON_String:
            reply = redis_command(dataspace, "SET %s %s", fullkey, value->valuestring);
            if (REDIS_IS_OK(reply))
            {
                count++;
                redis_expire(dataspace, fullkey, ttl);
            }
            break;
        case cJSON_Array:
            cJSON_ArrayForEach(element, value)
            {
                reply = redis_command(dataspace, "SADD %s %s", fullkey, value->valuestring);
                if (!REDIS_IS_OK(reply))
                {
                    break;
                }
                count++;
            }
            if (REDIS_IS_OK(reply))
            {
                redis_expire(dataspace, fullkey, ttl);
            }
            break;
        case cJSON_Object:
            cJSON_ArrayForEach(element, value)
            {
                reply = redis_command(dataspace, "HSET %s %s %s", fullkey, value->string, value->valuestring);
                if (!REDIS_IS_OK(reply))
                {
                    break;
                }
                count++;
            }
            if (REDIS_IS_OK(reply))
            {
                redis_expire(dataspace, fullkey, ttl);
            }
            break;
        }
    }
    freeReplyObject(reply);
    FREE_AND_NULL(fullkey);

    return count;
}

/**
 * !!! FOR TESTING ONLY !!!
 *
 * @param dataspace
 * @param object
 * @param ttl
 * @param ...
 * @return long long
 */
long long redisDS_store(char *name, cJSON *object, long long ttl)
{
    redis_dataspace *dataspace = redisDS_get(name);
    if (dataspace)
    {
        long long count = 0;

        cJSON *element = NULL;
        cJSON_ArrayForEach(element, object)
        {
            count += redisDS_write(dataspace, "%s", element, ttl, element->string);
        }

        return count;
    }
    errno = EINVAL;
    return 0;
}

/**
 * Returns redisDS version
 *
 * @return char*
 */
char *redisDS_version()
{
    return VERSION;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// statics
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Connect to REDIS context, auth and select base
 *
 * @param rhost REDIS host
 * @param rport REDIS port
 * @param rauth REDIS auth
 * @param timeout REDIS timeout in milliseconds
 * @param base REDIS base select
 *
 * @return struct redisContext | NULL
 **/
static struct redisContext *redis_connect(char *rhost, int rport, char *rauth, int timeout, int base)
{
    struct redisContext *redis = NULL;
    redis_disconnect(redis);
    struct timeval tv = {timeout / 1000, (timeout % 1000) * 1000};

    redis = redisConnectWithTimeout(rhost, rport, tv);
    if (redis                                                 // connected
        && !redis->err                                        // not error
        && (!(rauth && rauth[0]) || redis_auth(redis, rauth)) // auth
        && redis_select(redis, base)                          // select
    )
    {
        return redis;
    }
    return NULL;
}

/**
 * Disconnect and free REDIS context
 *
 * @param struct redisContext
 **/
static void redis_disconnect(struct redisContext *redis)
{
    if (redis)
    {
        redisFree(redis);
    }
}

/**
 * Auth REDIS if need
 *
 * @param struct redisContext
 * @param rauth REDIS auth
 *
 * @return 1 | 0
 **/
static int redis_auth(struct redisContext *redis, char *rauth)
{
    redisReply *reply = NULL;
    int ret = 0;

    if (redis)
    {
        reply = redisCommand(redis, "AUTH %s", rauth);
        ret = REDIS_IS_OK(reply);
    }

    freeReplyObject(reply);
    return ret;
}

/**
 * Select REDIS base
 *
 * @param struct redisContext
 * @param base REDIS base number
 *
 * @return 1 | 0
 **/
static int redis_select(struct redisContext *redis, int base)
{
    redisReply *reply = NULL;
    int ret = 0;

    if (redis)
    {
        reply = redisCommand(redis, "SELECT %d", base);
        ret = REDIS_IS_OK(reply);
    }

    freeReplyObject(reply);
    return ret;
}

/**
 * Execute the REDIS command with formatting
 *
 * @param redis REDIS subject
 * @param format
 * @param ...
 **/
static redisReply *redis_command(redis_dataspace *dataspace, char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    redisReply *reply = redis_vcommand(dataspace, format, ap);
    va_end(ap);

    return reply;
}

/**
 * Execute the REDIS command (thread safe)
 *
 * @param redis REDIS subject
 * @param format
 * @param ap
 **/
static redisReply *redis_vcommand(redis_dataspace *dataspace, char *format, va_list ap)
{
    redisReply *reply = NULL;
    redisContext *cx = dataspace->context;
    // on first connection
    if (!cx)
    {
        cx = redis_connect(_redis_server_.host, _redis_server_.port, _redis_server_.auth, _redis_server_.timeout, dataspace->base);
    }

    /** Lock redis **/
    pthread_mutex_lock(&redis_mutex);
    // try
    if (cx)
    {
        va_list ap0;
        va_copy(ap0, ap);
        reply = redisvCommand(cx, format, ap0);
        va_end(ap0);
    }

    // retry after reconnect
    if ((NULL == reply) && (cx = redis_connect(_redis_server_.host, _redis_server_.port, _redis_server_.auth, _redis_server_.timeout, dataspace->base)))
    {
        va_list ap1;
        va_copy(ap1, ap);

        reply = redisvCommand(cx, format, ap1);
        va_end(ap1);
    }
    pthread_mutex_unlock(&redis_mutex);
    /** Unlock redis **/

    return reply;
}
