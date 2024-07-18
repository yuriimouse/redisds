#ifndef UNIT_TEST_DEFS_H
#define UNIT_TEST_DEFS_H

#define _GNU_SOURCE

/**
 * Frees the allocated block, if it exists, 
 * and sets the pointer to NULL
 * 
 */
#define FREE_AND_NULL(x) \
    if (x)               \
        free(x);         \
    x = NULL

/**
 * Starts reading test data from the file 
 * with the name of the current function 
 * at the specified path  
 * 
 * The local variable `DATA_PATH` contains the file path of the test data
 * 
 */
#define START_USING_TEST_DATA(path)                                                         \
    {                                                                                       \
        FILE *_fp;                                                                          \
        char *_pathstr = NULL;                                                              \
        asprintf(&_pathstr, "%s%s", (path), __func__);                                      \
        char *DATA_PATH = _pathstr;                                                         \
        if (DATA_PATH && (_fp = fopen(DATA_PATH, "r")))                                     \
        {                                                                                   \
            size_t _len = 0;                                                                \
            char *_str = NULL;                                                              \
            for (ssize_t _read = 0; (_read = getline(&_str, &_len, _fp)) != -1; _len = 0)   \
            {                                                                               \
                if (strlen(_str) && _str[0] != '#')

/**
 * Scans variables from the current line, 
 * like the `sscanf` function
 * 
 */
#define USE_OF_THE_TEST_DATA(...) \
    sscanf(_str, __VA_ARGS__)

/**
 * Finishes reading test data 
 * and frees all variables 
 * 
 */
#define FINISH_USING_TEST_DATA                     \
                FREE_AND_NULL(_str);               \
            }                                      \
            FREE_AND_NULL(_str);                   \
            FREE_AND_NULL(_pathstr);               \
            fclose(_fp);                           \
        }                                          \
        else                                       \
        {                                          \
            printf("'%s' not found...", _pathstr); \
            FREE_AND_NULL(_pathstr);               \
            CU_ASSERT_TRUE_FATAL(0);               \
        }                                          \
    }

#define FILE_CONTENTS_READ(buffer, ...)                         \
    FILE *fp;                                                   \
    char *pathstr = NULL;                                       \
    asprintf(&pathstr, __VA_ARGS__);                            \
    if ((fp = fopen(pathstr, "r")))                             \
    {                                                           \
        char *buffer = NULL;                                    \
        size_t len = 0;                                         \
        ssize_t bytes_read = getdelim(&buffer, &len, '\0', fp); \
        if (bytes_read != -1)

#define FILE_CONTENTS_DONE                        \
        FREE_AND_NULL(buffer);                    \
        FREE_AND_NULL(pathstr);                   \
        fclose(fp);                               \
    }                                             \
    else                                          \
    {                                             \
        printf("\n'%s' not found...\n", pathstr); \
        FREE_AND_NULL(pathstr);                   \
        CU_ASSERT_TRUE_FATAL(0);                  \
    }

#endif // UNIT_TEST_DEFS_H
