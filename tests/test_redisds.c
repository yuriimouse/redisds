/**
 * @author Yurii Prudius
 **/
#include "defines.h"

#include <CUnit/Basic.h>
#include <cjson/cJSON.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <redisds/redis_ds.h>
#include <syslog.h>

static char host[] = "redis";
static int port = 6379;
static char auth[] = "";
static int timeout = 1500; // for compatibity, ignored
static long long ttl = 15;

static void test_store(void)
{
    printf("\n%s\n", __func__);
    openlog(NULL, 0, 0);

    int open = redisDS_serverOpen(host, port, auth, timeout);
    CU_ASSERT_EQUAL_FATAL(open, 1);

    START_USING_TEST_DATA("data/")
    {
        char *dataset = NULL;
        int database = 0;
        char *prefix = NULL;
        long long expected = 0;
        USE_OF_THE_TEST_DATA("%m[^ :] : %d = %ms %lld", &dataset, &database, &prefix, &expected);
        // +code
        {
            char *name = '@' == dataset[0] ? dataset + 1 : dataset;
            int reg = redisDS_register(name, database, "%s", prefix);

            syslog(LOG_INFO, "Register(%s) '%s' with prefix '%s'", __func__, name, prefix);
            CU_ASSERT_EQUAL_FATAL(reg, 1);

            FILE_CONTENTS_READ(buffer, "%s.data/%s.json", DATA_PATH, name)
            {
                syslog(LOG_INFO, "Store %s.data/%s.json", DATA_PATH, name);
                cJSON *json = cJSON_Parse(buffer);
                char *strjson = cJSON_PrintUnformatted(json);
                syslog(LOG_DEBUG, "%s", strjson);
                free(strjson);

                long long count = redisDS_store(name, json, ttl);
                syslog(LOG_DEBUG, "Storing result = %lld", count);
                cJSON_Delete(json);

                printf("%s %lld=%lld\n", name, expected, count);
                CU_ASSERT_EQUAL(count, expected);
            }
            FILE_CONTENTS_DONE;
        }
        // -code
        FREE_AND_NULL(prefix);
        FREE_AND_NULL(dataset);
    }
    FINISH_USING_TEST_DATA;

    redisDS_serverClose();
}

static void test_read(void)
{
    printf("\n%s\n", __func__);

    int open = redisDS_serverOpen(host, port, auth, timeout);
    CU_ASSERT_EQUAL_FATAL(open, 1);

    START_USING_TEST_DATA("data/")
    {
        char *dataset = NULL;
        int database = 0;
        char *prefix = NULL;
        long long expected = 0;
        USE_OF_THE_TEST_DATA("%m[^ :] : %d = %ms %lld", &dataset, &database, &prefix, &expected);
        // +code
        {
            char *name = '@' == dataset[0] ? dataset + 1 : dataset;
            int reg = redisDS_register(name, database, "%s", prefix);

            syslog(LOG_INFO, "Register(%s) '%s' with prefix '%s'", __func__, name, prefix);
            CU_ASSERT_EQUAL_FATAL(reg, 1);

            long long count = 0;
            FILE_CONTENTS_READ(buffer, "%s.data/%s.json", DATA_PATH, name)
            {
                syslog(LOG_INFO, "Read %s.data/%s.json", DATA_PATH, name);
                cJSON *estimated = cJSON_Parse(buffer);

                cJSON *element = NULL;
                cJSON_ArrayForEach(element, estimated)
                {
                    cJSON *actual = redisDS_read(name, element->string);
                    if (actual && (actual->type & (cJSON_String | cJSON_Array | cJSON_Object)))
                    {
                        CU_ASSERT_TRUE(cJSON_Compare(actual, element, 1));
                        count++;
                    }
                    cJSON_Delete(actual);
                }
                printf("%s %lld=%lld\n", name, expected, count);
                CU_ASSERT_EQUAL(count, expected);
                cJSON_Delete(estimated);
            }
            FILE_CONTENTS_DONE;
        }
        // -code
        FREE_AND_NULL(prefix);
        FREE_AND_NULL(dataset);
    }
    FINISH_USING_TEST_DATA;

    redisDS_serverClose();
}

static void test_set(void)
{
    printf("\n%s\n", __func__);

    int open = redisDS_serverOpen(host, port, auth, timeout);
    CU_ASSERT_EQUAL_FATAL(open, 1);

    START_USING_TEST_DATA("data/")
    {
        char *dataset = NULL;
        int database = 0;
        char *prefix = NULL;
        long long expected = 0;
        USE_OF_THE_TEST_DATA("%m[^ :] : %d = %ms %lld", &dataset, &database, &prefix, &expected);
        // +code
        {
            char *name = '@' == dataset[0] ? dataset + 1 : dataset;
            int reg = redisDS_register(name, database, "%s", prefix);

            syslog(LOG_INFO, "Register(%s) '%s' with prefix '%s'", __func__, name, prefix);
            CU_ASSERT_EQUAL_FATAL(reg, 1);

            long long count = 0;
            FILE_CONTENTS_READ(buffer, "%s.data/%s.json", DATA_PATH, name)
            {
                syslog(LOG_INFO, "Read %s.data/%s.json", DATA_PATH, name);
                cJSON *list = cJSON_Parse(buffer);

                cJSON *element = NULL;
                cJSON_ArrayForEach(element, list)
                {
                    if (cJSON_IsString(element))
                    {
                        long long newttl = redisDS_set(name, "%s", "%s", ttl, element->string, element->valuestring);
                        CU_ASSERT_TRUE(newttl > 0);
                        count++;
                    }
                }
                printf("%s %lld=%lld\n", name, expected, count);
                CU_ASSERT_EQUAL(count, expected);
                cJSON_Delete(list);
            }
            FILE_CONTENTS_DONE;
        }
        // -code
        FREE_AND_NULL(prefix);
        FREE_AND_NULL(dataset);
    }
    FINISH_USING_TEST_DATA;

    redisDS_serverClose();
}

