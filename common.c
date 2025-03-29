#include "common.h"

#include <stdlib.h>

#include <stdarg.h>

#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>

#define mkdir _mkdir
#else
#include <sys/stat.h>

#include <unistd.h>  // For POSIX mkdir
#endif

void panic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    printf("\nPANIC: %s\n\n", buffer);

    va_end(args);

    exit(1);
}

void warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    printf("\nWARN: %s\n\n", buffer);

    va_end(args);
}

char* getFilename(char* path) {
    char* lastSlash = strrchr(path, '/');
    if (!lastSlash)
        lastSlash = strrchr(path, '\\');

    if (!lastSlash)
        return path;

    return (char*)(lastSlash + 1);
}

char* getExtension(char* path) {
    char* lastDot = strrchr(path, '.');
    if (!lastDot)
        return path + strlen(path);

    return lastDot;
}

void createDirectory(const char* path) {
    #ifdef _WIN32
    struct _stat st = { 0 };
    #else
    struct stat st = { 0 };
    #endif

    if (stat(path, &st) == -1) {
        #ifdef _WIN32
        if (mkdir(path) != 0)
        #else
        if (mkdir(path, 0700) != 0)
        #endif
            panic("MKDIR failed");
    }
}

void createDirectoryTree(const char* path) {
    char tempPath[1024];
    char* c = NULL;
    u64 len;

    snprintf(tempPath, sizeof(tempPath), "%s", path);
    len = strlen(tempPath);

    for (c = tempPath + 1; c < tempPath + len; c++) {
        if (*c == PATH_SEPARATOR_C) {
            *c = '\0'; // Temporarily null-terminate string

            createDirectory(tempPath);

            *c = PATH_SEPARATOR_C;
        }
    }

    createDirectory(tempPath);
}