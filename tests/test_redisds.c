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
static int timeout = 1500;
static long long ttl = 15;

static void test_store(void)
{
    printf("\n%s\n", __func__);

    openlog(NULL, 0, LOG_MAIL);

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

            syslog(LOG_INFO, "Register '%s' with prefix '%s'", name, prefix);
            CU_ASSERT_EQUAL_FATAL(reg, 1);

            FILE_CONTENTS_READ(buffer, "data/common/%s.json", name)
            {
                syslog(LOG_INFO, "Store %s_data/%s.json", DATA_PATH, name);
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

    closelog();
}

CU_TestInfo testing_actions[] =
    {
        {"(test_store)", test_store},
        // {"(test_read)", test_read},
        // {"(test_set)", test_set},
        // {"(test_append)", test_append},
        // {"(test_increment)", test_increment},
        // {"(test_check)", test_check},
        CU_TEST_INFO_NULL,
};
