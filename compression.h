#ifndef COMPRESSION_H
#define COMPRESSION_H

#include "common.h"

typedef struct {
    union {
        s8* data_s8;
        u8* data_u8;

        void* data_void;
    };
    u32 size;
} CompressionResult;

CompressionResult decompressZlib(u8* zlibData, u32 deflatedSize, u32 inflatedSize);
CompressionResult decompressZstd(u8* deflatedData, u32 deflatedSize, u32 inflatedSize);

#endif // COMPRESSION_H