static void test_append(void)
{
    printf("\n%s\n", __func__);

    int open = redisDS_serverOpen(host, port, auth, timeout);
    CU_ASSERT_EQUAL_FATAL(open, 1);

    START_USING_TEST_DATA("data/")
    {
        char *dataset = NULL;
        int database = 0;
        char *prefix = NULL;
        long long expected = 0;
        USE_OF_THE_TEST_DATA("%m[^ :] : %d = %ms %lld", &dataset, &database, &prefix, &expected);
        // +code
        {
            char *name = '@' == dataset[0] ? dataset + 1 : dataset;
            int reg = redisDS_register(name, database, "%s", prefix);

            syslog(LOG_INFO, "Register(%s) '%s' with prefix '%s'", __func__, name, prefix);
            CU_ASSERT_EQUAL_FATAL(reg, 1);

            long long count = 0;
            FILE_CONTENTS_READ(buffer, "%s.data/%s.json", DATA_PATH, name)
            {
                syslog(LOG_INFO, "Read %s.data/%s.json", DATA_PATH, name);
                cJSON *list = cJSON_Parse(buffer);

                cJSON *element = NULL;
                cJSON_ArrayForEach(element, list)
                {
                    if (cJSON_IsArray(element))
                    {
                        cJSON *member = NULL;
                        cJSON_ArrayForEach(member, element)
                        {
                            if (cJSON_IsString(member))
                            {
                                long long newttl = redisDS_append(name, "%s", "%s", ttl, element->string, member->valuestring);
                                CU_ASSERT_TRUE(newttl > 0);
                                count++;
                            }
                        }
                    }
                }
                printf("%s %lld=%lld\n", name, expected, count);
                CU_ASSERT_EQUAL(count, expected);
                cJSON_Delete(list);
            }
            FILE_CONTENTS_DONE;
        }
        // -code
        FREE_AND_NULL(prefix);
        FREE_AND_NULL(dataset);
    }
    FINISH_USING_TEST_DATA;

    redisDS_serverClose();
}

static void test_increment(void)
{
    printf("\n%s\n", __func__);

    int open = redisDS_serverOpen(host, port, auth, timeout);
    CU_ASSERT_EQUAL_FATAL(open, 1);

    START_USING_TEST_DATA("data/")
    {
        char *dataset = NULL;
        int database = 0;
        char *prefix = NULL;
        long long expected = 0;
        USE_OF_THE_TEST_DATA("%m[^ :] : %d = %ms %lld", &dataset, &database, &prefix, &expected);
        // +code
        {
            char *name = '@' == dataset[0] ? dataset + 1 : dataset;
            int reg = redisDS_register(name, database, "%s", prefix);

            syslog(LOG_INFO, "Register(%s) '%s' with prefix '%s'", __func__, name, prefix);
            CU_ASSERT_EQUAL_FATAL(reg, 1);

            long long count = 0;
            FILE_CONTENTS_READ(buffer, "%s.data/%s.json", DATA_PATH, name)
            {
                syslog(LOG_INFO, "Read %s.data/%s.json", DATA_PATH, name);
                cJSON *list = cJSON_Parse(buffer);

                cJSON *element = NULL;
                cJSON_ArrayForEach(element, list)
                {
                    if (cJSON_IsArray(element))
                    {
                        long long newttl = redisDS_increment(name, "%s", cJSON_GetArraySize(element), ttl, element->string);
                        CU_ASSERT_TRUE(newttl > 0);
                        count++;
                    }
                }
                printf("%s %lld=%lld\n", name, expected, count);
                CU_ASSERT_EQUAL(count, expected);
                cJSON_Delete(list);
            }
            FILE_CONTENTS_DONE;
        }
        // -code
        FREE_AND_NULL(prefix);
        FREE_AND_NULL(dataset);
    }
    FINISH_USING_TEST_DATA;

    redisDS_serverClose();
}

CU_TestInfo testing_actions[] =
    {
        {"(test_store)", test_store},
        {"(test_read)", test_read},
        {"(test_set)", test_set},
        {"(test_append)", test_append},
        {"(test_increment)", test_increment},
        // {"(test_check)", test_check},
        CU_TEST_INFO_NULL,
};
