#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "files.h"

#include "beaProcess.h"
#include "luaProcess.h"
#include "opusProcess.h"
#include "bfsarProcess.h"
#include "bfwsdProcess.h"
#include "bfwavProcess.h"

#include "common.h"

void usage() {
    printf("usage: bezt [bea/lua/make_lua/opus/bfsar/bfwsd/bfwav] [filepath]\n");
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 3)
        usage();

    FileHandle fileHndl;
    fileHndl.data_void = NULL;
    fileHndl.size = 0;

    if (strcasecmp(argv[1], "bea") == 0) {
        fileHndl = FileCreateHandle(argv[2]);

        printf("Preprocess ..");

        BeaPreprocess(fileHndl.data_u8);

        LOG_OK;

        BeaExtractAll(fileHndl.data_u8);
    }
    else if (strcasecmp(argv[1], "lua") == 0) {
        fileHndl = FileCreateHandle(argv[2]);

        printf("Preprocess ..");

        LuaPreprocess(fileHndl.data_u8);

        LOG_OK;

        char path[256];
        sprintf(path, "%.*s.luac", (int)(getExtension(argv[2]) - argv[2]), argv[2]);

        FileHandle luacHndl = LuaCreateLuacFile(fileHndl.data_u8, fileHndl.size);
        FileWriteHandle(luacHndl, path);
        FileDestroyHandle(luacHndl);
    }
    else if (strcasecmp(argv[1], "make_lua") == 0) {
        fileHndl = FileCreateHandle(argv[2]);

        FileHandle luaHndl = LuaProduce(fileHndl.data_u8, fileHndl.size);

        char path[256];
        sprintf(path, "%.*s.lua", (int)(getExtension(argv[2]) - argv[2]), argv[2]);

        FileWriteHandle(luaHndl, path);

        FileDestroyHandle(luaHndl);
    }
    else if (strcasecmp(argv[1], "opus") == 0) {
        fileHndl = FileCreateHandle(argv[2]);

        printf("Preprocess ..");

        OpusPreprocess(fileHndl.data_u8);

        LOG_OK;

        OpusProcess(fileHndl.data_u8, "haha.wav");
    }
    else if (strcasecmp(argv[1], "bfsar") == 0) {
        fileHndl = FileCreateHandle(argv[2]);

        printf("Preprocess ..");

        BfsarPreprocess(fileHndl.data_u8);

        LOG_OK;

        BfsarProcess(fileHndl.data_u8);
    }
    else if (strcasecmp(argv[1], "bfwsd") == 0) {
        fileHndl = FileCreateHandle(argv[2]);

        printf("Preprocess ..");

        BfwsdPreprocess(fileHndl.data_u8);

        LOG_OK;
    }
    else if (strcasecmp(argv[1], "bfwav") == 0) {
        fileHndl = FileCreateHandle(argv[2]);

        printf("Preprocess ..");

        BfwavPreprocess(fileHndl.data_u8);

        LOG_OK;

        printf("Dat ..");

        s16* samples = BfwavGetSamples(fileHndl.data_u8);

        LOG_OK;

        free(samples);
    }
    else
        usage();

    FileDestroyHandle(fileHndl);

    return 0;
}